🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 23.2 — Pipes et communication inter-processus

## Chapitre 23 : Processus et IPC

---

## Introduction

Quand vous tapez `cat fichier.txt | grep "erreur" | wc -l` dans un terminal, trois processus sont lancés et reliés par des **pipes**. La sortie de `cat` alimente l'entrée de `grep`, dont la sortie alimente l'entrée de `wc`. Aucun fichier temporaire n'est créé — les données circulent directement en mémoire, du producteur vers le consommateur, à travers le noyau.

Les pipes sont le mécanisme IPC le plus ancien (Unix v3, 1973), le plus simple et le plus utilisé. Ils offrent un canal de communication **unidirectionnel**, **orienté flux d'octets** (comme TCP, section 22.1), géré entièrement par le noyau. Ils existent sous deux formes :

- **Pipes anonymes** — Créés par l'appel système `pipe()`. Ils n'ont pas de nom dans le système de fichiers et ne sont accessibles que par le processus qui les crée et ses descendants (via l'héritage des file descriptors par `fork`). C'est le mécanisme derrière l'opérateur `|` du shell.
- **Pipes nommés (FIFOs)** — Créés par `mkfifo()`. Ils apparaissent comme des fichiers spéciaux dans le système de fichiers, ce qui permet à des processus **sans lien de parenté** de communiquer en ouvrant le même chemin.

---

## Pipes anonymes

### L'appel système `pipe()`

```cpp
#include <unistd.h>

int pipe(int pipefd[2]);  
int pipe2(int pipefd[2], int flags);  // Version Linux avec flags  
```

`pipe()` crée une paire de file descriptors connectés :

- `pipefd[0]` — Le côté **lecture**. Les données écrites dans `pipefd[1]` sont lues ici.
- `pipefd[1]` — Le côté **écriture**. Les données écrites ici sont lues depuis `pipefd[0]`.

```
                    Noyau (buffer interne)
                    ┌──────────────────────┐
  pipefd[1] ───────►│  données en transit  │──────► pipefd[0]
  (écriture)        └──────────────────────┘        (lecture)
                    64 KiB par défaut sur Linux
```

Le buffer interne du noyau fait **64 KiB** par défaut sur Linux (16 pages). La taille d'un pipe individuel peut être ajustée avec `fcntl(fd, F_SETPIPE_SZ, size)`, dans la limite de `/proc/sys/fs/pipe-max-size` (1 MiB par défaut pour les utilisateurs non-root). Si le buffer est plein, `write()` bloque jusqu'à ce que le lecteur consomme des données. Si le buffer est vide, `read()` bloque jusqu'à ce que l'écrivain produise des données. Ce mécanisme crée naturellement un **backpressure** entre producteur et consommateur — le même concept vu avec les sockets en section 22.3.

### Création et utilisation de base

```cpp
#include <unistd.h>
#include <fcntl.h>
#include <print>
#include <cstring>

int main() {
    int pipefd[2];
    if (pipe2(pipefd, O_CLOEXEC) == -1) {
        throw std::system_error(errno, std::system_category(), "pipe2()");
    }

    // Écrire dans le pipe
    const char* message = "Hello depuis le pipe!";
    write(pipefd[1], message, strlen(message));

    // Lire depuis le pipe
    char buffer[256];
    ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
    buffer[n] = '\0';

    std::println("Lu: {}", buffer);

    close(pipefd[0]);
    close(pipefd[1]);
}
```

> 💡 Utilisez `pipe2()` avec `O_CLOEXEC` plutôt que `pipe()`, pour les mêmes raisons que `SOCK_CLOEXEC` sur les sockets (section 22.1.1) — éviter les race conditions avec `fork`/`exec` dans un programme multi-threadé.

### Communication parent → enfant

Le cas d'usage fondamental des pipes anonymes est la communication entre un processus parent et son enfant. Le pattern repose sur le fait que `fork()` duplique les file descriptors :

```cpp
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <print>
#include <cstring>

int main() {
    int pipefd[2];
    if (pipe2(pipefd, O_CLOEXEC) == -1) {
        throw std::system_error(errno, std::system_category(), "pipe2()");
    }

    pid_t pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork()");
    }

    if (pid == 0) {
        // ── ENFANT : lecteur ──
        close(pipefd[1]);  // Fermer le côté écriture (inutile ici)

        char buffer[256];
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            std::println("[Enfant] Reçu: {}", buffer);
        }

        close(pipefd[0]);
        _exit(0);
    }

    // ── PARENT : écrivain ──
    close(pipefd[0]);  // Fermer le côté lecture (inutile ici)

    const char* msg = "Message du parent";
    write(pipefd[1], msg, strlen(msg));
    close(pipefd[1]);  // Fermer → l'enfant verra EOF

    waitpid(pid, nullptr, 0);
    std::println("[Parent] Enfant terminé");
}
```

```
[Enfant] Reçu: Message du parent
[Parent] Enfant terminé
```

### La règle d'or : fermer les extrémités inutiles

C'est le point le plus important et la source de bugs la plus fréquente avec les pipes. Après un `fork()`, les **deux** processus ont les **deux** extrémités du pipe :

```
Avant fork() :
  Parent : pipefd[0] (lecture), pipefd[1] (écriture)

Après fork() :
  Parent : pipefd[0] (lecture), pipefd[1] (écriture)
  Enfant : pipefd[0] (lecture), pipefd[1] (écriture)
```

Si le parent écrit et l'enfant lit, le parent doit fermer `pipefd[0]` et l'enfant doit fermer `pipefd[1]`. Pourquoi est-ce critique ?

**EOF ne sera jamais détecté si un côté écriture reste ouvert.** Le `read()` de l'enfant ne retourne 0 (EOF) que lorsque **tous** les file descriptors du côté écriture sont fermés. Si l'enfant garde son propre `pipefd[1]` ouvert, `read()` bloquera indéfiniment même après que le parent ait fermé le sien — le noyau voit qu'un descripteur d'écriture est encore ouvert (celui de l'enfant lui-même) et attend d'éventuelles données.

**`SIGPIPE` ne sera pas envoyé si un côté lecture reste ouvert.** Si le parent garde son `pipefd[0]` ouvert et que l'enfant (lecteur) meurt, le `write()` du parent ne recevra pas `SIGPIPE` car le noyau voit qu'un lecteur existe encore (le parent lui-même, via son `pipefd[0]`).

```cpp
// Pattern correct :
if (pid == 0) {
    // Enfant = lecteur
    close(pipefd[1]);  // ✓ OBLIGATOIRE — fermer l'écriture inutilisée
    // ... read(pipefd[0], ...) ...
    close(pipefd[0]);
    _exit(0);
}
// Parent = écrivain
close(pipefd[0]);  // ✓ OBLIGATOIRE — fermer la lecture inutilisée
// ... write(pipefd[1], ...) ...
close(pipefd[1]);  // Fermer → signal EOF à l'enfant
```

### Communication bidirectionnelle : deux pipes

Un pipe est unidirectionnel. Pour une communication bidirectionnelle (parent ↔ enfant), il faut **deux pipes** :

```cpp
int parent_to_child[2];  // Parent écrit, enfant lit  
int child_to_parent[2];  // Enfant écrit, parent lit  

pipe2(parent_to_child, O_CLOEXEC);  
pipe2(child_to_parent, O_CLOEXEC);  

pid_t pid = fork();  
if (pid == 0) {  
    // Enfant
    close(parent_to_child[1]);  // Fermer l'écriture du pipe parent→enfant
    close(child_to_parent[0]);  // Fermer la lecture du pipe enfant→parent

    int read_fd  = parent_to_child[0];  // Lire les commandes du parent
    int write_fd = child_to_parent[1];  // Envoyer les réponses au parent

    // ... boucle lecture/écriture ...

    close(read_fd);
    close(write_fd);
    _exit(0);
}

// Parent
close(parent_to_child[0]);  // Fermer la lecture du pipe parent→enfant  
close(child_to_parent[1]);  // Fermer l'écriture du pipe enfant→parent  

int write_fd = parent_to_child[1];  // Envoyer les commandes à l'enfant  
int read_fd  = child_to_parent[0];  // Lire les réponses de l'enfant  
```

```
Parent                              Enfant
┌──────────────┐                    ┌──────────────┐
│              │  parent_to_child   │              │
│ write_fd ────┼──────────────────►─┤ read_fd      │
│              │                    │              │
│ read_fd  ◄───┼──────────────────◄─┤ write_fd     │
│              │  child_to_parent   │              │
└──────────────┘                    └──────────────┘
```

---

## Wrapper RAII pour les pipes

Les file descriptors de pipes doivent être fermés proprement. Voici un wrapper qui encapsule une paire de pipes pour la communication parent-enfant :

```cpp
#include <unistd.h>
#include <cerrno>
#include <utility>
#include <system_error>

class Pipe {  
public:  
    Pipe() {
        if (pipe2(fds_, O_CLOEXEC) == -1) {
            throw std::system_error(errno, std::system_category(), "pipe2()");
        }
    }

    ~Pipe() {
        close_read();
        close_write();
    }

    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    Pipe(Pipe&& other) noexcept {
        fds_[0] = std::exchange(other.fds_[0], -1);
        fds_[1] = std::exchange(other.fds_[1], -1);
    }

    Pipe& operator=(Pipe&& other) noexcept {
        if (this != &other) {
            close_read();
            close_write();
            fds_[0] = std::exchange(other.fds_[0], -1);
            fds_[1] = std::exchange(other.fds_[1], -1);
        }
        return *this;
    }

    [[nodiscard]] int read_fd() const noexcept { return fds_[0]; }
    [[nodiscard]] int write_fd() const noexcept { return fds_[1]; }

    void close_read() {
        if (fds_[0] != -1) { close(fds_[0]); fds_[0] = -1; }
    }

    void close_write() {
        if (fds_[1] != -1) { close(fds_[1]); fds_[1] = -1; }
    }

    // Lire des données
    ssize_t read_data(void* buf, size_t count) {
        ssize_t n;
        do {
            n = read(fds_[0], buf, count);
        } while (n == -1 && errno == EINTR);
        return n;
    }

    // Écrire des données
    ssize_t write_data(const void* buf, size_t count) {
        ssize_t n;
        do {
            n = write(fds_[1], buf, count);
        } while (n == -1 && errno == EINTR);
        return n;
    }

    // Lire tout jusqu'à EOF dans un string
    std::string read_all() {
        std::string result;
        char buffer[4096];
        ssize_t n;
        while ((n = read_data(buffer, sizeof(buffer))) > 0) {
            result.append(buffer, static_cast<size_t>(n));
        }
        if (n == -1) {
            throw std::system_error(errno, std::system_category(), "read()");
        }
        return result;
    }

private:
    int fds_[2] = {-1, -1};
};
```

Utilisation avec fork :

```cpp
Pipe p;

pid_t pid = fork();  
if (pid == 0) {  
    // Enfant : lecteur
    p.close_write();
    std::string data = p.read_all();
    std::println("[Enfant] Reçu {} octets: {}", data.size(), data);
    _exit(0);
}

// Parent : écrivain
p.close_read();  
std::string msg = "Données envoyées via RAII Pipe";  
p.write_data(msg.data(), msg.size());  
p.close_write();  // Signal EOF à l'enfant  

waitpid(pid, nullptr, 0);
```

---

## Redirection de stdin/stdout avec les pipes

### Le mécanisme : `dup2()`

La vraie puissance des pipes apparaît quand on les combine avec la redirection de file descriptors. `dup2()` permet de **remplacer** un file descriptor par un autre :

```cpp
#include <unistd.h>

int dup2(int oldfd, int newfd);
// Ferme newfd s'il est ouvert, puis fait pointer newfd vers oldfd
```

En remplaçant `STDIN_FILENO` (fd 0) ou `STDOUT_FILENO` (fd 1) par un bout de pipe, vous redirigez les entrées/sorties d'un processus de manière transparente — le programme cible n'a aucune idée qu'il lit/écrit dans un pipe plutôt que dans un terminal.

### Pattern : capturer stdout d'un processus enfant

Ce pattern est la version détaillée de la fonction `run_and_capture` de la section 23.1 :

```cpp
#include <unistd.h>
#include <sys/wait.h>
#include <print>
#include <string>

int main() {
    Pipe stdout_pipe;

    pid_t pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork()");
    }

    if (pid == 0) {
        // Enfant : remplacer stdout par le côté écriture du pipe
        stdout_pipe.close_read();

        // STDOUT_FILENO (fd 1) pointe maintenant vers le pipe
        dup2(stdout_pipe.write_fd(), STDOUT_FILENO);
        stdout_pipe.close_write();  // Le fd original n'est plus nécessaire

        // Tout ce que ce programme écrit sur stdout ira dans le pipe
        execlp("ls", "ls", "-la", "/tmp", nullptr);
        _exit(127);
    }

    // Parent : lire la sortie de l'enfant
    stdout_pipe.close_write();
    std::string output = stdout_pipe.read_all();

    int status;
    waitpid(pid, &status, 0);

    std::println("Sortie de 'ls -la /tmp' ({} octets):", output.size());
    std::print("{}", output);
}
```

### Pattern : alimenter stdin d'un processus enfant

L'inverse — envoyer des données sur l'entrée standard d'un processus :

```cpp
Pipe stdin_pipe;

pid_t pid = fork();  
if (pid == 0) {  
    // Enfant : remplacer stdin par le côté lecture du pipe
    stdin_pipe.close_write();
    dup2(stdin_pipe.read_fd(), STDIN_FILENO);
    stdin_pipe.close_read();

    // wc lira depuis le pipe (comme s'il lisait depuis le terminal)
    execlp("wc", "wc", "-l", nullptr);
    _exit(127);
}

// Parent : écrire dans le stdin de l'enfant
stdin_pipe.close_read();  
std::string data = "ligne 1\nligne 2\nligne 3\nligne 4\n";  
stdin_pipe.write_data(data.data(), data.size());  
stdin_pipe.close_write();  // EOF → wc peut terminer son comptage  

int status;  
waitpid(pid, &status, 0);  
// wc affiche "4" sur le terminal (son stdout n'est pas redirigé)
```

### Pattern : chaîne de pipes (pipeline)

Reproduisons `cat /etc/passwd | grep root | wc -l` en C++ :

```cpp
#include <unistd.h>
#include <sys/wait.h>
#include <print>

void exec_in_pipeline(Pipe* input, Pipe* output,
                       const char* program, char* const argv[]) {
    pid_t pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork()");
    }

    if (pid == 0) {
        // Rediriger stdin si un pipe d'entrée est fourni
        if (input) {
            input->close_write();
            dup2(input->read_fd(), STDIN_FILENO);
            input->close_read();
        }

        // Rediriger stdout si un pipe de sortie est fourni
        if (output) {
            output->close_read();
            dup2(output->write_fd(), STDOUT_FILENO);
            output->close_write();
        }

        execvp(program, argv);
        _exit(127);
    }
    // Le parent continue — le pid de l'enfant n'est pas stocké ici
    // (simplifié pour la lisibilité)
}

int main() {
    // cat /etc/passwd | grep root | wc -l
    Pipe pipe1;  // cat → grep
    Pipe pipe2;  // grep → wc

    // Lancer cat (pas de stdin redirigé, stdout → pipe1)
    char* cat_args[] = {
        const_cast<char*>("cat"),
        const_cast<char*>("/etc/passwd"),
        nullptr
    };
    exec_in_pipeline(nullptr, &pipe1, "cat", cat_args);

    // Lancer grep (stdin ← pipe1, stdout → pipe2)
    char* grep_args[] = {
        const_cast<char*>("grep"),
        const_cast<char*>("root"),
        nullptr
    };
    exec_in_pipeline(&pipe1, &pipe2, "grep", grep_args);

    // Lancer wc (stdin ← pipe2, pas de stdout redirigé)
    char* wc_args[] = {
        const_cast<char*>("wc"),
        const_cast<char*>("-l"),
        nullptr
    };
    exec_in_pipeline(&pipe2, nullptr, "wc", wc_args);

    // CRUCIAL : fermer tous les bouts de pipe dans le parent
    // Sinon les enfants ne verront jamais EOF
    pipe1.close_read();
    pipe1.close_write();
    pipe2.close_read();
    pipe2.close_write();

    // Attendre tous les enfants
    while (wait(nullptr) > 0) {}
}
```

La fermeture des pipes dans le parent après le lancement de tous les enfants est **critique**. Le parent a hérité de tous les file descriptors via les `fork()` successifs. Tant que le parent garde le côté écriture de `pipe1` ouvert, `grep` ne verra jamais EOF et bloquera indéfiniment sur `read()`.

---

## Pipes nommés (FIFOs)

### Le problème des pipes anonymes

Les pipes anonymes ne fonctionnent qu'entre processus ayant un ancêtre commun (parent-enfant, ou frères issus du même parent) — car ils partagent les file descriptors via `fork()`. Pour la communication entre processus **indépendants** (lancés séparément, sans lien de parenté), il faut un mécanisme de rendezvous.

### Les FIFOs : des pipes avec un nom

Un FIFO (First In, First Out) est un pipe qui apparaît dans le système de fichiers. Deux processus qui ouvrent le même chemin FIFO obtiennent les deux extrémités d'un pipe — l'un en lecture, l'autre en écriture.

```cpp
#include <sys/stat.h>

int mkfifo(const char* pathname, mode_t mode);
```

```bash
# Ou en ligne de commande
mkfifo /tmp/my_fifo
```

### Comportement à l'ouverture

L'ouverture d'un FIFO a un comportement bloquant particulier :

- `open(path, O_RDONLY)` bloque **jusqu'à ce qu'un autre processus ouvre le même FIFO en écriture**.
- `open(path, O_WRONLY)` bloque **jusqu'à ce qu'un autre processus ouvre le même FIFO en lecture**.

Les deux processus doivent ouvrir le FIFO pour que l'un ou l'autre progresse — c'est un **rendezvous**. Ce comportement est souvent surprenant. Vous pouvez le contourner avec `O_NONBLOCK`, mais les sémantiques deviennent plus complexes.

### Producteur / consommateur avec FIFO

**Processus producteur** (peut être lancé indépendamment) :

```cpp
// producer.cpp
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <print>
#include <string>

int main() {
    const char* fifo_path = "/tmp/my_fifo";

    // Créer le FIFO (ignorer l'erreur si déjà existant)
    mkfifo(fifo_path, 0666);

    std::println("Ouverture du FIFO en écriture (attend un lecteur)...");
    int fd = open(fifo_path, O_WRONLY);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category(), "open()");
    }

    std::println("Lecteur connecté — envoi de données");

    for (int i = 0; i < 5; ++i) {
        std::string msg = "Message " + std::to_string(i) + "\n";
        write(fd, msg.data(), msg.size());
        std::println("  Envoyé: {}", msg.substr(0, msg.size() - 1));
        sleep(1);
    }

    close(fd);
    std::println("Producteur terminé");
}
```

**Processus consommateur** (lancé dans un autre terminal) :

```cpp
// consumer.cpp
#include <fcntl.h>
#include <unistd.h>
#include <print>

int main() {
    const char* fifo_path = "/tmp/my_fifo";

    std::println("Ouverture du FIFO en lecture (attend un écrivain)...");
    int fd = open(fifo_path, O_RDONLY);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category(), "open()");
    }

    std::println("Écrivain connecté — lecture des données");

    char buffer[256];
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        std::print("  Reçu: {}", buffer);
    }

    close(fd);
    unlink(fifo_path);  // Nettoyer le FIFO
    std::println("Consommateur terminé");
}
```

```bash
# Terminal 1                         # Terminal 2
$ ./producer                         $ ./consumer
Ouverture du FIFO...                 Ouverture du FIFO...  
Lecteur connecté                     Écrivain connecté  
  Envoyé: Message 0                    Reçu: Message 0
  Envoyé: Message 1                    Reçu: Message 1
  Envoyé: Message 2                    Reçu: Message 2
  Envoyé: Message 3                    Reçu: Message 3
  Envoyé: Message 4                    Reçu: Message 4
Producteur terminé                   Consommateur terminé
```

### FIFOs et le système de fichiers

Un FIFO apparaît comme un fichier spécial :

```bash
$ ls -la /tmp/my_fifo
prw-rw-r-- 1 user user 0 Mar 14 10:00 /tmp/my_fifo
#^
# Le 'p' indique un pipe nommé
```

Malgré son apparence de fichier, un FIFO ne stocke **aucune donnée sur le disque**. Les données transitent uniquement par le buffer noyau en mémoire. Le « fichier » FIFO n'est qu'un point de rendezvous dans le namespace du système de fichiers.

### Nettoyage des FIFOs

Contrairement aux pipes anonymes (qui disparaissent quand tous les fd sont fermés), un FIFO persiste dans le système de fichiers jusqu'à ce qu'il soit explicitement supprimé avec `unlink()` ou `rm`. Pensez à nettoyer — un FIFO abandonné dans `/tmp` est une source de confusion.

```cpp
// Suppression programmatique
unlink("/tmp/my_fifo");
```

```bash
# Suppression en ligne de commande
rm /tmp/my_fifo
```

---

## Comportements importants des pipes

### `SIGPIPE` et `EPIPE`

Quand un processus écrit dans un pipe dont **tous** les lecteurs ont fermé leur extrémité, le noyau envoie `SIGPIPE` à l'écrivain. Par défaut, ce signal tue le processus — un comportement sensé pour les pipelines shell (si `grep` meurt, inutile que `cat` continue d'écrire), mais dangereux pour un serveur.

Le traitement est identique à celui des sockets (section 22.1.3) :

```cpp
// Option 1 : ignorer le signal globalement
signal(SIGPIPE, SIG_IGN);
// write() retournera -1 avec errno = EPIPE

// Option 2 : détecter EPIPE après chaque write
ssize_t n = write(pipe_fd, data, len);  
if (n == -1 && errno == EPIPE) {  
    // Le lecteur est parti — gérer proprement
}
```

### Atomicité des écritures

POSIX garantit que les écritures de **`PIPE_BUF` octets ou moins** sont **atomiques** — elles ne seront pas entrelacées avec les écritures d'autres processus. Sur Linux, `PIPE_BUF` vaut **4096 octets**.

Cela signifie que si deux processus écrivent simultanément des messages de 100 octets dans le même pipe, le lecteur recevra toujours des messages entiers de 100 octets — jamais un mélange des deux. En revanche, pour des écritures dépassant `PIPE_BUF`, les données de différents écrivains peuvent être entrelacées.

```cpp
#include <limits.h>  // PIPE_BUF

// Écriture atomique garantie si len <= PIPE_BUF
if (len <= PIPE_BUF) {
    write(fd, data, len);  // Atomique
}
```

Ce comportement est important quand plusieurs processus enfants écrivent dans le même pipe (par exemple, des workers qui envoient des résultats à un coordinateur).

### EOF et pipes

`read()` sur un pipe retourne 0 (EOF) uniquement quand **tous** les file descriptors du côté écriture sont fermés. C'est pourquoi la fermeture des extrémités inutiles est si critique — tant qu'un seul fd d'écriture existe quelque part (même dans un processus qui n'écrit jamais), le lecteur ne verra jamais EOF.

### Pipes non-bloquants

Les pipes supportent le mode non-bloquant, utile avec `poll()` ou `epoll` :

```cpp
// À la création
pipe2(fds, O_CLOEXEC | O_NONBLOCK);

// Ou après création
int flags = fcntl(fd, F_GETFL);  
fcntl(fd, F_SETFL, flags | O_NONBLOCK);  
```

En mode non-bloquant :

- `read()` retourne `-1` avec `errno = EAGAIN` s'il n'y a rien à lire.
- `write()` retourne `-1` avec `errno = EAGAIN` si le buffer est plein.

Combinés avec `epoll`, les pipes non-bloquants s'intègrent dans la même boucle événementielle que les sockets (section 22.3) :

```cpp
// Surveiller un pipe ET un socket dans le même epoll
epoll.add(pipe_read_fd, EPOLLIN);  
epoll.add(socket_fd, EPOLLIN);  

// Un seul epoll_wait pour les deux
int n = epoll.wait(events, MAX_EVENTS, -1);  
for (int i = 0; i < n; ++i) {  
    if (events[i].data.fd == pipe_read_fd) {
        // Données du pipe
    } else if (events[i].data.fd == socket_fd) {
        // Données du socket
    }
}
```

---

## Pipes vs sockets Unix : quand choisir ?

Les sockets Unix (`AF_UNIX`, section 22.1) et les pipes couvrent des cas d'usage qui se chevauchent. Voici les critères de choix :

```
Critère              Pipe                Socket Unix
──────────────────   ──────────────────  ──────────────────
Direction            Unidirectionnel     Bidirectionnel  
Processus liés       Oui (anonyme)       Non requis  
Processus non-liés   Oui (FIFO)          Oui (chemin filesystem)  
Passage de fd        Non                 Oui (sendmsg/SCM_RIGHTS)  
Datagrammes          Non (flux)          Oui (SOCK_DGRAM)  
Multicast            Non                 Non  
Complexité           Très faible         Moyenne  
Performance          Légèrement plus     Très bonne  
                     rapide (moins 
                     d'overhead noyau)
```

**Choisissez un pipe** quand la communication est unidirectionnelle, entre processus parent-enfant, et que vous n'avez pas besoin de fonctionnalités avancées. C'est le cas de la majorité des scénarios : capturer la sortie d'un processus, alimenter l'entrée d'un processus, construire un pipeline.

**Choisissez un socket Unix** quand vous avez besoin de communication bidirectionnelle, de passage de file descriptors entre processus, de datagrammes (messages délimités plutôt qu'un flux), ou de communication entre processus sans lien de parenté dans un contexte plus complexe qu'un simple producteur-consommateur.

---

## Résumé

Les pipes sont le mécanisme IPC le plus simple et le plus fondamental sous Linux :

- **Pipes anonymes** — `pipe2()` crée une paire de fd connectés. Communication unidirectionnelle parent-enfant. Le buffer noyau (~64 KiB) fournit un backpressure naturel.
- **Pipes nommés (FIFOs)** — `mkfifo()` crée un point de rendezvous dans le filesystem pour la communication entre processus indépendants. Même sémantique de flux que les pipes anonymes.
- **La règle d'or** — Fermer systématiquement les extrémités inutiles après `fork()`. Sans cela, EOF ne sera jamais détecté et les processus bloqueront indéfiniment.
- **Redirection avec `dup2()`** — Permet de connecter un pipe au stdin/stdout d'un processus, rendant la communication transparente pour le programme cible.
- **Atomicité** — Les écritures de `PIPE_BUF` octets ou moins (4096 sur Linux) sont atomiques et ne seront pas entrelacées.
- **`SIGPIPE`** — Envoyé quand tous les lecteurs ont fermé. Ignorez le signal ou gérez `EPIPE` pour un comportement robuste.
- **Intégration `epoll`** — Les pipes non-bloquants se multiplexent avec les sockets dans la même boucle événementielle.

---

> **Prochaine étape** → Section 23.3 : Shared memory et mmap — quand les pipes ne suffisent plus et que la performance exige du zéro-copie.

⏭️ [Shared memory et mmap](/23-processus-ipc/03-shared-memory.md)
