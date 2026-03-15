🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 19.4 — Permissions, droits et gestion des erreurs

## Module 7 — Programmation Système sur Linux *(Niveau Avancé)*

---

## Introduction

Les trois sections précédentes ont couvert les opérations sur le système de fichiers — parcourir, créer, copier, supprimer — en mentionnant les permissions et les erreurs sans les approfondir. Cette section comble ce manque en traitant deux sujets indissociables en programmation système sur Linux : le **modèle de permissions Unix** et la **gestion robuste des erreurs** d'I/O.

Sur un système multi-utilisateur comme Linux, chaque fichier et répertoire est protégé par un ensemble de droits qui détermine qui peut lire, écrire ou exécuter. Un programme qui ignore les permissions produit un outil qui fonctionne en développement (où l'on travaille souvent en tant que propriétaire des fichiers) mais échoue en production (où les utilisateurs, les services systemd et les conteneurs Docker opèrent avec des identités et des privilèges restreints).

La gestion des erreurs est l'autre pilier : chaque interaction avec le filesystem peut échouer pour des dizaines de raisons (permissions insuffisantes, disque plein, fichier verrouillé, filesystem en lecture seule, chemin trop long, lien symbolique cassé). Un code robuste anticipe ces échecs et les traite de manière explicite.

---

## Le modèle de permissions Unix

### Les trois catégories d'utilisateurs

Chaque fichier sur Linux est associé à un **propriétaire** (user), un **groupe** (group), et à la catégorie **autres** (others, tous les utilisateurs qui ne sont ni le propriétaire ni membres du groupe). Ces trois catégories sont évaluées dans cet ordre : si vous êtes le propriétaire, seuls les bits du propriétaire s'appliquent, même si les bits du groupe ou des autres sont plus permissifs.

### Les trois types de droits

| Droit | Fichier | Répertoire |
|---|---|---|
| **Read (r)** | Lire le contenu | Lister les entrées (`ls`, `directory_iterator`) |
| **Write (w)** | Modifier le contenu | Créer, supprimer, renommer des entrées |
| **Execute (x)** | Exécuter comme programme | Traverser (accéder aux fichiers à l'intérieur) |

La distinction pour les répertoires est essentielle et source de confusion fréquente. Le droit `r` sur un répertoire permet de lister son contenu, mais sans le droit `x`, vous ne pouvez pas accéder aux fichiers qu'il contient. Inversement, avec `x` mais sans `r`, vous pouvez accéder à un fichier si vous connaissez son nom exact, mais vous ne pouvez pas lister le répertoire. Le droit `w` sur un répertoire permet de créer et supprimer des fichiers à l'intérieur, indépendamment des permissions de ces fichiers eux-mêmes (sauf si le sticky bit est positionné).

### Représentation octale

Les permissions sont représentées par un nombre octal de 3 chiffres (plus un chiffre optionnel pour les bits spéciaux) :

```
       Owner  Group  Others
        rwx    rwx    rwx
  0644: rw-    r--    r--     Fichier standard (lecture pour tous, écriture pour le propriétaire)
  0755: rwx    r-x    r-x     Exécutable ou répertoire standard
  0600: rw-    ---    ---     Fichier confidentiel (clé privée, mot de passe)
  0700: rwx    ---    ---     Répertoire privé
  0666: rw-    rw-    rw-     Fichier accessible en écriture à tous (rare, souvent risqué)
  0777: rwx    rwx    rwx     Permissions totales (à éviter)
```

### Les bits spéciaux

Au-delà des 9 bits rwx, trois bits spéciaux modifient le comportement :

| Bit | Octal | Sur un fichier | Sur un répertoire |
|---|---|---|---|
| **setuid** | 4000 | Exécuté avec l'UID du propriétaire | — |
| **setgid** | 2000 | Exécuté avec le GID du groupe | Les fichiers créés héritent du groupe du répertoire |
| **sticky** | 1000 | — | Seul le propriétaire d'un fichier peut le supprimer |

Le sticky bit est le plus courant en pratique : il est positionné sur `/tmp` (permissions `1777`), ce qui empêche un utilisateur de supprimer les fichiers temporaires d'un autre utilisateur.

---

## Permissions avec `std::filesystem`

### Lire les permissions

`std::filesystem` expose les permissions via `fs::status()` et la classe `fs::perms` :

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

void show_permissions(const fs::path& p) {
    std::error_code ec;
    auto st = fs::status(p, ec);
    if (ec) {
        std::println("Erreur status({}) : {}", p.string(), ec.message());
        return;
    }

    fs::perms perm = st.permissions();

    auto flag = [](bool set, char c) -> char { return set ? c : '-'; };

    std::println("{} : {}{}{}{}{}{}{}{}{}",
        p.filename().string(),
        flag((perm & fs::perms::owner_read)   != fs::perms::none, 'r'),
        flag((perm & fs::perms::owner_write)  != fs::perms::none, 'w'),
        flag((perm & fs::perms::owner_exec)   != fs::perms::none, 'x'),
        flag((perm & fs::perms::group_read)   != fs::perms::none, 'r'),
        flag((perm & fs::perms::group_write)  != fs::perms::none, 'w'),
        flag((perm & fs::perms::group_exec)   != fs::perms::none, 'x'),
        flag((perm & fs::perms::others_read)  != fs::perms::none, 'r'),
        flag((perm & fs::perms::others_write) != fs::perms::none, 'w'),
        flag((perm & fs::perms::others_exec)  != fs::perms::none, 'x'));
}

int main() {
    show_permissions("/etc/passwd");   // rw-r--r--
    show_permissions("/etc/shadow");   // rw-r-----
    show_permissions("/usr/bin/bash"); // rwxr-xr-x
    show_permissions("/tmp");          // rwxrwxrwt (sticky bit non visible via fs::perms)
}
```

### Les constantes `fs::perms`

`fs::perms` est un type enum bitmask. Les constantes disponibles sont :

| Constante | Valeur octale | Description |
|---|---|---|
| `owner_read` | 0400 | Lecture propriétaire |
| `owner_write` | 0200 | Écriture propriétaire |
| `owner_exec` | 0100 | Exécution propriétaire |
| `owner_all` | 0700 | rwx propriétaire |
| `group_read` | 0040 | Lecture groupe |
| `group_write` | 0020 | Écriture groupe |
| `group_exec` | 0010 | Exécution groupe |
| `group_all` | 0070 | rwx groupe |
| `others_read` | 0004 | Lecture autres |
| `others_write` | 0002 | Écriture autres |
| `others_exec` | 0001 | Exécution autres |
| `others_all` | 0007 | rwx autres |
| `all` | 0777 | Tous les droits |
| `set_uid` | 04000 | Bit setuid |
| `set_gid` | 02000 | Bit setgid |
| `sticky_bit` | 01000 | Sticky bit |
| `mask` | 07777 | Tous les bits |
| `none` | 0 | Aucun droit |
| `unknown` | 0xFFFF | Permissions inconnues |

### Modifier les permissions

`fs::permissions()` modifie les droits d'un fichier ou répertoire. Le troisième paramètre `fs::perm_options` détermine **comment** les permissions sont appliquées :

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::path p = "/tmp/test_perms.txt";

    // Créer le fichier pour la démonstration
    std::ofstream{p};

    // REPLACE : remplace toutes les permissions par la valeur donnée
    fs::permissions(p, fs::perms::owner_read | fs::perms::owner_write);
    // Résultat : rw------- (0600)

    // ADD : ajoute des permissions sans toucher aux autres
    fs::permissions(p,
        fs::perms::group_read | fs::perms::others_read,
        fs::perm_options::add);
    // Résultat : rw-r--r-- (0644)

    // REMOVE : retire des permissions spécifiques
    fs::permissions(p,
        fs::perms::others_read,
        fs::perm_options::remove);
    // Résultat : rw-r----- (0640)

    // Avec gestion d'erreur via error_code
    std::error_code ec;
    fs::permissions(p, fs::perms::all, ec);
    if (ec) {
        std::println("Erreur : {}", ec.message());
    }
}
```

Les trois modes de `fs::perm_options` :

| Option | Comportement |
|---|---|
| `replace` (défaut) | Remplace toutes les permissions par la valeur donnée |
| `add` | Active les bits spécifiés, laisse les autres inchangés |
| `remove` | Désactive les bits spécifiés, laisse les autres inchangés |

Il existe aussi `nofollow`, combinable avec les trois précédents via `|`, qui modifie les permissions du lien symbolique lui-même au lieu de sa cible :

```cpp
fs::permissions(symlink_path,
    fs::perms::owner_all,
    fs::perm_options::replace | fs::perm_options::nofollow);
```

---

## Permissions avec les appels POSIX

### `chmod()` et `fchmod()`

Pour modifier les permissions via POSIX :

```cpp
#include <sys/stat.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    // Par chemin
    if (chmod("/tmp/test.txt", 0644) == -1) {
        std::println("chmod: {}", strerror(errno));
    }

    // Par descripteur (plus sûr — pas de race condition TOCTOU)
    int fd = open("/tmp/test.txt", O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        if (fchmod(fd, 0600) == -1) {
            std::println("fchmod: {}", strerror(errno));
        }
        close(fd);
    }
}
```

`fchmod()` est préférable à `chmod()` quand vous disposez d'un descripteur ouvert, car il opère sur l'inode déjà résolu, éliminant ainsi les race conditions TOCTOU (Time-Of-Check-Time-Of-Use) : entre l'instant où vous vérifiez le chemin et celui où vous modifiez les permissions, un attaquant pourrait avoir remplacé le fichier par un lien symbolique.

### `chown()` et `fchown()` : propriétaire et groupe

`std::filesystem` ne fournit **aucune** fonction pour changer le propriétaire ou le groupe d'un fichier. C'est un cas où le POSIX est la seule option :

```cpp
#include <unistd.h>
#include <sys/types.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    // Changer le propriétaire (UID 1000) et le groupe (GID 1000)
    if (chown("/tmp/test.txt", 1000, 1000) == -1) {
        std::println("chown: {}", strerror(errno));
    }

    // Changer uniquement le groupe (passer -1 pour ne pas modifier l'UID)
    if (chown("/tmp/test.txt", static_cast<uid_t>(-1), 33) == -1) {
        std::println("chown (groupe seul): {}", strerror(errno));
    }

    // lchown : même chose mais ne suit pas les liens symboliques
    if (lchown("/tmp/link", 1000, 1000) == -1) {
        std::println("lchown: {}", strerror(errno));
    }
}
```

`chown()` nécessite les privilèges root (ou la capacité `CAP_CHOWN`). Un utilisateur normal ne peut changer que le groupe d'un fichier dont il est propriétaire, et uniquement vers un groupe dont il est membre.

### Le umask

Le `umask` est un masque de bits appliqué par le noyau à chaque création de fichier ou répertoire. Il **retire** des permissions par défaut :

```
permissions effectives = mode demandé & ~umask
```

```cpp
#include <sys/stat.h>
#include <print>

int main() {
    // Lire le umask actuel (l'appel modifie ET retourne l'ancien umask)
    mode_t old_mask = umask(0);
    umask(old_mask);  // Restaurer immédiatement
    std::println("umask actuel : {:04o}", old_mask);
    // Typiquement : 0022

    // Avec umask 0022, une création avec mode 0666 donne :
    // 0666 & ~0022 = 0666 & 0755 = 0644 (rw-r--r--)

    // Temporairement restreindre le umask pour une opération sensible
    mode_t prev = umask(0077);  // Seul le propriétaire aura accès
    int fd = open("/tmp/secret.key", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    umask(prev);  // Restaurer
    if (fd >= 0) close(fd);
}
```

La manipulation du umask est globale au processus et **non thread-safe**. Dans un programme multi-threadé, modifier le umask dans un thread affecte tous les autres. La solution recommandée est de ne pas modifier le umask à la volée, mais de définir les permissions explicitement après la création avec `fchmod()` ou `fs::permissions()`.

---

## Vérification des accès

### Vérifier avant d'agir vs agir et gérer l'erreur

Deux philosophies s'opposent pour la gestion des permissions :

**LBYL (Look Before You Leap)** — Vérifier les permissions avant de tenter l'opération :

```cpp
// ❌ Pattern LBYL — vulnérable aux race conditions TOCTOU
namespace fs = std::filesystem;

void read_file(const fs::path& p) {
    if (!fs::exists(p)) {
        std::println("Le fichier n'existe pas");
        return;
    }
    // Entre exists() et l'ouverture, le fichier peut avoir été supprimé
    // ou ses permissions modifiées → race condition TOCTOU
    std::ifstream file(p);
    // ...
}
```

**EAFP (Easier to Ask Forgiveness than Permission)** — Tenter l'opération et gérer l'erreur :

```cpp
// ✅ Pattern EAFP — robuste, pas de race condition
void read_file(const fs::path& p) {
    std::ifstream file(p);
    if (!file) {
        std::println("Impossible d'ouvrir {} : vérifiez l'existence et les permissions",
            p.string());
        return;
    }
    // Le fichier est ouvert, on peut travailler
}
```

Le pattern EAFP est systématiquement préférable en programmation système. La vérification préalable (`exists()`, `is_regular_file()`) est utile pour le diagnostic et les messages d'erreur, mais elle ne doit **jamais** remplacer la gestion d'erreur sur l'opération elle-même.

### `access()` : vérification POSIX

L'appel système `access()` vérifie si le processus a les permissions demandées sur un fichier :

```cpp
#include <unistd.h>
#include <print>

int main() {
    const char* path = "/etc/shadow";

    if (access(path, R_OK) == 0) {
        std::println("{} : lecture autorisée", path);
    } else {
        std::println("{} : lecture refusée", path);
    }

    if (access(path, W_OK) == 0) {
        std::println("{} : écriture autorisée", path);
    } else {
        std::println("{} : écriture refusée", path);
    }

    // F_OK : teste uniquement l'existence
    // R_OK : teste la lecture
    // W_OK : teste l'écriture
    // X_OK : teste l'exécution
    // Combinables : R_OK | W_OK
}
```

`access()` vérifie les permissions en utilisant l'UID/GID **réel** du processus (pas l'UID/GID effectif). C'est une subtilité importante pour les programmes setuid. Pour vérifier avec l'UID effectif, utilisez `euidaccess()` (extension GNU) ou `faccessat()` avec le flag `AT_EACCESS`.

Malgré son utilité pour le diagnostic, `access()` souffre du même problème TOCTOU que toute vérification préalable : les permissions peuvent changer entre le test et l'utilisation.

---

## Gestion des erreurs filesystem

### Taxonomie des erreurs courantes

Les erreurs de filesystem se répartissent en quelques catégories récurrentes :

| Catégorie | Exemples `errno` | Causes typiques |
|---|---|---|
| **Permissions** | `EACCES`, `EPERM` | Droits insuffisants, opération réservée à root |
| **Existence** | `ENOENT`, `EEXIST` | Fichier introuvable, fichier déjà existant |
| **Type** | `EISDIR`, `ENOTDIR` | Opération incompatible avec le type d'entrée |
| **Ressources** | `ENOSPC`, `EMFILE`, `ENFILE`, `ENOMEM` | Disque plein, trop de fichiers ouverts, mémoire |
| **Filesystem** | `EROFS`, `EXDEV`, `ELOOP` | Lecture seule, cross-device, boucle de symlinks |
| **Chemin** | `ENAMETOOLONG`, `ENOTEMPTY` | Chemin trop long, répertoire non vide |
| **I/O** | `EIO` | Erreur matérielle, secteur défectueux |
| **Interruption** | `EINTR` | Appel interrompu par un signal |

### Gestion d'erreurs avec `std::filesystem`

Rappel de la section 19.1 : chaque fonction `std::filesystem` qui accède au disque existe en deux variantes. Voici les patterns de gestion selon le contexte :

**Opération unique critique** — les exceptions sont naturelles :

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

void deploy_config(const fs::path& source, const fs::path& dest) {
    try {
        fs::create_directories(dest.parent_path());
        fs::copy_file(source, dest, fs::copy_options::overwrite_existing);
        fs::permissions(dest, fs::perms::owner_read | fs::perms::owner_write);
    } catch (const fs::filesystem_error& e) {
        std::println("Déploiement échoué :");
        std::println("  Opération : {}", e.what());
        std::println("  path1 : {}", e.path1().string());
        if (!e.path2().empty()) {
            std::println("  path2 : {}", e.path2().string());
        }
        std::println("  Code  : {} ({})", e.code().value(), e.code().message());
        throw;  // Propager — le déploiement doit réussir
    }
}
```

`fs::filesystem_error` expose `path1()` et `path2()` (le second chemin est pertinent pour les opérations à deux chemins comme `copy_file` ou `rename`) et `code()` qui retourne un `std::error_code` exploitable programmatiquement.

**Boucle de traitement tolérant aux erreurs** — `error_code` évite l'interruption :

```cpp
#include <filesystem>
#include <print>
#include <system_error>

namespace fs = std::filesystem;

void cleanup_old_logs(const fs::path& log_dir, std::chrono::hours max_age) {
    std::error_code ec;
    auto now = fs::file_time_type::clock::now();
    int cleaned = 0;
    int errors = 0;

    auto it = fs::directory_iterator(log_dir, ec);
    if (ec) {
        std::println("Impossible d'ouvrir {} : {}", log_dir.string(), ec.message());
        return;
    }

    for (const auto& entry : it) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".log") continue;

        auto age = now - entry.last_write_time(ec);
        if (ec) { ++errors; ec.clear(); continue; }

        if (age > max_age) {
            fs::remove(entry.path(), ec);
            if (ec) {
                std::println("Impossible de supprimer {} : {}",
                    entry.path().filename().string(), ec.message());
                ++errors;
                ec.clear();
            } else {
                ++cleaned;
            }
        }
    }

    std::println("Nettoyage terminé : {} supprimés, {} erreurs", cleaned, errors);
}
```

Le `ec.clear()` après chaque gestion d'erreur est essentiel : sans lui, le même `error_code` resterait en état d'erreur et pourrait fausser les tests suivants.

### Gestion d'erreurs POSIX

La gestion d'erreurs POSIX repose sur trois mécanismes :

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <print>
#include <system_error>

void posix_error_handling(const char* path) {
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        int err = errno;  // Sauvegarder immédiatement — errno est volatile

        // 1. Message lisible avec strerror
        std::println("Erreur : {}", strerror(err));

        // 2. Diagnostic programmatique avec le code errno
        switch (err) {
            case ENOENT:
                std::println("Le fichier n'existe pas");
                break;
            case EACCES:
                std::println("Permission refusée — vérifiez les droits");
                break;
            case EMFILE:
                std::println("Trop de fichiers ouverts par ce processus");
                break;
            case ENFILE:
                std::println("Trop de fichiers ouverts dans le système");
                break;
            case ELOOP:
                std::println("Trop de niveaux de liens symboliques");
                break;
            case ENAMETOOLONG:
                std::println("Chemin trop long");
                break;
            default:
                std::println("Erreur inattendue (errno={})", err);
        }

        // 3. Conversion vers std::error_code pour interopérabilité C++
        std::error_code ec(err, std::generic_category());
        std::println("error_code : {}", ec.message());

        return;
    }

    // ... utilisation du fd ...
    close(fd);
}
```

Un piège classique : **`errno` peut être écrasé par n'importe quel appel système ou fonction de la libc**, y compris `std::println` lui-même. Il faut le sauvegarder dans une variable locale immédiatement après l'appel qui a échoué, avant tout autre appel.

### Convertir entre `errno` et `std::error_code`

La passerelle entre les deux mondes d'erreurs est `std::error_code` :

```cpp
#include <system_error>
#include <cerrno>
#include <print>

void demonstrate_error_bridge() {
    // errno → error_code
    errno = EACCES;  // Simuler une erreur
    std::error_code ec(errno, std::generic_category());
    std::println("Message : {}", ec.message());  // "Permission denied"
    std::println("Valeur  : {}", ec.value());     // 13 (valeur de EACCES)

    // error_code → test programmatique
    if (ec == std::errc::permission_denied) {
        std::println("Permission refusée détectée via std::errc");
    }

    // error_code → exception
    if (ec) {
        // throw std::system_error(ec, "Contexte de l'erreur");
    }
}
```

`std::errc` fournit des constantes portables correspondant aux codes errno POSIX. Utiliser `std::errc` dans les comparaisons rend le code indépendant des valeurs numériques spécifiques à la plateforme.

---

## Patterns de robustesse en production

### Création sécurisée de fichiers sensibles

Un fichier contenant des données sensibles (clé, token, mot de passe) ne doit jamais être lisible par d'autres utilisateurs, même brièvement :

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <print>
#include <cerrno>
#include <cstring>

namespace fs = std::filesystem;

bool write_secret_file(const fs::path& path, const std::string& content) {
    // O_EXCL : échoue si le fichier existe (pas d'écrasement accidentel)
    // Mode 0600 : rw------- (seul le propriétaire peut lire/écrire)
    int fd = open(path.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);

    if (fd == -1) {
        std::println("Impossible de créer {} : {}", path.string(), strerror(errno));
        return false;
    }

    // Vérifier que les permissions sont bien celles demandées
    // (paranoïa justifiée : le umask pourrait interférer sur certains systèmes)
    fchmod(fd, 0600);

    // Écriture complète
    const char* ptr = content.data();
    size_t remaining = content.size();
    while (remaining > 0) {
        ssize_t n = write(fd, ptr, remaining);
        if (n == -1) {
            if (errno == EINTR) continue;
            std::println("write: {}", strerror(errno));
            close(fd);
            fs::remove(path);  // Nettoyage en cas d'échec
            return false;
        }
        ptr += n;
        remaining -= static_cast<size_t>(n);
    }

    fsync(fd);
    close(fd);
    return true;
}
```

Les points clés sont : `O_EXCL` pour la création atomique (pas de race condition entre test et création), `0600` dès la création (pas de fenêtre temporelle avec des permissions trop larges), `fchmod()` en renfort pour contrer un umask inattendu, et nettoyage du fichier partiel en cas d'erreur.

### Vérification de l'espace disque avant écriture

```cpp
#include <filesystem>
#include <print>
#include <cstdint>

namespace fs = std::filesystem;

bool has_enough_space(const fs::path& target, std::uintmax_t required_bytes) {
    std::error_code ec;
    auto info = fs::space(target.parent_path(), ec);
    if (ec) {
        std::println("Impossible de vérifier l'espace : {}", ec.message());
        return false;  // En cas de doute, refuser
    }

    // Garder une marge de sécurité de 10%
    auto margin = required_bytes / 10;
    if (info.available < required_bytes + margin) {
        std::println("Espace insuffisant : {} Mo disponibles, {} Mo requis",
            info.available / (1024 * 1024),
            (required_bytes + margin) / (1024 * 1024));
        return false;
    }

    return true;
}
```

Cette vérification ne remplace pas la gestion de l'erreur `ENOSPC` sur les écritures elles-mêmes (l'espace peut être consommé par un autre processus entre la vérification et l'écriture), mais elle permet de fournir un message d'erreur anticipé et clair à l'utilisateur.

### Gestion des liens symboliques cassés

Un lien symbolique dont la cible n'existe plus est un piège courant lors du parcours de répertoires :

```cpp
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

void audit_symlinks(const fs::path& dir) {
    for (const auto& entry : fs::directory_iterator(
            dir, fs::directory_options::skip_permission_denied))
    {
        // symlink_status() ne suit PAS le lien — indispensable ici
        if (!entry.is_symlink()) continue;

        std::error_code ec;
        auto target = fs::read_symlink(entry.path(), ec);
        if (ec) {
            std::println("Impossible de lire le lien {} : {}",
                entry.path().string(), ec.message());
            continue;
        }

        // Vérifier si la cible existe
        if (!fs::exists(entry.path())) {
            // exists() suit le lien : si la cible n'existe pas, retourne false
            std::println("Lien cassé : {} -> {}", 
                entry.path().filename().string(), target.string());
        }
    }
}
```

Le subtilité ici est que `fs::exists()` suit les liens symboliques par défaut. Un lien dont la cible n'existe pas retourne `false` pour `exists()` mais `true` pour `fs::symlink_status(p).type() != fs::file_type::not_found`. C'est la combinaison des deux tests qui permet d'identifier un lien cassé.

### Gestion de `EINTR` dans une classe utilitaire

Pour éviter de répéter la boucle `EINTR` dans chaque appel POSIX, une fonction utilitaire qui relance automatiquement l'appel :

```cpp
#include <cerrno>
#include <utility>

// Relance automatiquement un appel système interrompu par un signal
template<typename Fn, typename... Args>  
auto eintr_safe(Fn fn, Args&&... args) {  
    decltype(fn(std::forward<Args>(args)...)) result;
    do {
        result = fn(std::forward<Args>(args)...);
    } while (result == -1 && errno == EINTR);
    return result;
}

// Utilisation
void example() {
    int fd = eintr_safe(open, "/tmp/test.txt", O_RDONLY | O_CLOEXEC);
    if (fd == -1) { /* erreur réelle */ }

    char buf[4096];
    ssize_t n = eintr_safe(read, fd, buf, sizeof(buf));
    if (n == -1) { /* erreur réelle */ }

    eintr_safe(close, fd);
}
```

---

## Erreurs spécifiques aux conteneurs Docker

Dans un contexte DevOps, les programmes C++ tournent souvent dans des conteneurs Docker. Plusieurs erreurs de permissions sont spécifiques à cet environnement :

**Filesystem en lecture seule** — Les images distroless ou les déploiements Kubernetes avec `readOnlyRootFilesystem: true` produisent `EROFS` sur toute tentative d'écriture. Le programme doit écrire dans des volumes montés ou dans `/tmp` :

```cpp
namespace fs = std::filesystem;

auto get_writable_dir() -> fs::path {
    // Tenter le répertoire de données monté
    std::error_code ec;
    fs::path data = "/data";
    auto info = fs::space(data, ec);
    if (!ec && info.available > 0) {
        return data;
    }

    // Fallback vers /tmp (généralement un tmpfs dans les conteneurs)
    return fs::temp_directory_path();
}
```

**UID mapping** — L'utilisateur dans le conteneur peut avoir un UID différent de celui sur l'hôte. Un fichier monté en volume depuis l'hôte avec `0600` et appartenant à l'UID 1000 sera inaccessible si le conteneur tourne en UID 0 (root) ou UID 65534 (nobody). La solution est de vérifier les permissions effectives plutôt que de supposer un UID.

**Capabilities** — Un conteneur peut fonctionner sans la capability `CAP_CHOWN` même en tant que root. Un `chown()` qui fonctionnerait sur une machine classique échoue avec `EPERM`. Traitez toujours `chown()` comme une opération qui peut échouer.

---

## Résumé : checklist de robustesse

Voici les vérifications à considérer pour du code filesystem de qualité production :

| Vérification | API recommandée |
|---|---|
| Chemin non vide avant opération | `path.empty()` |
| Existence (informatif, pas de sécurité) | `fs::exists()` avec `error_code` |
| Permissions avant écriture sensible | `open()` avec `O_CREAT | O_EXCL`, `fchmod()` |
| Espace disque suffisant | `fs::space()` |
| Erreur sur chaque appel POSIX | Test du retour + `errno` sauvegardé immédiatement |
| `EINTR` sur `read`/`write`/`close` | Boucle de relance ou `eintr_safe()` |
| Écritures partielles | Boucle `write_full()` |
| Nettoyage en cas d'échec | `fs::remove()` du fichier temporaire, RAII |
| Liens symboliques cassés | `exists()` vs `symlink_status()` |
| Filesystem en lecture seule | Gestion de `EROFS` |
| Disque plein | Gestion de `ENOSPC` |
| Race conditions TOCTOU | Pattern EAFP (agir puis gérer l'erreur) |

---

> 💡 **Note** — Les permissions et la gestion des erreurs sont les aspects les moins glamour de la programmation système, mais ce sont ceux qui déterminent la fiabilité d'un outil en production. Un programme qui fonctionne "sur ma machine" avec les fichiers du développeur, en tant que propriétaire, avec de l'espace disque abondant, est un prototype. Un programme qui gère correctement les permissions refusées, les disques pleins, les liens symboliques cassés, les interruptions par signaux et les race conditions TOCTOU est un outil de production. La différence entre les deux est rarement une question de complexité algorithmique — c'est une question de rigueur systématique dans la gestion de chaque valeur de retour.

⏭️ [Signaux POSIX](/20-signaux-posix/README.md)
