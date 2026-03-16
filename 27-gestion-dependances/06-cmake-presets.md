🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 27.6 CMake Presets : Standardisation des configurations ⭐

> **Objectif** : Maîtriser le système de CMake Presets pour standardiser les configurations de build d'un projet — du développement local à la CI/CD — en intégrant les gestionnaires de paquets (Conan, vcpkg), les toolchains, et les options spécifiques au projet dans une interface unique et versionnable.

---

## Le problème que les presets résolvent

Tout au long des chapitres 26 et 27, les commandes de configuration CMake sont devenues de plus en plus longues :

```bash
cmake -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DMY_PROJECT_BUILD_TESTS=ON \
    -DMY_PROJECT_BUILD_BENCHMARKS=OFF
```

Cette commande est correcte, mais elle pose plusieurs problèmes. Elle est longue et sujette aux erreurs de frappe. Elle vit dans la mémoire du développeur ou dans un README que personne ne lit. Elle diffère subtilement entre les machines. Elle est dupliquée (avec des variations) dans les scripts CI.

Les **CMake Presets** résolvent ce problème en encapsulant les paramètres de configuration dans un fichier JSON versionné avec le projet. Chaque preset est un ensemble nommé de paramètres — générateur, répertoire de build, variables de cache, toolchain, variables d'environnement — qu'on invoque en une seule commande :

```bash
cmake --preset release  
cmake --build --preset release  
ctest --preset release  
```

---

## Les deux fichiers de presets

CMake reconnaît deux fichiers de presets à la racine du projet :

**`CMakePresets.json`** : le fichier du **projet**. Il est versionné dans Git et partagé entre tous les développeurs. Il contient les configurations « officielles » du projet — celles que la CI utilise et que tout contributeur doit pouvoir reproduire.

**`CMakeUserPresets.json`** : le fichier de l'**utilisateur**. Il est listé dans `.gitignore` et propre à chaque développeur. Il peut hériter des presets du fichier projet et y ajouter des personnalisations locales (chemins spécifiques à la machine, options de développement, presets Conan générés automatiquement).

```gitignore
# .gitignore
CMakeUserPresets.json  
build*/  
```

Cette séparation est essentielle : le fichier projet garantit la reproductibilité, le fichier utilisateur offre la flexibilité.

---

## Structure d'un fichier de presets

```json
{
    "version": 6,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 25,
        "patch": 0
    },
    "configurePresets": [ ... ],
    "buildPresets": [ ... ],
    "testPresets": [ ... ],
    "workflowPresets": [ ... ]
}
```

### Versionnage du schéma

Le champ `"version"` indique la version du schéma de presets, pas la version de votre projet. Chaque version de schéma ajoute de nouvelles fonctionnalités :

| Version schéma | CMake minimum | Fonctionnalités ajoutées |
|:--------------:|:------------:|--------------------------|
| 2 | 3.20 | Presets de base (configure) |
| 3 | 3.21 | Build presets et test presets |
| 4 | 3.21 | `include` pour l'import de fichiers |
| 5 | 3.24 | Workflow presets |
| 6 | 3.25 | Build/test presets sans configure preset, améliorations |
| 10 | 3.31 | Commentaires `$comment`, graphviz |

En 2026, la version 6 est un bon choix : elle offre toutes les fonctionnalités essentielles et est compatible avec CMake 3.25+.

---

## Configure presets

Les configure presets sont le cœur du système. Chaque preset encapsule les paramètres d'une invocation `cmake -B build ...`.

### Preset minimal

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "debug",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build-debug",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            }
        }
    ]
}
```

```bash
cmake --preset debug
# Équivalent à : cmake -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

### Champs principaux

| Champ | Rôle | Exemple |
|-------|------|---------|
| `name` | Identifiant unique (utilisé en CLI) | `"release"` |
| `displayName` | Nom lisible (affiché par les IDE) | `"Release — GCC 15"` |
| `description` | Description longue | `"Build Release optimisé..."` |
| `generator` | Générateur CMake | `"Ninja"`, `"Ninja Multi-Config"` |
| `binaryDir` | Répertoire de build | `"${sourceDir}/build"` |
| `toolchainFile` | Fichier toolchain | `"cmake/toolchains/clang.cmake"` |
| `cacheVariables` | Variables CMake (`-D`) | `{"CMAKE_BUILD_TYPE": "Release"}` |
| `environment` | Variables d'environnement | `{"CC": "gcc-15"}` |
| `inherits` | Héritage d'un autre preset | `"base"` ou `["base", "linux"]` |
| `hidden` | Cacher le preset (base d'héritage) | `true` |
| `condition` | Condition d'activation (OS, etc.) | Voir ci-dessous |

### Héritage

L'héritage est le mécanisme clé pour éviter la duplication entre presets. Un preset enfant hérite de tous les champs de son parent et peut les surcharger :

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "base",
            "hidden": true,
            "generator": "Ninja",
            "cacheVariables": {
                "CMAKE_CXX_COMPILER_LAUNCHER": "ccache",
                "MY_PROJECT_BUILD_TESTS": "ON"
            }
        },
        {
            "name": "debug",
            "inherits": "base",
            "displayName": "Debug",
            "binaryDir": "${sourceDir}/build-debug",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            }
        },
        {
            "name": "release",
            "inherits": "base",
            "displayName": "Release",
            "binaryDir": "${sourceDir}/build-release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "MY_PROJECT_BUILD_TESTS": "OFF"
            }
        }
    ]
}
```

Le preset `base` est `hidden: true` — il n'apparaît pas dans `cmake --list-presets` et ne peut pas être invoqué directement. Il sert uniquement de base d'héritage. Les presets `debug` et `release` héritent de `base`, récupèrent Ninja et ccache, et ajoutent leur propre type de build.

L'héritage multiple est supporté via un tableau :

```json
{
    "name": "linux-debug",
    "inherits": ["linux", "debug"]
}
```

Les champs sont fusionnés de gauche à droite : les valeurs du dernier parent écrasent celles des précédents en cas de conflit.

### Conditions

Les conditions permettent de restreindre un preset à certaines plateformes :

```json
{
    "name": "linux-base",
    "hidden": true,
    "condition": {
        "type": "equals",
        "lhs": "${hostSystemName}",
        "rhs": "Linux"
    },
    "generator": "Ninja"
}
```

Ce preset ne sera proposé que sur Linux. Les macros `${hostSystemName}`, `${sourceDir}`, `${sourceParentDir}` et les variables d'environnement `$env{HOME}` sont disponibles dans les conditions et les valeurs.

---

## Build presets et test presets

Les configure presets ne couvrent que la phase de configuration (`cmake -B build`). Les build presets et test presets encapsulent respectivement `cmake --build` et `ctest` :

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "release",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build-release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "release",
            "configurePreset": "release",
            "configuration": "Release",
            "jobs": 0
        }
    ],
    "testPresets": [
        {
            "name": "release",
            "configurePreset": "release",
            "configuration": "Release",
            "output": {
                "outputOnFailure": true
            }
        }
    ]
}
```

Le champ `configurePreset` lie le build/test preset au configure preset correspondant — CMake sait ainsi quel répertoire de build utiliser. Le champ `jobs: 0` signifie « utiliser tous les cœurs disponibles ».

```bash
cmake --preset release           # Configure  
cmake --build --preset release   # Build  
ctest --preset release           # Test  
```

---

## Workflow presets

Les workflow presets (schéma version 5+) enchaînent configure, build et test en une seule commande :

```json
{
    "version": 6,
    "workflowPresets": [
        {
            "name": "ci-release",
            "displayName": "CI Release Pipeline",
            "steps": [
                { "type": "configure", "name": "release" },
                { "type": "build", "name": "release" },
                { "type": "test", "name": "release" }
            ]
        }
    ]
}
```

```bash
# Une seule commande pour tout le pipeline
cmake --workflow ci-release
```

C'est particulièrement utile en CI/CD où le pipeline enchaîne toujours les mêmes étapes.

---

## Intégration avec Conan

La section 27.2.4 a montré que Conan génère automatiquement un `CMakeUserPresets.json` via `cmake_layout()`. Ce fichier contient des configure presets (comme `conan-release`, `conan-debug`) qui pointent vers le toolchain Conan.

La stratégie recommandée est de faire hériter vos presets de projet des presets Conan :

```json
{
    "version": 6,
    "$comment": "Les presets conan-* sont définis dans CMakeUserPresets.json généré par Conan",
    "configurePresets": [
        {
            "name": "dev-debug",
            "displayName": "Dev Debug (Conan)",
            "inherits": "conan-debug",
            "cacheVariables": {
                "MY_PROJECT_BUILD_TESTS": "ON",
                "MY_PROJECT_BUILD_BENCHMARKS": "OFF"
            }
        },
        {
            "name": "dev-release",
            "displayName": "Dev Release (Conan)",
            "inherits": "conan-release",
            "cacheVariables": {
                "MY_PROJECT_BUILD_TESTS": "ON"
            }
        },
        {
            "name": "ci-release",
            "displayName": "CI Release (Conan)",
            "inherits": "conan-release",
            "cacheVariables": {
                "MY_PROJECT_BUILD_TESTS": "ON",
                "MY_PROJECT_BUILD_BENCHMARKS": "ON"
            }
        }
    ],
    "buildPresets": [
        { "name": "dev-debug", "configurePreset": "dev-debug" },
        { "name": "dev-release", "configurePreset": "dev-release" },
        { "name": "ci-release", "configurePreset": "ci-release" }
    ],
    "testPresets": [
        { "name": "dev-debug", "configurePreset": "dev-debug", "output": {"outputOnFailure": true} },
        { "name": "dev-release", "configurePreset": "dev-release", "output": {"outputOnFailure": true} },
        { "name": "ci-release", "configurePreset": "ci-release", "output": {"outputOnFailure": true} }
    ]
}
```

Le workflow quotidien devient :

```bash
# 1. Installer les dépendances Conan (génère CMakeUserPresets.json)
conan install . -pr=conan/profiles/gcc-debug --build=missing  
conan install . -pr=conan/profiles/gcc-release --build=missing  

# 2. Développement
cmake --preset dev-debug  
cmake --build --preset dev-debug  
ctest --preset dev-debug  
```

---

## Intégration avec vcpkg

Avec vcpkg, le toolchain est une variable de cache plutôt qu'un fichier généré. L'intégration passe directement par le `CMakePresets.json` :

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "vcpkg-base",
            "hidden": true,
            "generator": "Ninja",
            "cacheVariables": {
                "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
                "CMAKE_CXX_COMPILER_LAUNCHER": "ccache"
            }
        },
        {
            "name": "vcpkg-debug",
            "inherits": "vcpkg-base",
            "displayName": "Debug (vcpkg)",
            "binaryDir": "${sourceDir}/build-debug",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            }
        },
        {
            "name": "vcpkg-release",
            "inherits": "vcpkg-base",
            "displayName": "Release (vcpkg)",
            "binaryDir": "${sourceDir}/build-release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            }
        },
        {
            "name": "vcpkg-static",
            "inherits": "vcpkg-release",
            "displayName": "Release Static (vcpkg)",
            "binaryDir": "${sourceDir}/build-static",
            "cacheVariables": {
                "VCPKG_TARGET_TRIPLET": "x64-linux-static"
            }
        }
    ]
}
```

Notez `$env{VCPKG_ROOT}` : CMake résout la variable d'environnement au moment de la configuration. Le développeur doit avoir `VCPKG_ROOT` défini dans son shell (ce qui est le cas après l'installation de vcpkg — section 27.3).

---

## Presets pour la CI/CD

Les presets simplifient considérablement les pipelines CI/CD en remplaçant les commandes longues par des noms de presets :

### GitHub Actions

```yaml
name: Build & Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        preset: [ci-debug, ci-release]

    steps:
      - uses: actions/checkout@v4

      - name: Install tools
        run: |
          sudo apt-get update
          sudo apt-get install -y ninja-build ccache
          pipx install conan

      - name: Install dependencies
        run: |
          conan profile detect
          conan install . -pr=conan/profiles/${{ matrix.preset == 'ci-debug' && 'gcc-debug' || 'gcc-release' }} --build=missing

      - name: Configure
        run: cmake --preset ${{ matrix.preset }}

      - name: Build
        run: cmake --build --preset ${{ matrix.preset }}

      - name: Test
        run: ctest --preset ${{ matrix.preset }}
```

### GitLab CI

```yaml
stages:
  - build
  - test

.build-template:
  image: ubuntu:24.04
  before_script:
    - apt-get update && apt-get install -y g++ cmake ninja-build ccache python3-pip
    - pip3 install conan
    - conan profile detect

build-release:
  extends: .build-template
  stage: build
  script:
    - conan install . -pr=conan/profiles/gcc-release --build=missing
    - cmake --preset ci-release
    - cmake --build --preset ci-release
  artifacts:
    paths:
      - build-release/

test-release:
  extends: .build-template
  stage: test
  needs: [build-release]
  script:
    - ctest --preset ci-release
```

La CI utilise les mêmes presets que les développeurs. Si un build casse en CI, le développeur peut le reproduire localement avec `cmake --preset ci-release` — pas besoin de deviner quels flags le CI utilisait.

---

## Presets multi-compilateur et cross-compilation

Pour un projet qui supporte plusieurs configurations, les presets peuvent couvrir toute la matrice :

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "base",
            "hidden": true,
            "generator": "Ninja",
            "cacheVariables": {
                "CMAKE_CXX_COMPILER_LAUNCHER": "ccache"
            }
        },
        {
            "name": "gcc-debug",
            "inherits": "base",
            "binaryDir": "${sourceDir}/build-gcc-debug",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_C_COMPILER": "gcc-15",
                "CMAKE_CXX_COMPILER": "g++-15"
            }
        },
        {
            "name": "gcc-release",
            "inherits": "base",
            "binaryDir": "${sourceDir}/build-gcc-release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "CMAKE_C_COMPILER": "gcc-15",
                "CMAKE_CXX_COMPILER": "g++-15"
            }
        },
        {
            "name": "clang-release",
            "inherits": "base",
            "binaryDir": "${sourceDir}/build-clang-release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "CMAKE_C_COMPILER": "clang-20",
                "CMAKE_CXX_COMPILER": "clang++-20"
            }
        },
        {
            "name": "cross-arm64",
            "inherits": "base",
            "binaryDir": "${sourceDir}/build-arm64",
            "toolchainFile": "${sourceDir}/cmake/toolchains/aarch64-linux-gnu.cmake",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            }
        }
    ]
}
```

```bash
# Lister toutes les configurations disponibles
cmake --list-presets
# Available configure presets:
#   "gcc-debug"       - 
#   "gcc-release"     - 
#   "clang-release"   - 
#   "cross-arm64"     - 

# Choisir une configuration
cmake --preset clang-release  
cmake --build --preset clang-release  
```

Les presets documentent de manière exhaustive les configurations supportées par le projet. Un nouveau contributeur exécute `cmake --list-presets` et voit immédiatement ce qui est disponible.

---

## Intégration IDE

Les IDE modernes exploitent nativement les fichiers de presets.

**VS Code** (extension CMake Tools) : détecte automatiquement `CMakePresets.json` et propose les presets dans la barre de statut. La sélection d'un preset configure automatiquement le build.

**CLion** : supporte les presets CMake depuis la version 2022.1. Les presets apparaissent comme des configurations de build dans le sélecteur Run/Debug.

**Visual Studio** : supporte les presets depuis VS 2022. Le sélecteur de configuration utilise les noms de presets.

L'avantage est que la configuration IDE est **dérivée** du fichier de presets, pas définie séparément. Quand un preset change, tous les IDE se mettent à jour automatiquement.

---

## Bonnes pratiques

**Versionnez `CMakePresets.json`, ignorez `CMakeUserPresets.json`.** Le premier est la source de vérité partagée, le second contient les personnalisations locales et les presets générés par Conan.

**Utilisez l'héritage pour factoriser.** Un preset `base` caché avec les paramètres communs (générateur, ccache, options par défaut), des presets enfants pour chaque configuration. Pas de duplication.

**Nommez les presets de manière cohérente.** Adoptez une convention comme `<compilateur>-<type>` (`gcc-debug`, `gcc-release`, `clang-release`) ou `<outil>-<type>` (`conan-debug`, `vcpkg-release`). Les noms apparaissent en CLI et dans les IDE.

**Définissez des build et test presets.** Pas seulement des configure presets. Le workflow complet `configure → build → test` doit être couvert par les presets.

**Utilisez les workflow presets pour la CI.** Un seul `cmake --workflow ci-release` remplace trois commandes et garantit la cohérence.

**Ne mélangez pas les gestionnaires de paquets dans les presets.** Un preset est soit Conan, soit vcpkg, soit sans gestionnaire. Mélanger les toolchains dans un même preset produit des comportements imprévisibles.

**Documentez avec `displayName` et `$comment`.** Les noms courts sont pour la CLI, les display names et commentaires sont pour les humains qui lisent le fichier.

---

## Exemple complet : le fichier de presets de notre projet

Voici le `CMakePresets.json` complet pour le projet exemple, intégrant Conan et supportant Debug/Release avec GCC :

```json
{
    "version": 6,
    "$comment": "Presets pour my_project — Mars 2026",
    "cmakeMinimumRequired": { "major": 3, "minor": 25, "patch": 0 },
    "configurePresets": [
        {
            "name": "dev-debug",
            "displayName": "Dev Debug",
            "$comment": "Nécessite : conan install . -pr=conan/profiles/gcc-debug --build=missing",
            "inherits": "conan-debug",
            "cacheVariables": {
                "MY_PROJECT_BUILD_TESTS": "ON",
                "MY_PROJECT_BUILD_BENCHMARKS": "OFF"
            }
        },
        {
            "name": "dev-release",
            "displayName": "Dev Release",
            "$comment": "Nécessite : conan install . -pr=conan/profiles/gcc-release --build=missing",
            "inherits": "conan-release",
            "cacheVariables": {
                "MY_PROJECT_BUILD_TESTS": "ON",
                "MY_PROJECT_BUILD_BENCHMARKS": "OFF"
            }
        },
        {
            "name": "ci-release",
            "displayName": "CI Release",
            "inherits": "conan-release",
            "cacheVariables": {
                "MY_PROJECT_BUILD_TESTS": "ON",
                "MY_PROJECT_BUILD_BENCHMARKS": "ON"
            }
        }
    ],
    "buildPresets": [
        { "name": "dev-debug", "configurePreset": "dev-debug" },
        { "name": "dev-release", "configurePreset": "dev-release" },
        { "name": "ci-release", "configurePreset": "ci-release" }
    ],
    "testPresets": [
        {
            "name": "dev-debug",
            "configurePreset": "dev-debug",
            "output": { "outputOnFailure": true, "verbosity": "default" }
        },
        {
            "name": "dev-release",
            "configurePreset": "dev-release",
            "output": { "outputOnFailure": true }
        },
        {
            "name": "ci-release",
            "configurePreset": "ci-release",
            "output": { "outputOnFailure": true, "verbosity": "verbose" }
        }
    ],
    "workflowPresets": [
        {
            "name": "ci",
            "displayName": "CI Pipeline complet",
            "steps": [
                { "type": "configure", "name": "ci-release" },
                { "type": "build", "name": "ci-release" },
                { "type": "test", "name": "ci-release" }
            ]
        }
    ]
}
```

Workflow quotidien du développeur :

```bash
# Première fois (ou quand les dépendances changent)
conan install . -pr=conan/profiles/gcc-debug --build=missing  
conan install . -pr=conan/profiles/gcc-release --build=missing  

# Développement itératif
cmake --preset dev-debug  
cmake --build --preset dev-debug  
ctest --preset dev-debug  

# Pipeline CI
cmake --workflow ci
```

---

## Récapitulatif

| Aspect | Détail |
|--------|--------|
| Fichier partagé | `CMakePresets.json` — versionné dans Git |
| Fichier local | `CMakeUserPresets.json` — dans `.gitignore` |
| Types de presets | Configure, Build, Test, Workflow |
| Héritage | `"inherits"` — simple ou multiple, presets `hidden` comme bases |
| Conditions | `"condition"` — restriction par OS ou plateforme |
| Intégration Conan | Hériter des presets `conan-*` générés dans `CMakeUserPresets.json` |
| Intégration vcpkg | `CMAKE_TOOLCHAIN_FILE` avec `$env{VCPKG_ROOT}` dans `cacheVariables` |
| CI/CD | Workflow presets — `cmake --workflow ci` |
| IDE | Support natif dans VS Code, CLion, Visual Studio |
| Version du schéma | 6 (recommandé en 2026, compatible CMake 3.25+) |

---

> **Fin du chapitre 27.** Vous maîtrisez désormais la gestion des dépendances en C++ — du diagnostic du problème (27.1), à l'utilisation de Conan (27.2) et vcpkg (27.3), en passant par le choix du mode de linkage (27.4), la distribution de bibliothèques (27.5), et la standardisation avec les CMake Presets (27.6). Le chapitre 28 couvre Makefile, Ninja et les build systems alternatifs.

⏭️ [Makefile, Ninja et Build Automation](/28-make-ninja/README.md)
