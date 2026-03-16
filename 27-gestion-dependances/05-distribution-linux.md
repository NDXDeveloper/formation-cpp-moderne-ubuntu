🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 27.5 Installation et distribution de librairies sur Linux

> **Objectif** : Comprendre comment installer une bibliothèque C++ sur un système Linux pour qu'elle soit consommable par d'autres projets — les emplacements conventionnels, le mécanisme `pkg-config`, l'export CMake via `find_package()`, et les bonnes pratiques de distribution.

---

## Le problème de la distribution

Vous avez écrit une bibliothèque C++, elle compile, elle est testée. Maintenant vous voulez qu'un autre projet — le vôtre ou celui d'un collègue — puisse l'utiliser via un simple `find_package()`. Pour que cela fonctionne, trois conditions doivent être remplies :

1. Les **headers** doivent être installés dans un répertoire que le compilateur peut trouver.
2. Les **bibliothèques compilées** (`.a` ou `.so`) doivent être installées dans un répertoire que le linker peut trouver.
3. Des **métadonnées** doivent indiquer au build system quels headers, quelles bibliothèques, et quels flags utiliser.

Sur Linux, des conventions bien établies régissent chacun de ces aspects.

---

## Les emplacements conventionnels

### La hiérarchie standard (FHS)

Le *Filesystem Hierarchy Standard* définit deux préfixes principaux pour l'installation de bibliothèques :

| Préfixe | Usage | Géré par |
|---------|-------|----------|
| `/usr` | Bibliothèques de la distribution (paquets `apt`) | Le gestionnaire de paquets système |
| `/usr/local` | Bibliothèques installées manuellement | L'administrateur / le développeur |

Sous chaque préfixe, la structure est identique :

```
/usr/local/
├── include/              # Headers
│   └── my_project/
│       ├── core.h
│       └── network.h
├── lib/                  # Bibliothèques
│   ├── libmy_project_core.a
│   ├── libmy_project_core.so → libmy_project_core.so.1
│   ├── libmy_project_core.so.1 → libmy_project_core.so.1.2.0
│   ├── libmy_project_core.so.1.2.0
│   └── cmake/
│       └── my_project/   # Fichiers CMake pour find_package()
│           ├── my_projectConfig.cmake
│           ├── my_projectConfigVersion.cmake
│           └── my_projectTargets.cmake
├── lib/pkgconfig/        # Fichiers pkg-config
│   └── my_project.pc
└── share/
    └── doc/my_project/   # Documentation (optionnel)
```

La règle d'or : **ne jamais installer manuellement dans `/usr`**. Ce préfixe est réservé au gestionnaire de paquets de la distribution (`apt`, `dnf`). Installer des fichiers dans `/usr` crée des conflits avec les paquets système et peut casser les mises à jour. Utilisez `/usr/local` pour les installations manuelles.

### Architectures multi-lib

Sur les distributions 64 bits modernes, les bibliothèques peuvent se trouver dans des sous-répertoires spécifiques à l'architecture :

```
/usr/lib/x86_64-linux-gnu/        # Debian/Ubuntu — libs 64 bits
/usr/lib64/                        # Fedora/RHEL — libs 64 bits
/usr/local/lib/                    # Installation manuelle (pas de sous-répertoire arch)
```

CMake gère ces chemins automatiquement via `GNUInstallDirs` (voir plus loin).

---

## Installation avec CMake

CMake fournit les commandes `install()` pour déclarer les règles d'installation. Ces règles sont exécutées par `cmake --install build`.

### Déclaration des règles d'installation

Reprenons notre projet exemple et ajoutons les règles d'installation complètes :

```cmake
# CMakeLists.txt racine
include(GNUInstallDirs)  
include(CMakePackageConfigHelpers)  

# ── Installer les cibles ──────────────────────────────────────
install(TARGETS my_project_core my_project_utils
    EXPORT my_projectTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}          # .a
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}          # .so
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}          # exécutables (Windows DLLs)
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ── Installer les headers ─────────────────────────────────────
install(DIRECTORY include/my_project
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ── Installer les headers générés ─────────────────────────────
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/my_project
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ── Exporter les cibles pour find_package() ───────────────────
install(EXPORT my_projectTargets
    FILE my_projectTargets.cmake
    NAMESPACE my_project::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/my_project
)

# ── Générer et installer le fichier de configuration ──────────
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/my_projectConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/my_projectConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/my_project
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/my_projectConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/my_projectConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/my_projectConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/my_project
)
```

### Le module `GNUInstallDirs`

Le `include(GNUInstallDirs)` définit des variables conformes aux conventions Linux :

| Variable | Valeur par défaut | Chemin réel (préfixe `/usr/local`) |
|----------|------------------|------------------------------------|
| `CMAKE_INSTALL_INCLUDEDIR` | `include` | `/usr/local/include` |
| `CMAKE_INSTALL_LIBDIR` | `lib` (ou `lib64` sur certaines distros) | `/usr/local/lib` |
| `CMAKE_INSTALL_BINDIR` | `bin` | `/usr/local/bin` |
| `CMAKE_INSTALL_DATADIR` | `share` | `/usr/local/share` |

Utilisez toujours ces variables plutôt que des chemins en dur — elles s'adaptent automatiquement à la distribution et à l'architecture.

### Exécuter l'installation

```bash
# Compiler d'abord
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release  
cmake --build build  

# Installer dans /usr/local (défaut)
sudo cmake --install build

# Installer dans un préfixe personnalisé (sans sudo)
cmake --install build --prefix /opt/my_project
```

L'option `--prefix` surcharge `CMAKE_INSTALL_PREFIX`. C'est utile pour les installations locales ou pour préparer un paquet.

### Vérification post-installation

```bash
# Vérifier les fichiers installés
ls /usr/local/include/my_project/
# core.h  network.h  config.h  version.h

ls /usr/local/lib/libmy_project*
# libmy_project_core.a  libmy_project_utils.a

ls /usr/local/lib/cmake/my_project/
# my_projectConfig.cmake  my_projectConfigVersion.cmake  my_projectTargets.cmake

# Tester que find_package fonctionne
cmake -B /tmp/test-find -G Ninja -DCMAKE_PREFIX_PATH=/usr/local
# ... dans un CMakeLists.txt de test contenant find_package(my_project REQUIRED)
```

---

## Le fichier Config CMake

Le fichier `my_projectConfig.cmake.in` est un template qui sera traité par `configure_package_config_file()`. Son contenu est généralement minimal :

```cmake
# cmake/my_projectConfig.cmake.in
@PACKAGE_INIT@

include("${CMAKE_CURRENT_LIST_DIR}/my_projectTargets.cmake")

check_required_components(my_project)
```

`@PACKAGE_INIT@` est remplacé par du code boilerplate CMake qui initialise les chemins relatifs. `my_projectTargets.cmake` est le fichier généré par `install(EXPORT ...)` qui recrée les cibles importées (`my_project::core`, `my_project::utils`) avec toutes leurs propriétés.

### Avec des dépendances transitives

Si votre bibliothèque dépend d'OpenSSL en `PUBLIC`, le consommateur doit aussi trouver OpenSSL. Le fichier Config doit propager cette dépendance :

```cmake
# cmake/my_projectConfig.cmake.in
@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

# Dépendances transitives — le consommateur en a besoin
find_dependency(Threads REQUIRED)  
find_dependency(OpenSSL REQUIRED)  

include("${CMAKE_CURRENT_LIST_DIR}/my_projectTargets.cmake")

check_required_components(my_project)
```

`find_dependency()` est un wrapper de `find_package()` adapté aux fichiers Config : il propage correctement les erreurs et supporte le mode `REQUIRED` hérité du `find_package()` du consommateur.

La règle est simple : **chaque dépendance `PUBLIC` de votre bibliothèque doit avoir un `find_dependency()` correspondant** dans le fichier Config. Les dépendances `PRIVATE` n'ont pas besoin d'y figurer — le consommateur n'en a pas connaissance.

---

## `pkg-config` : l'alternative classique

`pkg-config` est un outil plus ancien que CMake pour décrire les métadonnées d'une bibliothèque installée. Il reste largement utilisé, notamment par les projets qui ne sont pas basés sur CMake (Meson, autotools) et par le système de build de nombreuses distributions Linux.

### Le fichier `.pc`

```ini
# my_project.pc (installé dans /usr/local/lib/pkgconfig/)
prefix=/usr/local  
exec_prefix=${prefix}  
libdir=${exec_prefix}/lib  
includedir=${prefix}/include  

Name: my_project  
Description: Exemple de bibliothèque C++ moderne  
Version: 1.2.0  
Requires: openssl >= 3.0  
Libs: -L${libdir} -lmy_project_core -lmy_project_utils  
Cflags: -I${includedir}  
```

### Utilisation par les consommateurs

```bash
# Vérifier qu'un paquet est disponible
pkg-config --exists my_project && echo "Trouvé"

# Obtenir les flags de compilation
pkg-config --cflags my_project
# -I/usr/local/include

# Obtenir les flags de linkage
pkg-config --libs my_project
# -L/usr/local/lib -lmy_project_core -lmy_project_utils

# Version
pkg-config --modversion my_project
# 1.2.0
```

### Générer le fichier `.pc` avec CMake

CMake ne génère pas de fichier `.pc` nativement (contrairement aux fichiers `*Config.cmake`). Vous pouvez en générer un via `configure_file()` :

```ini
# cmake/my_project.pc.in
prefix=@CMAKE_INSTALL_PREFIX@  
exec_prefix=${prefix}  
libdir=${exec_prefix}/@CMAKE_INSTALL_LIBDIR@  
includedir=${prefix}/@CMAKE_INSTALL_INCLUDEDIR@  

Name: @PROJECT_NAME@  
Description: @PROJECT_DESCRIPTION@  
Version: @PROJECT_VERSION@  
Libs: -L${libdir} -lmy_project_core  
Cflags: -I${includedir}  
```

```cmake
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/my_project.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/my_project.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/my_project.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
```

### `pkg-config` vs fichiers Config CMake

En 2026, pour un projet CMake, les fichiers Config (`*Config.cmake`) sont le mécanisme principal de distribution. Ils portent plus d'informations que les fichiers `.pc` (cibles avec propriétés, visibilité, configurations multiples) et s'intègrent nativement avec `find_package()`.

Fournir un fichier `.pc` en complément est une bonne pratique pour l'interopérabilité avec les build systems non-CMake. C'est un effort minimal (un template + un `install()`) qui élargit la base de consommateurs potentiels de votre bibliothèque.

---

## Le cache du linker dynamique

Quand vous installez des bibliothèques dynamiques (`.so`) dans un répertoire non standard, le dynamic linker ne les trouve pas automatiquement au runtime. Deux mécanismes existent pour résoudre ce problème.

### `ldconfig`

```bash
# Après installation dans /usr/local/lib
sudo ldconfig
```

`ldconfig` met à jour le cache du dynamic linker (`/etc/ld.so.cache`). Les chemins `/usr/local/lib` sont généralement déjà dans la configuration par défaut (`/etc/ld.so.conf.d/`). Si vous installez dans un chemin non standard :

```bash
# Ajouter un chemin personnalisé
echo "/opt/my_project/lib" | sudo tee /etc/ld.so.conf.d/my_project.conf  
sudo ldconfig  
```

### `RPATH`

Le RPATH est un chemin de recherche **encodé dans l'exécutable** qui indique au dynamic linker où trouver les `.so` au runtime. CMake gère le RPATH automatiquement :

```cmake
# CMake configure le RPATH par défaut pour les builds
# Après installation, le RPATH est ajusté automatiquement
set(CMAKE_INSTALL_RPATH_USE_LINK_RPATH TRUE)

# Pour pointer vers un répertoire relatif à l'exécutable
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
```

`$ORIGIN` est une variable spéciale qui représente le répertoire contenant l'exécutable. `$ORIGIN/../lib` signifie « le répertoire `lib/` un niveau au-dessus de l'exécutable » — un pattern courant pour les installations relocalisables.

---

## Distribution via Conan ou vcpkg

Au-delà de l'installation système, votre bibliothèque peut être publiée comme un **paquet Conan** ou un **port vcpkg** pour être consommée facilement par d'autres projets.

### Publier un paquet Conan

Pour publier votre bibliothèque dans un registre Conan (ConanCenter ou un serveur privé), le `conanfile.py` doit passer du rôle de **consommateur** à celui de **producteur**. Il déclare comment construire et empaqueter votre bibliothèque :

```python
from conan import ConanFile  
from conan.tools.cmake import CMake, cmake_layout, CMakeToolchain, CMakeDeps  


class MyProjectConan(ConanFile):
    name = "my_project"
    version = "1.2.0"
    license = "MIT"
    url = "https://github.com/you/my_project"
    description = "Exemple de bibliothèque C++ moderne"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    exports_sources = "CMakeLists.txt", "src/*", "include/*", "cmake/*", "libs/*"

    def requirements(self):
        self.requires("openssl/3.2.1")
        self.requires("spdlog/1.15.3")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["my_project_core", "my_project_utils"]
        self.cpp_info.set_property("cmake_file_name", "my_project")
        self.cpp_info.set_property("cmake_target_name", "my_project::core")
```

La méthode `package_info()` est cruciale : elle décrit comment les consommateurs utiliseront votre bibliothèque. Conan utilise ces informations pour générer les fichiers `*Config.cmake` côté consommateur.

### Créer un port vcpkg

Un port vcpkg est un répertoire contenant un `vcpkg.json` (métadonnées) et un `portfile.cmake` (instructions de build). Pour les bibliothèques CMake, le portfile est généralement concis :

```cmake
# portfile.cmake
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO you/my_project
    REF v1.2.0
    SHA512 abc123...
)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")  
vcpkg_cmake_install()  
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/my_project)  

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
```

Les ports peuvent être hébergés dans un **registre vcpkg privé** (un dépôt Git) pour une distribution au sein d'une organisation.

---

## Bonnes pratiques de distribution

**Fournissez toujours des fichiers CMake Config.** C'est le mécanisme standard pour les projets CMake, et il porte le plus d'informations (cibles, propriétés, configurations).

**Fournissez un fichier `pkg-config` en complément.** L'effort est minime et augmente l'interopérabilité.

**Utilisez `GNUInstallDirs` pour les chemins.** Ne codez jamais `lib/`, `include/` ou `bin/` en dur — les variables `CMAKE_INSTALL_*` s'adaptent aux conventions de la distribution.

**Propagez les dépendances transitives dans le fichier Config.** Un `find_dependency()` manquant provoque des erreurs cryptiques chez le consommateur.

**Préfixez vos headers.** Installez dans `include/my_project/`, pas dans `include/`. Cela évite les collisions entre bibliothèques.

**Testez l'installation.** Après `cmake --install`, créez un petit projet consommateur qui fait `find_package(my_project REQUIRED)` et vérifie que la compilation et le linkage fonctionnent. Ce test peut être automatisé dans votre CI.

**Versionez l'ABI avec le soname.** Pour les bibliothèques dynamiques, configurez le versioning :

```cmake
set_target_properties(my_project_core PROPERTIES
    VERSION ${PROJECT_VERSION}          # 1.2.0 — nom de fichier
    SOVERSION ${PROJECT_VERSION_MAJOR}  # 1 — soname
)
```

---

## Récapitulatif

| Aspect | Mécanisme | Fichier / commande |
|--------|-----------|-------------------|
| Installer les binaires | `cmake --install build` | Règles `install()` dans CMakeLists.txt |
| Chemins conventionnels | `GNUInstallDirs` | `CMAKE_INSTALL_LIBDIR`, `CMAKE_INSTALL_INCLUDEDIR` |
| Consommation CMake | `find_package()` | `*Config.cmake` + `*Targets.cmake` + `*ConfigVersion.cmake` |
| Consommation non-CMake | `pkg-config` | `*.pc` dans `lib/pkgconfig/` |
| Dépendances transitives | `find_dependency()` | Dans le `*Config.cmake.in` |
| Dynamic linker cache | `ldconfig` | `/etc/ld.so.conf.d/` |
| Exécutables relocalisables | RPATH | `CMAKE_INSTALL_RPATH`, `$ORIGIN` |
| Distribution Conan | Recette producteur | `conanfile.py` avec `build()`, `package()`, `package_info()` |
| Distribution vcpkg | Port | `portfile.cmake` + `vcpkg.json` |

---

> **À suivre** : La section 27.6 couvre les CMake Presets — le mécanisme de standardisation des configurations de build qui unifie CMake, Conan et vcpkg sous une interface commune.

⏭️ [CMake Presets : Standardisation des configurations](/27-gestion-dependances/06-cmake-presets.md)
