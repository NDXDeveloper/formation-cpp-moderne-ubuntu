🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 26.6 Toolchains et cross-compilation

> **Objectif** : Comprendre le mécanisme des fichiers toolchain de CMake pour contrôler le choix du compilateur, et maîtriser la cross-compilation — compiler sur une machine x86_64 des binaires destinés à une autre architecture (ARM, RISC-V) ou un autre système d'exploitation.

---

## Qu'est-ce qu'une toolchain ?

Dans le contexte de CMake, une **toolchain** désigne l'ensemble des outils utilisés pour transformer du code source en binaires exécutables : le compilateur C++, le compilateur C, le linker, l'assembleur, et les outils associés (archiver, strip, objcopy). Sur une machine de développement Linux classique, la toolchain par défaut est celle du système — GCC ou Clang installé via `apt`.

CMake détecte automatiquement la toolchain système lors de la configuration. Mais il est parfois nécessaire d'en choisir une différente : utiliser Clang au lieu de GCC, compiler pour une architecture ARM depuis une machine x86_64, ou cibler un SDK embarqué spécifique. C'est le rôle du **fichier toolchain**.

---

## Le fichier toolchain

Un fichier toolchain est un script CMake (convention : `*.cmake`) qui définit les variables contrôlant la compilation **avant** que CMake ne détecte quoi que ce soit. Il est passé lors de la configuration via `-DCMAKE_TOOLCHAIN_FILE` :

```bash
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64-linux.cmake
```

CMake lit ce fichier très tôt dans le processus de configuration — avant même l'appel `project()`. C'est ce qui lui permet de configurer correctement le compilateur, le sysroot, et les chemins de recherche pour la plateforme cible.

### Structure minimale

Un fichier toolchain contient typiquement les éléments suivants :

```cmake
# cmake/toolchains/arm64-linux.cmake

# 1. Système cible
set(CMAKE_SYSTEM_NAME Linux)  
set(CMAKE_SYSTEM_PROCESSOR aarch64)  

# 2. Compilateurs
set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)  
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)  

# 3. Sysroot (optionnel — racine du système cible)
# set(CMAKE_SYSROOT /opt/sysroots/aarch64-linux)

# 4. Chemins de recherche
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)  
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)  
```

Détaillons chaque section.

---

## Les variables essentielles

### `CMAKE_SYSTEM_NAME` : identifier la plateforme cible

```cmake
set(CMAKE_SYSTEM_NAME Linux)
```

Cette variable est le **déclencheur de la cross-compilation**. Quand `CMAKE_SYSTEM_NAME` est défini dans un fichier toolchain et diffère du système hôte, CMake sait qu'il s'agit d'une cross-compilation et ajuste son comportement en conséquence.

Valeurs courantes :

| Valeur | Plateforme cible |
|--------|-----------------|
| `Linux` | Linux (embarqué, serveur, desktop) |
| `Windows` | Windows |
| `Darwin` | macOS |
| `Android` | Android (NDK) |
| `iOS` | iOS |
| `Generic` | Bare-metal, RTOS, systèmes sans OS |

Si vous ne cross-compilez pas mais voulez simplement choisir un compilateur différent (Clang au lieu de GCC sur la même machine Linux), vous pouvez omettre `CMAKE_SYSTEM_NAME` — CMake comprendra qu'il s'agit d'un build natif avec un compilateur alternatif.

### `CMAKE_SYSTEM_PROCESSOR` : identifier l'architecture cible

```cmake
set(CMAKE_SYSTEM_PROCESSOR aarch64)
```

Indique l'architecture du processeur cible. Cette variable est informative — elle n'affecte pas directement le choix du compilateur, mais elle est utilisée par certains scripts CMake et modules Find pour sélectionner les bons binaires.

Valeurs courantes : `x86_64`, `aarch64` (ARM 64 bits), `armv7l` (ARM 32 bits), `riscv64`, `riscv32`.

### `CMAKE_C_COMPILER` et `CMAKE_CXX_COMPILER` : les compilateurs

```cmake
set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)  
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)  
```

Ces variables pointent vers les exécutables du compilateur. Pour un cross-compilateur, le nom suit la convention GNU : `<architecture>-<os>-<abi>-<outil>`. Le triplet `aarch64-linux-gnu` signifie « architecture AArch64, système Linux, ABI GNU ».

Si les compilateurs ne sont pas dans le `PATH`, utilisez des chemins absolus :

```cmake
set(CMAKE_C_COMPILER   /opt/toolchains/aarch64/bin/aarch64-linux-gnu-gcc)  
set(CMAKE_CXX_COMPILER /opt/toolchains/aarch64/bin/aarch64-linux-gnu-g++)  
```

### `CMAKE_SYSROOT` : la racine du système cible

```cmake
set(CMAKE_SYSROOT /opt/sysroots/aarch64-linux)
```

Le sysroot est un répertoire qui contient une copie des headers et des bibliothèques du système cible. Le compilateur utilise ce répertoire comme racine au lieu de `/usr/include` et `/usr/lib` du système hôte.

Un sysroot typique contient :

```
/opt/sysroots/aarch64-linux/
├── usr/
│   ├── include/          ← Headers système (stdio.h, stdlib.h, etc.)
│   │   └── aarch64-linux-gnu/
│   └── lib/
│       └── aarch64-linux-gnu/
│           ├── libc.so
│           ├── libm.so
│           ├── libpthread.so
│           └── ...
└── lib/
    └── aarch64-linux-gnu/
        └── ...
```

Le sysroot est fourni par le SDK de la plateforme cible, ou peut être généré à partir d'une installation Linux réelle (via `debootstrap` par exemple).

### Modes de recherche : isoler le système cible

```cmake
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)  
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)  
```

Ces quatre variables contrôlent le comportement des commandes de recherche (`find_program`, `find_library`, `find_path`, `find_package`) pendant la cross-compilation :

- `PROGRAM NEVER` : chercher les programmes (compilateurs, outils) uniquement sur le système **hôte**. Logique — vous ne pouvez pas exécuter un binaire ARM sur votre machine x86_64.
- `LIBRARY ONLY` : chercher les bibliothèques uniquement dans le sysroot / les chemins de la cible. Lier contre une `libc.so` x86_64 alors que vous compilez pour ARM serait désastreux.
- `INCLUDE ONLY` : idem pour les headers.
- `PACKAGE ONLY` : idem pour les fichiers de configuration CMake.

Sans ces restrictions, `find_package(OpenSSL)` pourrait trouver l'OpenSSL x86_64 de votre machine hôte et l'utiliser pour la compilation ARM — un bug silencieux qui se manifesterait au runtime sur la cible.

---

## Cross-compilation en pratique

### Prérequis : installer le cross-compilateur

Ubuntu fournit des cross-compilateurs dans ses dépôts :

```bash
# ARM 64 bits (AArch64)
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# ARM 32 bits (hard float)
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# RISC-V 64 bits
sudo apt install gcc-riscv64-linux-gnu g++-riscv64-linux-gnu
```

Vérification :

```bash
aarch64-linux-gnu-g++ --version
# aarch64-linux-gnu-g++ (Ubuntu ...) 13.x.x  (sur Ubuntu 24.04)
```

### Fichier toolchain ARM64

```cmake
# cmake/toolchains/aarch64-linux-gnu.cmake
# Cross-compilation pour ARM 64 bits (AArch64) avec GCC

set(CMAKE_SYSTEM_NAME Linux)  
set(CMAKE_SYSTEM_PROCESSOR aarch64)  

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)  
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)  

# Linker et outils associés (optionnel — déduits automatiquement du compilateur)
set(CMAKE_AR      aarch64-linux-gnu-ar)  
set(CMAKE_RANLIB  aarch64-linux-gnu-ranlib)  
set(CMAKE_STRIP   aarch64-linux-gnu-strip)  

# Modes de recherche
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)  
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)  
```

### Fichier toolchain RISC-V 64

```cmake
# cmake/toolchains/riscv64-linux-gnu.cmake
# Cross-compilation pour RISC-V 64 bits avec GCC

set(CMAKE_SYSTEM_NAME Linux)  
set(CMAKE_SYSTEM_PROCESSOR riscv64)  

set(CMAKE_C_COMPILER   riscv64-linux-gnu-gcc)  
set(CMAKE_CXX_COMPILER riscv64-linux-gnu-g++)  

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)  
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)  
```

### Compilation

```bash
# Configurer pour ARM64
cmake -B build-arm64 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Compiler
cmake --build build-arm64

# Vérifier l'architecture du binaire produit
file build-arm64/apps/my-app
# build-arm64/apps/my-app: ELF 64-bit LSB pie executable, ARM aarch64, ...
```

La commande `file` confirme que le binaire est bien un exécutable ARM AArch64 — produit sur votre machine x86_64 mais destiné à une machine ARM.

### Exécution sur la cible

Le binaire cross-compilé ne peut pas s'exécuter directement sur votre machine de développement. Vous devez le transférer sur la cible :

```bash
# Copier vers la cible ARM
scp build-arm64/apps/my-app user@arm-device:/home/user/

# Exécuter sur la cible
ssh user@arm-device /home/user/my-app
```

Pour le développement et les tests, QEMU offre une alternative pratique en émulant l'architecture cible sur votre machine :

```bash
# Installer QEMU user-mode pour ARM64
sudo apt install qemu-user qemu-user-binfmt

# Exécuter le binaire ARM64 localement via QEMU
qemu-aarch64 -L /usr/aarch64-linux-gnu build-arm64/apps/my-app
```

L'option `-L` pointe vers les bibliothèques dynamiques de l'architecture cible. Avec `binfmt_misc` configuré (le paquet `qemu-user-binfmt`), Linux reconnaît automatiquement les binaires ARM et les exécute via QEMU de manière transparente :

```bash
# Avec binfmt configuré, l'exécution est transparente
./build-arm64/apps/my-app
# QEMU est invoqué automatiquement
```

---

## Toolchain sans cross-compilation : choisir Clang

Un fichier toolchain n'est pas réservé à la cross-compilation. Il est aussi utile pour sélectionner un compilateur spécifique tout en gardant les `CMakeLists.txt` indépendants du choix de compilateur.

```bash
# Prérequis : installer Clang et le linker lld
sudo apt install clang lld
```

```cmake
# cmake/toolchains/clang.cmake
# Build natif avec Clang

set(CMAKE_C_COMPILER   clang)  
set(CMAKE_CXX_COMPILER clang++)  

# Utiliser le linker lld (plus rapide que ld ou gold)
set(CMAKE_EXE_LINKER_FLAGS_INIT    "-fuse-ld=lld")  
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")  
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")  
```

```bash
cmake -B build-clang -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/clang.cmake
```

Notez l'absence de `CMAKE_SYSTEM_NAME` — CMake comprend que c'est un build natif.

### Pourquoi un fichier toolchain plutôt que `-DCMAKE_CXX_COMPILER` ?

Vous pouvez aussi choisir le compilateur directement en ligne de commande :

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
```

Les deux approches fonctionnent pour un simple changement de compilateur. Le fichier toolchain devient avantageux quand la configuration va au-delà du choix du compilateur : linker spécifique, flags de base, sysroot, chemins de recherche. Il encapsule toute la configuration de la toolchain dans un seul fichier réutilisable et versionnable.

---

## Toolchains et CMake Presets

Les fichiers toolchain s'intègrent naturellement avec les CMake Presets (section 27.6) pour offrir une expérience de configuration fluide :

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "native-gcc",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build-gcc",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            }
        },
        {
            "name": "native-clang",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build-clang",
            "toolchainFile": "${sourceDir}/cmake/toolchains/clang.cmake",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            }
        },
        {
            "name": "cross-arm64",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build-arm64",
            "toolchainFile": "${sourceDir}/cmake/toolchains/aarch64-linux-gnu.cmake",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            }
        },
        {
            "name": "cross-riscv64",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build-riscv64",
            "toolchainFile": "${sourceDir}/cmake/toolchains/riscv64-linux-gnu.cmake",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            }
        }
    ]
}
```

```bash
# Un seul mot pour chaque configuration
cmake --preset native-gcc  
cmake --preset native-clang  
cmake --preset cross-arm64  
cmake --preset cross-riscv64  

# Build
cmake --build build-arm64
```

L'ensemble des configurations supportées par le projet est documenté dans le fichier `CMakePresets.json`, versionné et partagé entre tous les développeurs.

---

## Gestion des dépendances en cross-compilation

La cross-compilation complexifie la gestion des dépendances : les bibliothèques système de votre machine hôte (x86_64) ne sont pas utilisables sur la cible (ARM). Plusieurs stratégies existent.

### Dépendances dans le sysroot

Si vous disposez d'un sysroot complet de la cible, les bibliothèques système (OpenSSL, ZLIB, etc.) y sont déjà présentes. `find_package()` les trouvera automatiquement grâce aux modes de recherche `ONLY` configurés dans le fichier toolchain.

### FetchContent : compilation depuis les sources

`FetchContent` fonctionne naturellement en cross-compilation — les dépendances téléchargées sont compilées avec le cross-compilateur, puisque CMake utilise la toolchain définie pour l'ensemble du projet :

```cmake
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.3
)
FetchContent_MakeAvailable(spdlog)
# spdlog est compilé pour ARM64, pas pour x86_64
```

C'est l'approche la plus simple pour les dépendances légères.

### Conan avec profils de cross-compilation

Conan (section 27.2) supporte la cross-compilation via des profils :

```ini
# ~/.conan2/profiles/arm64
[settings]
os=Linux  
arch=armv8  
compiler=gcc  
compiler.version=14  
build_type=Release  

[conf]
tools.cmake.cmaketoolchain:toolchain_file=/path/to/aarch64-linux-gnu.cmake
```

```bash
conan install . --profile:host=arm64 --profile:build=default --build=missing
```

Le profil `host` décrit la plateforme cible (ARM64), le profil `build` décrit la plateforme de compilation (x86_64). Conan cross-compile les dépendances en conséquence.

---

## Pièges courants

### Piège n°1 : changer de compilateur après la configuration initiale

Le compilateur est détecté et verrouillé lors de la **première** configuration. Changer `CMAKE_CXX_COMPILER` dans un répertoire de build existant n'a aucun effet — CMake utilise la valeur en cache.

```bash
# ❌ Ne fonctionne PAS — le compilateur est déjà en cache
cmake -B build -DCMAKE_CXX_COMPILER=clang++

# ✅ Supprimer le cache et reconfigurer
rm -rf build  
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++  

# ✅ Ou utiliser un nouveau répertoire de build
cmake -B build-clang -G Ninja -DCMAKE_CXX_COMPILER=clang++
```

### Piège n°2 : oublier les modes de recherche

Sans les `CMAKE_FIND_ROOT_PATH_MODE_*`, `find_package` peut trouver les bibliothèques de votre machine hôte et les lier au binaire cross-compilé. Le build réussit, mais le binaire crashe sur la cible avec des erreurs d'incompatibilité de bibliothèques. Incluez toujours les quatre modes de recherche dans vos fichiers toolchain de cross-compilation.

### Piège n°3 : exécuter des programmes cible pendant la configuration

Certains projets CMake utilisent `try_run()` ou `execute_process()` pour exécuter des programmes pendant la configuration — par exemple pour détecter la taille d'un type ou le support d'une fonctionnalité. En cross-compilation, ces exécutions échouent car le binaire produit est pour l'architecture cible, non pour l'hôte.

CMake fournit `CMAKE_CROSSCOMPILING` (positionné automatiquement à `TRUE` en cross-compilation) pour détecter ce cas et fournir les résultats via des variables de cache au lieu de les détecter au runtime :

```cmake
if(CMAKE_CROSSCOMPILING)
    # Fournir la valeur manuellement — pas de try_run possible
    set(SIZEOF_VOID_P 8 CACHE STRING "Size of void*")
else()
    # Détection automatique
    check_type_size("void*" SIZEOF_VOID_P)
endif()
```

### Piège n°4 : linkage statique vs dynamique

En cross-compilation, le linkage dynamique (`.so`) nécessite que les bibliothèques partagées de la cible soient disponibles au runtime. C'est parfois problématique sur des systèmes embarqués minimalistes. Le linkage statique (`.a`) produit un binaire autonome qui ne dépend de rien au runtime (sauf la libc, dans la plupart des cas) :

```cmake
# Forcer le linkage statique pour la cross-compilation embarquée
set(BUILD_SHARED_LIBS OFF)
```

Pour un binaire totalement autonome (pas même de dépendance sur la libc dynamique) :

```cmake
target_link_options(my_app PRIVATE -static)
```

---

## Organisation recommandée des fichiers toolchain

```
my_project/
├── CMakeLists.txt
├── CMakePresets.json
├── cmake/
│   ├── toolchains/
│   │   ├── clang.cmake              # Build natif avec Clang + lld
│   │   ├── aarch64-linux-gnu.cmake  # Cross ARM 64 bits
│   │   ├── armv7-linux-gnueabihf.cmake  # Cross ARM 32 bits hard float
│   │   └── riscv64-linux-gnu.cmake  # Cross RISC-V 64 bits
│   ├── CompilerWarnings.cmake
│   └── Sanitizers.cmake
└── ...
```

Les fichiers toolchain vivent dans `cmake/toolchains/`, versionnés avec le projet. Chaque fichier est autonome et documenté. Les CMake Presets référencent ces fichiers via `toolchainFile`, offrant une interface utilisateur simple (`cmake --preset cross-arm64`).

---

## Récapitulatif

| Aspect | Recommandation |
|--------|---------------|
| Choix du compilateur (natif) | Fichier toolchain ou `-DCMAKE_CXX_COMPILER` en ligne de commande |
| Cross-compilation | Fichier toolchain obligatoire avec `CMAKE_SYSTEM_NAME` |
| Modes de recherche | Toujours inclure les quatre `CMAKE_FIND_ROOT_PATH_MODE_*` |
| Version du cross-compilateur | `apt install gcc-<triplet>` pour les architectures courantes |
| Sysroot | Fournir via `CMAKE_SYSROOT` si les headers/libs cible ne sont pas dans les chemins par défaut |
| Dépendances | FetchContent (simple), Conan avec profils (robuste), sysroot (système) |
| Test local | QEMU user-mode pour exécuter les binaires cross-compilés |
| Organisation | `cmake/toolchains/` + CMake Presets |
| Changement de compilateur | Toujours recréer le répertoire de build |

---

> **À suivre** : La section 26.7 couvre les nouveautés de CMake 3.31+ et les meilleures pratiques 2026 — les dernières fonctionnalités, les changements de policies, et les recommandations actualisées pour un projet CMake moderne.

⏭️ [CMake 3.31+ : Nouveautés et meilleures pratiques 2026](/26-cmake/07-cmake-3-31-nouveautes.md)
