🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 19.2 — Appels système POSIX : open, read, write, close

## Module 7 — Programmation Système sur Linux *(Niveau Avancé)*

---

## Introduction

La section 19.1 a présenté `std::filesystem`, l'API C++17 qui gère l'arborescence et les métadonnées du système de fichiers. Mais `std::filesystem` ne lit ni n'écrit le **contenu** des fichiers. Pour cela, deux options existent : les flux C++ (`<fstream>`) et les **appels système POSIX**.

Les appels système POSIX — `open`, `read`, `write`, `close` et leurs compagnons — constituent l'interface directe entre un programme et le noyau Linux. Chaque opération d'I/O, quelle que soit l'abstraction utilisée (iostream, fstream, FILE*, ou même `std::filesystem::copy_file`), finit par descendre à ce niveau. Comprendre cette couche est indispensable pour plusieurs raisons :

- **Performance** — Les appels système offrent un contrôle total sur le buffering, la taille des lectures/écritures et les stratégies d'I/O. Quand les abstractions de haut niveau deviennent un goulot d'étranglement, c'est ici qu'on descend.
- **Fonctionnalités Linux-spécifiques** — `O_DIRECT`, `O_TMPFILE`, `sendfile`, `splice`, `inotify`, `epoll` n'ont pas d'équivalent dans la bibliothèque standard C++.
- **Interopérabilité** — De nombreuses bibliothèques C et systèmes existants travaillent avec des descripteurs de fichiers (file descriptors). Les manipuler est inévitable.
- **Compréhension** — Savoir ce qui se passe sous le capot rend le débogage des problèmes d'I/O incomparablement plus efficace.

---

## Le descripteur de fichier (file descriptor)

Le concept central des I/O POSIX est le **file descriptor** (fd) : un entier non négatif qui identifie une ressource d'I/O ouverte dans le processus. C'est un index dans la table des fichiers ouverts du processus, maintenue par le noyau.

Trois descripteurs sont ouverts automatiquement au démarrage de tout processus :

| fd | Nom | Constante | Description |
|---|---|---|---|
| 0 | stdin | `STDIN_FILENO` | Entrée standard |
| 1 | stdout | `STDOUT_FILENO` | Sortie standard |
| 2 | stderr | `STDERR_FILENO` | Sortie d'erreur |

Chaque appel à `open()` retourne le plus petit fd disponible. Les file descriptors sont locaux au processus : le fd 5 dans un processus n'a aucun rapport avec le fd 5 dans un autre.

---

## Les headers nécessaires

```cpp
#include <fcntl.h>      // open, O_RDONLY, O_WRONLY, O_CREAT, etc.
#include <unistd.h>      // read, write, close, lseek, fsync, ftruncate
#include <sys/stat.h>    // mode_t, S_IRUSR, S_IWUSR, etc.
#include <cerrno>        // errno
#include <cstring>       // strerror
```

Ces headers sont disponibles sur tout système POSIX. Sur Ubuntu, ils font partie de la libc standard (glibc), installée par défaut.

---

## `open()` : ouvrir ou créer un fichier

```cpp
#include <fcntl.h>

int open(const char* pathname, int flags);  
int open(const char* pathname, int flags, mode_t mode);  
```

`open()` retourne un file descriptor en cas de succès, ou `-1` en cas d'erreur (avec `errno` positionné).

### Flags d'accès (obligatoire, exactement un)

| Flag | Description |
|---|---|
| `O_RDONLY` | Lecture seule |
| `O_WRONLY` | Écriture seule |
| `O_RDWR` | Lecture et écriture |

### Flags de création et comportement (combinables avec `|`)

| Flag | Description |
|---|---|
| `O_CREAT` | Crée le fichier s'il n'existe pas (nécessite le paramètre `mode`) |
| `O_EXCL` | Avec `O_CREAT`, échoue si le fichier existe déjà (création exclusive) |
| `O_TRUNC` | Tronque le fichier à zéro octet s'il existe |
| `O_APPEND` | Écritures toujours à la fin du fichier |
| `O_CLOEXEC` | Ferme automatiquement le fd lors d'un `exec()` |
| `O_NONBLOCK` | Ouvre en mode non-bloquant |
| `O_SYNC` | Chaque `write()` attend l'écriture physique sur disque |
| `O_DIRECT` | Contourne le page cache du noyau (I/O direct) |
| `O_TMPFILE` | Crée un fichier temporaire sans nom dans le répertoire donné |

### Exemples d'ouverture

```cpp
#include <fcntl.h>
#include <sys/stat.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    // Lecture seule d'un fichier existant
    int fd1 = open("/etc/hostname", O_RDONLY);
    if (fd1 == -1) {
        std::println("Erreur : {}", strerror(errno));
        return 1;
    }

    // Création d'un fichier en écriture (tronque s'il existe)
    // Le mode 0644 = rw-r--r-- (modifié par le umask)
    int fd2 = open("/tmp/output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd2 == -1) {
        std::println("Erreur : {}", strerror(errno));
        close(fd1);
        return 1;
    }

    // Création exclusive — échoue si le fichier existe déjà
    int fd3 = open("/tmp/lockfile", O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd3 == -1 && errno == EEXIST) {
        std::println("Le fichier de verrouillage existe déjà");
    }

    // Ouverture en ajout (append) — les écritures vont toujours à la fin
    int fd4 = open("/tmp/app.log", O_WRONLY | O_CREAT | O_APPEND, 0644);

    // ... utilisation ...

    close(fd1);
    close(fd2);
    if (fd3 != -1) close(fd3);
    if (fd4 != -1) close(fd4);
}
```

### Le paramètre `mode` et le umask

Le paramètre `mode` est requis lorsque `O_CREAT` ou `O_TMPFILE` est utilisé. Il spécifie les permissions du fichier créé, mais les permissions effectives sont `mode & ~umask`. Avec un umask typique de `022` :

```
mode demandé :  0666 (rw-rw-rw-)  
umask :         0022  
permissions :   0644 (rw-r--r--)  
```

Les constantes symboliques de `<sys/stat.h>` sont souvent plus lisibles :

```cpp
// Équivalent de 0644
mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

// Équivalent de 0600 (lecture/écriture propriétaire uniquement)
mode_t secure = S_IRUSR | S_IWUSR;

// Équivalent de 0755 (exécutable)
mode_t exec_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
```

### `O_CLOEXEC` : un flag souvent oublié, rarement optionnel

`O_CLOEXEC` ferme automatiquement le descripteur lorsque le processus appelle `execve()` pour exécuter un autre programme. Sans ce flag, les descripteurs ouverts sont **hérités** par le processus enfant après un `fork()` + `exec()`, ce qui constitue à la fois une fuite de ressources et un risque de sécurité (le processus enfant peut accéder à des fichiers qu'il ne devrait pas voir).

La bonne pratique en 2026 est d'utiliser `O_CLOEXEC` **systématiquement**, sauf besoin explicite de transmettre un fd à un processus enfant :

```cpp
// ✅ Bonne pratique
int fd = open("/tmp/data.bin", O_RDONLY | O_CLOEXEC);

// ❌ Fuite potentielle de descripteur vers les processus enfants
int fd = open("/tmp/data.bin", O_RDONLY);
```

---

## `read()` : lire des données

```cpp
#include <unistd.h>

ssize_t read(int fd, void* buf, size_t count);
```

`read()` tente de lire jusqu'à `count` octets depuis le descripteur `fd` dans le buffer `buf`. La valeur de retour est :

| Retour | Signification |
|---|---|
| `> 0` | Nombre d'octets effectivement lus |
| `0` | Fin de fichier (EOF) atteinte |
| `-1` | Erreur (consulter `errno`) |

### Lecture de base

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <array>
#include <cerrno>
#include <cstring>

int main() {
    int fd = open("/etc/hostname", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return 1;
    }

    std::array<char, 256> buf{};
    ssize_t n = read(fd, buf.data(), buf.size() - 1);  // -1 pour le '\0'

    if (n == -1) {
        std::println("read: {}", strerror(errno));
    } else {
        buf[static_cast<size_t>(n)] = '\0';
        std::println("Contenu ({} octets) : {}", n, buf.data());
    }

    close(fd);
}
```

### Le piège fondamental : les lectures partielles

`read()` n'est **pas** obligé de lire la totalité des `count` octets demandés. Même si le fichier contient suffisamment de données, `read()` peut retourner moins d'octets pour de multiples raisons : buffer interne du noyau, signal interrompu, ou simplement parce que le filesystem a décidé de ne pas charger plus de données en une seule fois.

C'est la source d'erreur numéro un chez les développeurs qui découvrent les I/O POSIX. La solution est une boucle de lecture complète :

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <print>

// Lecture complète : lit exactement count octets ou jusqu'à EOF
auto read_full(int fd, void* buf, size_t count) -> ssize_t {
    auto* ptr = static_cast<uint8_t*>(buf);
    size_t total = 0;

    while (total < count) {
        ssize_t n = read(fd, ptr + total, count - total);

        if (n == -1) {
            if (errno == EINTR) {
                continue;  // Interrompu par un signal, réessayer
            }
            return -1;  // Vraie erreur
        }
        if (n == 0) {
            break;  // EOF atteint
        }

        total += static_cast<size_t>(n);
    }

    return static_cast<ssize_t>(total);
}

int main() {
    int fd = open("/etc/passwd", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return 1;
    }

    char buf[4096];
    ssize_t n = read_full(fd, buf, sizeof(buf) - 1);
    if (n >= 0) {
        buf[n] = '\0';
        std::println("Lu {} octets", n);
    }

    close(fd);
}
```

Le test `errno == EINTR` est essentiel : sur Linux, un `read()` peut être interrompu par un signal (par exemple `SIGCHLD` ou un timer). Dans ce cas, il retourne `-1` avec `errno == EINTR`, et il suffit de réessayer. Ignorer cette condition provoque des pertes de données intermittentes et difficiles à reproduire.

---

## `write()` : écrire des données

```cpp
#include <unistd.h>

ssize_t write(int fd, const void* buf, size_t count);
```

`write()` tente d'écrire `count` octets depuis `buf` vers le descripteur `fd`. Comme `read()`, la valeur de retour indique le nombre d'octets effectivement écrits.

### Écriture de base

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cstring>
#include <cerrno>

int main() {
    int fd = open("/tmp/output.txt", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return 1;
    }

    const char* message = "Hello from POSIX write!\n";
    ssize_t n = write(fd, message, strlen(message));

    if (n == -1) {
        std::println("write: {}", strerror(errno));
    } else {
        std::println("{} octets écrits", n);
    }

    close(fd);
}
```

### Écritures partielles

Comme `read()`, `write()` peut écrire moins d'octets que demandé. C'est particulièrement fréquent avec les sockets et les pipes, mais cela peut aussi arriver avec des fichiers réguliers (signal `EINTR`, espace disque limité). Une boucle d'écriture complète est donc nécessaire :

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <print>

// Écriture complète : écrit exactement count octets ou échoue
auto write_full(int fd, const void* buf, size_t count) -> ssize_t {
    const auto* ptr = static_cast<const uint8_t*>(buf);
    size_t total = 0;

    while (total < count) {
        ssize_t n = write(fd, ptr + total, count - total);

        if (n == -1) {
            if (errno == EINTR) {
                continue;  // Signal, réessayer
            }
            return -1;  // Vraie erreur
        }

        total += static_cast<size_t>(n);
    }

    return static_cast<ssize_t>(total);
}

int main() {
    int fd = open("/tmp/data.bin", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return 1;
    }

    // Écrire un bloc de données
    constexpr size_t size = 1024 * 1024;  // 1 Mo
    auto data = std::make_unique<char[]>(size);
    std::memset(data.get(), 'A', size);

    ssize_t n = write_full(fd, data.get(), size);
    if (n == -1) {
        std::println("write_full: {}", strerror(errno));
    } else {
        std::println("{} octets écrits", n);
    }

    close(fd);
}
```

---

## `close()` : fermer un descripteur

```cpp
#include <unistd.h>

int close(int fd);
```

`close()` libère le descripteur de fichier. Les données bufferisées par le noyau sont normalement flushées, mais cette opération n'est **pas** garantie d'être synchrone. `close()` retourne `0` en cas de succès, `-1` en cas d'erreur.

### Pourquoi vérifier le retour de `close()`

La plupart des développeurs ignorent la valeur de retour de `close()`. C'est une erreur : sur certains systèmes de fichiers réseau (NFS, CIFS), `close()` est le moment où les erreurs d'écriture différées sont signalées. Un `close()` qui échoue peut signifier que les données écrites n'ont jamais atteint le disque :

```cpp
if (close(fd) == -1) {
    std::println("close: {} — les données ont pu être perdues !", strerror(errno));
}
```

En pratique, la gestion d'un `close()` échoué est complexe (on ne peut pas réessayer : le fd est invalide après un `close()`, même en cas d'erreur). La stratégie la plus courante est de journaliser l'erreur. Pour les fichiers critiques, un `fsync()` avant le `close()` permet de détecter les erreurs d'écriture à un moment où elles sont encore rattrapables.

---

## `lseek()` : positionner le curseur

```cpp
#include <unistd.h>

off_t lseek(int fd, off_t offset, int whence);
```

`lseek()` repositionne le curseur de lecture/écriture dans le fichier. Le paramètre `whence` détermine le point de référence :

| `whence` | Signification |
|---|---|
| `SEEK_SET` | Depuis le début du fichier |
| `SEEK_CUR` | Depuis la position courante |
| `SEEK_END` | Depuis la fin du fichier |

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    int fd = open("/tmp/data.bin", O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (fd == -1) return 1;

    // Écrire des données
    const char* msg = "ABCDEFGHIJ";
    write(fd, msg, 10);

    // Revenir au début
    lseek(fd, 0, SEEK_SET);

    // Lire les 5 premiers octets
    char buf[6] = {};
    read(fd, buf, 5);
    std::println("5 premiers : {}", buf);  // ABCDE

    // Se positionner à l'offset 3 depuis le début
    lseek(fd, 3, SEEK_SET);
    read(fd, buf, 5);
    buf[5] = '\0';
    std::println("Depuis offset 3 : {}", buf);  // DEFGH

    // Connaître la position courante
    off_t pos = lseek(fd, 0, SEEK_CUR);
    std::println("Position courante : {}", pos);  // 8

    // Connaître la taille du fichier
    off_t size = lseek(fd, 0, SEEK_END);
    std::println("Taille du fichier : {}", size);  // 10

    close(fd);
}
```

### Fichiers creux (sparse files)

`lseek()` permet de créer des **fichiers creux** en se positionnant au-delà de la fin du fichier avant d'écrire. Les octets entre l'ancienne fin et la nouvelle position ne consomment pas d'espace disque réel :

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <print>

int main() {
    int fd = open("/tmp/sparse.bin", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) return 1;

    // Écrire 1 octet au début
    write(fd, "A", 1);

    // Se positionner à 1 Go
    lseek(fd, 1L * 1024 * 1024 * 1024, SEEK_SET);

    // Écrire 1 octet à 1 Go
    write(fd, "B", 1);

    close(fd);

    // Le fichier fait 1 Go + 1 octet en taille apparente,
    // mais ne consomme que 2 blocs de 4 Ko sur le disque
    // Vérifiable avec : ls -l (taille) vs du -h (espace réel)
}
```

Les fichiers creux sont utilisés par les machines virtuelles (images disque), les bases de données et les systèmes de log pour pré-allouer de l'espace sans consommer de stockage réel.

---

## `fsync()` et `fdatasync()` : garantir la persistance

Par défaut, `write()` copie les données dans le **page cache** du noyau, pas directement sur le disque. Le noyau écrit les pages sales sur le disque de manière asynchrone (via le mécanisme de writeback). Cela signifie qu'après un `write()` réussi, une coupure de courant peut entraîner la perte des données.

Pour garantir que les données sont physiquement sur le support de stockage, il faut appeler `fsync()` ou `fdatasync()` :

```cpp
#include <unistd.h>

int fsync(int fd);       // Flush données + métadonnées (taille, timestamps)  
int fdatasync(int fd);   // Flush données uniquement (plus rapide)  
```

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>

void write_durable(const char* path, const void* data, size_t len) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return;
    }

    // Écrire les données
    const auto* ptr = static_cast<const char*>(data);
    size_t total = 0;
    while (total < len) {
        ssize_t n = write(fd, ptr + total, len - total);
        if (n == -1) {
            if (errno == EINTR) continue;
            std::println("write: {}", strerror(errno));
            close(fd);
            return;
        }
        total += static_cast<size_t>(n);
    }

    // Forcer l'écriture sur le disque
    if (fsync(fd) == -1) {
        std::println("fsync: {}", strerror(errno));
    }

    if (close(fd) == -1) {
        std::println("close: {}", strerror(errno));
    }
}
```

### `fsync()` vs `fdatasync()`

`fsync()` synchronise les données **et** les métadonnées du fichier (taille, timestamps, permissions). `fdatasync()` ne synchronise que les données et les métadonnées strictement nécessaires à la récupération des données (typiquement la taille du fichier, mais pas les timestamps).

`fdatasync()` est donc plus rapide quand seule l'intégrité des données importe (pas besoin que la date de modification soit exacte après un crash). En pratique, la différence de performance est mesurable sur les systèmes avec beaucoup d'écritures concurrentes.

### Écriture atomique POSIX complète

La section 19.1.3 a présenté le pattern write-then-rename avec `std::filesystem`. Voici la version POSIX complète avec `fsync()`, qui offre les garanties de durabilité les plus fortes :

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <print>
#include <cerrno>
#include <cstring>
#include <string>

namespace fs = std::filesystem;

bool atomic_write_durable(const fs::path& target, const std::string& content) {
    fs::path tmp = target;
    tmp += ".tmp";

    // 1. Ouvrir le fichier temporaire
    int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return false;
    }

    // 2. Écriture complète
    const char* ptr = content.data();
    size_t remaining = content.size();
    while (remaining > 0) {
        ssize_t n = write(fd, ptr, remaining);
        if (n == -1) {
            if (errno == EINTR) continue;
            std::println("write: {}", strerror(errno));
            close(fd);
            fs::remove(tmp);
            return false;
        }
        ptr += n;
        remaining -= static_cast<size_t>(n);
    }

    // 3. Forcer sur le disque AVANT le rename
    if (fsync(fd) == -1) {
        std::println("fsync: {}", strerror(errno));
        close(fd);
        fs::remove(tmp);
        return false;
    }

    // 4. Fermer
    if (close(fd) == -1) {
        std::println("close: {}", strerror(errno));
        // fd est invalide même en cas d'erreur, on ne retry pas
    }

    // 5. Renommage atomique
    fs::rename(tmp, target);

    // 6. (Optionnel) fsync du répertoire parent pour garantir
    // que l'entrée de répertoire est aussi persistée
    int dir_fd = open(target.parent_path().c_str(), O_RDONLY | O_CLOEXEC);
    if (dir_fd != -1) {
        fsync(dir_fd);
        close(dir_fd);
    }

    return true;
}
```

L'étape 6 (`fsync` du répertoire parent) est souvent omise mais importante pour une durabilité complète : le `rename()` modifie les entrées du répertoire, et sans `fsync` du répertoire, cette modification peut être perdue lors d'un crash. Les bases de données et les systèmes de fichiers journalisés (SQLite, PostgreSQL, etc.) font systématiquement ce `fsync` supplémentaire.

---

## Wrappers RAII pour les descripteurs

L'utilisation directe de `open`/`close` est fragile : chaque chemin de retour anticipé (erreur, exception) doit fermer le descripteur. Le principe RAII (section 6.3) s'applique naturellement :

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>
#include <utility>

class FileDescriptor {  
public:  
    FileDescriptor() noexcept = default;

    explicit FileDescriptor(int fd) noexcept : fd_(fd) {}

    // Ouverture directe
    FileDescriptor(const char* path, int flags, mode_t mode = 0)
        : fd_(open(path, flags, mode)) {}

    ~FileDescriptor() { close_if_open(); }

    // Non copiable
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    // Déplaçable
    FileDescriptor(FileDescriptor&& other) noexcept
        : fd_(std::exchange(other.fd_, -1)) {}

    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) {
            close_if_open();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    [[nodiscard]] bool is_open() const noexcept { return fd_ >= 0; }
    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] explicit operator bool() const noexcept { return is_open(); }

    // Libérer le fd sans le fermer (pour transfert à une API externe)
    int release() noexcept { return std::exchange(fd_, -1); }

private:
    void close_if_open() noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    int fd_ = -1;
};
```

Utilisation :

```cpp
#include <print>
#include <cerrno>
#include <cstring>

void process_file(const char* path) {
    FileDescriptor fd(path, O_RDONLY | O_CLOEXEC);
    if (!fd) {
        std::println("open: {}", strerror(errno));
        return;  // Pas besoin de close() — fd n'est pas ouvert
    }

    char buf[4096];
    ssize_t n = read(fd.get(), buf, sizeof(buf));
    if (n == -1) {
        std::println("read: {}", strerror(errno));
        return;  // Le destructeur de FileDescriptor appelle close()
    }

    std::println("Lu {} octets depuis {}", n, path);
    // close() automatique à la sortie de la fonction
}
```

Cette classe suit la Rule of Five (section 6.5) : destructeur personnalisé, copie supprimée, déplacement implémenté avec `std::exchange`. Le pattern est identique conceptuellement à `std::unique_ptr` avec un custom deleter, mais avec une sémantique spécifique aux file descriptors (la valeur invalide est `-1`, pas `nullptr`).

---

## `O_TMPFILE` : fichiers temporaires anonymes

Disponible depuis Linux 3.11, `O_TMPFILE` crée un fichier temporaire **sans nom** dans un répertoire donné. Le fichier n'a pas d'entrée dans le répertoire : il est invisible et sera automatiquement supprimé à la fermeture du dernier descripteur qui le référence.

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    // Crée un fichier temporaire anonyme dans /tmp
    // Le deuxième argument est le mode (permissions si le fichier est lié plus tard)
    int fd = open("/tmp", O_TMPFILE | O_RDWR | O_CLOEXEC, 0644);
    if (fd == -1) {
        std::println("O_TMPFILE: {}", strerror(errno));
        return 1;
    }

    // Écrire des données — le fichier n'est visible nulle part dans /tmp
    const char* data = "données temporaires sécurisées\n";
    write(fd, data, strlen(data));

    // Relire
    lseek(fd, 0, SEEK_SET);
    char buf[256] = {};
    read(fd, buf, sizeof(buf) - 1);
    std::println("Contenu : {}", buf);

    close(fd);  // Le fichier est automatiquement supprimé
}
```

Les avantages de `O_TMPFILE` par rapport à la création classique d'un fichier temporaire sont :

- **Sécurité** — Pas de race condition entre la vérification de nom et la création (attaque TOCTOU).
- **Nettoyage garanti** — Le fichier est supprimé même si le programme crash.
- **Pas de pollution** — Aucun fichier orphelin ne reste en cas de terminaison anormale.

`O_TMPFILE` est le mécanisme privilégié pour les fichiers temporaires en C++ système sur Linux moderne. Notez qu'il n'est pas supporté par tous les systèmes de fichiers : ext4, XFS et Btrfs le supportent ; tmpfs le supporte depuis Linux 3.15.

---

## `stat()` et `fstat()` : métadonnées de fichiers

Bien que `std::filesystem` expose la plupart des métadonnées, l'appel système `stat()` reste nécessaire pour accéder à certaines informations bas niveau :

```cpp
#include <sys/stat.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    struct stat st;

    if (stat("/etc/hostname", &st) == -1) {
        std::println("stat: {}", strerror(errno));
        return 1;
    }

    std::println("Taille        : {} octets", st.st_size);
    std::println("Inode         : {}", st.st_ino);
    std::println("Device        : {}", st.st_dev);
    std::println("Hard links    : {}", st.st_nlink);
    std::println("UID           : {}", st.st_uid);
    std::println("GID           : {}", st.st_gid);
    std::println("Blocs (512o)  : {}", st.st_blocks);
    std::println("Taille bloc   : {}", st.st_blksize);

    // Type de fichier
    if (S_ISREG(st.st_mode))  std::println("Type : fichier régulier");
    if (S_ISDIR(st.st_mode))  std::println("Type : répertoire");
    if (S_ISLNK(st.st_mode))  std::println("Type : lien symbolique");
    if (S_ISFIFO(st.st_mode)) std::println("Type : pipe nommé");
    if (S_ISSOCK(st.st_mode)) std::println("Type : socket");
}
```

`fstat()` fait la même chose mais sur un descripteur ouvert au lieu d'un chemin. C'est plus efficace quand on a déjà un fd, et c'est insensible aux race conditions (le fichier ne peut pas être remplacé entre l'ouverture et le `fstat`).

`lstat()` est la variante qui ne suit **pas** les liens symboliques : elle retourne les informations du lien lui-même, pas de sa cible. C'est l'équivalent de `fs::symlink_status()`.

---

## Lecture et écriture efficaces : choix de la taille du buffer

La taille du buffer passé à `read()`/`write()` a un impact direct sur la performance. Chaque appel système a un coût fixe (passage en mode noyau, copie de données), donc des lectures/écritures trop petites multiplient ce coût.

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <print>
#include <cerrno>
#include <cstring>
#include <array>
#include <cstdint>

auto copy_file_posix(const char* src, const char* dst) -> bool {
    int fd_in = open(src, O_RDONLY | O_CLOEXEC);
    if (fd_in == -1) return false;

    int fd_out = open(dst, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd_out == -1) {
        close(fd_in);
        return false;
    }

    // Utiliser la taille de bloc optimale du filesystem
    struct stat st;
    size_t buf_size = 65536;  // 64 Ko par défaut
    if (fstat(fd_in, &st) == 0 && st.st_blksize > 0) {
        buf_size = static_cast<size_t>(st.st_blksize);
        if (buf_size < 4096) buf_size = 4096;     // Plancher
        if (buf_size > 1048576) buf_size = 1048576; // Plafond 1 Mo
    }

    auto buf = std::make_unique<char[]>(buf_size);
    uint64_t total = 0;

    while (true) {
        ssize_t n = read(fd_in, buf.get(), buf_size);
        if (n == -1) {
            if (errno == EINTR) continue;
            std::println("read: {}", strerror(errno));
            close(fd_in);
            close(fd_out);
            return false;
        }
        if (n == 0) break;  // EOF

        const char* ptr = buf.get();
        size_t remaining = static_cast<size_t>(n);
        while (remaining > 0) {
            ssize_t w = write(fd_out, ptr, remaining);
            if (w == -1) {
                if (errno == EINTR) continue;
                std::println("write: {}", strerror(errno));
                close(fd_in);
                close(fd_out);
                return false;
            }
            ptr += w;
            remaining -= static_cast<size_t>(w);
        }

        total += static_cast<uint64_t>(n);
    }

    close(fd_in);
    close(fd_out);
    std::println("Copié {} octets", total);
    return true;
}
```

Les tailles de buffer typiques et leurs caractéristiques :

| Taille | Usage |
|---|---|
| 512 o – 1 Ko | Trop petit, trop d'appels système |
| 4 Ko | Taille de page mémoire Linux, minimum raisonnable |
| 8 Ko – 64 Ko | Bon compromis pour la plupart des cas |
| 128 Ko – 1 Mo | Optimal pour les grosses copies séquentielles |
| > 1 Mo | Rendements décroissants, consommation mémoire accrue |

La valeur `st_blksize` retournée par `fstat()` est la taille de bloc recommandée par le filesystem pour des I/O efficaces. C'est un bon point de départ, mais pour les transferts de gros volumes, des buffers de 64 Ko à 256 Ko offrent généralement les meilleures performances sur les disques SSD modernes.

---

## Résumé des appels système

| Appel | Signature simplifiée | Description |
|---|---|---|
| `open` | `int open(path, flags, mode)` | Ouvrir / créer un fichier |
| `read` | `ssize_t read(fd, buf, count)` | Lire des octets |
| `write` | `ssize_t write(fd, buf, count)` | Écrire des octets |
| `close` | `int close(fd)` | Fermer un descripteur |
| `lseek` | `off_t lseek(fd, offset, whence)` | Positionner le curseur |
| `fsync` | `int fsync(fd)` | Forcer l'écriture sur disque (données + métadonnées) |
| `fdatasync` | `int fdatasync(fd)` | Forcer l'écriture sur disque (données uniquement) |
| `ftruncate` | `int ftruncate(fd, length)` | Tronquer / étendre un fichier |
| `stat` | `int stat(path, &st)` | Métadonnées par chemin (suit les symlinks) |
| `lstat` | `int lstat(path, &st)` | Métadonnées par chemin (ne suit pas les symlinks) |
| `fstat` | `int fstat(fd, &st)` | Métadonnées par descripteur |

---

> 💡 **Note** — Les appels système POSIX sont l'interface la plus basse qu'un programme en espace utilisateur peut utiliser pour interagir avec le noyau. Ils n'offrent aucune abstraction protectrice : pas de RAII, pas de gestion d'exceptions, pas de type-safety. Chaque appel peut échouer, chaque valeur de retour doit être vérifiée, chaque buffer doit être correctement dimensionné. Cette rigueur est le prix du contrôle total. La bonne nouvelle est que les wrappers RAII présentés dans cette section (et les smart pointers du chapitre 9) permettent d'encapsuler cette complexité pour obtenir le meilleur des deux mondes : la performance et le contrôle du POSIX avec la sûreté du C++ moderne.

⏭️ [Comparaison : API C++ vs API système](/19-systeme-fichiers/03-comparaison-api.md)
