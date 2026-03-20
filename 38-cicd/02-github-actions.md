🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 38.2 GitHub Actions : Workflows pour C++

## Introduction

GitHub Actions est la plateforme CI/CD intégrée à GitHub. Lancée en 2019, elle s'est rapidement imposée comme le standard de facto pour les projets open source et occupe une place croissante dans les organisations professionnelles. Pour un projet C++, GitHub Actions offre un démarrage rapide grâce à ses runners hébergés (qui incluent déjà GCC et Clang), un marketplace d'actions pré-construites qui simplifient les tâches récurrentes, et une intégration étroite avec l'écosystème GitHub (pull requests, releases, packages).

Cette section présente les concepts fondamentaux de GitHub Actions dans le contexte d'un projet C++, en mettant en évidence les différences avec GitLab CI couvert en section 38.1, et en identifiant les points d'attention spécifiques à la plateforme.

## GitHub Actions vs GitLab CI : cartographie des concepts

Si vous avez lu la section 38.1, vous maîtrisez déjà les principes du CI/CD pour C++. La logique reste identique sur GitHub Actions — seule la terminologie et la syntaxe changent. Le tableau suivant établit la correspondance entre les deux plateformes :

| Concept | GitLab CI | GitHub Actions |
|---------|-----------|----------------|
| Fichier de configuration | `.gitlab-ci.yml` (racine) | `.github/workflows/*.yml` (un ou plusieurs) |
| Unité d'exécution | Job | Job |
| Regroupement séquentiel | Stage | Implicite via `needs` (pas de notion native de stage) |
| Déclencheur | `rules:` / `only:` | `on:` (push, pull_request, schedule…) |
| Template réutilisable | `extends:` / ancres YAML | Actions réutilisables / composite actions |
| Variables | `variables:` | `env:` (workflow, job, step) |
| Secrets | Variables CI/CD (Settings) | Secrets (Settings → Secrets) |
| Cache | `cache:` (intégré) | `actions/cache` (action externe) |
| Artifacts | `artifacts:` (intégré) | `actions/upload-artifact` / `download-artifact` |
| Conteneur d'exécution | `image:` | `container:` ou `runs-on:` (runner hébergé) |
| Exécution conditionnelle | `rules: if:` | `if:` sur les jobs ou steps |
| Matrice de builds | `parallel:matrix` | `strategy.matrix` |
| Runners auto-hébergés | GitLab Runner | Self-hosted runners |

La différence architecturale majeure est que **GitLab CI organise les jobs en stages séquentiels**, tandis que **GitHub Actions exécute tous les jobs en parallèle par défaut** et utilise `needs:` pour créer des dépendances explicites. Il n'y a pas de concept de "stage" dans GitHub Actions — l'ordre d'exécution est entièrement déterminé par le graphe de dépendances.

L'autre différence notable est la multiplicité des fichiers : GitLab CI repose sur un unique `.gitlab-ci.yml`, tandis que GitHub Actions permet (et encourage) de découper les workflows en plusieurs fichiers YAML dans `.github/workflows/`. Un projet C++ peut ainsi avoir un workflow pour le CI (build + test), un autre pour le packaging (déclenché sur les tags), et un troisième pour la publication de la documentation.

## Les runners GitHub : ce qui est disponible

### Runners hébergés (GitHub-hosted)

GitHub fournit des runners virtualisés prêts à l'emploi. Pour un projet C++, le runner le plus utilisé est `ubuntu-latest` (actuellement Ubuntu 24.04), qui inclut d'office une toolchain conséquente :

| Outil | Version pré-installée (mars 2026) |
|-------|-----------------------------------|
| GCC (g++) | 13, 14 |
| Clang (clang++) | 16, 17, 18 |
| CMake | 3.30+ |
| Ninja | 1.12+ |
| Make | 4.3 |
| Python | 3.12 |
| Git | 2.45+ |

Ces versions évoluent au fil des mises à jour de l'image `ubuntu-latest`. Deux points importants à noter :

**Les versions ne sont pas les toutes dernières.** Les runners hébergés embarquent les versions disponibles dans les dépôts Ubuntu officiels, qui sont souvent en retrait par rapport aux dernières releases. Si votre projet nécessite GCC 15 ou Clang 20 (pour le support C++23/C++26), vous devrez installer ces versions manuellement dans le workflow, utiliser une action comme `aminya/setup-cpp`, ou passer à un conteneur Docker personnalisé.

**La puissance matérielle est standard.** Les runners `ubuntu-latest` disposent de 4 vCPUs, 16 Go de RAM et 14 Go de SSD. C'est suffisant pour la majorité des projets, mais un projet C++ de grande envergure avec des temps de compilation élevés peut bénéficier de runners plus puissants (GitHub propose des "larger runners" en option payante, avec jusqu'à 64 vCPUs).

### Runners auto-hébergés (self-hosted)

Comme GitLab CI, GitHub Actions permet d'enregistrer vos propres machines comme runners. Les raisons d'utiliser des runners auto-hébergés pour un projet C++ sont les mêmes que pour GitLab : machines de build plus puissantes, accès à du matériel spécifique, réduction des coûts sur les gros volumes de CI, ou exigences de sécurité (le code ne quitte pas votre infrastructure).

La configuration s'effectue dans Settings → Actions → Runners du dépôt ou de l'organisation, et les runners auto-hébergés sont référencés dans le workflow via `runs-on: self-hosted` ou un label personnalisé (`runs-on: [self-hosted, linux, x64, cpp-builder]`).

## Le modèle d'exécution GitHub Actions

### Workflows, jobs et steps

La hiérarchie d'exécution dans GitHub Actions comporte trois niveaux :

**Workflow** — Un fichier YAML complet dans `.github/workflows/`. Un workflow est déclenché par un événement (push, pull request, tag, schedule, déclenchement manuel). Un dépôt peut avoir plusieurs workflows indépendants.

**Job** — Une unité de travail qui s'exécute sur un runner. Les jobs d'un même workflow s'exécutent en parallèle par défaut. La directive `needs:` crée des dépendances entre jobs. Chaque job dispose de son propre environnement (conteneur ou VM), ce qui signifie — exactement comme dans GitLab CI — que l'état n'est pas partagé entre les jobs sans passer par les artifacts ou le cache.

**Step** — Une commande ou une action au sein d'un job. Les steps s'exécutent séquentiellement dans le même environnement. C'est la granularité la plus fine. Un step peut être une commande shell (`run:`) ou l'invocation d'une action pré-construite (`uses:`).

Cette hiérarchie se traduit directement pour un projet C++ :

```
Workflow: ci.yml
  ├── Job: lint         (runs-on: ubuntu-latest)
  │     ├── Step: Checkout
  │     ├── Step: Run clang-format
  │     └── Step: Run clang-tidy
  ├── Job: build-gcc    (runs-on: ubuntu-latest)        ← parallèle avec build-clang
  │     ├── Step: Checkout
  │     ├── Step: Install GCC 15
  │     ├── Step: Restore ccache
  │     ├── Step: CMake configure
  │     ├── Step: CMake build
  │     └── Step: Upload artifacts
  ├── Job: build-clang  (runs-on: ubuntu-latest)        ← parallèle avec build-gcc
  │     └── ...
  ├── Job: test-gcc     (needs: build-gcc)
  │     ├── Step: Download artifacts
  │     └── Step: Run CTest
  └── Job: test-clang   (needs: build-clang)
        └── ...
```

### La notion d'action (Action)

Le concept d'**action** est ce qui distingue le plus GitHub Actions des autres plateformes CI. Une action est un composant réutilisable qui encapsule une tâche courante. Elle est référencée par `uses:` et peut être :

- **Officielle** (maintenue par GitHub) : `actions/checkout@v4`, `actions/cache@v4`, `actions/upload-artifact@v4`.  
- **Communautaire** (publiée sur le Marketplace) : `aminya/setup-cpp@v1`, `hendrikmuhs/ccache-action@v1`.  
- **Locale** (définie dans le dépôt) : une action composite dans `.github/actions/`.

Pour un projet C++, les actions remplacent une partie significative du boilerplate qu'il faut écrire manuellement dans GitLab CI. Par exemple, la configuration du cache ccache qui nécessite une dizaine de lignes dans GitLab CI se réduit à une invocation de `hendrikmuhs/ccache-action` dans GitHub Actions.

Cependant, cette commodité a un revers : chaque action est du code tiers dont il faut évaluer la fiabilité et la maintenance. Épingler les actions à un hash de commit plutôt qu'à un tag de version (`uses: actions/cache@v4` → `uses: actions/cache@0c907a...`) est une bonne pratique de sécurité pour les projets sensibles.

## Cache et artifacts dans GitHub Actions

La gestion du cache et des artifacts est conceptuellement identique à GitLab CI, mais l'implémentation diffère.

### Le cache (`actions/cache`)

Dans GitHub Actions, le cache n'est pas une directive native — c'est une action qu'il faut invoquer explicitement :

```yaml
- name: Restore ccache
  uses: actions/cache@v4
  with:
    path: ~/.cache/ccache
    key: ccache-gcc15-${{ runner.os }}-${{ hashFiles('src/**', 'include/**') }}
    restore-keys: |
      ccache-gcc15-${{ runner.os }}-
```

Le fonctionnement est similaire au cache GitLab CI, avec quelques différences :

**Clés avec fallback.** La directive `restore-keys` permet de définir des préfixes de clé de secours. Si la clé exacte n'est pas trouvée (parce que des fichiers source ont changé), GitHub recherche un cache dont la clé commence par le préfixe spécifié. Pour ccache, c'est idéal : même si le cache n'est pas parfaitement à jour, les fichiers objets des sources inchangées restent valides.

**Immutabilité des clés.** Contrairement à GitLab CI où le cache est écrasé à chaque exécution, le cache GitHub est immutable par clé : une fois qu'une clé est créée, elle ne peut pas être mise à jour. Le cache n'est sauvegardé que si la clé exacte n'existait pas au moment du restore. C'est pourquoi la clé inclut un hash des fichiers source — chaque modification de code crée une nouvelle clé de cache et sauvegarde le nouvel état de ccache.

**Limite de taille.** GitHub impose une limite de 10 Go de cache par dépôt. Les entrées les moins récemment utilisées sont automatiquement purgées lorsque cette limite est atteinte. Pour un projet C++ avec des caches ccache pour plusieurs combinaisons compilateur/standard, cette limite peut être atteinte. Surveiller l'utilisation dans Settings → Actions → Caches permet d'anticiper les problèmes.

### Les artifacts (`actions/upload-artifact` / `download-artifact`)

Le transfert de fichiers entre jobs passe par une paire d'actions :

```yaml
# Dans le job de build
- name: Upload build artifacts
  uses: actions/upload-artifact@v4
  with:
    name: build-gcc-release
    path: |
      build/bin/
      build/tests/
      build/CTestTestfile.cmake
    retention-days: 1

# Dans le job de test
- name: Download build artifacts
  uses: actions/download-artifact@v4
  with:
    name: build-gcc-release
    path: build/
```

Le mécanisme est fonctionnellement équivalent aux artifacts GitLab CI. La différence principale est l'explicitation : dans GitLab CI, les artifacts sont automatiquement disponibles dans les jobs des stages suivants ; dans GitHub Actions, chaque job doit explicitement uploader et downloader les artifacts dont il a besoin.

**`retention-days: 1`** définit la durée de conservation. Pour les artifacts intra-pipeline (binaires de test), une journée suffit. Pour les artifacts de release, augmentez cette valeur ou utilisez un stockage externe.

## Exécuter dans un conteneur Docker

Par défaut, les jobs GitHub Actions s'exécutent directement sur la VM du runner. Pour un projet C++ qui nécessite une toolchain précise, il est possible d'exécuter le job dans un conteneur Docker, de manière similaire à la directive `image:` de GitLab CI :

```yaml
jobs:
  build:
    runs-on: ubuntu-latest
    container:
      image: registry.exemple.com/cpp-build:latest
      credentials:
        username: ${{ secrets.REGISTRY_USER }}
        password: ${{ secrets.REGISTRY_PASSWORD }}
    steps:
      - uses: actions/checkout@v4
      - run: cmake -B build -G Ninja && cmake --build build
```

La directive `container:` indique à GitHub Actions de pull l'image spécifiée et d'exécuter tous les steps du job à l'intérieur du conteneur. C'est l'approche la plus proche du modèle GitLab CI et celle qui offre la meilleure reproductibilité.

L'alternative — installer les outils directement sur le runner via des steps d'installation — est plus simple pour les cas courants mais moins reproductible et plus lente si les installations sont nombreuses.

## Les matrix builds : la force de GitHub Actions

Le système de matrix builds est l'un des points forts de GitHub Actions pour le C++. La directive `strategy.matrix` génère automatiquement un job pour chaque combinaison des paramètres définis :

```yaml
jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      fail-fast: false
      matrix:
        compiler: [gcc, clang]
        build_type: [Debug, Release]
        include:
          - compiler: gcc
            cxx: g++-15
            cc: gcc-15
          - compiler: clang
            cxx: clang++-20
            cc: clang-20
    steps:
      - run: |
          cmake -B build \
            -DCMAKE_CXX_COMPILER=${{ matrix.cxx }} \
            -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}
          cmake --build build --parallel $(nproc)
```

Cette configuration génère quatre jobs : GCC Debug, GCC Release, Clang Debug, Clang Release. Chaque combinaison reçoit les variables correspondantes via `${{ matrix.xxx }}`.

**`fail-fast: false`** est une option importante pour les projets C++. Par défaut, GitHub Actions annule tous les jobs restants de la matrice dès qu'un job échoue. Pour un build C++, c'est rarement le comportement souhaité : si le build GCC Release échoue, vous voulez quand même savoir si le build Clang fonctionne. `fail-fast: false` laisse tous les jobs terminer indépendamment.

La directive `include:` enrichit la matrice avec des variables supplémentaires pour chaque combinaison. C'est le mécanisme qui associe le nom humain `gcc` aux chemins réels `g++-15` et `gcc-15`. On peut également utiliser `exclude:` pour retirer des combinaisons spécifiques de la matrice.

La section 38.7 (Matrix builds) couvrira en détail les stratégies de matrice avancées : matrices multi-standard (C++17/20/23), matrices multi-plateforme (Linux/macOS/Windows), et matrices conditionnelles selon le contexte (MR vs branche principale).

## Déclencheurs : quand exécuter le workflow

GitHub Actions offre une granularité fine sur les événements qui déclenchent un workflow. Les déclencheurs les plus pertinents pour un projet C++ sont :

```yaml
on:
  push:
    branches: [main, develop]
    tags: ['v*']
  pull_request:
    branches: [main]
    paths:
      - 'src/**'
      - 'include/**'
      - 'tests/**'
      - 'CMakeLists.txt'
      - '.github/workflows/**'
  schedule:
    - cron: '0 3 * * 1'       # Lundi à 3h du matin
  workflow_dispatch:            # Déclenchement manuel
```

**`push` sur les tags** déclenche le workflow de packaging/release lorsqu'un tag de version est poussé.

**`pull_request` avec `paths:`** limite l'exécution aux pull requests qui modifient effectivement du code C++ ou la configuration CI. Un PR qui ne touche que le README ou la documentation ne déclenchera pas de compilation — exactement comme la directive `changes:` dans les `rules:` GitLab CI.

**`schedule`** permet d'exécuter un build complet périodiquement, même sans nouveau commit. C'est utile pour détecter des régressions introduites par des mises à jour de dépendances externes ou des changements dans l'image Docker de build.

**`workflow_dispatch`** ajoute un bouton "Run workflow" dans l'interface GitHub, permettant un déclenchement manuel avec des paramètres optionnels. Pratique pour relancer un build de release ou tester une configuration spécifique.

## Limites et points d'attention

Avant de plonger dans la configuration concrète, voici les limites spécifiques à GitHub Actions qu'il faut avoir en tête pour un projet C++ :

**Temps d'exécution.** Les workflows sur runners hébergés sont limités à 6 heures par job et 35 jours de rétention des logs. Pour la plupart des projets C++, la limite de 6 heures est largement suffisante, mais un build from scratch d'un très gros projet sans cache pourrait s'en approcher.

**Minutes de CI.** Les dépôts privés disposent d'un quota mensuel de minutes CI (variable selon le plan GitHub). Les builds C++ étant gourmands, un projet actif avec une matrice large peut consommer ce quota rapidement. Les dépôts publics bénéficient de minutes illimitées sur les runners hébergés.

**Pas de merge profond pour les templates.** Contrairement à `extends:` dans GitLab CI qui effectue un deep merge, GitHub Actions n'a pas de mécanisme natif équivalent. La réutilisation de configuration passe par les composite actions, les workflows réutilisables (`workflow_call`), ou les ancres YAML — mais aucune de ces approches n'offre la même souplesse que le `extends:` avec deep merge de GitLab. La duplication de configuration est donc plus fréquente dans les workflows GitHub Actions complexes.

**Isolation des jobs.** Chaque job s'exécute dans une VM ou un conteneur distinct. Il n'y a aucun état partagé entre les jobs en dehors des artifacts et du cache. C'est le même modèle que GitLab CI, mais il est bon de le rappeler car c'est une source fréquente de confusion pour les développeurs habitués aux scripts shell locaux où tout partage le même système de fichiers.

## Ce qui suit

Les sous-sections suivantes détaillent la mise en œuvre concrète :

- **Section 38.2.1** — La structure d'un fichier workflow YAML : syntaxe, événements déclencheurs, jobs, steps, et un workflow complet pour un projet C++ CMake.  
- **Section 38.2.2** — Les actions pré-construites utiles pour C++ : `aminya/setup-cpp`, `hendrikmuhs/ccache-action`, `actions/cache`, et l'intégration des sanitizers et de l'analyse statique.

---

> 📎 *Si vous n'utilisez pas GitLab, cette section est autosuffisante. Si vous avez lu la section 38.1, vous constaterez que les principes (stages, cache ccache, artifacts binaires, sanitizers, matrix builds) sont identiques — seule la syntaxe change.*

⏭️ [Structure workflow YAML](/38-cicd/02.1-structure-workflow.md)
