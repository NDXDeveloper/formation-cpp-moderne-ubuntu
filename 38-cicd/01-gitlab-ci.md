🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 38.1 Pipeline GitLab CI : Build → Test → Package

## Introduction

GitLab CI/CD est l'une des plateformes d'intégration continue les plus matures et les plus adoptées pour les projets C++, particulièrement dans le monde professionnel et les organisations qui hébergent leur propre infrastructure. Son intégration native avec le dépôt Git, son système de runners flexibles et sa configuration déclarative en YAML en font un choix naturel pour automatiser le cycle complet d'un projet C++ : de la vérification du formatage à la génération d'artifacts binaires prêts à distribuer.

Cette section présente les concepts fondamentaux de GitLab CI dans le contexte spécifique d'un projet C++ construit avec CMake, et pose les bases nécessaires avant de plonger dans les fichiers de configuration concrets.

## Pourquoi GitLab CI pour du C++

GitLab CI offre plusieurs caractéristiques particulièrement adaptées aux contraintes d'un projet C++ :

**Runners auto-hébergés.** Contrairement aux plateformes qui imposent exclusivement des runners cloud, GitLab permet d'installer des runners sur vos propres machines. Pour un projet C++ de grande envergure, cela signifie disposer de machines de build puissantes (beaucoup de cœurs, SSD rapide, RAM suffisante) sans dépendre de la tarification à la minute d'un service cloud. C'est aussi indispensable lorsque le build nécessite l'accès à du matériel spécifique — un FPGA, un SoC ARM pour la cross-compilation, ou une licence de compilateur commercial.

**Cache et artifacts natifs.** GitLab CI propose un mécanisme de cache intégré qui permet de persister des répertoires entre les exécutions d'un pipeline. Pour un projet C++, cela signifie conserver le cache ccache, le répertoire de build CMake ou les dépendances Conan/vcpkg d'un run à l'autre, réduisant considérablement les temps de compilation. Le système d'artifacts permet quant à lui de transmettre les résultats d'un stage au suivant — typiquement, les binaires compilés dans le stage Build sont consommés par le stage Test.

**Services Docker intégrés.** Un job GitLab CI peut utiliser n'importe quelle image Docker comme environnement d'exécution. Pour un projet C++, on peut ainsi définir une image de build personnalisée contenant exactement la toolchain requise (version précise de GCC ou Clang, CMake, Ninja, ccache, Conan, les librairies de développement nécessaires) et la réutiliser de manière reproductible à chaque exécution du pipeline.

**Parallélisme et matrix builds.** GitLab CI supporte nativement l'exécution parallèle de jobs et la génération dynamique de matrices via `parallel:matrix`, ce qui permet de tester simultanément plusieurs combinaisons compilateur/standard/architecture sans dupliquer manuellement les définitions de jobs.

## Le modèle d'exécution GitLab CI

Avant de configurer un pipeline, il est essentiel de comprendre comment GitLab CI exécute les jobs. Le modèle repose sur trois concepts fondamentaux.

### Le fichier `.gitlab-ci.yml`

Toute la configuration du pipeline réside dans un unique fichier `.gitlab-ci.yml` placé à la racine du dépôt. Ce fichier décrit les stages (étapes séquentielles), les jobs (unités de travail), et les règles qui déterminent quand chaque job doit s'exécuter. Chaque commit poussé vers GitLab déclenche la lecture de ce fichier et l'exécution du pipeline correspondant.

### Les runners

Un runner est un agent qui exécute les jobs. Il peut fonctionner en mode **shell** (exécution directe sur la machine hôte), en mode **Docker** (chaque job tourne dans un conteneur éphémère) ou en mode **Kubernetes** (les jobs sont schedulés comme des pods). Pour un projet C++, le mode Docker est le plus courant : chaque job démarre dans un conteneur propre basé sur l'image spécifiée, ce qui garantit un environnement reproductible et isolé.

### Le cycle de vie d'un job

Lorsqu'un job est déclenché, le runner procède dans l'ordre suivant :

1. **Préparation de l'environnement** — Le runner pull l'image Docker spécifiée (si elle n'est pas déjà en cache local) et crée un conteneur.
2. **Restauration du cache** — Si un cache est configuré, le runner télécharge et extrait les répertoires cachés depuis l'exécution précédente.
3. **Checkout du code** — Le code source du dépôt est cloné ou fetché dans le conteneur.
4. **Exécution des scripts** — Les commandes définies dans `before_script`, `script` et `after_script` sont exécutées séquentiellement.
5. **Upload des artifacts** — Si le job produit des artifacts, ils sont archivés et rendus disponibles pour les jobs suivants ou pour téléchargement.
6. **Sauvegarde du cache** — Les répertoires cachés sont re-uploadés si leur contenu a changé.
7. **Nettoyage** — Le conteneur est détruit.

Cette compréhension est cruciale pour un projet C++ car elle détermine où placer le cache ccache (étape 2/6), comment transmettre les binaires compilés du stage Build au stage Test (étape 5), et pourquoi chaque job repart d'un environnement vierge — ce qui impose de penser explicitement à la persistance.

## Architecture d'un pipeline C++ typique sur GitLab

Pour un projet C++ construit avec CMake et testé avec Google Test, un pipeline GitLab CI s'organise classiquement en quatre à cinq stages :

```
Pipeline GitLab CI — Projet C++ CMake

  ┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
  │   lint   │────▶│  build   │────▶│   test   │────▶│ package  │────▶│  deploy  │
  └──────────┘     └──────────┘     └──────────┘     └──────────┘     └──────────┘
       │                │                │                │                │
       ▼                ▼                ▼                ▼                ▼
  clang-format     cmake --build    ctest /          dpkg-deb /       registry /
  clang-tidy       (GCC + Clang)    sanitizers       docker build     pages /
                                                                      release
```

**Stage `lint`** — Vérifie que le code est correctement formaté (clang-format --dry-run --Werror) et qu'il ne contient pas de problèmes détectables par l'analyse statique (clang-tidy). Ce stage ne compile pas le projet entièrement ; il utilise la compilation database (`compile_commands.json`) générée par CMake pour alimenter clang-tidy. Placer ce stage en premier permet de rejeter rapidement un code qui ne respecte pas les conventions, sans consommer de temps de compilation.

**Stage `build`** — Compile le projet avec CMake et Ninja. C'est ici que le cache ccache est exploité pour éviter de recompiler les fichiers source inchangés. Dans un pipeline mature, ce stage contient plusieurs jobs parallèles correspondant aux différentes combinaisons de la matrice (GCC 15 / Clang 20, Debug / Release, C++20 / C++23). Les binaires compilés — exécutables et librairies — sont déclarés comme artifacts pour être disponibles dans le stage suivant.

**Stage `test`** — Exécute les tests unitaires et d'intégration en utilisant les binaires produits par le stage Build. Ce stage utilise CTest (intégré à CMake) ou lance directement les binaires Google Test. Des jobs supplémentaires peuvent compiler et exécuter les tests avec les sanitizers activés (AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer) pour détecter des bugs que les tests classiques ne révèlent pas.

**Stage `package`** — Génère les artifacts de distribution. Selon le mode de livraison du projet, cela peut inclure la création de paquets DEB ou RPM, la construction d'une image Docker optimisée via un multi-stage build, ou simplement l'archivage des binaires statiques. Ce stage ne s'exécute généralement que sur la branche principale ou sur les tags de version.

**Stage `deploy`** — Publie les artifacts : push de l'image Docker vers un registry, upload du paquet sur un dépôt APT, création d'une release GitLab avec les binaires attachés, ou déploiement de la documentation sur GitLab Pages. Ce stage est souvent déclenché manuellement ou uniquement sur les tags.

## L'image Docker de build : un choix structurant

Le choix de l'image Docker utilisée par les jobs de build est l'une des décisions les plus impactantes pour la performance et la maintenabilité d'un pipeline C++. Trois approches sont courantes.

### Utiliser une image de base et installer les outils à chaque job

C'est l'approche naïve : partir d'une image `ubuntu:24.04` et installer GCC, CMake, Ninja et les dépendances dans le `before_script` de chaque job. Cette méthode fonctionne mais gaspille du temps à chaque exécution (l'installation de `build-essential`, CMake et les librairies peut prendre 30 à 60 secondes) et rend le pipeline fragile face aux mises à jour des dépôts APT.

### Construire une image de build personnalisée

L'approche recommandée consiste à construire une image Docker dédiée contenant toute la toolchain, la publier dans le registry GitLab du projet (ou un registry interne), et la référencer dans le pipeline. Cette image est reconstruite ponctuellement lorsque la toolchain évolue, pas à chaque exécution du pipeline.

Un Dockerfile de build typique pour un projet C++ moderne sur Ubuntu ressemble à ceci :

```dockerfile
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++-15 \
    clang-20 \
    cmake \
    ninja-build \
    ccache \
    git \
    pkg-config \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Installation de Conan 2.x pour la gestion des dépendances
RUN pip3 install --break-system-packages conan

# Configuration de ccache
ENV CCACHE_DIR=/cache/ccache  
ENV CCACHE_MAXSIZE=2G  
ENV CMAKE_C_COMPILER_LAUNCHER=ccache  
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache  
```

Cette image est ensuite référencée dans `.gitlab-ci.yml` via la directive `image:`, et tous les jobs de build démarrent avec un environnement complet et identique.

### Utiliser les images officielles des compilateurs

GCC et LLVM fournissent des images Docker officielles (`gcc:15`, `silkeh/clang:20`) qui contiennent le compilateur préinstallé. Elles sont pratiques pour les matrix builds (un job par image) mais nécessitent souvent l'installation supplémentaire de CMake, Ninja et des outils annexes.

## Le cache : clé de la performance en CI C++

Dans un pipeline C++, le cache est ce qui fait la différence entre un build de 15 minutes et un build de 2 minutes. GitLab CI propose un mécanisme de cache déclaratif qui permet de persister des répertoires entre les exécutions.

Pour un projet C++, les répertoires à cacher sont typiquement :

- **Le cache ccache** (`~/.cache/ccache` ou un chemin personnalisé) — Contient les résultats de compilation précédents. Lorsqu'un fichier source n'a pas changé, ccache retourne directement le fichier objet depuis le cache au lieu de relancer le compilateur. Sur un projet de taille moyenne, le taux de cache hit atteint facilement 80-95% sur les builds incrémentaux, réduisant le temps de compilation de manière spectaculaire.

- **Le répertoire des dépendances Conan** (`~/.conan2/`) — Évite de retélécharger et recompiler les dépendances tierces à chaque exécution.

- **Le répertoire de build CMake** (`build/`) — Dans certaines configurations, conserver le répertoire de build permet à CMake de ne reconfigurer et recompiler que ce qui a changé. Cette approche est toutefois plus fragile que ccache car le répertoire de build peut devenir incohérent si la structure du projet change.

La distinction entre **cache** et **artifacts** dans GitLab CI est importante : le cache est un mécanisme best-effort de persistance entre exécutions du même job, tandis que les artifacts sont des fichiers produits par un job et transmis de manière fiable aux jobs suivants dans le même pipeline. Les binaires compilés doivent être des artifacts ; le cache ccache doit être dans le cache.

## Prérequis pour suivre cette section

Les exemples de configuration présentés dans les sous-sections suivantes supposent un projet C++ avec la structure suivante :

```
mon-projet/
├── CMakeLists.txt              # Build system principal
├── src/
│   ├── main.cpp
│   └── lib/
│       ├── core.cpp
│       └── core.hpp
├── tests/
│   ├── CMakeLists.txt
│   └── test_core.cpp           # Tests Google Test
├── .clang-format               # Configuration du formatage
├── .clang-tidy                 # Configuration de l'analyse statique
├── .gitlab-ci.yml              # Pipeline CI/CD (ce que nous allons écrire)
└── Dockerfile.build            # Image de build personnalisée (optionnel)
```

Le `CMakeLists.txt` racine est configuré pour :
- générer la compilation database (`set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`), nécessaire pour clang-tidy,  
- supporter le choix du compilateur via `-DCMAKE_CXX_COMPILER`,  
- intégrer Google Test via FetchContent ou find_package,  
- exposer les tests via CTest (`enable_testing()` et `add_test()`).

Ces éléments ont été couverts dans les chapitres 26 (CMake) et 33 (Google Test).

## Ce qui suit

Les sous-sections suivantes détaillent la mise en œuvre concrète :

- **Section 38.1.1** — La structure du fichier `.gitlab-ci.yml` : syntaxe, stages, jobs, variables, et les directives spécifiques à un projet C++.  
- **Section 38.1.2** — La définition des jobs et des stages : comment organiser les jobs de lint, build, test et package, et comment les relier entre eux via les artifacts et les dépendances.

Chaque sous-section fournit des fichiers de configuration complets, commentés et directement réutilisables.

⏭️ [Structure .gitlab-ci.yml](/38-cicd/01.1-structure-gitlab-ci.md)
