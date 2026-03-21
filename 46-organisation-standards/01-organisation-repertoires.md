🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 46.1 — Organisation des répertoires (`src/`, `include/`, `tests/`, `docs/`)

> **Chapitre 46 : Organisation et Standards** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert · **Prérequis** : Chapitres 26 (CMake), 33 (Google Test), 38 (CI/CD)

---

## Le problème : pourquoi la structure compte

Dans un projet C++ de taille non triviale, l'organisation physique des fichiers n'est pas une simple commodité : elle détermine la façon dont CMake découvre les cibles, elle conditionne ce qui est exposé comme API publique lors de l'installation d'une bibliothèque, elle influence les temps de compilation incrémentale, et elle dicte la lisibilité du projet pour tout nouveau développeur qui clone le dépôt.

Contrairement à Python ou Go, le C++ n'impose aucune convention de répertoires au niveau du langage. Le compilateur se contente de recevoir des fichiers sources et des chemins d'include via des flags. Cette liberté totale est un piège : sans convention partagée, chaque projet invente sa propre structure, et l'intégration de bibliothèques tierces devient un casse-tête. L'écosystème a progressivement convergé vers un modèle canonique, porté par CMake, Conan, vcpkg et les pratiques de l'industrie.

---

## La structure canonique d'un projet C++

Voici l'arborescence de référence pour un projet professionnel. Nous allons détailler chaque répertoire, son rôle et les règles qui le gouvernent.

```
mon-projet/
├── CMakeLists.txt              # Build system racine
├── CMakePresets.json            # Presets de configuration (section 27.6)
├── conanfile.py                 # Gestion des dépendances (section 27.2)
├── README.md
├── LICENSE
├── .clang-format                # Standard de formatage (section 32.3)
├── .clang-tidy                  # Règles d'analyse statique (section 32.1)
├── .gitignore
│
├── include/                     # Headers publics (API exportée)
│   └── mon-projet/
│       ├── core/
│       │   ├── engine.h
│       │   └── config.h
│       └── utils/
│           └── string_utils.h
│
├── src/                         # Sources d'implémentation + headers internes
│   ├── CMakeLists.txt
│   ├── core/
│   │   ├── engine.cpp
│   │   ├── engine_internal.h    # Header interne, non exporté
│   │   └── config.cpp
│   └── utils/
│       └── string_utils.cpp
│
├── apps/                        # Exécutables (points d'entrée)
│   ├── CMakeLists.txt
│   ├── main.cpp
│   └── cli.cpp
│
├── tests/                       # Tests unitaires et d'intégration
│   ├── CMakeLists.txt
│   ├── core/
│   │   ├── engine_test.cpp
│   │   └── config_test.cpp
│   └── utils/
│       └── string_utils_test.cpp
│
├── benchmarks/                  # Benchmarks (Google Benchmark)
│   ├── CMakeLists.txt
│   └── engine_bench.cpp
│
├── docs/                        # Documentation
│   ├── Doxyfile                 # Configuration Doxygen (section 46.4)
│   └── architecture.md
│
├── cmake/                       # Modules CMake personnalisés
│   ├── CompilerWarnings.cmake
│   └── Sanitizers.cmake
│
├── scripts/                     # Scripts utilitaires (CI, packaging)
│   ├── build.sh
│   └── run_tests.sh
│
└── third_party/                 # Dépendances vendored (si nécessaire)
    └── ...
```

Cette structure n'est pas arbitraire. Chaque répertoire répond à un besoin précis, et les choix de placement ont des conséquences techniques concrètes.

---

## `include/` — L'API publique

Le répertoire `include/` contient exclusivement les fichiers d'en-tête qui constituent l'interface publique du projet. C'est ce que les consommateurs de votre bibliothèque incluront dans leur propre code.

### La règle du sous-répertoire nommé

Les headers ne sont jamais placés directement à la racine de `include/`. Ils sont toujours sous un sous-répertoire portant le nom du projet :

```
include/
└── mon-projet/
    └── engine.h
```

Cela permet aux consommateurs d'écrire :

```cpp
#include <mon-projet/engine.h>
```

Cette convention a deux avantages majeurs. D'abord, elle élimine les collisions de noms : si deux bibliothèques exposent un `config.h`, le préfixe les différencie sans ambiguïté. Ensuite, elle rend les dépendances explicites dans le code source — en lisant un `#include`, on sait immédiatement de quel projet provient le header.

### Intégration avec CMake

Dans le `CMakeLists.txt`, le répertoire `include/` est déclaré comme chemin d'include **public** de la cible :

```cmake
add_library(mon-projet
    src/core/engine.cpp
    src/utils/string_utils.cpp
)

target_include_directories(mon-projet
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
```

Le mot-clé `PUBLIC` (voir section 26.2.4) signifie que ce chemin d'include sera automatiquement propagé à toute cible qui fera `target_link_libraries(... mon-projet)`. Les generator expressions `BUILD_INTERFACE` et `INSTALL_INTERFACE` distinguent le chemin utilisé lors de la compilation locale et celui utilisé après installation sur le système.

### Ce qui n'a pas sa place dans `include/`

Les headers d'implémentation interne — structures de données privées, helpers non documentés, détails d'implémentation — n'ont rien à faire dans `include/`. Les exposer revient à promettre une stabilité d'API sur du code interne, ce qui contraint inutilement les refactorings futurs. Ces headers vivent dans `src/`, à côté des fichiers `.cpp` qui les utilisent.

---

## `src/` — L'implémentation

Le répertoire `src/` contient les fichiers sources (`.cpp`) et les headers internes (`.h` ou `_internal.h`) qui ne font pas partie de l'API publique.

### Organisation en sous-répertoires

La structure interne de `src/` reflète généralement l'organisation logique du projet, souvent par module ou par composant :

```
src/
├── core/
│   ├── engine.cpp
│   ├── engine_internal.h
│   └── config.cpp
└── utils/
    └── string_utils.cpp
```

Cette organisation par composants a un avantage direct avec CMake : il est facile de créer des sous-bibliothèques internes si le projet grandit, en ajoutant un `CMakeLists.txt` par sous-répertoire et en utilisant `add_subdirectory()`.

### Headers internes vs headers publics

La distinction est fondamentale et souvent négligée dans les projets mal structurés. Un header dans `include/mon-projet/` est une promesse contractuelle envers les utilisateurs de la bibliothèque. Un header dans `src/` est un détail d'implémentation qui peut changer sans préavis.

Concrètement, si une classe `Engine` expose une interface publique dans `include/mon-projet/core/engine.h`, mais utilise en interne un pool de threads dont les détails n'intéressent pas les consommateurs, la structure du pool sera déclarée dans `src/core/engine_internal.h`. Ce header est inclus par `engine.cpp` mais jamais par le code externe.

Dans CMake, cette séparation est reflétée par le mot-clé `PRIVATE` sur le chemin d'include de `src/` :

```cmake
target_include_directories(mon-projet
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
```

Les cibles qui dépendent de `mon-projet` ne verront pas les headers de `src/`.

### Convention de nommage des fichiers

Plusieurs conventions coexistent dans l'industrie. Les plus courantes sont le `snake_case` (`string_utils.cpp`) et le `PascalCase` (`StringUtils.cpp`). L'important n'est pas le choix en lui-même mais sa cohérence sur l'ensemble du projet. Le Google C++ Style Guide (section 46.5.1) recommande le `snake_case` avec extension `.cc` ; le LLVM Style (section 46.5.2) utilise le `PascalCase` avec extension `.cpp`. Choisissez une convention, documentez-la, et appliquez-la avec `clang-format` et les pre-commit hooks (chapitre 47).

---

## `apps/` — Les points d'entrée

Les exécutables du projet — le programme principal, les outils CLI, les utilitaires — sont séparés des sources de la bibliothèque. Cette séparation a une raison technique simple : elle permet de compiler le cœur du projet en tant que bibliothèque (statique ou dynamique), puis de linker cette bibliothèque avec différents exécutables.

```
apps/
├── CMakeLists.txt
├── main.cpp          # Point d'entrée principal
└── cli.cpp           # Outil en ligne de commande secondaire
```

Le `CMakeLists.txt` de `apps/` est minimal :

```cmake
add_executable(mon-app main.cpp)  
target_link_libraries(mon-app PRIVATE mon-projet)  

add_executable(mon-cli cli.cpp)  
target_link_libraries(mon-cli PRIVATE mon-projet)  
```

Ce découplage rend les tests unitaires possibles sans passer par un exécutable. Les tests incluent les headers de la bibliothèque et linkent directement avec elle, sans avoir besoin de tester le `main()`.

Certains projets utilisent le nom `app/` (singulier) quand il n'y a qu'un seul exécutable, ou `tools/` pour des utilitaires annexes. La convention importe moins que la séparation elle-même.

---

## `tests/` — Les tests

Le répertoire `tests/` regroupe tous les tests du projet : tests unitaires, tests d'intégration, et éventuellement des fixtures ou données de test.

### Structure miroir

La bonne pratique consiste à reproduire dans `tests/` la structure de `src/`, de sorte qu'un développeur sache immédiatement où trouver les tests d'un composant donné :

```
src/                          tests/
├── core/                     ├── core/
│   ├── engine.cpp            │   ├── engine_test.cpp
│   └── config.cpp            │   └── config_test.cpp
└── utils/                    └── utils/
    └── string_utils.cpp          └── string_utils_test.cpp
```

Le suffixe `_test.cpp` est la convention la plus répandue. Google Test et CTest s'y intègrent naturellement.

### Intégration CMake et CTest

Le `CMakeLists.txt` de `tests/` enregistre chaque test auprès de CTest :

```cmake
find_package(GTest REQUIRED)

add_executable(engine_test core/engine_test.cpp)  
target_link_libraries(engine_test PRIVATE mon-projet GTest::gtest_main)  
add_test(NAME engine_test COMMAND engine_test)  

add_executable(config_test core/config_test.cpp)  
target_link_libraries(config_test PRIVATE mon-projet GTest::gtest_main)  
add_test(NAME config_test COMMAND config_test)  
```

L'utilisation de `GTest::gtest_main` évite d'écrire un `main()` dans chaque fichier de test — Google Test fournit le point d'entrée.

Les tests sont ensuite exécutables via `ctest --test-dir build` depuis la racine du projet, ce qui s'intègre directement dans les pipelines CI/CD (chapitre 38).

### Données de test

Si les tests nécessitent des fichiers d'entrée (JSON de configuration, fixtures, fichiers binaires de référence), on les place dans un sous-répertoire dédié :

```
tests/
├── data/
│   ├── sample_config.json
│   └── expected_output.bin
└── ...
```

Un pattern courant consiste à définir un chemin de base dans CMake via `configure_file` ou une macro de préprocesseur pour que les tests trouvent ces fichiers indépendamment du répertoire de build :

```cmake
target_compile_definitions(engine_test PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
```

---

## `benchmarks/` — Les benchmarks de performance

Les benchmarks ne sont pas des tests : ils ne vérifient pas un comportement correct, ils mesurent une performance. Les mélanger avec les tests unitaires crée de la confusion dans le pipeline CI (un benchmark qui "passe" n'a pas le même sens qu'un test qui "passe") et allonge inutilement la durée des runs de tests.

```
benchmarks/
├── CMakeLists.txt
└── engine_bench.cpp
```

Le `CMakeLists.txt` est analogue à celui des tests, mais avec Google Benchmark :

```cmake
find_package(benchmark REQUIRED)

add_executable(engine_bench engine_bench.cpp)  
target_link_libraries(engine_bench PRIVATE mon-projet benchmark::benchmark)  
```

En CI, les benchmarks sont typiquement dans un stage séparé, optionnel, ou déclenché uniquement sur certaines branches (chapitre 38).

---

## `docs/` — La documentation

Le répertoire `docs/` contient tout ce qui relève de la documentation du projet : la configuration Doxygen (détaillée en section 46.4), les guides d'architecture, les ADR (Architecture Decision Records) et toute documentation narrative qui ne tient pas dans un simple commentaire de code.

```
docs/
├── Doxyfile                   # Configuration Doxygen
├── architecture.md            # Vue d'ensemble architecturale
├── adr/                       # Architecture Decision Records
│   ├── 001-choix-json-lib.md
│   └── 002-threading-model.md
└── diagrams/
    └── system-overview.svg
```

### Architecture Decision Records (ADR)

Les ADR sont des documents courts et structurés qui capturent les décisions techniques importantes : pourquoi on a choisi nlohmann/json plutôt que RapidJSON, pourquoi le projet utilise des threads plutôt que des processus, pourquoi on a opté pour le linkage statique. Chaque ADR suit un format simple — contexte, décision, conséquences — et porte un numéro séquentiel.

Ces documents sont précieux pour les nouveaux arrivants et pour le "vous du futur" qui se demandera pourquoi telle décision a été prise. Ils vivent dans le dépôt Git, versionnés avec le code.

---

## `cmake/` — Modules CMake personnalisés

Les fichiers `.cmake` réutilisables — configuration des warnings, activation des sanitizers, détection de la plateforme — sont regroupés dans un répertoire dédié plutôt que d'encombrer le `CMakeLists.txt` racine :

```
cmake/
├── CompilerWarnings.cmake
├── Sanitizers.cmake
└── StaticAnalysis.cmake
```

Ces modules sont inclus depuis le `CMakeLists.txt` racine :

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")  
include(CompilerWarnings)  
include(Sanitizers)  
```

Un module `CompilerWarnings.cmake` typique définit une fonction réutilisable :

```cmake
function(set_project_warnings target)
    target_compile_options(${target} PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -Wshadow -Wnon-virtual-dtor -Wold-style-cast
        -Wcast-align -Wunused -Woverloaded-virtual
        -Wconversion -Wsign-conversion -Wnull-dereference
        -Wformat=2 -Wimplicit-fallthrough
    )
endfunction()
```

Ce pattern maintient le `CMakeLists.txt` racine lisible et permet de réutiliser la même configuration de warnings sur toutes les cibles du projet.

---

## `scripts/` — Scripts utilitaires

Les scripts shell, Python ou autres qui gravitent autour du projet — scripts de build, de packaging, de déploiement, de génération de documentation — sont regroupés dans `scripts/` :

```
scripts/
├── build.sh              # Build locale avec options par défaut
├── run_tests.sh          # Exécution des tests avec formatage
├── generate_docs.sh      # Génération Doxygen
└── package_deb.sh        # Création du paquet DEB (section 39.1)
```

Ces scripts ne remplacent pas CMake ou le pipeline CI : ils fournissent des raccourcis pour les opérations fréquentes en développement local. Un nouveau développeur devrait pouvoir cloner le dépôt et lancer `./scripts/build.sh` pour obtenir un build fonctionnel sans lire trente pages de documentation.

---

## `third_party/` — Dépendances vendored

Dans l'idéal, toutes les dépendances sont gérées par Conan ou vcpkg (chapitre 27). Dans la pratique, certains projets intègrent directement des bibliothèques tierces — soit parce qu'elles sont header-only et minuscules, soit parce qu'elles ont été patchées localement, soit pour des contraintes de reproductibilité stricte.

```
third_party/
├── catch2/              # Header-only, version fixe
└── stb/                 # stb_image.h, vendored
```

Si vous empruntez cette voie, deux règles doivent être respectées. Premièrement, chaque dépendance vendored doit porter un fichier indiquant sa version et sa licence. Deuxièmement, les dépendances vendored ne doivent jamais être modifiées silencieusement — si un patch est nécessaire, il doit être documenté et idéalement conservé sous forme de fichier `.patch` versionné.

Pour les projets gérés par Conan ou vcpkg, ce répertoire peut être absent ou ne contenir que les cas exceptionnels.

---

## Le `CMakeLists.txt` racine : le chef d'orchestre

La structure de répertoires prend vie dans le `CMakeLists.txt` racine, qui orchestre l'ensemble du projet :

```cmake
cmake_minimum_required(VERSION 3.25)  
project(mon-projet  
    VERSION 1.2.0
    LANGUAGES CXX
    DESCRIPTION "Description du projet"
)

# Standard C++ et options globales
set(CMAKE_CXX_STANDARD 23)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
set(CMAKE_CXX_EXTENSIONS OFF)  

# Modules CMake personnalisés
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")  
include(CompilerWarnings)  
include(Sanitizers)  

# La bibliothèque principale
add_subdirectory(src)

# Exécutables
add_subdirectory(apps)

# Tests (optionnel, contrôlé par une option)
option(BUILD_TESTS "Build unit tests" ON)  
if(BUILD_TESTS)  
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarks (optionnel)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)  
if(BUILD_BENCHMARKS)  
    add_subdirectory(benchmarks)
endif()

# Documentation (optionnel)
option(BUILD_DOCS "Build documentation" OFF)  
if(BUILD_DOCS)  
    add_subdirectory(docs)
endif()
```

Les options `BUILD_TESTS`, `BUILD_BENCHMARKS` et `BUILD_DOCS` permettent de désactiver les composants non essentiels lors d'un build de production ou pour accélérer le développement local. Ces options s'intègrent naturellement avec les CMake Presets (section 27.6) :

```json
{
    "configurePresets": [
        {
            "name": "dev",
            "cacheVariables": {
                "BUILD_TESTS": "ON",
                "BUILD_BENCHMARKS": "OFF",
                "CMAKE_BUILD_TYPE": "Debug"
            }
        },
        {
            "name": "release",
            "cacheVariables": {
                "BUILD_TESTS": "OFF",
                "BUILD_BENCHMARKS": "OFF",
                "CMAKE_BUILD_TYPE": "Release"
            }
        }
    ]
}
```

---

## Fichiers de configuration à la racine

La racine du projet accueille les fichiers de configuration transversaux. Chacun a sa raison d'être :

| Fichier | Rôle |
|---|---|
| `CMakeLists.txt` | Build system racine |
| `CMakePresets.json` | Presets de configuration standardisés |
| `conanfile.py` | Déclaration des dépendances Conan |
| `.clang-format` | Règles de formatage automatique |
| `.clang-tidy` | Règles d'analyse statique |
| `.gitignore` | Exclusions Git (build/, .cache/, etc.) |
| `.pre-commit-config.yaml` | Hooks pre-commit (chapitre 47) |
| `README.md` | Documentation d'accueil |
| `LICENSE` | Licence du projet |

Ces fichiers ne sont pas du bruit : ils constituent l'infrastructure invisible qui permet au projet de fonctionner de façon reproductible sur n'importe quelle machine de développeur et dans n'importe quel pipeline CI.

---

## Variantes selon la taille du projet

### Le petit projet (un seul exécutable, pas de bibliothèque)

Pour un utilitaire CLI simple ou un exercice, la structure complète est excessive. Une version allégée suffit :

```
mon-outil/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   └── utils.cpp
├── include/
│   └── mon-outil/
│       └── utils.h
└── tests/
    └── utils_test.cpp
```

La séparation `include/`/`src/` reste pertinente même à petite échelle, car elle prend zéro effort supplémentaire et facilite une éventuelle extraction en bibliothèque.

### Le monorepo multi-bibliothèques

Les grands projets contiennent souvent plusieurs bibliothèques internes. Chaque bibliothèque a sa propre structure canonique, imbriquée sous un répertoire `libs/` :

```
mon-monorepo/
├── CMakeLists.txt
├── apps/
│   └── main.cpp
├── libs/
│   ├── core/
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── core/
│   │   │       └── engine.h
│   │   ├── src/
│   │   │   └── engine.cpp
│   │   └── tests/
│   │       └── engine_test.cpp
│   └── networking/
│       ├── CMakeLists.txt
│       ├── include/
│       │   └── networking/
│       │       └── tcp_server.h
│       ├── src/
│       │   └── tcp_server.cpp
│       └── tests/
│           └── tcp_server_test.cpp
└── tests/
    └── integration/            # Tests d'intégration cross-libs
        └── full_stack_test.cpp
```

Chaque bibliothèque est autonome : elle possède ses headers publics, ses sources, ses tests et son `CMakeLists.txt`. Le `CMakeLists.txt` racine se contente de faire `add_subdirectory(libs/core)` et `add_subdirectory(libs/networking)`. Les dépendances entre bibliothèques internes sont déclarées via `target_link_libraries`.

Cette structure supporte la croissance : ajouter une nouvelle bibliothèque revient à créer un nouveau répertoire sous `libs/` en copiant la structure existante.

---

## Les erreurs fréquentes à éviter

**Mélanger headers publics et privés dans le même répertoire.** C'est le piège le plus courant. Quand tout est dans `include/`, il est impossible de distinguer ce qui fait partie de l'API stable de ce qui est un détail d'implémentation. Les consommateurs finissent par inclure des headers internes, ce qui rend tout refactoring cassant.

**Placer le `main()` dans la bibliothèque.** Si le fichier contenant la fonction `main()` est compilé avec les sources de la bibliothèque, il devient impossible de linker cette bibliothèque avec les tests unitaires (conflit de symboles : deux `main()`). La séparation `src/` vs `apps/` résout ce problème structurellement.

**Avoir un seul `CMakeLists.txt` monolithique.** Un projet de plus de quelques fichiers mérite des `CMakeLists.txt` distribués dans les sous-répertoires. Le fichier racine orchestre ; les fichiers enfants décrivent leurs cibles locales. Cela améliore la lisibilité, facilite la maintenance et permet à chaque sous-répertoire d'être potentiellement compilé indépendamment.

**Ignorer le répertoire `build/`.** Le build doit toujours être out-of-source, dans un répertoire séparé (typiquement `build/` ou `out/`) qui est dans le `.gitignore`. Les builds in-source polluent l'arborescence avec des fichiers générés et rendent le `git status` illisible.

**Stocker les binaires et artefacts de build dans Git.** Les fichiers `.o`, `.a`, `.so`, les exécutables compilés et les artefacts de build n'ont pas leur place dans le dépôt. Le `.gitignore` doit exclure le répertoire de build, les caches de ccache et les fichiers générés par Conan.

---

## Résumé

L'organisation physique d'un projet C++ repose sur une séparation claire des responsabilités : `include/` pour l'API publique, `src/` pour l'implémentation, `apps/` pour les exécutables, `tests/` pour les tests, `docs/` pour la documentation, et `cmake/` pour les modules de build réutilisables. Cette structure n'est pas une convention esthétique mais un choix technique qui s'articule avec CMake, les gestionnaires de dépendances et les pipelines CI/CD. Elle s'adapte en taille — du petit utilitaire au monorepo — tout en conservant les mêmes principes fondamentaux : séparation public/privé, découplage bibliothèque/exécutable, et build out-of-source systématique.

---


⏭️ [Séparation .h/.cpp et compilation incrémentale](/46-organisation-standards/02-separation-h-cpp.md)
