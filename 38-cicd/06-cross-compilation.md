🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 38.6 Cross-compilation : ARM, RISC-V depuis x86_64

## Introduction

La majorité des runners CI — hébergés ou auto-hébergés — fonctionnent sur des machines x86_64. Pourtant, les projets C++ modernes doivent de plus en plus produire des binaires pour d'autres architectures : ARM64 (aarch64) pour les serveurs Graviton d'AWS et les SBC type Raspberry Pi, ARMv7 pour l'embarqué et l'IoT, et RISC-V pour l'écosystème émergent de processeurs ouverts. Compiler nativement sur chaque architecture cible nécessiterait des runners dédiés pour chacune — un luxe que peu d'organisations peuvent se permettre.

La cross-compilation résout ce problème : un compilateur tournant sur x86_64 produit du code machine pour une architecture différente. Le binaire résultant ne peut pas s'exécuter sur la machine de build, mais il est prêt à être déployé sur la cible. En CI, cela signifie qu'un seul pool de runners x86_64 peut produire des binaires pour ARM64, ARMv7, RISC-V et toute autre architecture supportée par le compilateur.

Cette section couvre la mise en place de la cross-compilation dans un pipeline CI/CD, depuis l'installation des toolchains jusqu'à l'exécution des tests sur l'architecture cible via émulation.

## Concepts fondamentaux

### Terminologie

La cross-compilation introduit une terminologie spécifique qu'il faut maîtriser pour comprendre les fichiers de configuration :

**Host** — La machine qui exécute le compilateur. En CI, c'est le runner, typiquement x86_64.

**Target** — L'architecture pour laquelle le code est généré. C'est l'architecture du binaire produit (ARM64, ARMv7, RISC-V).

**Triplet** — Une chaîne qui identifie complètement une cible de compilation sous la forme `architecture-vendor-os-abi`. Les triplets les plus courants :

| Triplet | Architecture | Usage typique |
|---------|-------------|---------------|
| `x86_64-linux-gnu` | x86_64 | Compilation native classique |
| `aarch64-linux-gnu` | ARM 64-bit | Serveurs ARM, Raspberry Pi 4+, Apple Silicon (Linux) |
| `arm-linux-gnueabihf` | ARM 32-bit (hard float) | Raspberry Pi 2/3, embarqué ARM |
| `riscv64-linux-gnu` | RISC-V 64-bit | SBC RISC-V, simulateurs |

**Sysroot** — Un répertoire contenant les headers et les librairies de la plateforme cible. Le cross-compilateur y cherche les fichiers d'inclusion (`<stdio.h>`, `<vector>`) et les librairies de linkage (`libc.so`, `libstdc++.so`). Sans sysroot correct, le compilateur ne peut pas résoudre les dépendances de la cible.

### Ce qui change par rapport à la compilation native

En compilation native, le compilateur, les headers et les librairies appartiennent tous au même système. En cross-compilation, trois éléments doivent être coordonnés :

1. **Le cross-compilateur** — Un compilateur qui tourne sur x86_64 mais génère du code pour la cible (par exemple `aarch64-linux-gnu-g++`).
2. **Les headers et librairies de la cible** — Les fichiers d'en-tête de la libc, de libstdc++ et des librairies système de la cible.
3. **Les dépendances tierces** — Les librairies du projet (installées via Conan, vcpkg ou le système) doivent être compilées pour l'architecture cible, pas pour x86_64.

Le point 3 est souvent le plus problématique : un `apt-get install libssl-dev` installe la version x86_64 de OpenSSL, inutilisable par un cross-compilateur ARM64. Il faut installer la version ARM64 de la librairie, soit via les paquets multiarch de Debian/Ubuntu, soit via une compilation croisée de la dépendance.

## Installation des toolchains de cross-compilation

### Toolchains GCC sur Ubuntu

Ubuntu fournit des cross-compilateurs GCC prépackagés pour les architectures les plus courantes :

```bash
# ARM64 (aarch64)
sudo apt-get install -y \
  g++-14-aarch64-linux-gnu \
  gcc-14-aarch64-linux-gnu \
  binutils-aarch64-linux-gnu

# ARM 32-bit (hard float)
sudo apt-get install -y \
  g++-14-arm-linux-gnueabihf \
  gcc-14-arm-linux-gnueabihf \
  binutils-arm-linux-gnueabihf

# RISC-V 64-bit
sudo apt-get install -y \
  g++-14-riscv64-linux-gnu \
  gcc-14-riscv64-linux-gnu \
  binutils-riscv64-linux-gnu
```

Après installation, les compilateurs sont disponibles sous leur nom complet avec le triplet en préfixe :

```bash
$ aarch64-linux-gnu-g++-14 --version
aarch64-linux-gnu-g++-14 (Ubuntu 14.2.0-1ubuntu1) 14.2.0

$ aarch64-linux-gnu-g++-14 -dumpmachine
aarch64-linux-gnu
```

Ces paquets incluent automatiquement le sysroot correspondant : les headers et librairies ARM64 de la libc et de libstdc++ sont installés sous `/usr/aarch64-linux-gnu/`.

> ⚠️ **Versions disponibles.** Les versions des cross-compilateurs dans les dépôts Ubuntu peuvent être en retrait par rapport au compilateur natif. Si votre projet nécessite GCC 15 en cross-compilation et que le paquet n'est pas encore disponible, vous devrez soit utiliser un PPA, soit construire le cross-compilateur depuis les sources, soit utiliser une approche Docker (section suivante).

### Toolchain Clang (cross-compilation unifiée)

Clang a une approche fondamentalement différente de GCC pour la cross-compilation : un seul binaire `clang++` peut cibler n'importe quelle architecture supportée. Il n'y a pas de `aarch64-linux-gnu-clang++` — on utilise le même `clang++` avec l'option `--target` :

```bash
# Clang cible ARM64
clang++-20 --target=aarch64-linux-gnu \
  --sysroot=/usr/aarch64-linux-gnu \
  -o hello hello.cpp

# Clang cible RISC-V 64
clang++-20 --target=riscv64-linux-gnu \
  --sysroot=/usr/riscv64-linux-gnu \
  -o hello hello.cpp
```

Cette architecture unifiée simplifie la gestion des toolchains en CI : une seule installation de Clang suffit pour toutes les cibles. Il faut cependant installer les sysroots séparément (les headers et librairies de chaque architecture cible) :

```bash
# Installer les sysroots sans le compilateur GCC complet
sudo apt-get install -y \
  libc6-dev-arm64-cross \
  libstdc++-14-dev-arm64-cross \
  libc6-dev-riscv64-cross \
  libstdc++-14-dev-riscv64-cross
```

Clang a également besoin du linker de la cible. Par défaut, il tente d'utiliser `ld`, qui est le linker x86_64. Deux options :

```bash
# Option 1 : utiliser le linker GNU de la cible
clang++-20 --target=aarch64-linux-gnu \
  -fuse-ld=/usr/bin/aarch64-linux-gnu-ld \
  ...

# Option 2 : utiliser LLD (linker LLVM, multi-cible comme Clang)
clang++-20 --target=aarch64-linux-gnu \
  -fuse-ld=lld \
  ...
```

LLD (le linker LLVM) est recommandé pour la cross-compilation avec Clang car, comme Clang lui-même, un seul binaire supporte toutes les cibles. C'est la combinaison la plus propre : `clang++ --target=<triplet> -fuse-ld=lld` suffit pour cibler n'importe quelle architecture.

### Image Docker de cross-compilation

Pour un pipeline CI reproductible, une image Docker dédiée à la cross-compilation est l'approche la plus fiable :

```dockerfile
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Activer le support multiarch
RUN dpkg --add-architecture arm64 && \
    dpkg --add-architecture armhf

RUN apt-get update && apt-get install -y --no-install-recommends \
    # Toolchain native
    g++-15 cmake ninja-build ccache \
    # Cross-compilateurs GCC
    g++-14-aarch64-linux-gnu \
    g++-14-arm-linux-gnueabihf \
    g++-14-riscv64-linux-gnu \
    # Clang (cross-compilation unifiée)
    clang-20 lld-20 \
    # Sysroots
    libc6-dev-arm64-cross libstdc++-14-dev-arm64-cross \
    libc6-dev-armhf-cross libstdc++-14-dev-armhf-cross \
    libc6-dev-riscv64-cross libstdc++-14-dev-riscv64-cross \
    # Émulation pour les tests
    qemu-user-static binfmt-support \
    && rm -rf /var/lib/apt/lists/*
```

Cette image contient tout le nécessaire pour cross-compiler vers ARM64, ARMv7 et RISC-V, avec le choix entre GCC et Clang, et l'émulation QEMU pour exécuter les tests (détaillée plus loin).

## Fichiers toolchain CMake

CMake gère la cross-compilation via des **fichiers toolchain** — des fichiers CMake qui décrivent la cible et les outils de compilation. Le fichier toolchain est passé à CMake via `-DCMAKE_TOOLCHAIN_FILE=<chemin>`.

### Fichier toolchain GCC pour ARM64

```cmake
# cmake/toolchains/aarch64-linux-gnu-gcc.cmake

# Système cible
set(CMAKE_SYSTEM_NAME Linux)  
set(CMAKE_SYSTEM_PROCESSOR aarch64)  

# Triplet (réutilisable dans les chemins)
set(CROSS_TRIPLET "aarch64-linux-gnu")

# Compilateurs
set(CMAKE_C_COMPILER   ${CROSS_TRIPLET}-gcc-14)  
set(CMAKE_CXX_COMPILER ${CROSS_TRIPLET}-g++-14)  

# Sysroot (headers et librairies de la cible)
set(CMAKE_SYSROOT /usr/${CROSS_TRIPLET})

# Où chercher les librairies et headers de la cible
set(CMAKE_FIND_ROOT_PATH /usr/${CROSS_TRIPLET})

# Ne pas chercher les programmes (comme protoc) dans le sysroot —
# ils doivent tourner sur l'hôte, pas sur la cible
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Chercher les librairies et headers uniquement dans le sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)  
```

### Fichier toolchain Clang pour ARM64

```cmake
# cmake/toolchains/aarch64-linux-gnu-clang.cmake

set(CMAKE_SYSTEM_NAME Linux)  
set(CMAKE_SYSTEM_PROCESSOR aarch64)  

set(CROSS_TRIPLET "aarch64-linux-gnu")

# Clang : même binaire, option --target
set(CMAKE_C_COMPILER   clang-20)  
set(CMAKE_CXX_COMPILER clang++-20)  
set(CMAKE_C_COMPILER_TARGET   ${CROSS_TRIPLET})  
set(CMAKE_CXX_COMPILER_TARGET ${CROSS_TRIPLET})  

# Utiliser LLD comme linker (multi-cible)
set(CMAKE_EXE_LINKER_FLAGS_INIT    "-fuse-ld=lld-20")  
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld-20")  

set(CMAKE_SYSROOT /usr/${CROSS_TRIPLET})  
set(CMAKE_FIND_ROOT_PATH /usr/${CROSS_TRIPLET})  

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)  
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)  
```

### Fichier toolchain pour RISC-V 64

```cmake
# cmake/toolchains/riscv64-linux-gnu-gcc.cmake

set(CMAKE_SYSTEM_NAME Linux)  
set(CMAKE_SYSTEM_PROCESSOR riscv64)  

set(CROSS_TRIPLET "riscv64-linux-gnu")

set(CMAKE_C_COMPILER   ${CROSS_TRIPLET}-gcc-14)  
set(CMAKE_CXX_COMPILER ${CROSS_TRIPLET}-g++-14)  

set(CMAKE_SYSROOT /usr/${CROSS_TRIPLET})  
set(CMAKE_FIND_ROOT_PATH /usr/${CROSS_TRIPLET})  

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)  
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)  
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)  
```

### Conventions de rangement

Les fichiers toolchain sont versionnés dans le dépôt, typiquement sous `cmake/toolchains/` :

```
cmake/
└── toolchains/
    ├── aarch64-linux-gnu-gcc.cmake
    ├── aarch64-linux-gnu-clang.cmake
    ├── arm-linux-gnueabihf-gcc.cmake
    ├── riscv64-linux-gnu-gcc.cmake
    └── native.cmake              # Optionnel : compilation native explicite
```

La commande CMake en cross-compilation devient :

```bash
cmake -B build-arm64 \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu-gcc.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm64 --parallel $(nproc)
```

Les trois directives `CMAKE_FIND_ROOT_PATH_MODE_*` sont les plus importantes et les plus mal comprises :

**`PROGRAM NEVER`** — Les programmes (outils de génération de code comme `protoc`, scripts de build) doivent être exécutables sur l'hôte. Un `protoc` compilé pour ARM64 ne peut pas tourner sur un runner x86_64. CMake doit donc chercher les programmes en dehors du sysroot, dans les chemins standard de l'hôte.

**`LIBRARY ONLY` et `INCLUDE ONLY`** — Les librairies et les headers doivent provenir exclusivement du sysroot de la cible. Linker un binaire ARM64 avec une librairie x86_64 produirait un binaire corrompu. Le mode `ONLY` empêche CMake de "tomber" accidentellement sur les librairies x86_64 installées sur l'hôte.

## Intégration dans le pipeline CI

### GitLab CI

```yaml
.cross_build:
  stage: build
  image: registry.exemple.com/cpp-cross-build:latest
  script:
    - cmake -B ${BUILD_DIR}
        -G Ninja
        -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/${TOOLCHAIN_FILE}
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
        -DCMAKE_CXX_STANDARD=${CPP_STANDARD}
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
    - cmake --build ${BUILD_DIR} --parallel $(nproc)
  cache:
    paths:
      - .ccache/
    policy: pull-push
  artifacts:
    paths:
      - ${BUILD_DIR}/bin/
      - ${BUILD_DIR}/lib/
      - ${BUILD_DIR}/tests/
      - ${BUILD_DIR}/CTestTestfile.cmake
    expire_in: 2 hours

build-arm64-gcc:
  extends: .cross_build
  variables:
    TOOLCHAIN_FILE: "aarch64-linux-gnu-gcc.cmake"
    BUILD_TYPE: "Release"
    CPP_STANDARD: "20"
    BUILD_DIR: "build-arm64"
  cache:
    key: "ccache-arm64-gcc-${CI_COMMIT_REF_SLUG}"

build-riscv64-gcc:
  extends: .cross_build
  variables:
    TOOLCHAIN_FILE: "riscv64-linux-gnu-gcc.cmake"
    BUILD_TYPE: "Release"
    CPP_STANDARD: "20"
    BUILD_DIR: "build-riscv64"
  cache:
    key: "ccache-riscv64-gcc-${CI_COMMIT_REF_SLUG}"
```

Le cache ccache est séparé par architecture cible : le cache d'un build ARM64 n'est pas réutilisable par un build RISC-V (les fichiers objets sont évidemment incompatibles).

### GitHub Actions

```yaml
jobs:
  cross-build:
    name: "Cross-build — ${{ matrix.target }}"
    runs-on: ubuntu-latest
    strategy:
      fail-fast: false
      matrix:
        include:
          - target: aarch64
            toolchain: aarch64-linux-gnu-gcc.cmake
            packages: g++-14-aarch64-linux-gnu
            qemu_arch: aarch64
          - target: armv7
            toolchain: arm-linux-gnueabihf-gcc.cmake
            packages: g++-14-arm-linux-gnueabihf
            qemu_arch: arm
          - target: riscv64
            toolchain: riscv64-linux-gnu-gcc.cmake
            packages: g++-14-riscv64-linux-gnu
            qemu_arch: riscv64

    steps:
      - uses: actions/checkout@v4

      - name: Install cross-compilation toolchain
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            ${{ matrix.packages }} \
            ninja-build ccache \
            qemu-user-static

      - name: Setup ccache
        uses: hendrikmuhs/ccache-action@v1
        with:
          key: cross-${{ matrix.target }}
          max-size: 1G

      - name: Cross-compile
        run: |
          cmake -B build-${{ matrix.target }} \
            -G Ninja \
            -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/${{ matrix.toolchain }} \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_CXX_STANDARD=20 \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
          cmake --build build-${{ matrix.target }} --parallel $(nproc)

      - name: Verify binary architecture
        run: |
          file build-${{ matrix.target }}/bin/* | head -5
          # Doit afficher "ELF 64-bit LSB ... ARM aarch64" ou similaire

      - name: Upload cross-compiled artifacts
        uses: actions/upload-artifact@v4
        with:
          name: build-${{ matrix.target }}
          path: |
            build-${{ matrix.target }}/bin/
            build-${{ matrix.target }}/lib/
            build-${{ matrix.target }}/tests/
            build-${{ matrix.target }}/CTestTestfile.cmake
```

Le step **"Verify binary architecture"** est une vérification de sanité importante. La commande `file` affiche le format du binaire et son architecture cible. Si le fichier toolchain est mal configuré, le binaire produit pourrait être en x86_64 au lieu de ARM64 — ce step détecte immédiatement ce problème :

```bash
$ file build-arm64/bin/mon-application
build-arm64/bin/mon-application: ELF 64-bit LSB pie executable, ARM aarch64, version 1 (GNU/Linux), dynamically linked, ...

$ file build-riscv64/bin/mon-application
build-riscv64/bin/mon-application: ELF 64-bit LSB pie executable, UCB RISC-V, RVC, double-float ABI, version 1 (GNU/Linux), dynamically linked, ...
```

## Exécution des tests via émulation QEMU

Le binaire cross-compilé ne peut pas s'exécuter nativement sur le runner x86_64. Pour exécuter les tests sans matériel ARM ou RISC-V, on utilise **QEMU user-mode emulation** : QEMU intercepte les instructions de l'architecture cible et les traduit en instructions x86_64 à la volée.

### Configuration de QEMU en CI

```bash
# Installation
sudo apt-get install -y qemu-user-static binfmt-support

# Vérification
qemu-aarch64-static --version
```

Le paquet `binfmt-support` enregistre QEMU comme interpréteur pour les binaires ELF d'architectures étrangères. Après installation, le noyau Linux redirige automatiquement l'exécution de tout binaire ARM64 vers `qemu-aarch64-static`, de manière transparente :

```bash
# Ceci fonctionne sur un hôte x86_64 grâce à binfmt_misc + QEMU
$ ./build-arm64/bin/mon-application --version
mon-application v1.2.3
```

Le binaire ARM64 s'exécute comme s'il était natif — sans commande `qemu-aarch64-static` explicite. Cette transparence est essentielle pour CTest, qui lance les binaires de test sans savoir qu'ils sont cross-compilés.

### Exécution des tests cross-compilés

```yaml
# GitLab CI
test-arm64:
  stage: test
  image: registry.exemple.com/cpp-cross-build:latest
  needs:
    - job: build-arm64-gcc
      artifacts: true
  variables:
    QEMU_LD_PREFIX: "/usr/aarch64-linux-gnu"
  script:
    - chmod +x build-arm64/tests/* build-arm64/bin/* 2>/dev/null || true
    - cd build-arm64
    - ctest --parallel $(nproc) --timeout 300 --output-on-failure
  allow_failure: true
```

```yaml
# GitHub Actions
  test-cross:
    name: "Test — ${{ matrix.target }}"
    runs-on: ubuntu-latest
    needs: [cross-build]
    strategy:
      matrix:
        include:
          - target: aarch64
            qemu_ld_prefix: /usr/aarch64-linux-gnu
          - target: riscv64
            qemu_ld_prefix: /usr/riscv64-linux-gnu
    steps:
      - uses: actions/checkout@v4

      - name: Install QEMU
        run: sudo apt-get install -y qemu-user-static binfmt-support

      - uses: actions/download-artifact@v4
        with:
          name: build-${{ matrix.target }}
          path: build-${{ matrix.target }}/

      - name: Run cross-compiled tests
        env:
          QEMU_LD_PREFIX: ${{ matrix.qemu_ld_prefix }}
        run: |
          chmod +x build-${{ matrix.target }}/tests/* || true
          cd build-${{ matrix.target }}
          ctest --parallel 2 --timeout 300 --output-on-failure
```

**`QEMU_LD_PREFIX`** est une variable d'environnement cruciale. Elle indique à QEMU où trouver les librairies partagées de la cible. Un binaire ARM64 linké dynamiquement a besoin de `libc.so.6` et `libstdc++.so.6` en version ARM64 — ces librairies se trouvent sous `/usr/aarch64-linux-gnu/`, pas dans les chemins standards `/lib/` et `/usr/lib/` qui contiennent les versions x86_64. Sans `QEMU_LD_PREFIX`, QEMU tente de charger les librairies x86_64 dans un processus ARM64, ce qui échoue immédiatement.

**Parallélisme réduit** (`--parallel 2`). L'émulation QEMU est significativement plus lente que l'exécution native — un facteur 5x à 20x est courant. Exécuter trop de tests en parallèle sous émulation peut saturer le CPU et la mémoire du runner. Deux tests simultanés sont un point de départ prudent.

**`allow_failure: true`** (GitLab CI) est parfois utilisé pour les tests cross-compilés car certains tests sensibles au timing (tests de concurrence, benchmarks) peuvent échouer sous émulation en raison de la lenteur et du comportement non-déterministe de QEMU. C'est un compromis pragmatique — l'objectif est de valider la fonctionnalité, pas les performances.

**Timeout augmenté** (`--timeout 300`). Le facteur de ralentissement de QEMU impose un timeout plus généreux que pour les tests natifs.

### Limites de l'émulation QEMU

L'émulation en user-mode couvre la grande majorité des cas mais présente des limites :

| Limitation | Impact | Contournement |
|-----------|--------|---------------|
| Performance 5-20x plus lente | Tests longs, benchmarks inutilisables | Accepter la lenteur ou tester sur du matériel réel |
| Threads : comportement de scheduling différent | Tests de concurrence potentiellement non représentatifs | Tests de concurrence sur matériel réel ou `allow_failure` |
| Appels système exotiques | Quelques syscalls non implémentés | Rare en pratique — la majorité des applications fonctionnent |
| Pas d'accès matériel (GPIO, périphériques) | Tests matériels impossibles | Nécessite du matériel réel (hors scope CI standard) |

Pour les projets où l'émulation est insuffisante (embarqué avec accès matériel, tests de performance critiques), un runner CI auto-hébergé sur du matériel ARM est la solution. Un Raspberry Pi 4 ou 5 peut servir de runner GitLab CI ou GitHub Actions pour les tests ARM64 — la compilation reste sur x86_64, seuls les tests s'exécutent sur ARM.

## Gestion des dépendances en cross-compilation

### Multiarch Debian

Ubuntu et Debian supportent l'installation de librairies pour plusieurs architectures simultanément via le système **multiarch** :

```bash
# Activer l'architecture ARM64
sudo dpkg --add-architecture arm64

# Installer une librairie pour ARM64
sudo apt-get update  
sudo apt-get install -y libssl-dev:arm64 zlib1g-dev:arm64  
```

Le suffixe `:arm64` indique au gestionnaire de paquets d'installer la version ARM64 de la librairie. Les fichiers sont installés sous `/usr/lib/aarch64-linux-gnu/` et sont trouvés automatiquement par CMake lorsque le `CMAKE_FIND_ROOT_PATH` pointe vers `/usr/aarch64-linux-gnu`.

Le multiarch fonctionne bien pour les dépendances système courantes (OpenSSL, zlib, libcurl), mais n'est pas toujours disponible pour les librairies plus spécialisées.

### Conan 2.0 avec profils de cross-compilation

Conan est le gestionnaire de dépendances le mieux adapté à la cross-compilation en C++. Il utilise un système de **profils** qui décrivent séparément la machine hôte et la machine cible :

```ini
# ~/.conan2/profiles/arm64
[settings]
os=Linux  
arch=armv8  
compiler=gcc  
compiler.version=14  
compiler.libcxx=libstdc++11  
build_type=Release  

[conf]
tools.cmake.cmaketoolchain:toolchain_file=cmake/toolchains/aarch64-linux-gnu-gcc.cmake

[buildenv]
CC=aarch64-linux-gnu-gcc-14  
CXX=aarch64-linux-gnu-g++-14  
```

L'installation des dépendances cross-compilées :

```bash
# --profile:host = cible, --profile:build = machine de build
conan install conanfile.py \
  --profile:host=arm64 \
  --profile:build=default \
  --build=missing
```

Le paramètre `--build=missing` compile les dépendances qui n'existent pas en version ARM64 dans le cache Conan. La première exécution est lente (chaque dépendance est cross-compilée), mais les résultats sont cachés et réutilisés lors des builds suivants.

### vcpkg avec triplets personnalisés

vcpkg supporte également la cross-compilation via ses triplets :

```cmake
# vcpkg/triplets/community/arm64-linux.cmake
set(VCPKG_TARGET_ARCHITECTURE arm64)  
set(VCPKG_CRT_LINKAGE dynamic)  
set(VCPKG_LIBRARY_LINKAGE static)  
set(VCPKG_CMAKE_SYSTEM_NAME Linux)  
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE  
    "${CMAKE_CURRENT_LIST_DIR}/../../cmake/toolchains/aarch64-linux-gnu-gcc.cmake")
```

```bash
vcpkg install openssl zlib --triplet=arm64-linux
```

## Packaging multi-architecture

Pour distribuer des paquets pour plusieurs architectures, le pipeline de release doit produire un artifact par cible :

```yaml
# GitHub Actions — release multi-architecture
jobs:
  release-assets:
    strategy:
      matrix:
        include:
          - target: linux-amd64
            toolchain: ""                    # Compilation native
            arch_deb: amd64
          - target: linux-arm64
            toolchain: cmake/toolchains/aarch64-linux-gnu-gcc.cmake
            arch_deb: arm64
          - target: linux-riscv64
            toolchain: cmake/toolchains/riscv64-linux-gnu-gcc.cmake
            arch_deb: riscv64

    steps:
      - uses: actions/checkout@v4

      - name: Install toolchain
        run: |
          sudo apt-get update
          sudo apt-get install -y ninja-build ccache
          if [ -n "${{ matrix.toolchain }}" ]; then
            sudo apt-get install -y \
              g++-14-$(echo ${{ matrix.target }} | sed 's/linux-//')-linux-gnu \
              || true
          fi

      - name: Build
        run: |
          TOOLCHAIN_ARG=""
          if [ -n "${{ matrix.toolchain }}" ]; then
            TOOLCHAIN_ARG="-DCMAKE_TOOLCHAIN_FILE=${{ matrix.toolchain }}"
          fi
          cmake -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            ${TOOLCHAIN_ARG} \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
          cmake --build build --parallel $(nproc)
          strip build/bin/* 2>/dev/null || true

      - name: Create archive
        run: |
          VERSION="${GITHUB_REF_NAME#v}"
          ARCHIVE="mon-projet-${VERSION}-${{ matrix.target }}"
          mkdir -p ${ARCHIVE}/bin
          cp build/bin/* ${ARCHIVE}/bin/
          cp LICENSE README.md ${ARCHIVE}/
          tar czf "${ARCHIVE}.tar.gz" ${ARCHIVE}/
          sha256sum "${ARCHIVE}.tar.gz" > "${ARCHIVE}.tar.gz.sha256"

      - uses: actions/upload-artifact@v4
        with:
          name: release-${{ matrix.target }}
          path: |
            *.tar.gz
            *.sha256
```

La page de release résultante propose des téléchargements pour chaque architecture :

```
Assets:
  mon-projet-1.2.3-linux-amd64.tar.gz         (3.2 MB)
  mon-projet-1.2.3-linux-amd64.tar.gz.sha256
  mon-projet-1.2.3-linux-arm64.tar.gz          (2.9 MB)
  mon-projet-1.2.3-linux-arm64.tar.gz.sha256
  mon-projet-1.2.3-linux-riscv64.tar.gz        (3.0 MB)
  mon-projet-1.2.3-linux-riscv64.tar.gz.sha256
```

### Images Docker multi-architecture

Docker supporte les images multi-architecture via les **manifestes multi-plateforme**. Une seule référence d'image (`mon-projet:1.2.3`) peut contenir des variantes pour x86_64, ARM64, et d'autres architectures. Docker sélectionne automatiquement la bonne variante en fonction de l'architecture de la machine qui exécute `docker pull`.

L'action `docker/build-push-action` combinée avec QEMU supporte cette fonctionnalité :

```yaml
- name: Set up QEMU
  uses: docker/setup-qemu-action@v3

- name: Set up Docker Buildx
  uses: docker/setup-buildx-action@v3

- name: Build and push multi-arch image
  uses: docker/build-push-action@v6
  with:
    context: .
    platforms: linux/amd64,linux/arm64
    push: true
    tags: ghcr.io/${{ github.repository }}:${{ github.ref_name }}
```

Avec `platforms: linux/amd64,linux/arm64`, Docker Buildx exécute le Dockerfile une fois par architecture (en utilisant QEMU pour émuler ARM64) et crée un manifeste unifié. L'utilisateur final n'a aucune manipulation à faire — `docker pull` récupère automatiquement la bonne variante.

> ⚠️ **Performance.** Le build Docker sous émulation QEMU est lent — la compilation C++ à l'intérieur d'un Dockerfile émulé ARM64 peut être 10-20x plus lente qu'en natif. L'approche recommandée est de cross-compiler les binaires en dehors de Docker (avec les toolchains décrites plus haut), puis de ne faire que le `COPY` des binaires pré-compilés dans un Dockerfile minimal, sans compilation intra-Docker.

## Résumé : matrice de cross-compilation type

| Élément | x86_64 (natif) | ARM64 | ARMv7 | RISC-V 64 |
|---------|----------------|-------|-------|-----------|
| Cross-compilateur GCC | — | `aarch64-linux-gnu-g++-14` | `arm-linux-gnueabihf-g++-14` | `riscv64-linux-gnu-g++-14` |
| Cross-compilateur Clang | — | `clang++-20 --target=aarch64-linux-gnu` | `clang++-20 --target=arm-linux-gnueabihf` | `clang++-20 --target=riscv64-linux-gnu` |
| Fichier toolchain CMake | Non requis | `aarch64-linux-gnu-gcc.cmake` | `arm-linux-gnueabihf-gcc.cmake` | `riscv64-linux-gnu-gcc.cmake` |
| Tests via QEMU | Natif | `qemu-aarch64-static` | `qemu-arm-static` | `qemu-riscv64-static` |
| `QEMU_LD_PREFIX` | — | `/usr/aarch64-linux-gnu` | `/usr/arm-linux-gnueabihf` | `/usr/riscv64-linux-gnu` |
| Paquet DEB arch | `amd64` | `arm64` | `armhf` | `riscv64` |
| Profil Conan arch | `x86_64` | `armv8` | `armv7hf` | `riscv64` |

---

> **Section suivante** : 38.7 Matrix builds : Multi-compilateur, multi-version — Stratégies avancées pour les matrices de build : combinaisons compilateur × standard × architecture × sanitizer, matrices conditionnelles, et optimisation du temps total de pipeline.

⏭️ [Matrix builds : Multi-compilateur, multi-version](/38-cicd/07-matrix-builds.md)
