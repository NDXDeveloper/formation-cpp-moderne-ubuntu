🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 27.3 vcpkg : Alternative Microsoft

> **Objectif** : Comprendre le fonctionnement de vcpkg — son installation, le mode manifest, le système de triplets, l'intégration avec CMake, et le cache binaire — pour être capable de l'utiliser comme alternative ou complément à Conan dans un projet C++ professionnel.

---

## Philosophie de vcpkg

vcpkg est un gestionnaire de paquets C/C++ développé par Microsoft, open source et multiplateforme (Linux, Windows, macOS). Sa philosophie est la **simplicité d'usage** : un fichier JSON pour déclarer les dépendances, un toolchain CMake pour les consommer, et une intégration automatique lors du build.

Là où Conan expose la matrice de configurations complète (settings, options, profils Python), vcpkg fait des choix par défaut pragmatiques et cache la complexité derrière le concept de **triplets**. Le résultat est une courbe d'apprentissage plus douce, au prix d'une flexibilité moindre dans les scénarios avancés.

En mars 2026, le registre vcpkg contient environ 2750 ports (recettes de bibliothèques), validés sur 15 triplets principaux à chaque mise à jour.

---

## Installation

vcpkg se distribue comme un dépôt Git que vous clonez et bootstrappez. Contrairement à Conan, il ne nécessite pas Python.

```bash
# Cloner le dépôt vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/.vcpkg

# Bootstrapper — compile l'outil vcpkg
cd ~/.vcpkg
./bootstrap-vcpkg.sh

# Vérifier l'installation
./vcpkg --version
```

Pour rendre `vcpkg` disponible globalement, ajoutez-le à votre `PATH` et définissez la variable `VCPKG_ROOT` :

```bash
# ~/.bashrc
export VCPKG_ROOT="$HOME/.vcpkg"  
export PATH="$VCPKG_ROOT:$PATH"  
```

Après un `source ~/.bashrc` :

```bash
vcpkg --version
# vcpkg package management program version 2026-01-16-...
```

### Mise à jour

vcpkg est un dépôt Git — la mise à jour se fait par `git pull` suivi d'un re-bootstrap :

```bash
cd $VCPKG_ROOT  
git pull  
./bootstrap-vcpkg.sh
```

Cela met à jour à la fois l'outil vcpkg et le catalogue de ports (les recettes de bibliothèques).

---

## Mode manifest : déclaration des dépendances

Le **mode manifest** est le mode recommandé depuis vcpkg 2021, et le seul que nous couvrons ici (le « mode classic » global est déprécié pour les projets). Vous déclarez vos dépendances dans un fichier `vcpkg.json` à la racine du projet :

```json
{
    "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
    "name": "my-project",
    "version": "1.2.0",
    "dependencies": [
        "openssl",
        "spdlog",
        "zlib",
        {
            "name": "gtest",
            "host": false
        }
    ],
    "builtin-baseline": "c82f74667287d3dc386bce81e44964370d91c4a9"
}
```

### Éléments clés

**`dependencies`** : la liste des bibliothèques nécessaires, par nom de port. vcpkg résout automatiquement les dépendances transitives.

**`builtin-baseline`** : un hash de commit du dépôt vcpkg qui fixe les **versions par défaut** de tous les ports. C'est le mécanisme de reproductibilité de vcpkg — deux développeurs avec le même baseline obtiendront exactement les mêmes versions. Pour obtenir un baseline récent :

```bash
cd $VCPKG_ROOT  
git log -1 --format="%H"  
# Copier ce hash dans builtin-baseline
```

**`version`** : la version de votre projet (informatif, non utilisé par vcpkg pour la résolution).

### Contraintes de version

Par défaut, vcpkg utilise la version définie par le baseline pour chaque port. Vous pouvez spécifier des contraintes minimales ou des overrides :

```json
{
    "dependencies": [
        {
            "name": "fmt",
            "version>=": "11.0.0"
        },
        "zlib"
    ],
    "builtin-baseline": "c82f74667287d3dc386bce81e44964370d91c4a9",
    "overrides": [
        {
            "name": "zlib",
            "version": "1.3.1"
        }
    ]
}
```

`version>=` exprime une contrainte minimale — vcpkg choisira la version du baseline si elle satisfait la contrainte, sinon il échouera. `overrides` force une version exacte, en ignorant le baseline et toutes les contraintes.

### Features (composants optionnels)

Certains ports exposent des features optionnelles. Vous pouvez les activer dans le manifest :

```json
{
    "dependencies": [
        {
            "name": "curl",
            "features": ["ssl", "http2"]
        },
        {
            "name": "boost",
            "features": ["filesystem", "system"]
        }
    ]
}
```

---

## Triplets : la matrice simplifiée

vcpkg encapsule la matrice de configurations (OS, architecture, mode de linkage) dans des **triplets**. Un triplet est un fichier qui décrit une cible de compilation.

### Triplets intégrés courants

| Triplet | OS | Arch | Linkage |
|---------|-----|------|---------|
| `x64-linux` | Linux | x86_64 | Dynamique |
| `x64-linux-static` | Linux | x86_64 | **Statique** (rpath fix) |
| `x64-linux-release` | Linux | x86_64 | Dynamique, Release uniquement |
| `arm64-linux` | Linux | AArch64 | Dynamique |
| `x64-windows` | Windows | x86_64 | Dynamique |
| `x64-windows-static` | Windows | x86_64 | Statique |
| `x64-osx` | macOS | x86_64 | Dynamique |
| `arm64-osx` | macOS | ARM64 (Apple Silicon) | Dynamique |

Le triplet par défaut sur Linux est `x64-linux`. Vous pouvez le changer via la variable d'environnement ou la variable CMake `VCPKG_TARGET_TRIPLET` :

```bash
# Via variable d'environnement
export VCPKG_DEFAULT_TRIPLET=x64-linux-static

# Via CMake
cmake -B build -DVCPKG_TARGET_TRIPLET=x64-linux-static ...
```

### Triplets custom

Pour des besoins spécifiques (cross-compilation avec un compilateur particulier, flags custom), vous pouvez créer des triplets personnalisés :

```cmake
# triplets/arm64-linux-custom.cmake
set(VCPKG_TARGET_ARCHITECTURE arm64)  
set(VCPKG_CRT_LINKAGE dynamic)  
set(VCPKG_LIBRARY_LINKAGE static)  
set(VCPKG_CMAKE_SYSTEM_NAME Linux)  

# Cross-compilateur
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE
    ${CMAKE_CURRENT_LIST_DIR}/../cmake/toolchains/aarch64-linux-gnu.cmake)
```

Le répertoire de triplets custom se déclare via `VCPKG_OVERLAY_TRIPLETS` :

```bash
cmake -B build -DVCPKG_OVERLAY_TRIPLETS=triplets ...
```

### Triplets vs profils Conan

Le triplet vcpkg est conceptuellement analogue au profil Conan, mais plus restreint. Un triplet ne couvre que l'architecture, l'OS et le mode de linkage. La version du compilateur, le standard C++, et les options spécifiques aux paquets ne font pas partie du triplet — ils sont gérés respectivement par le système, les variables CMake, et les features du manifest.

Cette simplification rend vcpkg plus facile à prendre en main, mais limite le contrôle dans les scénarios où la compatibilité ABI est critique (mélange de compilateurs, builds multi-standard).

---

## Intégration CMake

L'intégration de vcpkg avec CMake repose sur un **fichier toolchain** fourni par vcpkg. Ce toolchain intercepte les appels `find_package()` et les redirige vers les bibliothèques installées par vcpkg.

### Configuration de base

```bash
cmake -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release
```

C'est la seule ligne nécessaire. Le toolchain vcpkg fait le reste :

1. Il détecte le fichier `vcpkg.json` dans votre projet.
2. Il installe automatiquement les dépendances lors de la configuration CMake (pas besoin d'un `vcpkg install` séparé).
3. Il configure `CMAKE_PREFIX_PATH` pour que `find_package()` trouve les bibliothèques installées.

Vos `CMakeLists.txt` restent identiques — aucune mention de vcpkg :

```cmake
find_package(OpenSSL REQUIRED)  
find_package(spdlog CONFIG REQUIRED)  
find_package(ZLIB REQUIRED)  

target_link_libraries(my_project_core
    PRIVATE OpenSSL::SSL spdlog::spdlog ZLIB::ZLIB
)
```

### Intégration via CMake Presets

La méthode recommandée pour éviter de taper le chemin du toolchain à chaque fois est d'utiliser un CMake Preset :

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "vcpkg-release",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build",
            "cacheVariables": {
                "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
                "CMAKE_BUILD_TYPE": "Release"
            }
        },
        {
            "name": "vcpkg-debug",
            "inherits": "vcpkg-release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            }
        },
        {
            "name": "vcpkg-static",
            "inherits": "vcpkg-release",
            "cacheVariables": {
                "VCPKG_TARGET_TRIPLET": "x64-linux-static"
            }
        }
    ]
}
```

```bash
cmake --preset vcpkg-release  
cmake --build build  
```

### Installation automatique vs manuelle

En mode manifest avec le toolchain CMake, vcpkg installe les dépendances **automatiquement** lors de la configuration CMake. Vous n'avez pas besoin de lancer `vcpkg install` séparément — c'est une différence notable avec Conan, qui nécessite toujours un `conan install` avant `cmake`.

Cette installation automatique peut être désactivée si vous préférez un contrôle explicite :

```bash
# Installation manuelle (sans le toolchain automatique)
vcpkg install --triplet x64-linux

# Puis configuration CMake avec le prefix path
cmake -B build -G Ninja \
    -DCMAKE_PREFIX_PATH=$VCPKG_ROOT/installed/x64-linux \
    -DCMAKE_BUILD_TYPE=Release
```

Mais dans la grande majorité des cas, l'installation automatique via le toolchain est la méthode recommandée.

---

## Cache binaire

vcpkg supporte le **cache binaire** pour éviter de recompiler les dépendances à chaque build propre. Par défaut, un cache local est utilisé. En CI/CD, vous pouvez configurer un cache distant.

### Cache local (par défaut)

vcpkg cache automatiquement les binaires dans `~/.cache/vcpkg/archives/` (Linux). Les installations suivantes avec le même triplet et le même baseline réutilisent les binaires en cache.

### Cache distant pour la CI

```bash
# Cache sur un partage NFS ou un bucket S3
export VCPKG_BINARY_SOURCES="clear;files,/shared/vcpkg-cache,readwrite"

# Cache GitHub Actions (via le cache action)
export VCPKG_BINARY_SOURCES="clear;x-gha,readwrite"
```

Le cache binaire est particulièrement précieux en CI/CD : les pipelines ne recompilent les dépendances que lorsque le baseline ou le triplet change, ce qui réduit les temps de build de plusieurs minutes à quelques secondes.

---

## Configuration avancée : `vcpkg-configuration.json`

Le fichier `vcpkg-configuration.json` (à côté de `vcpkg.json`) permet de configurer des registres personnalisés et des overlay ports :

```json
{
    "default-registry": {
        "kind": "git",
        "baseline": "c82f74667287d3dc386bce81e44964370d91c4a9",
        "repository": "https://github.com/microsoft/vcpkg"
    },
    "registries": [
        {
            "kind": "git",
            "repository": "https://github.com/my-company/vcpkg-registry",
            "baseline": "a1b2c3d4...",
            "packages": ["internal-lib", "custom-codec"]
        }
    ]
}
```

Les registres personnalisés permettent aux organisations de distribuer des bibliothèques internes via le même mécanisme que les ports publics. Les overlay ports permettent de surcharger un port du registre principal avec une version modifiée, sans forker le dépôt vcpkg.

---

## Workflow complet

Voici le workflow vcpkg de bout en bout pour notre projet exemple :

### Arborescence

```
my_project/
├── CMakeLists.txt           # Inchangé
├── CMakePresets.json         # Toolchain vcpkg
├── vcpkg.json               # Dépendances vcpkg
├── vcpkg-configuration.json  # Configuration registres (optionnel)
├── src/
├── apps/
├── tests/
└── ...
```

### `vcpkg.json`

```json
{
    "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
    "name": "my-project",
    "version": "1.2.0",
    "dependencies": [
        "openssl",
        "spdlog",
        "zlib",
        "gtest"
    ],
    "builtin-baseline": "c82f74667287d3dc386bce81e44964370d91c4a9"
}
```

### Build

```bash
# Configurer (vcpkg installe les dépendances automatiquement)
cmake --preset vcpkg-release

# Compiler
cmake --build build

# Tester
ctest --test-dir build --output-on-failure
```

Trois commandes. Pas de `vcpkg install` séparé. Pas de toolchain Conan à générer. Les dépendances sont installées lors de la configuration CMake.

---

## vcpkg vs Conan : guide de décision

Après avoir couvert les deux outils en détail, voici un guide de décision pratique :

| Critère | Choisir **Conan** | Choisir **vcpkg** |
|---------|:-----------------:|:-----------------:|
| Contrôle ABI fin (version compilateur, standard) | ✅ | ⚠️ Limité |
| Cross-compilation complexe | ✅ Profils host/build | ⚠️ Triplets custom |
| Simplicité d'installation et d'usage | Modérée | ✅ |
| Intégration CMake la plus transparente | Bonne (preset Conan) | ✅ (auto-install) |
| Serveur de paquets privé (entreprise) | ✅ Artifactory natif | ✅ Registres Git |
| Projet mixte Windows + Linux | ✅ | ✅ |
| Équipe utilisant principalement Visual Studio | Possible | ✅ Intégration native |
| Pas de Python sur les machines de build | ❌ Python requis | ✅ Pas de Python |
| Recettes personnalisées complexes | ✅ Python | ⚠️ CMake portfiles |

Pour la plupart des projets Linux-centric couverts par cette formation, **les deux outils fonctionnent bien**. Si votre équipe n'a pas de préférence établie, Conan offre un contrôle plus fin sur la matrice de configurations, tandis que vcpkg offre une expérience plus directe avec moins de concepts à assimiler.

Le plus important est de **choisir un outil et de s'y tenir** dans un projet donné. Mélanger Conan et vcpkg dans le même projet est techniquement possible mais crée une complexité inutile.

---

## Récapitulatif

| Aspect | Commande / fichier |
|--------|-------------------|
| Installation | `git clone` + `./bootstrap-vcpkg.sh` |
| Déclaration des dépendances | `vcpkg.json` (mode manifest) |
| Versioning | `builtin-baseline` + `overrides` |
| Configuration cible | Triplets (`x64-linux`, `x64-linux-static`, custom) |
| Intégration CMake | `-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake` |
| Installation des dépendances | Automatique lors de `cmake -B build` avec le toolchain |
| Cache binaire | `VCPKG_BINARY_SOURCES` (local par défaut, distant configurable) |
| Registres privés | `vcpkg-configuration.json` |
| Mise à jour | `cd $VCPKG_ROOT && git pull && ./bootstrap-vcpkg.sh` |

---

> **À suivre** : La section 27.4 couvre le linkage statique versus dynamique — les implications pratiques du choix entre `.a` et `.so`, et les recommandations selon le contexte de déploiement.

⏭️ [Linkage statique (.a) vs dynamique (.so)](/27-gestion-dependances/04-linkage-statique-dynamique.md)
