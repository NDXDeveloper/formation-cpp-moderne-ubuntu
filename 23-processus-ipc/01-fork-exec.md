🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 23.1 — fork, exec et gestion de processus

## Chapitre 23 : Processus et IPC

---

## Introduction

Sur Linux, chaque programme en cours d'exécution est un **processus**. Votre terminal, votre éditeur de texte, le compilateur que vous lancez, le serveur gRPC du chapitre 22 — ce sont tous des processus avec leur propre espace d'adressage, leur propre pile d'appels, leurs propres file descriptors. Et à une exception près (le processus `init`, PID 1), chaque processus a été **créé par un autre processus**.

Le modèle Unix de création de processus est à la fois élégant et déroutant pour qui vient d'un autre système d'exploitation. Il repose sur deux opérations distinctes :

- **`fork()`** — Crée une copie quasi-identique du processus appelant. Le processus original (le **parent**) et la copie (l'**enfant**) continuent l'exécution à partir du même point dans le code, avec le même état — mais dans des espaces d'adressage séparés.
- **`exec()`** — Remplace l'image mémoire du processus courant par un nouveau programme. Le PID reste le même, mais le code, les données, la pile — tout est remplacé.

La combinaison `fork` + `exec` est le mécanisme fondamental par lequel **tout programme est lancé** sous Linux. Quand vous tapez `ls` dans un terminal, le shell fait un `fork` pour créer un enfant, puis l'enfant fait un `exec("ls")` pour se transformer en `ls`. C'est ce même mécanisme que vos programmes C++ utilisent pour lancer des commandes externes.

---

## Le cycle de vie d'un processus

```
  Parent                          Enfant
  ──────                          ──────

  fork() ─────────────────────►  (né ici)
    │                               │
    │  retour: PID de l'enfant      │  retour: 0
    │                               │
    │                               ├── exec("programme")
    │                               │   (remplacé par le nouveau programme)
    │                               │
    │                               │   ... exécution ...
    │                               │
    │                               ├── exit(code)
    │                               │   (terminaison)
    │                               │
    ├── waitpid(pid) ◄──────────────┘
    │   (récupère le code de sortie)
    │
    │   (l'enfant est nettoyé)
    ▼
```

Les quatre étapes sont : **créer** (`fork`), **transformer** (`exec`), **terminer** (`exit`), **récupérer** (`wait`). Omettre la dernière étape crée un processus **zombie** — un sujet couvert plus loin dans cette section.

---

## `fork()` — Cloner le processus

### Signature

```cpp
#include <unistd.h>

pid_t fork();
```

### Comportement

`fork()` crée un nouveau processus qui est une **copie** du processus appelant. Au retour de `fork()`, il y a deux processus qui exécutent le même code, au même point d'exécution. La seule différence est la **valeur de retour** :

- Dans le **parent** : `fork()` retourne le **PID de l'enfant** (un entier positif).
- Dans l'**enfant** : `fork()` retourne **0**.
- En cas d'**erreur** : `fork()` retourne **-1** (pas d'enfant créé).

```cpp
#include <unistd.h>
#include <sys/wait.h>
#include <print>
#include <cstdio>

int main() {
    std::println("Avant fork — PID {}", getpid());
    std::fflush(nullptr);  // Flush tous les buffers AVANT fork

    pid_t pid = fork();

    if (pid == -1) {
        // Erreur — fork a échoué
        throw std::system_error(errno, std::system_category(), "fork()");
    }

    if (pid == 0) {
        // ── Code exécuté par l'ENFANT ──
        std::println("Enfant — PID {}, parent PID {}", getpid(), getppid());
        std::fflush(stdout);  // Flush avant _exit (qui ne flush pas les buffers)
        _exit(0);  // Terminer l'enfant
    }

    // ── Code exécuté par le PARENT ──
    std::println("Parent — PID {}, enfant PID {}", getpid(), pid);

    // Attendre que l'enfant se termine
    int status;
    waitpid(pid, &status, 0);
    std::println("Enfant terminé avec le code {}", WEXITSTATUS(status));
}
```

```
Avant fork — PID 12345  
Parent — PID 12345, enfant PID 12346  
Enfant — PID 12346, parent PID 12345  
Enfant terminé avec le code 0  
```

### Ce que l'enfant hérite

L'enfant reçoit une **copie** de presque tout l'état du parent :

- L'espace d'adressage (code, données, heap, stack) — copié via **copy-on-write** (voir ci-dessous).
- Les file descriptors ouverts — l'enfant a ses propres copies, pointant vers les mêmes objets noyau (fichiers, sockets, pipes).
- Les variables d'environnement.
- Le répertoire de travail courant.
- Le masque de signaux et les handlers de signaux.
- Les UID/GID réels et effectifs.

### Ce que l'enfant ne hérite PAS

- Le **PID** — L'enfant a un nouveau PID unique.
- Les **locks** (`flock`, locks POSIX) — Non hérités.
- Les **timers** (`timer_create`) — Non hérités.
- Les **signaux en attente** — Vidés dans l'enfant.
- La valeur de retour de `fork()` — 0 dans l'enfant, PID de l'enfant dans le parent.

### Copy-on-write (COW)

`fork()` ne copie pas réellement toute la mémoire du parent — ce serait prohibitif pour un processus de plusieurs GiB. À la place, le noyau utilise le mécanisme de **copy-on-write** : les pages mémoire sont partagées entre parent et enfant, marquées en lecture seule. Ce n'est que lorsqu'un des deux processus **modifie** une page que le noyau en crée une copie privée.

```
Immédiatement après fork() :

  Parent                  Enfant
  ┌──────┐               ┌──────┐
  │ Page │───── COW ─────│ Page │  ← Même page physique
  │  1   │               │  1   │    (lecture seule)
  ├──────┤               ├──────┤
  │ Page │───── COW ─────│ Page │
  │  2   │               │  2   │
  └──────┘               └──────┘

Après que l'enfant modifie la page 2 :

  Parent                  Enfant
  ┌──────┐               ┌──────┐
  │ Page │───── COW ─────│ Page │  ← Toujours partagée
  │  1   │               │  1   │
  ├──────┤               ├──────┤
  │ Page │               │ Page │  ← Copiée, maintenant indépendante
  │  2   │               │ 2'   │
  └──────┘               └──────┘
```

Grâce au COW, un `fork()` suivi d'un `exec()` est très efficace : les pages du parent ne sont jamais réellement copiées car l'enfant les remplace immédiatement par le nouveau programme.

### `_exit()` vs `exit()`

Dans un processus enfant créé par `fork()`, utilisez `_exit()` (avec le underscore) plutôt que `exit()` :

```cpp
if (pid == 0) {
    // Enfant
    _exit(0);   // BON — terminaison immédiate
    // exit(0); // DANGEREUX — exécute les handlers atexit du parent
}
```

`exit()` exécute les fonctions enregistrées avec `atexit()` et flush les buffers `stdio`. Ces actions ont été enregistrées par le parent et ne devraient pas être exécutées par l'enfant — cela peut corrompre des fichiers ou fermer des connexions que le parent utilise encore. `_exit()` termine le processus immédiatement sans rien exécuter.

Cette précaution ne s'applique que si l'enfant n'appelle pas `exec()`. Après un `exec()`, l'image du processus est entièrement remplacée et ce problème ne se pose plus.

---

## La famille `exec()` — Remplacer l'image du processus

### Les variantes

La famille `exec` comprend plusieurs fonctions qui diffèrent par la façon dont elles acceptent les arguments et l'environnement :

```cpp
#include <unistd.h>

// Par liste d'arguments (terminée par nullptr)
int execl(const char* path, const char* arg0, ... /*, nullptr */);  
int execlp(const char* file, const char* arg0, ... /*, nullptr */);  
int execle(const char* path, const char* arg0, ... /*, nullptr, char* envp[] */);  

// Par tableau d'arguments
int execv(const char* path, char* const argv[]);  
int execvp(const char* file, char* const argv[]);  
int execvpe(const char* file, char* const argv[], char* const envp[]);  // Extension GNU  

// Version moderne avec file descriptor (Linux 3.19+)
int fexecve(int fd, char* const argv[], char* const envp[]);           // POSIX  
int execveat(int dirfd, const char* pathname,                           // Linux uniquement  
             char* const argv[], char* const envp[], int flags);
```

Le nommage suit un pattern mnémotechnique :

| Suffixe | Signification |
|---------|---------------|
| `l` | Arguments par **liste** (variadic) |
| `v` | Arguments par **vecteur** (tableau) |
| `p` | Recherche dans le **PATH** (pas besoin du chemin complet) |
| `e` | **Environnement** explicite |
| `f` | Exécution via **file descriptor** (`fexecve`) |

### Comportement

`exec` **ne retourne jamais** en cas de succès — le processus courant est intégralement remplacé par le nouveau programme. Si `exec` retourne, c'est qu'une erreur s'est produite.

```cpp
// Avec execvp (vecteur + PATH) — la variante la plus courante
pid_t pid = fork();  
if (pid == 0) {  
    // Enfant — se transformer en "ls -la /tmp"
    char* args[] = {
        const_cast<char*>("ls"),
        const_cast<char*>("-la"),
        const_cast<char*>("/tmp"),
        nullptr  // Terminateur obligatoire
    };

    execvp("ls", args);

    // Si on arrive ici, exec a échoué
    perror("execvp");
    _exit(127);  // Convention : 127 = commande non trouvée
}
```

```cpp
// Avec execlp (liste + PATH) — plus concis pour peu d'arguments
pid_t pid = fork();  
if (pid == 0) {  
    execlp("ls", "ls", "-la", "/tmp", nullptr);
    perror("execlp");
    _exit(127);
}
```

### Ce que `exec` préserve

Le nouveau programme hérite de certains attributs du processus :

- Le **PID** — Identique avant et après exec.
- Les **file descriptors ouverts** — Sauf ceux marqués `FD_CLOEXEC` (ou `O_CLOEXEC`/`SOCK_CLOEXEC`, voir chapitres 19 et 22). C'est le mécanisme par lequel les pipes et les redirections fonctionnent.
- Le **répertoire de travail**.
- Les **UID/GID**.
- Le **masque de signaux** (mais les handlers de signaux sont réinitialisés à `SIG_DFL`).

### Ce que `exec` remplace

- Le **code**, les **données**, le **heap**, la **stack** — Intégralement remplacés par le nouveau programme.
- Les **handlers de signaux** — Réinitialisés (le code des handlers n'existe plus).
- Les **mappings mémoire** — Remplacés.

---

## `wait()` et `waitpid()` — Récupérer le statut de l'enfant

### Pourquoi attendre ?

Quand un processus se termine, il ne disparaît pas immédiatement. Le noyau conserve une entrée minimale dans la table des processus (le PID, le code de sortie, les statistiques de ressources) jusqu'à ce que le parent la récupère avec `wait()` ou `waitpid()`. Tant que cette récupération n'a pas lieu, le processus terminé est un **zombie**.

### Signatures

```cpp
#include <sys/wait.h>

pid_t wait(int* status);                           // Attend n'importe quel enfant  
pid_t waitpid(pid_t pid, int* status, int options); // Attend un enfant spécifique  
```

### Utilisation de base

```cpp
pid_t pid = fork();  
if (pid == 0) {  
    execlp("g++", "g++", "-o", "main", "main.cpp", nullptr);
    _exit(127);
}

// Parent — attendre l'enfant spécifique
int status;  
pid_t result = waitpid(pid, &status, 0);  // 0 = bloquant  

if (result == -1) {
    throw std::system_error(errno, std::system_category(), "waitpid()");
}

// Analyser le status
if (WIFEXITED(status)) {
    int code = WEXITSTATUS(status);
    std::println("Enfant terminé avec le code {}", code);

    if (code == 0) {
        std::println("Compilation réussie");
    } else {
        std::println("Compilation échouée");
    }
}
else if (WIFSIGNALED(status)) {
    int sig = WTERMSIG(status);
    std::println("Enfant tué par le signal {} ({})",
                 sig, strsignal(sig));
}
```

### Macros d'analyse du status

Le `status` retourné par `wait`/`waitpid` est un entier encodé qui doit être décodé avec des macros :

| Macro | Signification |
|-------|---------------|
| `WIFEXITED(status)` | `true` si l'enfant s'est terminé normalement (`exit`, `_exit`, retour de `main`) |
| `WEXITSTATUS(status)` | Code de sortie (0-255). Valide seulement si `WIFEXITED` est vrai. |
| `WIFSIGNALED(status)` | `true` si l'enfant a été tué par un signal |
| `WTERMSIG(status)` | Numéro du signal qui a tué l'enfant. Valide seulement si `WIFSIGNALED` est vrai. |
| `WCOREDUMP(status)` | `true` si un core dump a été produit (non-standard mais universel sur Linux) |
| `WIFSTOPPED(status)` | `true` si l'enfant est stoppé (avec `WUNTRACED`) |
| `WSTOPSIG(status)` | Signal qui a stoppé l'enfant |

### Options de `waitpid`

| Option | Effet |
|--------|-------|
| `0` | Bloquant — attend indéfiniment |
| `WNOHANG` | Non-bloquant — retourne immédiatement si aucun enfant n'a terminé (retour 0) |
| `WUNTRACED` | Signale aussi les enfants stoppés (pas seulement terminés) |

```cpp
// Vérification non-bloquante (polling)
pid_t result = waitpid(pid, &status, WNOHANG);  
if (result == 0) {  
    // L'enfant tourne encore
} else if (result == pid) {
    // L'enfant a terminé
} else {
    // Erreur
}
```

---

## Processus zombies

### Le problème

Un processus zombie est un enfant qui a terminé mais dont le parent n'a pas appelé `wait()`. Le zombie occupe une entrée dans la table des processus — il ne consomme ni CPU ni mémoire, mais son PID ne peut pas être réutilisé. Sur un système avec un nombre limité de PIDs (`/proc/sys/kernel/pid_max` — 32768 en 32 bits, 4194304 en 64 bits depuis Linux 4.15), des zombies qui s'accumulent peuvent empêcher la création de nouveaux processus.

```bash
# Voir les zombies
ps aux | grep 'Z'
# ou
ps -eo pid,ppid,stat,cmd | grep Z
```

### Les causes courantes

- Le parent lance des enfants dans une boucle sans jamais appeler `wait()`.
- Le parent fait un `waitpid()` bloquant sur un enfant spécifique, mais d'autres enfants terminent entre-temps sans être récupérés.
- Le parent ignore `SIGCHLD` sans appeler `wait()` dans le handler.

### Solution 1 : Récolte avec `SIGCHLD`

Le noyau envoie `SIGCHLD` au parent quand un enfant change d'état (terminaison, stop, reprise). En installant un handler qui appelle `waitpid` en boucle, vous récoltez les zombies automatiquement :

```cpp
#include <csignal>
#include <sys/wait.h>

void sigchld_handler(int) {
    // Récolter TOUS les enfants terminés (plusieurs peuvent être arrivés)
    // waitpid avec -1 = n'importe quel enfant, WNOHANG = non-bloquant
    int saved_errno = errno;  // Sauvegarder errno (async-signal-safe)
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
        // Enfant récolté
    }
    errno = saved_errno;
}

int main() {
    struct sigaction sa{};
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, nullptr);

    // Lancer des enfants...
}
```

**`SA_RESTART`** — Les appels système interrompus par `SIGCHLD` sont relancés automatiquement. Sans ce flag, un `read()` ou `accept()` en cours retournerait `EINTR` à chaque mort d'enfant.

**`SA_NOCLDSTOP`** — Ne pas recevoir `SIGCHLD` quand un enfant est stoppé (SIGSTOP) — uniquement à la terminaison.

**Boucle `while`** — Plusieurs enfants peuvent terminer entre deux délivrances de `SIGCHLD` (les signaux ne sont pas empilés). Un seul `waitpid` ne suffit pas — il faut boucler jusqu'à ce que `waitpid` retourne 0 ou -1.

### Solution 2 : Ignorer `SIGCHLD` explicitement

Sur Linux, positionner le handler de `SIGCHLD` à `SIG_IGN` indique au noyau de récolter automatiquement les enfants :

```cpp
signal(SIGCHLD, SIG_IGN);  // Le noyau récolte les zombies automatiquement
```

C'est la solution la plus simple si vous n'avez pas besoin de connaître le code de sortie des enfants.

### Solution 3 : Double fork

Le double fork est un pattern classique pour lancer un processus complètement détaché du parent :

```cpp
pid_t pid = fork();  
if (pid == 0) {  
    // Premier enfant
    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Petit-enfant — le processus que vous voulez lancer
        // Son parent (le premier enfant) va mourir immédiatement,
        // donc il sera adopté par init (PID 1) qui le récoltera.
        execlp("daemon", "daemon", nullptr);
        _exit(127);
    }
    // Le premier enfant meurt immédiatement
    _exit(0);
}

// Le parent récolte le premier enfant (qui a déjà terminé)
waitpid(pid, nullptr, 0);
// Le petit-enfant est maintenant orphelin, adopté par init
```

---

## `posix_spawn()` — L'alternative moderne

Le pattern `fork` + `exec` est puissant mais verbeux et source d'erreurs. `posix_spawn` combine les deux en un seul appel, avec des attributs pour configurer le processus enfant :

```cpp
#include <spawn.h>
#include <sys/wait.h>
#include <print>
#include <cstring>

extern char** environ;  // Variables d'environnement héritées

void run_command(const char* program, char* const argv[]) {
    pid_t pid;
    
    // Attributs du nouveau processus
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);

    // Actions sur les file descriptors (optionnel)
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    // Lancer le processus
    int err = posix_spawnp(&pid, program, &actions, &attr, argv, environ);

    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&actions);

    if (err != 0) {
        std::println(stderr, "posix_spawnp failed: {}", strerror(err));
        return;
    }

    std::println("Lancé {} avec PID {}", program, pid);

    // Attendre la fin
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        std::println("{} terminé avec le code {}", program, WEXITSTATUS(status));
    }
}

int main() {
    char* args[] = {
        const_cast<char*>("ls"),
        const_cast<char*>("-la"),
        const_cast<char*>("/tmp"),
        nullptr
    };
    run_command("ls", args);
}
```

### Avantages de `posix_spawn`

- **Plus sûr** — Pas de fenêtre entre `fork` et `exec` où le code du parent tourne dans l'enfant.
- **Plus performant sur certains systèmes** — Peut utiliser `vfork` ou `clone` en interne plutôt qu'un `fork` complet.
- **Redirection de fichiers intégrée** — Les `file_actions` permettent de rediriger stdin/stdout/stderr de l'enfant sans manipuler les file descriptors manuellement.

### Redirections avec `posix_spawn`

```cpp
posix_spawn_file_actions_t actions;  
posix_spawn_file_actions_init(&actions);  

// Rediriger stdout de l'enfant vers un fichier
posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO,
    "output.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);

// Rediriger stderr vers le même fichier
posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);

// Fermer un fd que l'enfant ne devrait pas hériter
posix_spawn_file_actions_addclose(&actions, some_fd);

posix_spawnp(&pid, "my_program", &actions, &attr, argv, environ);

posix_spawn_file_actions_destroy(&actions);
```

---

## Wrapper RAII : la classe `Process`

Comme pour les sockets (chapitre 22), encapsulons la gestion de processus dans un wrapper RAII :

```cpp
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>
#include <optional>
#include <system_error>
#include <print>
#include <chrono>
#include <thread>

extern char** environ;

class Process {  
public:  
    struct Result {
        int exit_code;
        bool signaled;
        int signal_num;
    };

    // Lancer un processus
    Process(const std::string& program, std::vector<std::string> args) {
        // Construire le tableau argv C
        std::vector<char*> c_args;
        c_args.push_back(const_cast<char*>(program.c_str()));
        for (auto& a : args) {
            c_args.push_back(const_cast<char*>(a.c_str()));
        }
        c_args.push_back(nullptr);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);

        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);

        int err = posix_spawnp(&pid_, program.c_str(),
                               &actions, &attr,
                               c_args.data(), environ);

        posix_spawnattr_destroy(&attr);
        posix_spawn_file_actions_destroy(&actions);

        if (err != 0) {
            throw std::system_error(err, std::system_category(),
                                    "posix_spawnp(" + program + ")");
        }
    }

    // Non copiable
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    // Déplaçable
    Process(Process&& other) noexcept : pid_{other.pid_}, waited_{other.waited_} {
        other.pid_ = -1;
        other.waited_ = true;
    }

    Process& operator=(Process&& other) noexcept {
        if (this != &other) {
            try_wait();  // Récolter l'ancien processus si nécessaire
            pid_ = other.pid_;
            waited_ = other.waited_;
            other.pid_ = -1;
            other.waited_ = true;
        }
        return *this;
    }

    // Destructeur — récolte l'enfant pour éviter les zombies
    ~Process() {
        try_wait();
    }

    // PID de l'enfant
    [[nodiscard]] pid_t pid() const noexcept { return pid_; }

    // Attendre la fin (bloquant)
    Result wait() {
        if (waited_) {
            throw std::logic_error("Process already waited");
        }

        int status;
        if (waitpid(pid_, &status, 0) == -1) {
            throw std::system_error(errno, std::system_category(), "waitpid()");
        }

        waited_ = true;
        return decode_status(status);
    }

    // Vérifier si le processus a terminé (non-bloquant)
    std::optional<Result> try_get_result() {
        if (waited_) return std::nullopt;

        int status;
        pid_t result = waitpid(pid_, &status, WNOHANG);

        if (result == 0) return std::nullopt;  // Encore en cours

        if (result == -1) {
            if (errno == ECHILD) {
                waited_ = true;
                return std::nullopt;
            }
            throw std::system_error(errno, std::system_category(), "waitpid()");
        }

        waited_ = true;
        return decode_status(status);
    }

    // Attendre avec timeout
    std::optional<Result> wait_for(std::chrono::milliseconds timeout) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        constexpr auto poll_interval = std::chrono::milliseconds(10);

        while (std::chrono::steady_clock::now() < deadline) {
            if (auto r = try_get_result()) {
                return r;
            }
            std::this_thread::sleep_for(poll_interval);
        }

        return std::nullopt;  // Timeout
    }

    // Envoyer un signal
    void send_signal(int sig) {
        if (!waited_ && pid_ > 0) {
            if (kill(pid_, sig) == -1 && errno != ESRCH) {
                throw std::system_error(errno, std::system_category(), "kill()");
            }
        }
    }

    // Demander un arrêt propre
    void terminate() { send_signal(SIGTERM); }

    // Forcer l'arrêt
    void force_kill() { send_signal(SIGKILL); }

private:
    void try_wait() {
        if (!waited_ && pid_ > 0) {
            int status;
            pid_t r = waitpid(pid_, &status, WNOHANG);
            if (r == 0) {
                kill(pid_, SIGTERM);
                waitpid(pid_, &status, 0);
            }
            waited_ = true;
        }
    }

    static Result decode_status(int status) {
        if (WIFEXITED(status)) {
            return {WEXITSTATUS(status), false, 0};
        }
        if (WIFSIGNALED(status)) {
            return {-1, true, WTERMSIG(status)};
        }
        return {-1, false, 0};
    }

    pid_t pid_ = -1;
    bool waited_ = false;
};
```

### Utilisation

```cpp
int main() {
    // Lancer une commande
    Process gcc("g++", {"-std=c++20", "-o", "main", "main.cpp"});
    std::println("Compilation lancée (PID {})", gcc.pid());

    auto result = gcc.wait();
    if (result.exit_code == 0) {
        std::println("Compilation réussie");
    } else {
        std::println("Compilation échouée (code {})", result.exit_code);
    }

    // Lancer avec timeout
    Process slow("sleep", {"30"});
    auto r = slow.wait_for(std::chrono::seconds(2));

    if (!r) {
        std::println("Timeout — envoi de SIGTERM");
        slow.terminate();
        slow.wait();  // Attendre la terminaison après le signal
    }
}
```

Ce wrapper gère le cycle de vie complet : lancement via `posix_spawn`, attente bloquante ou avec timeout, polling non-bloquant, envoi de signaux, et récolte automatique dans le destructeur (pas de zombie). Le pattern est similaire au wrapper `Socket` du chapitre 22.

---

## Capturer la sortie d'un processus

Un besoin extrêmement courant : lancer un processus et récupérer sa sortie standard (stdout) et/ou sa sortie d'erreur (stderr) dans une chaîne. Cela combine `fork`/`exec` (ou `posix_spawn`) avec les **pipes** (section 23.2) :

```cpp
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#include <fcntl.h>
#include <array>
#include <string>
#include <vector>

extern char** environ;

struct CommandResult {
    std::string output;
    int exit_code;
};

CommandResult run_and_capture(const std::string& program,
                               std::vector<std::string> args) {
    // Créer un pipe : pipe_fds[0] = lecture, pipe_fds[1] = écriture
    int pipe_fds[2];
    if (pipe2(pipe_fds, O_CLOEXEC) == -1) {
        throw std::system_error(errno, std::system_category(), "pipe2()");
    }

    // Configurer posix_spawn pour rediriger stdout vers le pipe
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipe_fds[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipe_fds[0]);
    posix_spawn_file_actions_addclose(&actions, pipe_fds[1]);

    // Construire argv
    std::vector<char*> c_args;
    c_args.push_back(const_cast<char*>(program.c_str()));
    for (auto& a : args) c_args.push_back(const_cast<char*>(a.c_str()));
    c_args.push_back(nullptr);

    pid_t pid;
    int err = posix_spawnp(&pid, program.c_str(),
                           &actions, nullptr, c_args.data(), environ);
    posix_spawn_file_actions_destroy(&actions);

    // Fermer le côté écriture du pipe dans le parent
    close(pipe_fds[1]);

    if (err != 0) {
        close(pipe_fds[0]);
        throw std::system_error(err, std::system_category(), "posix_spawnp");
    }

    // Lire toute la sortie de l'enfant
    std::string output;
    std::array<char, 4096> buffer;
    ssize_t n;
    while ((n = read(pipe_fds[0], buffer.data(), buffer.size())) > 0) {
        output.append(buffer.data(), static_cast<size_t>(n));
    }
    close(pipe_fds[0]);

    // Attendre la fin du processus
    int status;
    waitpid(pid, &status, 0);

    int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return {std::move(output), code};
}
```

Utilisation :

```cpp
int main() {
    auto [output, code] = run_and_capture("uname", {"-a"});

    if (code == 0) {
        std::println("Système : {}", output);
    }

    auto [files, rc] = run_and_capture("ls", {"-1", "/tmp"});
    std::println("Fichiers dans /tmp ({} lignes):",
                 std::count(files.begin(), files.end(), '\n'));
    std::print("{}", files);
}
```

> 💡 Ce pattern `pipe` + `posix_spawn` + `read` + `waitpid` est tellement courant que la libc fournit `popen()` / `pclose()` qui fait la même chose en une ligne. Mais `popen` est limité (pas de contrôle sur stderr, pas de timeout, pas d'accès au PID), et l'implémentation manuelle est nécessaire dès que vos besoins dépassent le cas trivial.

---

## Bonnes pratiques

### Toujours récolter les enfants

Chaque `fork()` ou `posix_spawn()` doit être suivi d'un `waitpid()` — soit bloquant, soit via un handler `SIGCHLD`, soit via `SIG_IGN`. Il n'y a pas d'exception.

### Fermer les file descriptors inutiles

Après un `fork()`, l'enfant hérite de **tous** les file descriptors du parent. Un enfant qui garde ouverts des sockets ou des fichiers du parent peut provoquer des fuites de ressources et des comportements inattendus (un serveur qui ne libère pas le port parce qu'un enfant a encore le fd ouvert). Utilisez `O_CLOEXEC` / `SOCK_CLOEXEC` systématiquement (comme recommandé au chapitre 22), ou fermez manuellement les fd inutiles après le fork.

### Préférer `posix_spawn` à `fork` + `exec`

Sauf si vous avez besoin de manipulations complexes entre `fork` et `exec` (changement de répertoire, modification de variables d'environnement dynamiques, setup de namespaces), `posix_spawn` est plus sûr, plus concis, et potentiellement plus performant.

### Gérer les timeouts

Un processus enfant qui ne termine jamais (boucle infinie, deadlock, attente réseau) bloque votre programme si vous faites un `waitpid` sans timeout. Implémentez toujours un mécanisme de timeout (polling avec `WNOHANG`, ou `signalfd` + `epoll`) pour les processus dont vous ne contrôlez pas le code.

### Vérifier les codes de sortie

Le code de sortie d'un processus est une information critique. Un `exit(0)` signifie succès, tout autre valeur est un échec. Les conventions courantes :

| Code | Signification |
|------|---------------|
| `0` | Succès |
| `1` | Erreur générique |
| `2` | Mauvais usage (arguments invalides) |
| `126` | Commande trouvée mais pas exécutable |
| `127` | Commande non trouvée |
| `128 + N` | Tué par le signal N (convention bash) |

---

## Résumé

La gestion de processus sous Linux repose sur un modèle simple mais puissant :

- **`fork()`** clone le processus. L'enfant et le parent se distinguent par la valeur de retour. Le copy-on-write rend l'opération efficace.
- **`exec()`** remplace l'image du processus par un nouveau programme. Ne retourne jamais en cas de succès. Utilisez `execvp` pour la recherche dans le PATH.
- **`waitpid()`** récupère le statut de l'enfant et libère son entrée dans la table des processus. Indispensable pour éviter les zombies.
- **`posix_spawn()`** combine fork + exec en un seul appel, plus sûr et plus concis. Recommandé pour la majorité des cas.
- **Les zombies** sont des enfants non récoltés. Trois solutions : `waitpid` explicite, handler `SIGCHLD`, ou `signal(SIGCHLD, SIG_IGN)`.
- **La capture de sortie** combine pipes et `posix_spawn` — un pattern fondamental pour les outils CLI et DevOps.

---

> **Prochaine étape** → Section 23.2 : Pipes et communication inter-processus — le mécanisme IPC le plus simple et le plus utilisé sous Linux.

⏭️ [Pipes et communication inter-processus](/23-processus-ipc/02-pipes.md)
