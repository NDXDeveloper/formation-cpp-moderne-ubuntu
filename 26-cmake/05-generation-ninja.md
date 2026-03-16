🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 26.5 Génération pour Ninja : `cmake -G Ninja` (recommandé) ⭐

> **Objectif** : Comprendre pourquoi Ninja est le backend de build recommandé en 2026, comment l'utiliser avec CMake, quels gains concrets il apporte par rapport à Make, et comment en tirer le meilleur parti dans un workflow quotidien et en CI/CD.

---

## Le rôle du backend de build

Rappelons le modèle couvert en introduction du chapitre (section 26) : CMake est un **méta-build system** — il génère les instructions pour un build system sous-jacent qui effectue la compilation réelle. Ce build system est choisi via l'option `-G` (*generator*) :

```bash
cmake -B build -G "Unix Makefiles"   # Génère des Makefiles (défaut historique Linux)  
cmake -B build -G Ninja              # Génère des fichiers build.ninja  
cmake -B build -G "Ninja Multi-Config"  # Ninja multi-configuration  
```

Le choix du backend n'affecte pas le contenu de vos `CMakeLists.txt` — ils restent identiques. Seules les performances du build et quelques comportements secondaires changent.

---

## Pourquoi Ninja plutôt que Make ?

GNU Make est le défaut historique de CMake sur les systèmes Unix. Il fonctionne, il est universel, il est installé partout. Mais il souffre de limitations architecturales qui deviennent palpables sur les projets de taille moyenne et grande.

Ninja a été créé en 2012 par Evan Martin, ingénieur chez Google, pour accélérer le build de Chromium. Sa philosophie est radicale : Ninja ne fait **qu'une seule chose** — exécuter des commandes de build le plus vite possible — et délègue toute la logique de configuration à un outil de plus haut niveau (CMake, Meson, GN). Cette spécialisation lui permet d'être significativement plus rapide que Make sur les aspects qui comptent au quotidien.

### Démarrage plus rapide

Quand vous lancez un build, le build system doit d'abord **charger** la description du projet (quelles cibles, quelles dépendances, quels fichiers ont changé) avant de lancer la moindre compilation. Make parse ses Makefiles à chaque invocation — un processus qui devient coûteux quand le projet génère des centaines de règles avec des dépendances complexes.

Ninja utilise un format binaire optimisé (`build.ninja`) conçu pour un parsing quasi instantané. Sur un projet de taille moyenne (~200 fichiers sources), la différence de démarrage est de l'ordre de quelques centaines de millisecondes. Sur un grand projet (~5000+ fichiers), elle peut atteindre plusieurs secondes — un écart qui s'accumule au fil des dizaines de rebuilds quotidiens.

### No-op builds quasi instantanés

Un *no-op build* est un build lancé alors que rien n'a changé — le build system vérifie que tout est à jour et sort immédiatement. C'est le scénario le plus fréquent pendant le développement : vous compilez, lancez un test, recompilez sans avoir modifié de fichier.

Ninja est optimisé pour ce cas précis. Sa vérification des timestamps est extrêmement efficace, avec des no-op builds typiquement sous les 100 millisecondes, même sur de grands projets. Make est sensiblement plus lent dans ce scénario, avec des no-op qui peuvent prendre une à plusieurs secondes sur les mêmes projets.

### Parallélisation optimale par défaut

Make nécessite l'option `-j` pour activer la parallélisation, et il faut spécifier le nombre de jobs :

```bash
make -j$(nproc)   # Parallélisation manuelle
```

Ninja détecte automatiquement le nombre de cœurs CPU et parallélise en conséquence, sans intervention :

```bash
ninja   # Parallélisation automatique optimale
```

De plus, Ninja gère le pool de jobs de manière plus fine que Make. Il surveille la charge système et ajuste dynamiquement le nombre de compilations concurrentes, ce qui produit une utilisation CPU plus régulière et des builds globalement plus rapides.

### Sortie console propre

Make affiche chaque commande exécutée par défaut, ce qui produit une sortie verbeuse et difficile à lire. Ninja affiche une barre de progression compacte avec le nombre de tâches restantes :

```
[42/187] Building CXX object src/CMakeFiles/my_project_core.dir/core.cpp.o
```

En cas d'erreur, Ninja arrête proprement et affiche uniquement la sortie du compilateur pertinente, sans la noyer dans les logs des compilations parallèles réussies.

### Génération de `compile_commands.json`

Les générateurs Ninja et Makefile supportent la génération du fichier `compile_commands.json` dans le répertoire de build. Ce fichier contient la ligne de compilation exacte de chaque fichier source, et il est utilisé par les outils d'analyse et les IDE :

- **clangd** (serveur de langage) : autocomplétion, navigation, diagnostics en temps réel
- **clang-tidy** : analyse statique (section 32.1)
- **VS Code** (extension C/C++) : IntelliSense basé sur les flags réels
- **CLion** : indexation du projet

Pour l'activer, ajoutez dans votre `CMakeLists.txt` racine ou en ligne de commande :

```cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

```bash
# Ou en ligne de commande
cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Vous pouvez aussi positionner la variable d'environnement `CMAKE_EXPORT_COMPILE_COMMANDS=ON` dans votre shell pour que tous vos projets en bénéficient. C'est une bonne pratique à combiner avec un CMake Preset (section 27.6) pour que tous les contributeurs du projet aient cette option activée.

---

## Installation

Ninja est disponible dans les dépôts de toutes les distributions Linux majeures :

```bash
# Ubuntu / Debian
sudo apt install ninja-build

# Vérification
ninja --version
# 1.11.1 ou supérieur
```

C'est la seule étape nécessaire. Ninja n'a aucune dépendance et pèse quelques centaines de Ko.

---

## Utilisation avec CMake

### Configuration et build

Le workflow est identique à celui avec Make, seule l'option `-G` change :

```bash
# Configuration (une seule fois, ou quand le CMakeLists.txt change)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Build avec cible spécifique
cmake --build build --target my_project_app

# Build parallèle limité (si nécessaire)
cmake --build build -j 4
```

La commande `cmake --build build` est un wrapper portable qui appelle Ninja en coulisse. Vous pouvez aussi invoquer Ninja directement :

```bash
cd build  
ninja              # Build complet  
ninja my_project_app   # Cible spécifique  
ninja -j4          # Limiter le parallélisme  
```

### Nettoyage

```bash
# Via CMake
cmake --build build --target clean

# Via Ninja directement
ninja -C build clean
```

Ou simplement supprimer le répertoire de build — c'est l'approche recommandée pour un nettoyage total :

```bash
rm -rf build
```

### Lancer les tests

```bash
# Via CTest (intégré à CMake)
cd build && ctest --output-on-failure

# Ou depuis la racine du projet
cmake --build build --target test
```

---

## Ninja Multi-Config

Le générateur standard `Ninja` est *single-config* : un répertoire de build correspond à un seul type de build (Debug, Release, RelWithDebInfo, MinSizeRel). Pour basculer entre Debug et Release, il faut maintenir deux répertoires de build séparés :

```bash
cmake -B build-debug   -G Ninja -DCMAKE_BUILD_TYPE=Debug  
cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release  
```

Le générateur `Ninja Multi-Config`, introduit dans CMake 3.17, permet de gérer plusieurs configurations dans un **seul** répertoire de build :

```bash
# Configuration unique — pas de CMAKE_BUILD_TYPE
cmake -B build -G "Ninja Multi-Config"

# Build en Debug
cmake --build build --config Debug

# Build en Release
cmake --build build --config Release
```

Les fichiers objets de chaque configuration sont placés dans des sous-répertoires séparés (`build/Debug/`, `build/Release/`), sans interférence.

### Quand utiliser Multi-Config ?

Le Multi-Config est pratique quand vous basculez fréquemment entre Debug et Release sans vouloir maintenir deux arbres de build. Il est aussi utile en CI/CD pour tester les deux configurations à partir d'une seule étape de configuration.

Pour le développement quotidien, le single-config avec un répertoire `build/` en Debug est souvent suffisant. La plupart des développeurs ne recompilent en Release que pour les benchmarks ou les releases.

---

## Rendre Ninja le défaut

Si vous voulez que CMake utilise Ninja par défaut sans spécifier `-G Ninja` à chaque fois, plusieurs options s'offrent à vous.

### Variable d'environnement

```bash
# Dans ~/.bashrc ou ~/.zshrc
export CMAKE_GENERATOR=Ninja
```

Après cette configuration, un simple `cmake -B build` utilisera Ninja automatiquement.

### CMake Presets (recommandé)

Les CMake Presets (couverts en section 27.6) permettent de standardiser la configuration pour tous les contributeurs du projet :

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "default",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_CXX_COMPILER_LAUNCHER": "ccache"
            }
        },
        {
            "name": "release",
            "inherits": "default",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            }
        }
    ]
}
```

```bash
# Utilisation — Ninja est configuré dans le preset
cmake --preset default  
cmake --build build  
```

L'avantage des presets est que la configuration est versionnée avec le projet. Chaque contributeur obtient le même setup sans avoir à configurer de variable d'environnement.

---

## Ninja et ccache : la combinaison optimale

La combinaison de Ninja (build rapide) et ccache (cache de compilation, section 2.3) produit les meilleurs temps de build possibles :

```bash
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

Avec cette configuration :

- Ninja gère la parallélisation et la détection de changements de manière optimale ;
- ccache court-circuite la compilation des fichiers qui n'ont pas changé, même après un `rm -rf build` (le cache survit au nettoyage du répertoire de build).

Le résultat en pratique : un no-op build en quelques dizaines de millisecondes, un rebuild complet après nettoyage en quelques secondes (cache hit ccache), et un build initial en temps minimal grâce à la parallélisation agressive de Ninja.

---

## Différences comportementales avec Make

Au-delà de la performance, quelques différences comportementales méritent d'être connues.

### Pas de mode verbose par défaut

Ninja affiche une barre de progression, pas les commandes complètes. Pour voir les commandes exactes (utile pour le débogage de flags) :

```bash
# Via CMake
cmake --build build -- -v

# Via Ninja directement
ninja -C build -v
```

### Pas de build partiel par répertoire

Avec Make, on peut lancer `make` dans un sous-répertoire pour ne compiler que les cibles de ce répertoire. Ninja n'a pas ce concept — le fichier `build.ninja` est global et le build se lance toujours depuis la racine :

```bash
# ❌ Ne fonctionne pas avec Ninja
cd build/src && ninja

# ✅ Correct — spécifier la cible
ninja -C build my_project_core
```

Ce n'est pas une limitation en pratique : spécifier la cible est plus précis que compiler par répertoire.

### Détection automatique des changements de build system

Si vous modifiez un `CMakeLists.txt`, Ninja détecte automatiquement que la configuration doit être régénérée et relance CMake avant de compiler. Ce comportement existe aussi avec Make, mais Ninja le fait de manière plus fiable et plus rapide.

```bash
# Vous modifiez src/CMakeLists.txt, puis :
cmake --build build
# Ninja détecte le changement, relance cmake, puis compile
# Tout est transparent
```

---

## Ninja en CI/CD

Ninja est particulièrement adapté aux pipelines CI/CD où chaque seconde compte.

### GitHub Actions

```yaml
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: sudo apt-get install -y ninja-build ccache

      - name: Configure
        run: cmake -B build -G Ninja
               -DCMAKE_BUILD_TYPE=Release
               -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

      - name: Build
        run: cmake --build build

      - name: Test
        run: ctest --test-dir build --output-on-failure
```

### GitLab CI

```yaml
build:
  image: ubuntu:24.04
  before_script:
    - apt-get update && apt-get install -y g++ cmake ninja-build ccache
  script:
    - cmake -B build -G Ninja
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
    - cmake --build build
    - ctest --test-dir build --output-on-failure
  cache:
    paths:
      - .ccache/
```

L'association Ninja + ccache + cache CI réduit considérablement les temps de build dans les pipelines. Les sections 38.1 à 38.3 couvriront en détail l'optimisation des pipelines CI/CD pour les projets C++.

---

## Cas où Make reste pertinent

Malgré les avantages de Ninja, quelques situations rendent Make préférable ou nécessaire :

**Environnements embarqués très contraints** où Ninja n'est pas disponible et ne peut pas être installé. Make est quasiment toujours présent sur les systèmes Unix.

**Scripts Makefile existants** qui ne passent pas par CMake. Si votre workflow repose sur des Makefiles écrits à la main avec des cibles personnalisées complexes (deploy, package, etc.), migrer vers Ninja nécessite de réécrire ces cibles. Notez que ce cas ne concerne pas les projets CMake — CMake génère ses propres Makefiles, indépendants de tout Makefile artisanal.

**Compatibilité avec des outils legacy** qui supposent la présence de Makefiles dans le répertoire de build. Certains anciens IDE ou systèmes de build lisent directement les Makefiles générés par CMake.

Dans tous les autres cas — développement local, CI/CD, projets nouveaux — Ninja est le choix recommandé en 2026.

---

## Récapitulatif

| Aspect | Make | Ninja |
|--------|------|-------|
| Démarrage | Lent sur grands projets | Quasi instantané |
| No-op build | Secondes | Dizaines de millisecondes |
| Parallélisation | Manuelle (`-j`) | Automatique |
| `compile_commands.json` | Supporté (`CMAKE_EXPORT_COMPILE_COMMANDS`) | Supporté (`CMAKE_EXPORT_COMPILE_COMMANDS`) |
| Sortie console | Verbose par défaut | Barre de progression compacte |
| Multi-config | Non | Oui (`Ninja Multi-Config`) |
| Disponibilité | Partout | `apt install ninja-build` |
| Reconfiguration auto | Oui | Oui (plus rapide) |

**Recommandation** : utilisez `cmake -G Ninja` pour tous vos projets CMake. Configurez `CMAKE_GENERATOR=Ninja` dans votre shell ou dans un CMake Preset pour ne plus y penser.

---

> **À suivre** : La section 26.6 couvre les toolchains et la cross-compilation — comment compiler pour ARM ou RISC-V depuis une machine x86_64 en utilisant les fichiers toolchain CMake.

⏭️ [Toolchains et cross-compilation](/26-cmake/06-toolchains-cross-compilation.md)
