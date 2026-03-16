🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 26.4 Configuration et génération de fichiers

> **Objectif** : Maîtriser les mécanismes de CMake pour générer des fichiers à partir de templates — headers de version, fichiers de configuration, métadonnées de build — et comprendre comment ces fichiers s'intègrent dans le workflow de compilation.

---

## Pourquoi générer des fichiers ?

Certaines informations ne sont connues qu'au moment de la **configuration CMake**, pas au moment de l'écriture du code source. La version du projet, le type de build (Debug/Release), le compilateur utilisé, la présence d'une dépendance optionnelle, le hash du commit Git courant — toutes ces données sont dynamiques et ne peuvent pas être codées en dur dans les fichiers sources sans créer un cauchemar de maintenance.

CMake résout ce problème avec la commande `configure_file()` : elle lit un fichier template, substitue les variables CMake par leurs valeurs, et produit un fichier de sortie dans le répertoire de build. Ce fichier généré est ensuite inclus normalement par le code C++.

Le flux est le suivant :

```
config.h.in (template dans l'arbre source)
      │
      │  configure_file()  ← substitution des variables CMake
      ▼
config.h (fichier généré dans le répertoire de build)
      │
      │  #include <my_project/config.h>
      ▼
code C++ (utilise les constantes définies)
```

---

## `configure_file()` : syntaxe et fonctionnement

```cmake
configure_file(<input> <output> [options...])
```

La commande lit le fichier `<input>`, remplace les variables et les placeholders, et écrit le résultat dans `<output>`. Le fichier d'entrée est généralement dans l'arbre source (convention : suffixe `.in`). Le fichier de sortie est dans le répertoire de build.

```cmake
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/my_project/config.h
)
```

### Mécanismes de substitution

`configure_file()` reconnaît deux formes de placeholders dans le fichier template :

**Les variables CMake `${VAR}` et `@VAR@`** sont remplacées par la valeur de la variable CMake correspondante au moment de l'appel.

```c
// config.h.in (template)
#define PROJECT_VERSION "${PROJECT_VERSION}"
#define PROJECT_VERSION_MAJOR @PROJECT_VERSION_MAJOR@
```

```c
// config.h (généré) — si PROJECT_VERSION est "1.2.0"
#define PROJECT_VERSION "1.2.0"
#define PROJECT_VERSION_MAJOR 1
```

La forme `@VAR@` est recommandée car elle est sans ambiguïté. La forme `${VAR}` peut provoquer des substitutions accidentelles dans du code qui utilise légitimement `${}` (scripts shell embarqués, chaînes de formatage). L'option `@ONLY` restreint la substitution à la forme `@VAR@` exclusivement :

```cmake
configure_file(config.h.in config.h @ONLY)
```

C'est une bonne pratique systématique — utilisez toujours `@ONLY` sauf besoin explicite de la substitution `${}`.

**Les directives `#cmakedefine`** produisent des `#define` conditionnels :

```c
// config.h.in
#cmakedefine HAS_ZLIB
#cmakedefine01 USE_SSL
```

```c
// Si HAS_ZLIB est défini et truthy en CMake, USE_SSL est ON :
#define HAS_ZLIB
#define USE_SSL 1

// Si HAS_ZLIB n'est pas défini ou est FALSE, USE_SSL est OFF :
/* #undef HAS_ZLIB */
#define USE_SSL 0
```

`#cmakedefine VAR` produit `#define VAR` si la variable CMake VAR est définie et truthy (non vide, non `FALSE`, non `0`, non `OFF`, non `NOTFOUND`), ou `/* #undef VAR */` sinon. `#cmakedefine01 VAR` produit `#define VAR 1` ou `#define VAR 0`, ce qui est souvent plus pratique pour les tests conditionnels dans le code C++.

---

## Cas d'usage n°1 : header de version

Le cas le plus courant est la génération d'un header exposant la version du projet. Combiné avec `project(VERSION ...)`, cela automatise entièrement le versioning.

### Le template

```c
// version.h.in
#pragma once

// Version du projet — générée automatiquement par CMake
// Ne pas modifier ce fichier manuellement.

#define @PROJECT_NAME@_VERSION       "@PROJECT_VERSION@"
#define @PROJECT_NAME@_VERSION_MAJOR  @PROJECT_VERSION_MAJOR@
#define @PROJECT_NAME@_VERSION_MINOR  @PROJECT_VERSION_MINOR@
#define @PROJECT_NAME@_VERSION_PATCH  @PROJECT_VERSION_PATCH@
```

### La configuration CMake

```cmake
# CMakeLists.txt racine
project(my_project VERSION 1.2.0 LANGUAGES CXX)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/my_project/version.h
    @ONLY
)
```

### Le fichier généré

```c
// build/generated/my_project/version.h
#pragma once

#define my_project_VERSION       "1.2.0"
#define my_project_VERSION_MAJOR  1
#define my_project_VERSION_MINOR  2
#define my_project_VERSION_PATCH  0
```

### Intégration dans le build

Le répertoire contenant le header généré doit être ajouté aux include directories :

```cmake
target_include_directories(my_project_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:include>
)
```

Le code C++ peut alors inclure le header de manière transparente :

```cpp
#include <my_project/version.h>
#include <my_project/core.h>
#include <print>

void print_version() {
    std::println("my_project v{}", my_project_VERSION);
}
```

La version est définie en un seul endroit — l'appel `project(VERSION 1.2.0)` — et propagée automatiquement au code C++, aux métadonnées de packaging, et à la documentation.

---

## Cas d'usage n°2 : configuration de build

Un header de configuration expose les caractéristiques du build courant — dépendances optionnelles détectées, fonctionnalités activées, paramètres de la plateforme.

### Le template

```c
// config.h.in
#pragma once

// Configuration de build — générée par CMake.

// Dépendances optionnelles
#cmakedefine01 MY_PROJECT_HAS_ZLIB
#cmakedefine01 MY_PROJECT_HAS_SSL

// Plateforme
#define MY_PROJECT_SYSTEM_NAME   "@CMAKE_SYSTEM_NAME@"
#define MY_PROJECT_COMPILER_ID   "@CMAKE_CXX_COMPILER_ID@"
#define MY_PROJECT_COMPILER_VER  "@CMAKE_CXX_COMPILER_VERSION@"

// Type de build
#define MY_PROJECT_BUILD_TYPE    "@CMAKE_BUILD_TYPE@"
```

### La configuration CMake

```cmake
# Détection des dépendances optionnelles
find_package(ZLIB QUIET)  
find_package(OpenSSL QUIET)  

# Positionner les variables pour configure_file
set(MY_PROJECT_HAS_ZLIB ${ZLIB_FOUND})  
set(MY_PROJECT_HAS_SSL ${OPENSSL_FOUND})  

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/my_project/config.h
    @ONLY
)
```

### Utilisation dans le code

```cpp
#include <my_project/config.h>

#if MY_PROJECT_HAS_ZLIB
#include <zlib.h>
#endif

std::vector<uint8_t> compress(std::span<const uint8_t> data) {
#if MY_PROJECT_HAS_ZLIB
    // Implémentation avec zlib
    // ...
#else
    // Fallback sans compression
    return {data.begin(), data.end()};
#endif
}
```

Le code s'adapte automatiquement aux capacités du système de build, sans intervention manuelle.

---

## Cas d'usage n°3 : informations Git au build

Embarquer le hash du commit Git dans le binaire est précieux pour le diagnostic en production — vous savez exactement quel code tourne.

### Récupérer le hash Git dans CMake

```cmake
# Récupérer le hash court du commit courant
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Vérifier si le working tree est propre
execute_process(
    COMMAND git diff-index --quiet HEAD --
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE GIT_DIRTY
)

# Fallback si git n'est pas disponible
if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
elseif(GIT_DIRTY)
    set(GIT_COMMIT_HASH "${GIT_COMMIT_HASH}-dirty")
endif()
```

### Le template

```c
// build_info.h.in
#pragma once

#define MY_PROJECT_GIT_HASH     "@GIT_COMMIT_HASH@"
#define MY_PROJECT_BUILD_DATE   "@BUILD_TIMESTAMP@"
```

### La configuration

```cmake
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/build_info.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/my_project/build_info.h
    @ONLY
)
```

### Utilisation

```cpp
#include <my_project/build_info.h>
#include <my_project/version.h>
#include <print>

void print_build_info() {
    std::println("Version : {}", my_project_VERSION);
    std::println("Commit  : {}", MY_PROJECT_GIT_HASH);
    std::println("Build   : {}", MY_PROJECT_BUILD_DATE);
}
```

### Limitation : reconfiguration nécessaire

`configure_file()` est exécutée au moment de la **configuration** CMake, pas à chaque build. Le hash Git et le timestamp ne sont mis à jour que quand vous relancez `cmake -B build`. Un simple `cmake --build build` après un nouveau commit ne régénère pas le header.

Pour des builds de production où le hash Git doit être toujours à jour, une solution plus robuste utilise `add_custom_command` :

```cmake
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/my_project/build_info.h
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/generated/my_project/build_info.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBuildInfo.cmake
    DEPENDS ${CMAKE_SOURCE_DIR}/.git/HEAD ${CMAKE_SOURCE_DIR}/.git/index
    COMMENT "Generating build_info.h"
)
```

Le script `cmake/GenerateBuildInfo.cmake` effectue les mêmes opérations (`execute_process` + `configure_file`) mais est relancé à chaque build si le HEAD Git a changé, grâce à la dépendance sur `.git/HEAD` et `.git/index`.

---

## Cas d'usage n°4 : Doxyfile pour la documentation

La même technique s'applique à la configuration de Doxygen :

```cmake
# docs/CMakeLists.txt
find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME ${PROJECT_NAME})
    set(DOXYGEN_PROJECT_VERSION ${PROJECT_VERSION})
    set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/include)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        @ONLY
    )

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Génération de la documentation Doxygen"
        VERBATIM
    )
endif()
```

La documentation se génère alors via :

```bash
cmake --build build --target docs
```

---

## Génération de fichiers de configuration d'export

Au-delà de `configure_file()`, CMake fournit des commandes spécialisées pour générer les fichiers nécessaires à l'exportation d'un projet — c'est-à-dire les fichiers qu'un consommateur utilisera via `find_package()` après installation.

### `write_basic_package_version_file`

Génère le fichier `<Package>ConfigVersion.cmake` qui permet à `find_package()` de vérifier la compatibilité de version :

```cmake
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/my_projectConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
```

Le paramètre `COMPATIBILITY` contrôle la politique de compatibilité :

| Politique | Signification |
|-----------|--------------|
| `ExactVersion` | Seule la version exacte est acceptée |
| `SameMajorVersion` | Toute version avec le même major est compatible (ex: 1.x.x) |
| `SameMinorVersion` | Même major et même minor (ex: 1.2.x) |
| `AnyNewerVersion` | Toute version ≥ à la demandée |

`SameMajorVersion` est le choix le plus courant, en accord avec le Semantic Versioning : les versions 1.2.0, 1.3.0 et 1.99.0 sont mutuellement compatibles, mais pas 2.0.0.

### `configure_package_config_file`

Génère le fichier `<Package>Config.cmake` principal à partir d'un template :

```cmake
# cmake/my_projectConfig.cmake.in
@PACKAGE_INIT@

include("${CMAKE_CURRENT_LIST_DIR}/my_projectTargets.cmake")

check_required_components(my_project)
```

```cmake
# CMakeLists.txt
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/my_projectConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/my_projectConfig.cmake
    INSTALL_DESTINATION lib/cmake/my_project
)
```

Le placeholder `@PACKAGE_INIT@` est remplacé par du code CMake de boilerplate qui initialise les chemins relatifs correctement. C'est une forme spécialisée de `configure_file()`, adaptée aux fichiers de configuration de paquets.

### Export des cibles

La commande `install(EXPORT ...)` complète le tableau en générant `my_projectTargets.cmake` — le fichier qui recrée les cibles importées lors d'un `find_package()` :

```cmake
# Installer les binaires
install(TARGETS my_project_core my_project_utils
    EXPORT my_projectTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    INCLUDES DESTINATION include
)

# Installer les headers
install(
    DIRECTORY include/my_project
    DESTINATION include
)

# Installer les headers générés
install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated/my_project
    DESTINATION include
)

# Générer et installer le fichier d'export des cibles
install(EXPORT my_projectTargets
    FILE my_projectTargets.cmake
    NAMESPACE my_project::
    DESTINATION lib/cmake/my_project
)

# Installer les fichiers de configuration et de version
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/my_projectConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/my_projectConfigVersion.cmake
    DESTINATION lib/cmake/my_project
)
```

Après `cmake --install build`, la structure installée ressemble à :

```
/usr/local/
├── include/
│   └── my_project/
│       ├── core.h
│       ├── network.h
│       ├── config.h        ← Header généré, installé avec les autres
│       └── version.h       ← Header généré
└── lib/
    ├── libmy_project_core.a
    ├── libmy_project_utils.a
    └── cmake/my_project/
        ├── my_projectConfig.cmake
        ├── my_projectConfigVersion.cmake
        └── my_projectTargets.cmake
```

Un consommateur peut alors utiliser votre bibliothèque via :

```cmake
find_package(my_project 1.2 REQUIRED)  
target_link_libraries(their_app PRIVATE my_project::core)  
```

---

## Bonnes pratiques

**Utilisez toujours `@ONLY`.** La substitution `@VAR@` est explicite et évite les surprises avec `${}` dans les templates.

**Placez les fichiers générés dans un sous-répertoire namespacé.** Générez dans `${CMAKE_CURRENT_BINARY_DIR}/generated/my_project/`, pas directement dans `${CMAKE_CURRENT_BINARY_DIR}/`. Cela évite les collisions de noms et permet l'inclusion via `<my_project/config.h>`, cohérent avec les headers statiques.

**Ajoutez un commentaire d'avertissement dans les templates.** Les fichiers `.in` produisent des fichiers qui ne doivent pas être modifiés à la main :

```c
// Ce fichier est généré automatiquement par CMake.
// Toute modification sera écrasée. Éditez config.h.in à la place.
```

**Versionnez les templates, pas les fichiers générés.** Les fichiers `.in` sont dans l'arbre source et versionnés dans Git. Les fichiers générés sont dans `build/` et listés dans `.gitignore`.

**Préfixez vos macros.** Utilisez `MY_PROJECT_VERSION`, pas `VERSION`. Utilisez `MY_PROJECT_HAS_ZLIB`, pas `HAS_ZLIB`. Les noms non préfixés entrent en conflit avec les macros des autres bibliothèques et du système.

**N'abusez pas de `configure_file()` pour les informations Git.** Pour les builds où le hash Git doit être toujours à jour, un `add_custom_command` avec dépendance sur `.git/HEAD` est plus fiable qu'un `configure_file()` exécuté uniquement à la configuration.

---

> **À suivre** : La section 26.5 couvre la génération pour Ninja — pourquoi Ninja est le backend recommandé en 2026, comment l'utiliser avec CMake, et les gains de performance concrets par rapport à Make.

⏭️ [Génération pour Ninja : cmake -G Ninja (recommandé)](/26-cmake/05-generation-ninja.md)
