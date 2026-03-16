# Exemples du Chapitre 27 — Gestion des Dépendances

Ce répertoire contient les exemples pratiques du chapitre 27. Les exemples se concentrent sur les aspects compilables et testables : linkage statique/dynamique, installation CMake, et CMake Presets.

> **Note** : Les sections 27.2 (Conan) et 27.3 (vcpkg) contiennent des fichiers de configuration (`conanfile.py`, `vcpkg.json`, profils) qui ne sont pas des programmes C++ compilables. Ces configurations sont documentées directement dans les .md et nécessitent Conan ou vcpkg installés pour être testées.

## Prérequis

```bash
g++-15 --version    # GCC 15  
cmake --version     # CMake 3.25+  
ninja --version     # Ninja  
```

---

## Liste des exemples

### 01\_linkage\_statique\_dynamique

| | |
|---|---|
| **Section** | 27.4 |
| **Fichier .md** | `04-linkage-statique-dynamique.md` |
| **Description** | Création manuelle (sans CMake) d'une bibliothèque statique (`.a` via `ar`) et dynamique (`.so` via `-shared`), linkage, comparaison des tailles, vérification avec `ldd`, `nm`, `file`. |

**Fichiers :**
- `greeter.h` / `greeter.cpp` — bibliothèque greeter
- `main.cpp` — programme utilisant greeter
- `build.sh` — script de build démontrant les deux modes

**Exécution :**
```bash
cd 01_linkage_statique_dynamique  
bash build.sh  
```

**Sortie attendue :**
```
=== 4. Linkage STATIQUE (forcer avec -Bstatic) ===
Taille   : 31K  
Deps     : 0 dépendance(s) greeter  
Sortie   :  
Hello, world!  
Version: 1.0.0  

=== 5. Linkage DYNAMIQUE ===
Taille   : 25K  
Deps     :  
	libgreeter.so => not found
Sortie   :  
Hello, world!  
Version: 1.0.0  
```

**Comportement attendu :**
- Le binaire statique n'a aucune dépendance `greeter` (le code est embarqué)
- Le binaire dynamique dépend de `libgreeter.so` (chargé au runtime)
- Le script nettoie automatiquement les artefacts en fin d'exécution

---

### 02\_cmake\_linkage

| | |
|---|---|
| **Section** | 27.4 |
| **Fichier .md** | `04-linkage-statique-dynamique.md` |
| **Description** | Projet CMake illustrant `BUILD_SHARED_LIBS` : le même `CMakeLists.txt` produit une `.a` ou une `.so` selon la valeur de `-DBUILD_SHARED_LIBS`. |

**Compilation et exécution :**
```bash
cd 02_cmake_linkage

# Mode statique (.a)
cmake -B build-static -G Ninja -DCMAKE_CXX_COMPILER=g++-15 -DBUILD_SHARED_LIBS=OFF  
cmake --build build-static  
file build-static/src/libgreeter.a  
./build-static/apps/demo

# Mode dynamique (.so)
cmake -B build-shared -G Ninja -DCMAKE_CXX_COMPILER=g++-15 -DBUILD_SHARED_LIBS=ON  
cmake --build build-shared  
file build-shared/src/libgreeter.so  
LD_LIBRARY_PATH=build-shared/src ./build-shared/apps/demo  
```

**Sortie attendue :**
```
Hello, CMake!
```

**Comportement attendu :**
- `BUILD_SHARED_LIBS=OFF` → `libgreeter.a` (archive statique)
- `BUILD_SHARED_LIBS=ON` → `libgreeter.so` (shared object)
- Le programme affiche le même résultat dans les deux cas

---

### 03\_installation\_cmake

| | |
|---|---|
| **Section** | 27.5 |
| **Fichier .md** | `05-distribution-linux.md` |
| **Description** | Projet CMake complet avec règles d'installation : `install(TARGETS)`, `install(EXPORT)`, `GNUInstallDirs`, `configure_package_config_file`, fichier `pkg-config`. Inclut un sous-projet consommateur pour tester `find_package()`. |

**Structure :**
```
03_installation_cmake/
├── CMakeLists.txt              # Bibliothèque avec règles install()
├── include/mylib/greet.h       # Header public
├── src/greet.cpp               # Implémentation
├── cmake/
│   ├── mylibConfig.cmake.in    # Template Config CMake
│   └── mylib.pc.in             # Template pkg-config
└── test_consumer/              # Projet consommateur
    ├── CMakeLists.txt
    └── main.cpp
```

**Compilation, installation et test :**
```bash
cd 03_installation_cmake

# Build et installation dans un préfixe local
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15 -DCMAKE_INSTALL_PREFIX=/tmp/mylib_install  
cmake --build build  
cmake --install build  

# Vérifier la structure installée
find /tmp/mylib_install -type f | sort

# Tester find_package() depuis le projet consommateur
cmake -B test_consumer/build -G Ninja -DCMAKE_CXX_COMPILER=g++-15 \
    -DCMAKE_PREFIX_PATH=/tmp/mylib_install -S test_consumer
cmake --build test_consumer/build
./test_consumer/build/app
```

**Sortie attendue (consommateur) :**
```
Hello, find_package!
```

**Structure installée attendue :**
```
/tmp/mylib_install/
├── include/mylib/greet.h
├── lib/
│   ├── libmylib.a
│   ├── cmake/mylib/
│   │   ├── mylibConfig.cmake
│   │   ├── mylibConfigVersion.cmake
│   │   ├── mylibTargets.cmake
│   │   └── mylibTargets-noconfig.cmake
│   └── pkgconfig/mylib.pc
```

---

### 04\_cmake\_presets

| | |
|---|---|
| **Section** | 27.6 |
| **Fichier .md** | `06-cmake-presets.md` |
| **Description** | Projet avec `CMakePresets.json` complet : preset `base` caché (héritage), presets `debug` et `release`, build presets, test presets, et workflow preset pour le CI. |

**Compilation et exécution :**
```bash
cd 04_cmake_presets

# Lister les presets disponibles
cmake --list-presets

# Build Debug avec tests
cmake --preset debug  
cmake --build --preset debug  
ctest --preset debug  

# Build Release sans tests
cmake --preset release  
cmake --build --preset release  
./build-release/demo

# Pipeline CI complet (configure + build + test)
cmake --workflow --preset ci
```

**Sortie attendue (debug) :**
```
Configuration: Debug  
CMake Presets fonctionnent !  
```

**Sortie attendue (release) :**
```
Configuration: Release  
CMake Presets fonctionnent !  
```

**Sortie attendue (CTest) :**
```
1/1 Test #1: demo_test ........................   Passed
100% tests passed, 0 tests failed out of 1
```

**Comportement attendu :**
- `cmake --list-presets` affiche `debug` et `release` (pas `base` qui est `hidden`)
- Le preset `release` désactive les tests (`DEMO_BUILD_TESTS=OFF`)
- Le workflow `ci` enchaîne configure → build → test en une commande

---

## Nettoyage

```bash
# Depuis le répertoire exemples/
rm -rf */build* /tmp/mylib_install
```
