🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 26.1 Structure d'un projet CMake moderne

> **Objectif** : Comprendre l'organisation conventionnelle d'un projet C++ piloté par CMake, du répertoire racine jusqu'aux sous-répertoires de bibliothèques internes et de tests. Poser les fondations sur lesquelles les sections suivantes (26.2 à 26.7) viendront construire.

---

## Le principe directeur : séparation des responsabilités

Un projet C++ bien structuré reflète dans son arborescence de fichiers la même rigueur qu'on attend du code lui-même. Chaque répertoire a un rôle précis, et un développeur qui découvre le projet doit pouvoir en comprendre l'organisation en quelques secondes. CMake n'impose techniquement aucune structure particulière — vous êtes libre d'organiser vos fichiers comme bon vous semble. Mais des conventions solides se sont établies au fil des années dans l'écosystème C++, et les respecter offre des avantages concrets : intégration fluide avec les IDE, compatibilité immédiate avec les gestionnaires de dépendances (Conan, vcpkg), et lisibilité pour tout contributeur expérimenté.

---

## Arborescence de référence

Voici la structure que nous utiliserons comme fil conducteur tout au long de ce chapitre. Elle correspond à un projet de taille moyenne — suffisamment réaliste pour illustrer tous les mécanismes de CMake, sans complexité superflue.

```
my_project/
├── CMakeLists.txt              # CMakeLists.txt racine
├── CMakePresets.json            # Presets de configuration (section 27.6)
├── LICENSE
├── README.md
│
├── cmake/                       # Modules CMake personnalisés
│   ├── CompilerWarnings.cmake
│   └── Sanitizers.cmake
│
├── include/                     # Headers publics (API exposée)
│   └── my_project/
│       ├── core.h
│       └── network.h
│
├── src/                         # Sources et headers privés
│   ├── CMakeLists.txt
│   ├── core.cpp
│   ├── network.cpp
│   └── internal/
│       └── detail.h             # Header privé (non exposé)
│
├── apps/                        # Exécutables (points d'entrée)
│   ├── CMakeLists.txt
│   └── main.cpp
│
├── libs/                        # Bibliothèques internes supplémentaires
│   └── utils/
│       ├── CMakeLists.txt
│       ├── include/
│       │   └── utils/
│       │       └── string_utils.h
│       └── src/
│           └── string_utils.cpp
│
├── tests/                       # Tests unitaires
│   ├── CMakeLists.txt
│   ├── test_core.cpp
│   └── test_network.cpp
│
├── benchmarks/                  # Benchmarks (Google Benchmark)
│   ├── CMakeLists.txt
│   └── bench_core.cpp
│
├── docs/                        # Documentation (Doxygen, etc.)
│   └── Doxyfile.in
│
├── third_party/                 # Dépendances embarquées (si nécessaire)
│
└── build/                       # Répertoire de build (JAMAIS versionné)
```

Cette arborescence peut sembler imposante au premier abord, mais chaque élément répond à un besoin précis. Détaillons-les un par un.

---

## Le CMakeLists.txt racine : le point d'entrée

Le fichier `CMakeLists.txt` à la racine du projet est le premier fichier lu par CMake. Son rôle est de **déclarer le projet**, de **fixer les contraintes globales**, et de **déléguer** aux sous-répertoires. Il ne doit pas contenir la logique de compilation détaillée — c'est un chef d'orchestre, pas un musicien.

```cmake
cmake_minimum_required(VERSION 3.25)

project(my_project
    VERSION 1.2.0
    DESCRIPTION "Un projet exemple pour illustrer CMake moderne"
    LANGUAGES CXX
)

# ── Standards et options globales ──────────────────────────────
set(CMAKE_CXX_STANDARD 23)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
set(CMAKE_CXX_EXTENSIONS OFF)  

# ── Chemin vers les modules CMake personnalisés ────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Options du projet ──────────────────────────────────────────
option(MY_PROJECT_BUILD_TESTS "Compiler les tests unitaires" ON)  
option(MY_PROJECT_BUILD_BENCHMARKS "Compiler les benchmarks" OFF)  

# ── Sous-répertoires ──────────────────────────────────────────
add_subdirectory(libs/utils)  
add_subdirectory(src)  
add_subdirectory(apps)  

if(MY_PROJECT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(MY_PROJECT_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
```

Analysons les choix importants de ce fichier.

### `project()` : plus qu'un nom

La commande `project()` ne se contente pas de nommer le projet. Elle définit plusieurs variables automatiquement exploitables dans le reste de la configuration :

| Variable | Valeur dans notre exemple |
|----------|--------------------------|
| `PROJECT_NAME` | `my_project` |
| `PROJECT_VERSION` | `1.2.0` |
| `PROJECT_VERSION_MAJOR` | `1` |
| `PROJECT_VERSION_MINOR` | `2` |
| `PROJECT_VERSION_PATCH` | `0` |
| `PROJECT_DESCRIPTION` | `Un projet exemple...` |

Ces variables sont précieuses pour générer automatiquement des fichiers de version, des headers de configuration ou des métadonnées de packaging (nous y reviendrons en section 26.4).

Le paramètre `LANGUAGES CXX` indique que le projet utilise uniquement C++. Cela évite à CMake de chercher un compilateur C inutilement. Si votre projet mélange C et C++, utilisez `LANGUAGES C CXX`.

### Standards C++ : trois lignes essentielles

```cmake
set(CMAKE_CXX_STANDARD 23)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
set(CMAKE_CXX_EXTENSIONS OFF)  
```

Ces trois lignes forment un bloc indissociable. `CMAKE_CXX_STANDARD` fixe le standard désiré (C++23 dans notre cas). `CMAKE_CXX_STANDARD_REQUIRED ON` garantit que CMake échouera si le compilateur ne supporte pas ce standard — sans cette option, CMake se rabattrait silencieusement sur un standard plus ancien, ce qui peut causer des erreurs de compilation obscures bien plus tard. Enfin, `CMAKE_CXX_EXTENSIONS OFF` désactive les extensions propriétaires du compilateur (GNU extensions sous GCC, par exemple) pour obtenir le flag `-std=c++23` plutôt que `-std=gnu++23`, ce qui améliore la portabilité du code.

> 💡 **Scope des variables** : les `set()` effectués dans le `CMakeLists.txt` racine s'appliquent à l'ensemble du projet, y compris les sous-répertoires ajoutés via `add_subdirectory()`. C'est pourquoi les standards C++ se définissent ici et non dans chaque sous-répertoire. Toutefois, en *modern CMake*, on préfère parfois attacher le standard directement aux cibles via `target_compile_features()` — nous verrons cette approche en section 26.2.

### Options conditionnelles

```cmake
option(MY_PROJECT_BUILD_TESTS "Compiler les tests unitaires" ON)  
option(MY_PROJECT_BUILD_BENCHMARKS "Compiler les benchmarks" OFF)  
```

Les commandes `option()` définissent des variables booléennes que l'utilisateur peut activer ou désactiver au moment de la configuration :

```bash
# Désactiver les tests, activer les benchmarks
cmake -B build -G Ninja -DMY_PROJECT_BUILD_TESTS=OFF -DMY_PROJECT_BUILD_BENCHMARKS=ON
```

Notez le préfixe `MY_PROJECT_` devant chaque option. C'est une bonne pratique pour éviter les collisions de noms : si votre projet est inclus comme sous-projet d'un autre via `add_subdirectory()` ou `FetchContent`, des options nommées simplement `BUILD_TESTS` entreraient en conflit avec celles des autres projets. Le préfixage avec le nom du projet élimine ce risque.

---

## Le répertoire `include/` : l'interface publique

```
include/
└── my_project/
    ├── core.h
    └── network.h
```

Ce répertoire contient les **headers publics** de votre projet — ceux que les consommateurs de votre bibliothèque incluront. Le sous-répertoire `my_project/` crée un **namespace de fichiers** qui évite les collisions d'includes entre différents projets. Concrètement, les utilisateurs de votre bibliothèque écriront :

```cpp
#include <my_project/core.h>
#include <my_project/network.h>
```

Plutôt que le dangereux :

```cpp
#include <core.h>   // ⚠ Quel core.h ? Le vôtre ? Celui de Boost ?
```

Cette convention est universellement adoptée dans l'écosystème C++ : Boost inclut ses headers via `<boost/...>`, Abseil via `<absl/...>`, etc.

La séparation physique entre `include/` (public) et `src/` (privé) a une conséquence directe en CMake : lorsque vous déclarez votre bibliothèque, vous exposez `include/` comme `PUBLIC` et `src/` comme `PRIVATE`. Seul le contenu de `include/` sera visible par les projets qui dépendent de votre bibliothèque.

---

## Le répertoire `src/` : l'implémentation

```
src/
├── CMakeLists.txt
├── core.cpp
├── network.cpp
└── internal/
    └── detail.h
```

C'est ici que réside l'implémentation de votre bibliothèque principale. Les fichiers `.cpp` fournissent les définitions correspondant aux déclarations de `include/`. Le sous-répertoire `internal/` contient des headers **privés** — des détails d'implémentation qui ne font pas partie de l'API publique et ne doivent pas être accessibles aux consommateurs de la bibliothèque.

Le `CMakeLists.txt` de ce répertoire déclare la cible bibliothèque :

```cmake
add_library(my_project_core
    core.cpp
    network.cpp
)

target_include_directories(my_project_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
)
```

Les **generator expressions** `$<BUILD_INTERFACE:...>` et `$<INSTALL_INTERFACE:...>` méritent une explication. Elles permettent de spécifier des chemins différents selon le contexte :

- lors du **build** (quand le projet est compilé depuis ses sources), le chemin des headers publics est `${PROJECT_SOURCE_DIR}/include` — un chemin absolu dans votre arborescence ;
- lors de l'**installation** (quand la bibliothèque est installée sur le système et consommée par un autre projet), le chemin est simplement `include` — un chemin relatif au préfixe d'installation.

Cette distinction est fondamentale pour que votre bibliothèque soit correctement utilisable à la fois en tant que sous-projet (via `add_subdirectory`) et en tant que paquet installé (via `find_package`). Nous détaillerons ces mécanismes en section 26.2 et 26.3.

---

## Le répertoire `apps/` : les exécutables

```
apps/
├── CMakeLists.txt
└── main.cpp
```

Ce répertoire contient les **points d'entrée** de votre application — les fichiers avec une fonction `main()`. La séparation entre `src/` (bibliothèque) et `apps/` (exécutables) est un choix architectural délibéré qui apporte deux avantages majeurs.

Premièrement, la logique métier vit dans la bibliothèque (`src/`) et peut être réutilisée, testée et liée indépendamment. L'exécutable dans `apps/` n'est qu'un **client** de cette bibliothèque. Deuxièmement, les tests unitaires (dans `tests/`) peuvent lier directement contre la bibliothèque sans avoir à compiler les sources une deuxième fois. Sans cette séparation, vous vous retrouveriez à compiler votre code deux fois — une fois pour l'application, une fois pour les tests — ou à recourir à des hacks fragiles.

Le `CMakeLists.txt` correspondant est simple :

```cmake
add_executable(my_app main.cpp)  
target_link_libraries(my_app PRIVATE my_project_core)  
```

L'exécutable `my_app` dépend de la bibliothèque `my_project_core`. CMake résout automatiquement les include directories, les flags de compilation et les bibliothèques transitives — c'est toute la puissance de l'approche *target-centric*.

---

## Le répertoire `libs/` : bibliothèques internes

```
libs/
└── utils/
    ├── CMakeLists.txt
    ├── include/
    │   └── utils/
    │       └── string_utils.h
    └── src/
        └── string_utils.cpp
```

Pour un projet de taille conséquente, toute la logique ne rentre pas dans une seule bibliothèque. Le répertoire `libs/` héberge des **bibliothèques internes** au projet — des modules fonctionnels autonomes avec leurs propres headers publics et leur propre `CMakeLists.txt`.

Chaque bibliothèque dans `libs/` suit la même micro-structure `include/` + `src/` que le projet principal. Cette régularité est importante : un développeur qui comprend la structure d'une bibliothèque comprend instantanément toutes les autres.

```cmake
# libs/utils/CMakeLists.txt
add_library(my_project_utils
    src/string_utils.cpp
)

target_include_directories(my_project_utils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
```

La bibliothèque principale peut ensuite dépendre de cette bibliothèque utilitaire :

```cmake
# Dans src/CMakeLists.txt
target_link_libraries(my_project_core PRIVATE my_project_utils)
```

> 💡 **Convention de nommage** : préfixez vos cibles internes avec le nom du projet (`my_project_core`, `my_project_utils`) pour éviter les conflits. Un nom de cible comme `utils` est garanti de provoquer une collision tôt ou tard dans un projet qui intègre des dépendances tierces.

---

## Le répertoire `tests/` : les tests unitaires

```
tests/
├── CMakeLists.txt
├── test_core.cpp
└── test_network.cpp
```

Les tests sont séparés du code de production et conditionnés à l'option `MY_PROJECT_BUILD_TESTS`. Le `CMakeLists.txt` des tests est typiquement structuré ainsi :

```cmake
find_package(GTest REQUIRED)

add_executable(test_core test_core.cpp)  
target_link_libraries(test_core  
    PRIVATE
        my_project_core
        GTest::gtest_main
)
add_test(NAME test_core COMMAND test_core)

add_executable(test_network test_network.cpp)  
target_link_libraries(test_network  
    PRIVATE
        my_project_core
        GTest::gtest_main
)
add_test(NAME test_network COMMAND test_network)
```

Remarquez que les tests lient directement contre `my_project_core`. Grâce à la séparation `src/` / `apps/`, tout le code testable est dans la bibliothèque — les tests n'ont pas besoin de recompiler les sources, ils réutilisent la bibliothèque déjà compilée.

La commande `add_test()` enregistre chaque exécutable de test auprès de CTest, le framework de test intégré à CMake. Une fois le projet compilé, vous pouvez lancer tous les tests d'un coup :

```bash
cd build  
ctest --output-on-failure  
```

Nous reviendrons sur l'intégration de Google Test en section 26.3 (via `FetchContent`) et couvrirons le testing en profondeur au chapitre 33.

---

## Le répertoire `cmake/` : modules personnalisés

```
cmake/
├── CompilerWarnings.cmake
└── Sanitizers.cmake
```

Au fur et à mesure qu'un projet grandit, certaines logiques CMake deviennent réutilisables : configuration des warnings, activation des sanitizers, détection de fonctionnalités optionnelles. Plutôt que d'encombrer le `CMakeLists.txt` racine, ces logiques sont extraites dans des **modules CMake** — des fichiers `.cmake` stockés dans le répertoire `cmake/`.

Pour que CMake les trouve, on ajoute ce répertoire au chemin de recherche des modules dans le `CMakeLists.txt` racine :

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
```

Ensuite, n'importe quel `CMakeLists.txt` du projet peut inclure un module via :

```cmake
include(CompilerWarnings)  
include(Sanitizers)  
```

Un module `CompilerWarnings.cmake` typique pourrait définir une fonction réutilisable :

```cmake
function(set_project_warnings target_name)
    set(GCC_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )

    set(CLANG_WARNINGS
        ${GCC_WARNINGS}
        -Wno-unknown-warning-option
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(PROJECT_WARNINGS ${CLANG_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(PROJECT_WARNINGS ${GCC_WARNINGS})
    endif()

    target_compile_options(${target_name} PRIVATE ${PROJECT_WARNINGS})
endfunction()
```

Ce module peut ensuite être appliqué à n'importe quelle cible :

```cmake
include(CompilerWarnings)  
set_project_warnings(my_project_core)  
set_project_warnings(my_project_utils)  
```

L'intérêt est double : centralisation (un seul endroit à modifier si la politique de warnings évolue) et réutilisabilité (la même fonction s'applique à toutes les cibles).

---

## Le répertoire `build/` : le territoire généré

Le répertoire `build/` est créé par CMake lors de la configuration et contient **exclusivement** des fichiers générés : fichiers de build (Makefiles ou build.ninja), fichiers objets, binaires compilés, résultats de tests, cache CMake. Ce répertoire ne doit **jamais** être versionné.

Ajoutez-le à votre `.gitignore` :

```gitignore
# .gitignore (à la racine du projet)
build/
```

Le nettoyage complet du projet se fait simplement en supprimant ce répertoire :

```bash
rm -rf build/
```

C'est l'un des avantages du *out-of-source build* : aucun fichier généré ne se mélange avec vos sources, et un `rm -rf build/` vous ramène à un état parfaitement propre.

> 💡 **Plusieurs répertoires de build** : rien ne vous empêche de maintenir plusieurs configurations en parallèle. Par exemple `build-debug/`, `build-release/`, `build-asan/` (avec AddressSanitizer). Chaque répertoire contient sa propre configuration indépendante :  
>
> ```bash
> cmake -B build-debug   -G Ninja -DCMAKE_BUILD_TYPE=Debug
> cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
> cmake -B build-asan    -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address"
> ```

---

## Les répertoires secondaires

### `docs/`

Contient la configuration de Doxygen (souvent un `Doxyfile.in` traité par `configure_file()` — voir section 26.4), des fichiers Markdown de documentation, ou tout autre support documentaire. Ce répertoire n'interagit généralement pas avec le build CMake, sauf si vous ajoutez une cible personnalisée pour générer la documentation.

### `benchmarks/`

Même structure que `tests/`, mais dédié aux benchmarks de performance (typiquement avec Google Benchmark, couvert au chapitre 35). Conditionné à l'option `MY_PROJECT_BUILD_BENCHMARKS` pour ne pas ralentir le build courant.

### `third_party/`

Ce répertoire sert de point de chute pour les dépendances embarquées directement dans le dépôt — une pratique courante quand une dépendance n'est pas disponible via Conan ou vcpkg, ou quand vous en maintenez un fork. Avec l'avènement de `FetchContent` (section 26.3), ce répertoire tend à devenir moins nécessaire, mais il reste utile pour les cas où vous souhaitez un contrôle total sur une dépendance sans accès réseau au moment du build.

---

## Le graphe de dépendances entre cibles

Avec cette structure, le graphe de dépendances de notre projet ressemble à ceci :

```
my_app  (exécutable)
  └── my_project_core  (bibliothèque)
        └── my_project_utils  (bibliothèque)

test_core  (exécutable de test)
  ├── my_project_core
  └── GTest::gtest_main

test_network  (exécutable de test)
  ├── my_project_core
  └── GTest::gtest_main
```

Ce graphe est **acyclique** — une cible ne peut pas dépendre directement ou indirectement d'elle-même. CMake vérifie cette contrainte automatiquement. L'avantage d'un graphe propre est que CMake (et le build system sous-jacent) sait exactement ce qui doit être recompilé quand un fichier change : modifier `string_utils.cpp` ne recompile que `my_project_utils`, puis `my_project_core` (qui en dépend), puis `my_app` et les tests. Les fichiers non impactés restent intacts.

---

## Variantes de structure

La structure présentée ci-dessus est une **convention répandue**, mais ce n'est pas la seule organisation viable. Selon la taille et la nature du projet, vous rencontrerez des variantes.

### Petit projet (une seule bibliothèque, un exécutable)

Pour un projet simple — un outil CLI par exemple — il est courant de fusionner `src/` et `apps/` :

```
small_tool/
├── CMakeLists.txt
├── include/
│   └── small_tool/
│       └── config.h
├── src/
│   ├── main.cpp
│   └── config.cpp
└── tests/
    ├── CMakeLists.txt
    └── test_config.cpp
```

Un seul `CMakeLists.txt` racine suffit alors, et la cible bibliothèque est déclarée comme `OBJECT` library pour partager les fichiers objets entre l'exécutable et les tests sans créer de `.a` ou `.so` intermédiaire.

### Grand monorepo (multiples composants)

À l'échelle d'un monorepo d'entreprise, la structure s'aplatit souvent en composants de premier niveau :

```
monorepo/
├── CMakeLists.txt
├── components/
│   ├── auth/
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   ├── src/
│   │   └── tests/
│   ├── database/
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   ├── src/
│   │   └── tests/
│   └── api_gateway/
│       ├── CMakeLists.txt
│       ├── include/
│       ├── src/
│       └── tests/
└── apps/
    └── server/
        ├── CMakeLists.txt
        └── main.cpp
```

Chaque composant est une unité autonome avec sa propre triade `include/` + `src/` + `tests/`. Le `CMakeLists.txt` racine orchestre l'ensemble.

### Header-only library

Pour une bibliothèque entièrement header-only, le répertoire `src/` disparaît :

```
header_lib/
├── CMakeLists.txt
├── include/
│   └── header_lib/
│       ├── core.h
│       └── utils.h
└── tests/
    ├── CMakeLists.txt
    └── test_core.cpp
```

La bibliothèque est déclarée comme `INTERFACE` — elle n'a pas de sources propres, seulement des headers :

```cmake
add_library(header_lib INTERFACE)  
target_include_directories(header_lib  
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
```

---

## Récapitulatif des rôles

| Répertoire / Fichier | Rôle | Versionné ? |
|----------------------|------|:-----------:|
| `CMakeLists.txt` (racine) | Point d'entrée CMake, déclaration du projet, options globales | ✅ |
| `cmake/` | Modules CMake réutilisables (warnings, sanitizers, etc.) | ✅ |
| `include/project_name/` | Headers publics — l'API exposée aux consommateurs | ✅ |
| `src/` | Sources d'implémentation et headers privés | ✅ |
| `apps/` | Exécutables (fonctions `main()`) | ✅ |
| `libs/` | Bibliothèques internes additionnelles | ✅ |
| `tests/` | Tests unitaires | ✅ |
| `benchmarks/` | Benchmarks de performance | ✅ |
| `docs/` | Documentation (Doxygen, guides) | ✅ |
| `third_party/` | Dépendances embarquées | ✅ |
| `build/` | Fichiers générés par CMake et le build system | ❌ |
| `CMakePresets.json` | Configurations de build standardisées (section 27.6) | ✅ |

---

> **À suivre** : La section 26.2 détaille l'écriture des `CMakeLists.txt` — comment déclarer des cibles, des bibliothèques et des exécutables, et comment le système de visibilité `PUBLIC` / `PRIVATE` / `INTERFACE` propage les propriétés entre les cibles.

⏭️ [Écrire un CMakeLists.txt : targets, libraries, executables](/26-cmake/02-cmakelists.md)
