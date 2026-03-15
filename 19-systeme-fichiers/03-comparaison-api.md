🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 19.3 — Comparaison : API C++ vs API système

## Module 7 — Programmation Système sur Linux *(Niveau Avancé)*

---

## Introduction

Les sections 19.1 et 19.2 ont présenté deux couches distinctes pour interagir avec le système de fichiers sur Linux : l'API C++ moderne (`std::filesystem` + `<fstream>`) et les appels système POSIX (`open`, `read`, `write`, `close` et compagnons). Ces deux couches ne sont pas des alternatives exclusives — elles coexistent naturellement dans un même projet, chacune ayant ses forces et ses limites.

Cette section propose une comparaison systématique pour vous aider à faire le bon choix selon le contexte. L'objectif n'est pas de déclarer un vainqueur, mais de construire une grille de décision claire que vous pourrez appliquer dans vos projets.

---

## Vue d'ensemble des couches d'I/O sur Linux

Un programme C++ sur Linux dispose en réalité de quatre couches pour les I/O fichier, chacune construite sur la précédente :

```
┌─────────────────────────────────────────────────┐
│  std::filesystem + std::fstream    (C++17)      │  ← Abstraction la plus haute
├─────────────────────────────────────────────────┤
│  std::FILE* (fopen, fread, fwrite)  (C stdio)   │
├─────────────────────────────────────────────────┤
│  Appels système POSIX (open, read, write)       │  ← Interface noyau
├─────────────────────────────────────────────────┤
│  Noyau Linux (VFS → ext4/XFS/Btrfs → driver)    │
└─────────────────────────────────────────────────┘
```

`std::fstream` et `std::FILE*` ajoutent un **buffering en espace utilisateur** par-dessus les appels système. Cela signifie que les petites lectures/écritures sont regroupées avant de traverser la frontière noyau, ce qui améliore les performances pour les patterns d'accès typiques (lecture ligne par ligne, écritures fréquentes de petits messages).

`std::filesystem`, lui, n'effectue pas d'I/O sur le contenu des fichiers : il gère l'arborescence et les métadonnées en appelant directement les syscalls correspondants (`stat`, `rename`, `unlink`, `mkdir`, etc.).

---

## Comparaison détaillée

### Gestion de l'arborescence (métadonnées, parcours, opérations)

| Critère | `std::filesystem` | POSIX (`stat`, `opendir`, `mkdir`…) |
|---|---|---|
| **Portabilité** | Windows, macOS, Linux | Unix/Linux uniquement |
| **Type-safety** | `fs::path`, types fortement typés | `const char*`, entiers bruts |
| **Gestion d'erreurs** | Exceptions ou `std::error_code` | `errno` global, retour `-1` |
| **Parcours de répertoires** | `directory_iterator`, `recursive_directory_iterator` | `opendir`/`readdir`/`closedir` |
| **Normalisation de chemins** | `lexically_normal()`, `canonical()` | `realpath()` uniquement |
| **Liens symboliques** | API explicite (`symlink_status`, `read_symlink`) | `lstat()`, `readlink()` |
| **Permissions** | `fs::permissions()`, `fs::perms` | `chmod()`, `chown()`, constantes S_I* |
| **ACL, xattr, inotify** | Non supporté | Supporté via headers dédiés |
| **Espace disque** | `fs::space()` | `statvfs()` |

**Verdict** — Pour la gestion d'arborescence, `std::filesystem` est le choix par défaut. Il offre une API plus sûre, plus lisible et portable. Les appels POSIX ne sont nécessaires que pour les fonctionnalités non couvertes : ACL, attributs étendus (xattr), surveillance de fichiers (inotify), ou contrôle fin des permissions (chown avec UID/GID numériques).

### Lecture et écriture de contenu

| Critère | `std::fstream` | POSIX (`open`/`read`/`write`) |
|---|---|---|
| **Buffering** | Automatique (espace utilisateur) | Aucun (uniquement page cache noyau) |
| **Formatage** | `<<`, `>>`, `std::getline` | Aucun (octets bruts) |
| **Performance brute** | Bon pour les petites I/O | Supérieur pour les grosses I/O |
| **Contrôle du buffer** | Limité (`rdbuf()->pubsetbuf`) | Total (taille arbitraire) |
| **`fsync` / durabilité** | Pas d'accès direct | `fsync()`, `fdatasync()` |
| **I/O non-bloquantes** | Non supporté | `O_NONBLOCK`, `poll`, `epoll` |
| **I/O directes (bypass cache)** | Non supporté | `O_DIRECT` |
| **Multiplexage** | Non supporté | `select`, `poll`, `epoll` |
| **Scatter/Gather I/O** | Non supporté | `readv()`, `writev()` |
| **Memory-mapped I/O** | Non supporté | `mmap()` |
| **Fichiers temporaires anonymes** | Non supporté | `O_TMPFILE` |
| **Transfert zero-copy** | Non supporté | `sendfile()`, `splice()` |

**Verdict** — `std::fstream` convient pour les I/O textuelles, la lecture de fichiers de configuration et les écritures modérées. Les appels POSIX sont nécessaires dès que la performance est critique, que le contrôle fin du comportement d'I/O est requis, ou que des fonctionnalités Linux-spécifiques sont utilisées.

---

## Scénarios de décision

### Scénario 1 : Lire un fichier de configuration

Un fichier de quelques Ko, lu une seule fois au démarrage.

```cpp
// ✅ std::fstream — simple, lisible, suffisant
#include <fstream>
#include <string>
#include <sstream>
#include <print>

std::string read_config(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Impossible d'ouvrir " + path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
```

Le POSIX n'apporte aucun avantage ici. Le fichier est petit, lu une seule fois, et le buffering de `fstream` est parfaitement adapté.

### Scénario 2 : Copier un fichier volumineux

Un fichier de plusieurs Go transféré entre deux répertoires.

```cpp
// ✅ std::filesystem — une seule ligne, optimisé en interne
namespace fs = std::filesystem;  
fs::copy_file(source, dest, fs::copy_options::overwrite_existing);  
```

`fs::copy_file()` est le meilleur choix ici. Sur Linux, les implémentations modernes de la libc (glibc 2.27+) utilisent `copy_file_range()` sous le capot, un appel système qui permet au noyau d'effectuer la copie directement, potentiellement sans transférer les données en espace utilisateur (zero-copy entre fichiers sur le même filesystem).

Un `read()`/`write()` POSIX manuel serait **moins performant** que `fs::copy_file()` dans ce cas, sauf si vous avez besoin de transformer les données pendant le transfert.

### Scénario 3 : Écrire un fichier critique (base de données, configuration persistante)

Un fichier dont la perte ou la corruption est inacceptable.

```cpp
// ✅ POSIX — fsync() est indispensable pour la garantie de durabilité
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>

namespace fs = std::filesystem;

bool write_critical(const fs::path& target, const std::string& data) {
    fs::path tmp = target;
    tmp += ".tmp";

    int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) return false;

    // Écriture complète (boucle write)
    const char* ptr = data.data();
    size_t remaining = data.size();
    while (remaining > 0) {
        ssize_t n = write(fd, ptr, remaining);
        if (n == -1) {
            if (errno == EINTR) continue;
            close(fd);
            return false;
        }
        ptr += n;
        remaining -= static_cast<size_t>(n);
    }

    // Durabilité garantie
    fsync(fd);
    close(fd);

    // Renommage atomique
    fs::rename(tmp, target);

    // fsync du répertoire parent
    int dir_fd = open(target.parent_path().c_str(), O_RDONLY | O_CLOEXEC);
    if (dir_fd >= 0) { fsync(dir_fd); close(dir_fd); }

    return true;
}
```

`std::fstream` ne fournit pas d'accès à `fsync()`. On pourrait extraire le file descriptor depuis un `fstream` via des extensions non-standard, mais cela annule l'intérêt de l'abstraction. Quand la durabilité est un requis, le POSIX est le bon outil.

### Scénario 4 : Serveur réseau lisant des fichiers statiques

Un serveur HTTP servant des fichiers depuis le disque vers des sockets réseau.

```cpp
// ✅ POSIX — sendfile() pour un transfert zero-copy
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

void serve_file(int socket_fd, const char* filepath) {
    int file_fd = open(filepath, O_RDONLY | O_CLOEXEC);
    if (file_fd == -1) return;

    struct stat st;
    fstat(file_fd, &st);

    // Transfert direct noyau → socket, sans copie en espace utilisateur
    off_t offset = 0;
    sendfile(socket_fd, file_fd, &offset, static_cast<size_t>(st.st_size));

    close(file_fd);
}
```

`sendfile()` transfère les données directement du page cache vers le buffer de la socket, sans jamais les copier en espace utilisateur. C'est la technique utilisée par Nginx, Apache et tous les serveurs de fichiers performants sur Linux. Aucune API C++ standard ne permet cela.

### Scénario 5 : Parcourir une arborescence pour trouver des fichiers

Recherche récursive de fichiers `.cpp` dans un projet.

```cpp
// ✅ std::filesystem — lisible, robuste, cache intelligent
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

auto find_sources(const fs::path& root) -> std::vector<fs::path> {
    std::vector<fs::path> result;

    for (const auto& entry : fs::recursive_directory_iterator(
            root, fs::directory_options::skip_permission_denied))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
            result.push_back(entry.path());
        }
    }

    return result;
}
```

L'équivalent POSIX nécessiterait `opendir`/`readdir`/`closedir` avec une gestion manuelle de la récursion, du filtrage, des erreurs et de la fermeture des handles. Le code serait trois à quatre fois plus long pour un résultat fonctionnellement identique, sans gain de performance significatif (les deux utilisent `getdents64` sous le capot).

### Scénario 6 : Surveiller les modifications d'un répertoire

Réagir en temps réel aux créations, suppressions et modifications de fichiers.

```cpp
// ✅ POSIX — inotify est la seule option sur Linux
#include <sys/inotify.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>
#include <array>

void watch_directory(const char* path) {
    int inotify_fd = inotify_init1(IN_CLOEXEC);
    if (inotify_fd == -1) {
        std::println("inotify_init1: {}", strerror(errno));
        return;
    }

    int wd = inotify_add_watch(inotify_fd, path,
        IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd == -1) {
        std::println("inotify_add_watch: {}", strerror(errno));
        close(inotify_fd);
        return;
    }

    std::println("Surveillance de {} ...", path);

    std::array<char, 4096> buf;
    while (true) {
        ssize_t len = read(inotify_fd, buf.data(), buf.size());
        if (len <= 0) break;

        const char* ptr = buf.data();
        while (ptr < buf.data() + len) {
            const auto* event = reinterpret_cast<const inotify_event*>(ptr);

            if (event->mask & IN_CREATE)  std::println("Créé    : {}", event->name);
            if (event->mask & IN_DELETE)  std::println("Supprimé : {}", event->name);
            if (event->mask & IN_MODIFY)  std::println("Modifié  : {}", event->name);

            ptr += sizeof(inotify_event) + event->len;
        }
    }

    close(inotify_fd);
}
```

`std::filesystem` ne propose aucun mécanisme de surveillance. La seule alternative serait un polling périodique (parcourir le répertoire en boucle et comparer les timestamps), ce qui est à la fois moins réactif et plus coûteux en ressources.

### Scénario 7 : I/O haute performance sur un gros dataset

Traitement séquentiel d'un fichier de plusieurs Go avec transformation en mémoire.

```cpp
// ✅ POSIX — mmap() pour un accès mémoire direct sans copie
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <print>
#include <cerrno>
#include <cstring>
#include <cstdint>

void process_large_file(const char* path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) return;

    struct stat st;
    fstat(fd, &st);
    auto size = static_cast<size_t>(st.st_size);

    // Mapper le fichier en mémoire — le noyau gère le chargement à la demande
    void* addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);  // Le fd peut être fermé immédiatement après mmap

    if (addr == MAP_FAILED) {
        std::println("mmap: {}", strerror(errno));
        return;
    }

    // Accéder aux données comme un tableau en mémoire
    const auto* data = static_cast<const uint8_t*>(addr);

    // Conseiller le noyau sur le pattern d'accès (AVANT le traitement)
    madvise(addr, size, MADV_SEQUENTIAL);

    // Exemple : compter les lignes
    uint64_t lines = 0;
    for (size_t i = 0; i < size; ++i) {
        if (data[i] == '\n') ++lines;
    }
    std::println("{} lignes dans {} ({} Mo)", lines, path, size / (1024*1024));

    munmap(addr, size);
}
```

`mmap()` élimine la copie entre le page cache du noyau et le buffer utilisateur : le programme lit directement dans les pages mémoire du cache. C'est la technique la plus performante pour le traitement séquentiel ou aléatoire de gros fichiers. Les inconvénients sont la gestion manuelle de la taille du mapping, l'impossibilité d'utiliser `mmap()` sur certains pseudo-systèmes de fichiers, et le risque de `SIGBUS` si le fichier est tronqué pendant le mapping.

---

## Grille de décision rapide

| Besoin | Choix recommandé |
|---|---|
| Parcourir des répertoires | `std::filesystem` |
| Créer / supprimer des fichiers et répertoires | `std::filesystem` |
| Manipuler des chemins | `std::filesystem` |
| Copier des fichiers | `std::filesystem` (utilise `copy_file_range` en interne) |
| Lire un fichier texte / config | `std::fstream` |
| Écrire un fichier de sortie standard | `std::fstream` |
| Écriture avec garantie de durabilité (`fsync`) | POSIX |
| Pattern write-then-rename atomique | POSIX + `std::filesystem` (rename) |
| Servir des fichiers réseau (zero-copy) | POSIX (`sendfile`) |
| Fichiers memory-mapped | POSIX (`mmap`) |
| I/O non-bloquantes | POSIX (`O_NONBLOCK`, `epoll`) |
| Surveillance de répertoire | POSIX (`inotify`) |
| I/O directes (bypass page cache) | POSIX (`O_DIRECT`) |
| ACL, xattr, attributs étendus | POSIX |
| Fichiers temporaires anonymes | POSIX (`O_TMPFILE`) |
| Contrôle fin des permissions (chown) | POSIX |
| Scatter/Gather I/O | POSIX (`readv`, `writev`) |

---

## Cohabitation dans un même projet

En pratique, un projet C++ système mature utilise les deux couches. La frontière typique est :

- **`std::filesystem`** pour tout ce qui concerne l'**arborescence** : parcours, création de répertoires, copie, déplacement, vérification d'existence, normalisation de chemins, espace disque.
- **`std::fstream`** pour la lecture et l'écriture de **contenu textuel** de taille modérée : fichiers de configuration, rapports, logs simples.
- **POSIX** pour les opérations qui exigent un **contrôle bas niveau** : durabilité (`fsync`), I/O haute performance (`mmap`, `sendfile`), multiplexage (`epoll`), surveillance (`inotify`), fichiers temporaires sécurisés (`O_TMPFILE`).

Les trois couches s'interfacent facilement grâce à `fs::path::c_str()` qui fournit le `const char*` attendu par les fonctions POSIX :

```cpp
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

namespace fs = std::filesystem;

void example(const fs::path& dir) {
    // std::filesystem pour la structure
    fs::create_directories(dir / "data");

    // std::fstream pour l'écriture textuelle
    std::ofstream meta(dir / "data" / "metadata.json");
    meta << R"({"version": 1, "format": "binary"})";
    meta.close();

    // POSIX pour l'écriture durable du fichier de données
    fs::path data_path = dir / "data" / "payload.bin";
    int fd = open(data_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd >= 0) {
        // ... écriture binaire + fsync ...
        fsync(fd);
        close(fd);
    }
}
```

Cette cohabitation n'est pas un compromis, c'est l'approche idiomatique en C++ système sur Linux : utiliser l'abstraction la plus haute qui satisfait le besoin, descendre au POSIX quand les exigences l'imposent.

---

## Ce que `std::filesystem` ne fera jamais

Certaines fonctionnalités sont structurellement hors du périmètre de la bibliothèque standard C++, car elles sont spécifiques à un système d'exploitation :

- **`mmap`** — Le memory-mapping dépend du modèle de mémoire virtuelle du noyau.
- **`inotify` / `fanotify`** — La surveillance de fichiers est une fonctionnalité noyau sans équivalent portable.
- **`epoll`** — Le multiplexage d'I/O haute performance est spécifique à Linux.
- **`sendfile` / `splice`** — Le zero-copy est une optimisation noyau.
- **`O_DIRECT`** — Le contournement du page cache est une décision d'architecture de stockage.
- **`chown`** — Le changement de propriétaire est lié au modèle de sécurité Unix (UID/GID).
- **ACL et xattr** — Les listes de contrôle d'accès et les attributs étendus sont des extensions du modèle de permissions POSIX.

Pour ces cas d'usage, le POSIX n'est pas un pis-aller : c'est l'interface native, conçue pour ce besoin exact.

---

## Impact sur les performances : mesurer avant de décider

La question "est-ce que POSIX est plus rapide que fstream ?" n'a pas de réponse universelle. Le résultat dépend du pattern d'accès, de la taille des I/O, du système de fichiers et du matériel.

Quelques tendances observables sur Linux avec ext4 et un SSD NVMe :

**Petites lectures répétées** (< 1 Ko, milliers de fois) — `std::fstream` est souvent **plus rapide** que POSIX brut, car son buffering en espace utilisateur réduit le nombre d'appels système. Chaque `read()` POSIX traverse la frontière noyau, tandis que `fstream` sert les données depuis son buffer interne.

**Grosses lectures séquentielles** (> 64 Ko par appel) — Les performances sont **comparables**. Le buffering de `fstream` n'apporte plus rien car chaque lecture est déjà assez grande pour amortir le coût du syscall.

**`mmap` vs `read()`** — Pour le traitement séquentiel de très gros fichiers (> 100 Mo), `mmap` est généralement plus rapide car il élimine une copie mémoire. Pour les petits fichiers, `read()` est souvent préférable car `mmap` a un coût fixe (création et destruction du mapping).

**Copie de fichiers** — `fs::copy_file()` est généralement le plus rapide car il peut utiliser `copy_file_range()`, une opération noyau optimisée qui évite le transfert en espace utilisateur.

La règle pragmatique : commencez avec l'API C++ de haut niveau. Profilez (chapitre 31). Descendez au POSIX uniquement si les mesures montrent un goulot d'étranglement d'I/O, ou si vous avez besoin d'une fonctionnalité non disponible dans la bibliothèque standard.

---

> 💡 **Note** — Le choix entre les couches d'I/O n'est pas une question de purisme. Un développeur qui utilise exclusivement `std::filesystem` et `fstream` écrira du code portable et maintenable, mais sera bloqué face aux exigences de durabilité, de performance et de fonctionnalités Linux-spécifiques. Un développeur qui utilise exclusivement POSIX écrira du code verbeux, fragile et difficile à maintenir pour des opérations qui tiennent en une ligne avec `std::filesystem`. La compétence qui fait la différence est de savoir **où tracer la frontière** dans chaque projet — et cette frontière se dessine au cas par cas, guidée par les besoins réels et les mesures de performance.

⏭️ [Permissions, droits et gestion des erreurs](/19-systeme-fichiers/04-permissions-droits.md)
