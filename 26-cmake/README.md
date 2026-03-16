🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 26. CMake : Le Standard de l'Industrie 🔥

> **Niveau** : Avancé  
> **Partie** : IV — Tooling et Build Systems  
> **Module** : 9 — Build Systems et Gestion de Projet  
> **Durée estimée** : 6–8 heures  
> **Prérequis** : Module 2 (Fondamentaux du langage), Section 2.2 (Outils essentiels), familiarité avec la compilation en ligne de commande (g++, clang++)

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez capable de :

- comprendre le rôle de CMake dans l'écosystème C++ moderne et pourquoi il s'est imposé comme standard *de facto* ;
- structurer un projet CMake propre, modulaire et maintenable ;
- déclarer des cibles (`targets`) avec leurs propriétés (sources, headers, dépendances, flags) selon l'approche *modern CMake* ;
- gérer des dépendances externes via `find_package` et `FetchContent` ;
- générer des fichiers de configuration et de version automatiquement ;
- utiliser Ninja comme backend de génération pour accélérer vos builds ;
- configurer des toolchains pour la cross-compilation (ARM, RISC-V) ;
- appliquer les meilleures pratiques de CMake 3.31+ dans un contexte professionnel 2026.

---

## Pourquoi CMake ?

Tout projet C++ sérieux dépasse rapidement le stade où une commande `g++ main.cpp -o app` suffit. Dès que le code se répartit en plusieurs fichiers sources, que des bibliothèques tierces entrent en jeu, ou que le projet doit compiler sur plusieurs plateformes, la gestion manuelle de la compilation devient un cauchemar de maintenance. C'est exactement le problème que les *build systems* résolvent — et dans cet espace, CMake occupe une position dominante.

### Un méta-build system, pas un build system

Une distinction fondamentale s'impose d'emblée : CMake n'est **pas** un build system au sens strict. C'est un **méta-build system** — un générateur de projets. Concrètement, CMake lit vos fichiers `CMakeLists.txt`, interprète la description de votre projet (sources, cibles, dépendances, options), puis génère les fichiers nécessaires pour un *vrai* build system sous-jacent :

- des **Makefiles** pour GNU Make (le défaut historique sur Linux) ;
- des fichiers **build.ninja** pour Ninja (recommandé en 2026 — voir section 26.5) ;
- des projets **Visual Studio** (.sln/.vcxproj) sur Windows ;
- des projets **Xcode** sur macOS.

Cette couche d'abstraction est la clé de la portabilité de CMake. Vous décrivez votre projet **une seule fois**, et CMake se charge de produire les instructions de build adaptées à chaque plateforme et chaque outil.

```text
┌──────────────────────┐
│   CMakeLists.txt     │  ← Vous écrivez ceci (description déclarative)
└──────────┬───────────┘
           │  cmake -G "..."
           ▼
┌──────────────────────┐
│  Fichiers de build   │  ← CMake génère ceci
│  (Makefile, .ninja,  │
│   .sln, .xcodeproj)  │
└──────────┬───────────┘
           │  make / ninja / msbuild
           ▼
┌──────────────────────┐
│  Binaires finaux     │  ← Le build system compile ceci
│  (exécutables, .so,  │
│   .a, tests, ...)    │
└──────────────────────┘
```

### L'adoption massive par l'industrie

CMake est aujourd'hui utilisé par la très grande majorité des projets C++ open source et professionnels. Quelques exemples emblématiques : LLVM/Clang, Qt, OpenCV, Boost (migration en cours), KDE, TensorFlow (composants C++), gRPC, ou encore le noyau Zephyr pour l'embarqué. Les gestionnaires de dépendances C++ majeurs — Conan et vcpkg — sont conçus pour s'intégrer nativement avec CMake. Les IDE modernes (CLion, VS Code avec l'extension CMake Tools, Visual Studio) offrent un support de première classe pour les projets CMake.

Cette adoption massive crée un cercle vertueux : parce que tout le monde utilise CMake, les bibliothèques publient des fichiers de configuration CMake, ce qui rend CMake encore plus pratique, ce qui renforce son adoption. En 2026, ne pas maîtriser CMake pour un développeur C++ professionnel n'est tout simplement plus une option.

### Les alternatives et leur positionnement

CMake n'est pas le seul outil dans ce domaine. Meson (couvert en section 28.4) est un build system montant, apprécié pour sa syntaxe plus claire et ses performances. Bazel (Google) est populaire dans les très grands monorepos. Buck2 (Meta) cible des cas similaires. Cependant, aucune de ces alternatives n'a atteint la masse critique de CMake en termes d'écosystème, de support IDE et de compatibilité avec les bibliothèques existantes.

> 💡 **Note pratique** : même si vous adoptez Meson ou Bazel pour vos propres projets, vous devrez interagir avec CMake pour intégrer la plupart des dépendances C++ tierces. La maîtrise de CMake reste incontournable.

---

## Modern CMake : un changement de paradigme

Si vous avez déjà rencontré du CMake « ancien style » (pré-3.0), vous avez peut-être vu des fichiers truffés de `include_directories()`, `link_directories()`, `add_definitions()` et de variables globales manipulées dans tous les sens. Ce style est **obsolète** et ne doit plus être utilisé.

Le **Modern CMake** (à partir de CMake 3.0, et affiné à chaque version depuis) repose sur un principe central : **tout est une cible** (*target*). Une cible, c'est un exécutable, une bibliothèque statique, une bibliothèque partagée, ou même une bibliothèque d'interface (*header-only*). Chaque cible possède des **propriétés** (include directories, flags de compilation, dépendances) qui se propagent automatiquement aux cibles qui en dépendent, grâce au système de visibilité `PUBLIC` / `PRIVATE` / `INTERFACE`.

Comparons les deux approches :

```cmake
# ❌ Ancien style (NE PAS reproduire)
include_directories(${PROJECT_SOURCE_DIR}/include)  
include_directories(${SOME_LIB_INCLUDE_DIR})  
add_definitions(-DUSE_FEATURE_X)  
link_directories(${SOME_LIB_DIR})  
add_executable(app main.cpp util.cpp)  
target_link_libraries(app some_lib)  
```

```cmake
# ✅ Modern CMake (à suivre)
add_executable(app main.cpp util.cpp)  
target_include_directories(app PRIVATE ${PROJECT_SOURCE_DIR}/include)  
target_compile_definitions(app PRIVATE USE_FEATURE_X)  
target_link_libraries(app PRIVATE some_lib)  
```

La différence peut sembler cosmétique, mais elle est structurelle. Dans l'ancien style, les directives `include_directories()` et `add_definitions()` polluent la portée globale : *toutes* les cibles du répertoire courant en héritent, qu'elles en aient besoin ou non. Dans le style moderne, chaque propriété est attachée à une cible précise, avec un niveau de visibilité explicite. Le résultat : des projets plus modulaires, des dépendances clairement tracées, et des erreurs de configuration qui apparaissent plus tôt.

---

## Le workflow CMake en pratique

Un cycle de travail CMake typique se décompose en trois étapes distinctes : **configuration**, **génération** et **build**.

### 1. Configuration et génération

```bash
# Depuis la racine du projet
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

Cette commande fait deux choses. D'abord, CMake **configure** le projet : il lit le `CMakeLists.txt` racine, résout les dépendances, détecte le compilateur, évalue les options, et vérifie que tout est cohérent. Ensuite, il **génère** les fichiers de build dans le répertoire `build/`. L'option `-G Ninja` indique que le backend sera Ninja (plutôt que Make). L'option `-DCMAKE_BUILD_TYPE=Release` définit le type de build.

### 2. Compilation (build)

```bash
cmake --build build
```

Cette commande invoque le build system sous-jacent (Ninja ici) pour effectuer la compilation proprement dite. Elle est équivalente à `ninja -C build`, mais l'avantage de passer par `cmake --build` est que la commande reste identique quel que soit le backend choisi.

### 3. Installation (optionnel)

```bash
cmake --install build --prefix /usr/local
```

Si le projet définit des règles d'installation, cette commande copie les binaires, bibliothèques et headers aux emplacements appropriés.

> 💡 **Bonne pratique** : utilisez toujours un répertoire de build séparé (*out-of-source build*). Ne lancez jamais `cmake .` à la racine de votre projet — cela mélange les fichiers générés avec vos sources et rend le nettoyage pénible. La syntaxe `cmake -B build` crée automatiquement le répertoire `build/` et y place tous les fichiers générés.

---

## Version minimale requise

Tout `CMakeLists.txt` commence par spécifier la version minimale de CMake requise :

```cmake
cmake_minimum_required(VERSION 3.25)
```

Ce n'est pas une formalité. Cette déclaration active les *policies* correspondant à la version demandée, ce qui affecte le comportement de nombreuses fonctionnalités CMake. Spécifier une version trop ancienne peut désactiver des comportements modernes souhaitables ; spécifier une version trop récente peut exclure des utilisateurs dont le système fournit une version plus ancienne.

**Recommandation 2026** : pour un nouveau projet, `VERSION 3.25` est un minimum raisonnable. Cette version est disponible nativement sur Ubuntu 24.04 LTS et offre toutes les fonctionnalités *modern CMake* essentielles, y compris les presets, `FetchContent` mature, et les policies modernes. Si vous ciblez exclusivement des environnements récents (CI maison, containers Docker), vous pouvez monter jusqu'à `VERSION 3.31` pour bénéficier des dernières améliorations (voir section 26.7).

---

## Plan du chapitre

Ce chapitre est organisé en sept sections progressives :

| Section | Thème | Ce que vous apprendrez |
|---------|-------|----------------------|
| **26.1** | Structure d'un projet CMake moderne | Organisation des répertoires, `CMakeLists.txt` racine et sous-répertoires |
| **26.2** | Écrire un `CMakeLists.txt` | Cibles, bibliothèques, exécutables, visibilité `PUBLIC`/`PRIVATE`/`INTERFACE` |
| **26.3** | Gestion des dépendances | `find_package`, `FetchContent`, `add_subdirectory` |
| **26.4** | Configuration et génération de fichiers | `configure_file`, fichiers de version, headers générés |
| **26.5** | Génération pour Ninja ⭐ | Pourquoi Ninja, comment l'utiliser, gains de performance |
| **26.6** | Toolchains et cross-compilation | Fichiers toolchain, compilation ARM/RISC-V depuis x86_64 |
| **26.7** | CMake 3.31+ : Nouveautés 2026 ⭐ | Dernières fonctionnalités et meilleures pratiques actuelles |

Le chapitre suit un fil conducteur pratique : chaque section enrichit progressivement un projet exemple, de sorte que les concepts s'accumulent de manière cohérente. La section 26.1 pose les fondations avec la structure du projet ; la section 26.2 remplit les `CMakeLists.txt` ; la section 26.3 introduit les dépendances externes ; et ainsi de suite.

---

## Prérequis techniques

Avant de commencer, assurez-vous que les outils suivants sont installés sur votre système Ubuntu :

```bash
# CMake (version 3.25+)
sudo apt update && sudo apt install cmake

# Ninja (build system recommandé)
sudo apt install ninja-build

# Vérification des versions
cmake --version    # doit afficher 3.25 ou supérieur  
ninja --version    # doit afficher 1.11 ou supérieur  
```

Si votre version d'Ubuntu fournit un CMake trop ancien, vous pouvez installer une version récente via le dépôt officiel Kitware :

```bash
# Ajout du dépôt Kitware (si CMake système est trop ancien)
sudo apt install gpg wget  
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \  
    gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" | \
    sudo tee /etc/apt/sources.list.d/kitware.list
sudo apt update && sudo apt install cmake
```

Vous aurez également besoin d'un compilateur C++ (GCC 15 ou Clang 20, couverts en section 2.1) et des outils de base (`build-essential`), normalement déjà installés si vous avez suivi les modules précédents.

---

## Conventions utilisées dans ce chapitre

Tout au long de ce chapitre, les conventions suivantes s'appliquent :

- les commandes CMake sont écrites en **minuscules** (`add_executable`, `target_link_libraries`), conformément à la convention moderne — l'ancien style en majuscules (`ADD_EXECUTABLE`) est obsolète ;
- les variables CMake sont en **MAJUSCULES** (`CMAKE_BUILD_TYPE`, `PROJECT_NAME`) ;
- les noms de cibles suivent la convention **snake_case** (`my_app`, `network_lib`) ;
- le répertoire de build est toujours nommé `build/` et séparé des sources ;
- Ninja est le backend de génération par défaut dans tous les exemples (option `-G Ninja`).

---

> **À suivre** : La section 26.1 détaille la structure recommandée pour un projet CMake moderne, du répertoire racine jusqu'aux sous-répertoires de bibliothèques et de tests.

⏭️ [Structure d'un projet CMake moderne](/26-cmake/01-structure-projet.md)
