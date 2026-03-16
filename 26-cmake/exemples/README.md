# Exemples du Chapitre 26 — CMake : Le Standard de l'Industrie

Ce répertoire contient les exemples pratiques du chapitre 26. Chaque exemple est un **mini-projet CMake autonome** qui illustre un concept ou un mécanisme spécifique.

## Prérequis

```bash
# Compilateur C++ (GCC 15)
g++-15 --version

# CMake 3.25+
cmake --version

# Ninja (build system)
ninja --version

# Cross-compilateurs (pour l'exemple 06)
aarch64-linux-gnu-g++ --version  
riscv64-linux-gnu-g++ --version  

# QEMU user-mode (pour l'exemple 06)
qemu-aarch64 --version
```

---

## Liste des exemples

### 01\_projet\_structure

| | |
|---|---|
| **Section** | 26.1, 26.2, 26.2.1, 26.2.2, 26.2.3, 26.2.4 |
| **Fichiers .md** | `01-structure-projet.md`, `02-cmakelists.md`, `02.1-add-executable-library.md`, `02.2-target-link-libraries.md`, `02.3-target-include-directories.md`, `02.4-public-private-interface.md` |
| **Description** | Projet complet illustrant l'arborescence de référence CMake : cibles (`add_library`, `add_executable`), alias namespacés, visibilité `PUBLIC`/`PRIVATE`, propagation transitive, module de warnings personnalisé, et CTest. |

**Structure :**
```
01_projet_structure/
├── CMakeLists.txt                  # Racine du projet
├── cmake/CompilerWarnings.cmake    # Module CMake personnalisé
├── include/my_project/             # Headers publics (core.h, network.h)
├── src/                            # Bibliothèque principale + headers privés
├── libs/utils/                     # Bibliothèque interne (string_utils)
├── apps/main.cpp                   # Exécutable (my-app)
└── tests/test_core.cpp             # Test unitaire
```

**Compilation et exécution :**
```bash
cd 01_projet_structure  
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build  
./build/apps/my-app
cd build && ctest --output-on-failure
```

**Sortie attendue :**
```
[Hello, WORLD!]
Host: localhost:8080
```
```
Test project .../build
    Start 1: test_core
1/1 Test #1: test_core ........................   Passed
```

---

### 02\_configure\_file

| | |
|---|---|
| **Section** | 26.4 |
| **Fichier .md** | `04-configuration-generation.md` |
| **Description** | Illustration de `configure_file()` pour générer des headers de version (`version.h`), de configuration (`config.h`) et d'informations de build (`build_info.h`) à partir de templates `.in`. |

**Fichiers template :**
- `version.h.in` — version du projet via `@PROJECT_VERSION@`
- `config.h.in` — `#cmakedefine01` pour dépendances optionnelles (ZLIB, SSL)
- `build_info.h.in` — hash Git et date de build

**Compilation et exécution :**
```bash
cd 02_configure_file  
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15 -DCMAKE_BUILD_TYPE=Release  
cmake --build build  
./build/show_config
```

**Sortie attendue :**
```
=== Version ===
Version : 1.2.0  
Major   : 1  
Minor   : 2  
Patch   : 0  

=== Configuration ===
System  : Linux  
Compiler: GNU 15.2.0  
Build   : Release  
ZLIB    : oui  
SSL     : oui  

=== Build Info ===
Commit  : <hash git ou "unknown">  
Date    : <date UTC du build>  
```

---

### 03\_fetchcontent

| | |
|---|---|
| **Section** | 26.3.2 |
| **Fichier .md** | `03.2-fetchcontent.md` |
| **Description** | Téléchargement et intégration automatique de GoogleTest via `FetchContent`. Illustre `FetchContent_Declare`, `FetchContent_MakeAvailable`, désactivation des options superflues (`BUILD_GMOCK OFF`), et lancement des tests via CTest. |

**Compilation et exécution :**
```bash
cd 03_fetchcontent  
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build  
cd build && ctest --output-on-failure  
```

**Sortie attendue (CTest) :**
```
Test project .../build
    Start 1: my_test
1/1 Test #1: my_test ..........................   Passed
100% tests passed, 0 tests failed out of 1
```

> **Note** : la première configuration télécharge GoogleTest (~2s). Les builds suivants réutilisent le cache dans `build/_deps/`.

---

### 04\_interface\_targets

| | |
|---|---|
| **Section** | 26.2.4 |
| **Fichier .md** | `02.4-public-private-interface.md` |
| **Description** | Cibles `INTERFACE` comme collections de propriétés : `project_warnings` (flags de warning), `project_sanitizers` (AddressSanitizer/ThreadSanitizer), `project_config` (standard C++, définitions Debug/Release). |

**Compilation et exécution :**
```bash
cd 04_interface_targets

# Build Debug
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15 -DCMAKE_BUILD_TYPE=Debug  
cmake --build build  
./build/demo

# Build Release
rm -rf build  
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15 -DCMAKE_BUILD_TYPE=Release  
cmake --build build  
./build/demo

# Avec AddressSanitizer
rm -rf build  
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15 -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON  
cmake --build build  
./build/demo
```

**Sortie attendue (Debug) :**
```
Mode: Debug  
Les cibles INTERFACE fonctionnent !  
```

**Sortie attendue (Release) :**
```
Mode: Release  
Les cibles INTERFACE fonctionnent !  
```

---

### 05\_ninja\_multiconfig

| | |
|---|---|
| **Section** | 26.5 |
| **Fichier .md** | `05-generation-ninja.md` |
| **Description** | Utilisation du générateur `Ninja Multi-Config` pour gérer Debug et Release dans un seul répertoire de build. Une seule configuration, deux builds. |

**Compilation et exécution :**
```bash
cd 05_ninja_multiconfig

# Configuration unique (pas de CMAKE_BUILD_TYPE)
cmake -B build -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=g++-15

# Build Debug et Release
cmake --build build --config Debug  
cmake --build build --config Release  

# Exécution
./build/Debug/hello
./build/Release/hello
```

**Sortie attendue :**
```
Configuration: Debug (NDEBUG non défini)  
Ninja Multi-Config fonctionne !  
```
```
Configuration: Release (NDEBUG défini)  
Ninja Multi-Config fonctionne !  
```

---

### 06\_cross\_compilation

| | |
|---|---|
| **Section** | 26.6 |
| **Fichier .md** | `06-toolchains-cross-compilation.md` |
| **Description** | Cross-compilation pour ARM64 (AArch64) et RISC-V 64 bits via fichiers toolchain. Vérification de l'architecture avec `file(1)` et exécution via QEMU user-mode. |

**Prérequis :**
```bash
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
                 gcc-riscv64-linux-gnu g++-riscv64-linux-gnu \
                 qemu-user qemu-user-binfmt
```

**Compilation et exécution :**
```bash
cd 06_cross_compilation

# ARM64
cmake -B build-arm64 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake
cmake --build build-arm64  
file build-arm64/hello  
qemu-aarch64 -L /usr/aarch64-linux-gnu build-arm64/hello  

# RISC-V 64
cmake -B build-riscv64 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-linux-gnu.cmake
cmake --build build-riscv64  
file build-riscv64/hello  
qemu-riscv64 -L /usr/riscv64-linux-gnu build-riscv64/hello  
```

**Sortie attendue (ARM64) :**
```
build-arm64/hello: ELF 64-bit LSB pie executable, ARM aarch64, ...  
Architecture: ARM64 (AArch64)  
Cross-compilation fonctionne !  
```

**Sortie attendue (RISC-V 64) :**
```
build-riscv64/hello: ELF 64-bit LSB pie executable, UCB RISC-V, ...  
Architecture: RISC-V  
Cross-compilation fonctionne !  
```

---

### 07\_cmake\_presets

| | |
|---|---|
| **Section** | 26.5, 26.6 |
| **Fichiers .md** | `05-generation-ninja.md`, `06-toolchains-cross-compilation.md` |
| **Description** | Standardisation des configurations via `CMakePresets.json` — presets `default` (Debug + Ninja) et `release` (Release, hérite de default). |

**Compilation et exécution :**
```bash
cd 07_cmake_presets

# Utiliser le preset "default"
cmake --preset default  
cmake --build build  
./build/hello

# Ou le preset "release"
rm -rf build  
cmake --preset release  
cmake --build build  
./build/hello
```

**Sortie attendue (preset default) :**
```
Preset: default/debug (NDEBUG non défini)  
CMake Presets fonctionnent !  
```

**Sortie attendue (preset release) :**
```
Preset: release (NDEBUG défini)  
CMake Presets fonctionnent !  
```

---

## Nettoyage

Pour nettoyer tous les répertoires de build :

```bash
# Depuis le répertoire exemples/
rm -rf */build */build-*
```
