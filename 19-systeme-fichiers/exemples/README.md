# Chapitre 19 — Système de fichiers : exemples

Compilation : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -o <nom> <nom>.cpp`

---

## 19.1 — std::filesystem (01-std-filesystem.md)

| Fichier | Description | Sortie attendue |
|---|---|---|
| `01_path_construction.cpp` | Construction de `fs::path` depuis string literal, `std::string`, `string_view` | Affiche les chemins construits |
| `01_decomposition.cpp` | Décomposition : `root_path()`, `filename()`, `stem()`, `extension()` | Composants d'un chemin `/home/user/projet/main.cpp` |
| `01_concatenation.cpp` | Opérateur `/` et `+=` pour chemins | Chemins concaténés |
| `01_normalisation.cpp` | `lexically_normal()` et `lexically_relative()` | Chemins normalisés et relatifs |
| `01_resolution.cpp` | `canonical()`, `weakly_canonical()`, `current_path()` | Chemins résolus physiquement |
| `01_interroger_fs.cpp` | `exists()`, `is_regular_file()`, `file_size()`, `space()` | Métadonnées de fichiers et espace disque |
| `01_gestion_erreurs.cpp` | Exceptions `filesystem_error` vs `std::error_code` | Messages d'erreur pour chemins inexistants |
| `01_operations.cpp` | Création, copie, déplacement, suppression, liens symboliques | Opérations CRUD sur le filesystem |
| `01_performance.cpp` | Éviter les appels `stat()` redondants avec `fs::status()` | Comparaison de performance |

## 19.1.1 — Parcours de répertoires (01.1-parcours-repertoires.md)

| Fichier | Description | Sortie attendue |
|---|---|---|
| `011_directory_iterator.cpp` | `directory_iterator` avec tri alphabétique | Liste triée des fichiers de `/usr/include` |
| `011_directory_entry.cpp` | Cache de `directory_entry` pour type et taille | Informations de fichiers sans stat redondant |
| `011_recursive_iterator.cpp` | Parcours récursif avec exclusion de répertoires | Arborescence filtrée (sans build, .git) |
| `011_error_code_iteration.cpp` | Itération tolérante aux erreurs avec `increment(ec)` | Parcours malgré les erreurs de permission |
| `011_patterns.cpp` | `find_by_extension()`, `directory_size()`, fichiers récents | Filtrage par extension, calcul de taille |
| `011_ranges.cpp` | C++20 ranges avec `views::filter` et `views::transform` | Fichiers filtrés via pipeline ranges |

## 19.1.2 — Manipulation de chemins (01.2-manipulation-chemins.md)

| Fichier | Description | Sortie attendue |
|---|---|---|
| `012_anatomie_chemin.cpp` | Itération sur les composants, distinction absolu/relatif | Composants décomposés un par un |
| `012_normalisation.cpp` | Normalisation syntaxique vs physique (`lexically_normal`, `canonical`) | Chemins nettoyés avec/sans accès disque |
| `012_modification.cpp` | `replace_filename()`, `replace_extension()`, `remove_filename()` | Chemins modifiés |
| `012_comparaison.cpp` | Comparaison de chemins, dotfiles, trailing slash | Tests d'égalité lexicale |
| `012_interop.cpp` | Chemins XDG (`XDG_CONFIG_HOME`, `XDG_CACHE_HOME`), `temp_directory_path()` | Chemins de configuration et cache |

## 19.1.3 — Opérations sur fichiers (01.3-operations-fichiers.md)

| Fichier | Description | Sortie attendue |
|---|---|---|
| `013_creation_repertoires.cpp` | `create_directory()`, `create_directories()` | Création de répertoires avec gestion d'erreur |
| `013_copie_fichiers.cpp` | `copy_file()` avec `copy_options` | Copie, écrasement, skip |
| `013_deplacement_suppression.cpp` | `rename()`, `remove()`, `remove_all()` | Déplacement et suppression avec diagnostics |
| `013_liens.cpp` | `create_symlink()`, `create_hard_link()`, `read_symlink()` | Liens symboliques et hard links |
| `013_atomic_write.cpp` | Pattern write-then-rename pour écriture atomique | Configuration écrite de manière atomique |
| `013_metadonnees.cpp` | `file_size()`, `resize_file()`, `last_write_time()`, `clock_cast` | Taille, troncation, dates de fichiers |
| `013_temp_files.cpp` | `make_temp_path()` avec PID et timestamp | Création de fichier temporaire unique |
| `013_temp_directory.cpp` | Classe `TempDirectory` RAII avec nettoyage automatique | Répertoire créé puis supprimé automatiquement |
| `013_space_info.cpp` | `fs::space()` pour informations d'espace disque | Capacité, libre, disponible, % utilisation |

## 19.2 — Appels système POSIX (02-appels-posix.md)

| Fichier | Description | Sortie attendue |
|---|---|---|
| `02_open.cpp` | `open()` : flags `O_RDONLY`, `O_CREAT`, `O_EXCL`, `O_APPEND`, `O_CLOEXEC` | File descriptors ouverts, erreur EEXIST |
| `02_read.cpp` | `read()` basique et `read_full()` pour lectures complètes | Contenu de `/etc/hostname` et `/etc/passwd` |
| `02_write.cpp` | `write()` basique et `write_full()` pour écritures complètes | Octets écrits (24 + 1 Mo) |
| `02_lseek.cpp` | `lseek()` : `SEEK_SET`, `SEEK_CUR`, `SEEK_END`, fichiers creux | Positions, taille, sparse file stats |
| `02_fsync.cpp` | `fsync()` et pattern write-then-rename durable complet | Écriture durable avec fsync + rename |
| `02_file_descriptor.cpp` | Classe `FileDescriptor` RAII (Rule of Five) | Ouverture, lecture, move semantics |
| `02_tmpfile.cpp` | `O_TMPFILE` pour fichiers temporaires anonymes | Fichier invisible créé et supprimé auto |
| `02_stat.cpp` | `stat()` et `fstat()` : taille, inode, UID, GID, type | Métadonnées de `/etc/hostname` et `/tmp` |
| `02_copy_file_posix.cpp` | Copie avec buffer adaptatif basé sur `fstat().st_blksize` | Copie vérifiée avec tailles identiques |

## 19.3 — Comparaison API C++ vs POSIX (03-comparaison-api.md)

| Fichier | Description | Sortie attendue |
|---|---|---|
| `03_read_config.cpp` | Lecture de config avec `std::fstream` (scénario 1) | Contenu YAML lu |
| `03_write_critical.cpp` | Écriture critique avec `fsync` (scénario 3) | Fichier écrit avec durabilité |
| `03_find_sources.cpp` | Recherche récursive de `.cpp` (scénario 5) | 4 fichiers .cpp trouvés |
| `03_mmap.cpp` | `mmap()` pour traitement de fichier volumineux (scénario 7) | Comptage de lignes via mmap |
| `03_cohabitation.cpp` | Mélange filesystem + fstream + POSIX (cohabitation) | Structure avec metadata.json + payload.bin |

## 19.4 — Permissions et gestion des erreurs (04-permissions-droits.md)

| Fichier | Description | Sortie attendue |
|---|---|---|
| `04_show_permissions.cpp` | Affichage rwx avec `fs::status()` et `fs::perms` | Permissions de passwd, shadow, bash |
| `04_modify_permissions.cpp` | `fs::permissions()` : replace, add, remove | Permissions modifiées étape par étape |
| `04_chmod.cpp` | `chmod()` et `fchmod()` POSIX | Permissions modifiées par chemin et fd |
| `04_umask.cpp` | Lecture et impact du umask sur les créations | Permissions effectives vs demandées |
| `04_access.cpp` | `access()` : test R_OK, W_OK, X_OK, F_OK | Permissions de /etc/shadow, /etc/passwd, bash |
| `04_error_handling.cpp` | Gestion d'erreurs POSIX : errno, strerror, switch | Diagnostic par type d'erreur |
| `04_error_bridge.cpp` | Passerelle `errno` ↔ `std::error_code` ↔ `std::errc` | Conversions EACCES, ENOENT, ENOSPC |
| `04_write_secret.cpp` | Création sécurisée : `O_EXCL` + mode 0600 + `fchmod` | Fichier 0600 créé, recréation refusée |
| `04_has_enough_space.cpp` | `fs::space()` avec marge de sécurité 10% | 1 Mo OK, 1 To insuffisant |
| `04_eintr_safe.cpp` | Template `eintr_safe()` pour relance automatique | Lecture/écriture avec protection EINTR |
