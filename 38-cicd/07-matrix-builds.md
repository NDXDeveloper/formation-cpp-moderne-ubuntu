🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 38.7 Matrix builds : Multi-compilateur, multi-version ⭐

## Introduction

Un projet C++ professionnel doit garantir sa compatibilité sur plusieurs axes simultanés : compilateurs (GCC, Clang), versions de ces compilateurs (GCC 14, GCC 15), standards du langage (C++17, C++20, C++23), types de build (Debug, Release), et potentiellement architectures cibles (x86_64, ARM64). Tester manuellement chaque combinaison est impensable — la combinatoire explose rapidement. Un simple croisement de 2 compilateurs × 2 types de build × 3 standards génère déjà 12 configurations distinctes.

Les **matrix builds** résolvent ce problème en déclarant les axes de variation dans la configuration du pipeline, et en laissant la plateforme CI générer automatiquement un job pour chaque combinaison. Le développeur écrit un seul job paramétré ; la plateforme en exécute douze.

Cette section couvre la conception de matrices de build efficaces : comment choisir les axes de variation, comment maîtriser l'explosion combinatoire, comment adapter la matrice au contexte (merge request vs branche principale vs nightly), et comment exploiter les spécificités de GitLab CI et GitHub Actions pour des matrices avancées.

## L'explosion combinatoire : le problème central

Considérons un projet qui veut valider sa compatibilité sur :

- 2 compilateurs : GCC 15, Clang 20  
- 3 standards : C++17, C++20, C++23  
- 2 types de build : Debug, Release  
- 2 sanitizers : ASan, TSan  
- 3 architectures : x86_64, ARM64, RISC-V

Le produit cartésien complet donne **2 × 3 × 2 × 2 × 3 = 72 combinaisons**. À 8 minutes par build (sans cache), cela représente 9 heures 36 minutes de compilation séquentielle. Même avec 8 runners en parallèle et un cache chaud, le pipeline durerait plus d'une heure.

Personne n'exécute 72 combinaisons à chaque commit. La stratégie de matrice consiste précisément à **sélectionner un sous-ensemble pertinent** de cette combinatoire, adapté à chaque contexte d'exécution. L'art du matrix build, c'est trouver le minimum de combinaisons qui détecte le maximum de problèmes.

## Conception d'une matrice : principes directeurs

### Principe 1 — Chaque axe doit capturer une classe de bugs distincte

Un axe n'a de valeur dans la matrice que s'il peut révéler des bugs que les autres axes ne détectent pas :

**Compilateur** (GCC vs Clang) — Détecte les dépendances à un comportement spécifique d'un compilateur, les extensions non standard, les différences d'interprétation du standard. GCC et Clang divergent sur certains cas limites de résolution de surcharge, d'instanciation de templates, et de diagnostic des comportements indéfinis. Un code qui compile avec GCC mais pas avec Clang (ou inversement) est presque toujours un bug dans le code.

**Standard C++** (C++17 vs C++20 vs C++23) — Détecte les dépendances à des fonctionnalités dépréciées ou supprimées entre versions du standard, et valide que le code est compatible avec le standard minimum déclaré par le projet. Par exemple, `std::auto_ptr` (supprimé en C++17), les `volatile` compound assignments (dépréciés en C++20), ou l'utilisation de `std::expected` (C++23 uniquement).

**Type de build** (Debug vs Release) — Détecte les bugs sensibles aux optimisations : variables non initialisées qui ont une valeur "correcte" en Debug mais aléatoire en Release, comportements indéfinis que `-O2` exploite, bugs de timing en code concurrent que les optimisations révèlent ou masquent.

**Sanitizer** (ASan vs TSan vs UBSan) — Détecte des catégories de bugs fondamentalement différentes : ASan pour les erreurs mémoire, TSan pour les data races, UBSan pour les comportements indéfinis. Chaque sanitizer justifie un axe propre car il détecte des bugs invisibles aux autres.

**Architecture** (x86_64 vs ARM64) — Détecte les problèmes d'endianness (rare entre x86_64 et ARM64 qui sont tous deux little-endian, mais pertinent pour les architectures big-endian), les hypothèses sur la taille des types (`long` est 64 bits sur Linux x86_64 et ARM64, mais 32 bits sur Windows), et les bugs liés à l'alignement mémoire (ARM est plus strict que x86_64 sur certains accès non alignés).

### Principe 2 — Combiner les axes orthogonaux, ne pas multiplier les axes corrélés

Deux axes sont **orthogonaux** si les bugs détectés par l'un sont indépendants de la valeur de l'autre. Compilateur et standard sont largement orthogonaux : un bug lié à une fonctionnalité C++17 dépréciée se manifeste indépendamment du compilateur.

Deux axes sont **corrélés** si les bugs détectés par l'un sont prévisibles en fonction de l'autre. Version du compilateur et standard sont partiellement corrélés : GCC 14 supporte C++23 de manière plus complète que GCC 13. Tester GCC 13 avec C++23 peut ne produire que des erreurs de compilation liées au support incomplet, pas des bugs dans le code du projet.

**Règle pratique** : pour les axes corrélés, testez les combinaisons réalistes plutôt que le produit cartésien complet. Il est inutile de tester GCC 13 avec C++23 si votre projet requiert des fonctionnalités C++23 que GCC 13 ne supporte pas.

### Principe 3 — Calibrer la matrice au contexte d'exécution

La matrice doit être plus petite sur les merge requests (feedback rapide) et plus large sur la branche principale et les builds nocturnes (couverture exhaustive) :

| Contexte | Objectif | Taille de matrice typique |
|----------|----------|--------------------------|
| Merge request | Feedback en <5 min | 2-4 combinaisons |
| Push sur `main` | Validation complète | 6-10 combinaisons |
| Build nocturne | Couverture exhaustive | 15-30 combinaisons |
| Tag de release | Validation critique | 8-12 combinaisons + cross-compilation |

## Matrice sur GitHub Actions

### Syntaxe de base

GitHub Actions utilise `strategy.matrix` pour définir les axes et générer automatiquement les combinaisons :

```yaml
jobs:
  build:
    name: "${{ matrix.compiler }} / C++${{ matrix.std }} / ${{ matrix.build_type }}"
    runs-on: ubuntu-latest
    strategy:
      fail-fast: false
      matrix:
        compiler: [gcc, clang]
        std: ['17', '20', '23']
        build_type: [Debug, Release]
    steps:
      - # ...
```

Cette déclaration génère 2 × 3 × 2 = **12 jobs**. Chaque job reçoit les variables `matrix.compiler`, `matrix.std` et `matrix.build_type` avec une combinaison unique.

### Enrichir avec `include`

Le problème de la matrice de base est qu'elle ne contient que les noms abstraits (`gcc`, `clang`). Les commandes de compilation ont besoin des chemins réels (`g++-15`, `clang++-20`). La directive `include` associe des variables supplémentaires à chaque valeur d'un axe :

```yaml
    strategy:
      fail-fast: false
      matrix:
        compiler: [gcc, clang]
        std: ['20']
        build_type: [Release]
        include:
          # Variables associées à chaque compilateur
          - compiler: gcc
            cxx: g++-15
            cc: gcc-15
            cache_key: gcc15
            install_cmd: |
              sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
              sudo apt-get update && sudo apt-get install -y g++-15
          - compiler: clang
            cxx: clang++-20
            cc: clang-20
            cache_key: clang20
            install_cmd: |
              wget -qO- https://apt.llvm.org/llvm.sh | sudo bash -s -- 20

          # Combinaisons supplémentaires hors produit cartésien
          - compiler: gcc
            std: '17'
            build_type: Release
            cxx: g++-15
            cc: gcc-15
            cache_key: gcc15
          - compiler: clang
            std: '23'
            build_type: Debug
            cxx: clang++-20
            cc: clang-20
            cache_key: clang20
```

Le bloc `include` a un double rôle. Premièrement, les entrées qui correspondent à une valeur existante d'un axe (**enrichissement**) ajoutent des variables à toutes les combinaisons contenant cette valeur : `compiler: gcc` → toutes les combinaisons GCC reçoivent `cxx: g++-15`. Deuxièmement, les entrées qui spécifient une combinaison complète non présente dans le produit cartésien (**ajout**) créent de nouveaux jobs : la combinaison `gcc / C++17 / Release` est ajoutée bien qu'elle ne fasse pas partie du produit cartésien initial (qui ne contient que `std: '20'`).

### Réduire avec `exclude`

La directive `exclude` retire des combinaisons du produit cartésien :

```yaml
    strategy:
      matrix:
        compiler: [gcc, clang]
        std: ['17', '20', '23']
        build_type: [Debug, Release]
        exclude:
          # C++17 n'a pas besoin de Debug — c'est un test de rétrocompatibilité
          - std: '17'
            build_type: Debug
          # Clang Debug est couvert par les sanitizers
          - compiler: clang
            build_type: Debug
            std: '17'
```

Chaque entrée dans `exclude` retire toutes les combinaisons qui correspondent aux clés spécifiées. L'entrée `std: '17', build_type: Debug` retire les deux combinaisons `gcc/C++17/Debug` et `clang/C++17/Debug`.

### Matrice asymétrique : la méthode `include`-only

Pour les matrices complexes où le produit cartésien ne correspond pas aux combinaisons souhaitées, il est souvent plus clair de n'utiliser que `include` avec un axe trivial :

```yaml
    strategy:
      fail-fast: false
      matrix:
        config:
          - name: "GCC 15 / C++20 / Release"
            cxx: g++-15
            cc: gcc-15
            std: '20'
            build_type: Release
            cache_key: gcc15-20-rel
            sanitizer: ""

          - name: "GCC 15 / C++17 / Release"
            cxx: g++-15
            cc: gcc-15
            std: '17'
            build_type: Release
            cache_key: gcc15-17-rel
            sanitizer: ""

          - name: "Clang 20 / C++20 / Release"
            cxx: clang++-20
            cc: clang-20
            std: '20'
            build_type: Release
            cache_key: clang20-20-rel
            sanitizer: ""

          - name: "Clang 20 / C++23 / Release"
            cxx: clang++-20
            cc: clang-20
            std: '23'
            build_type: Release
            cache_key: clang20-23-rel
            sanitizer: ""

          - name: "Clang 20 / C++20 / ASan+UBSan"
            cxx: clang++-20
            cc: clang-20
            std: '20'
            build_type: Debug
            cache_key: clang20-20-asan
            sanitizer: "-fsanitize=address,undefined -fno-omit-frame-pointer"

          - name: "Clang 20 / C++20 / TSan"
            cxx: clang++-20
            cc: clang-20
            std: '20'
            build_type: Debug
            cache_key: clang20-20-tsan
            sanitizer: "-fsanitize=thread -fno-omit-frame-pointer"

    env:
      CXX: ${{ matrix.config.cxx }}
      CC: ${{ matrix.config.cc }}
    name: ${{ matrix.config.name }}

    steps:
      - uses: actions/checkout@v4

      - name: Setup ccache
        uses: hendrikmuhs/ccache-action@v1
        with:
          key: ${{ matrix.config.cache_key }}

      - name: CMake configure
        run: |
          SANITIZER_FLAGS="${{ matrix.config.sanitizer }}"
          cmake -B build \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=${{ matrix.config.build_type }} \
            -DCMAKE_CXX_STANDARD=${{ matrix.config.std }} \
            -DCMAKE_CXX_COMPILER=${{ env.CXX }} \
            -DCMAKE_C_COMPILER=${{ env.CC }} \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
            ${SANITIZER_FLAGS:+-DCMAKE_CXX_FLAGS="$SANITIZER_FLAGS"} \
            ${SANITIZER_FLAGS:+-DCMAKE_EXE_LINKER_FLAGS="$SANITIZER_FLAGS"}

      - name: CMake build
        run: cmake --build build --parallel $(nproc)

      - name: Run tests
        run: cd build && ctest --parallel $(nproc) --timeout 300 --output-on-failure
        env:
          ASAN_OPTIONS: ${{ contains(matrix.config.sanitizer, 'address') && 'detect_leaks=1:halt_on_error=1' || '' }}
          TSAN_OPTIONS: ${{ contains(matrix.config.sanitizer, 'thread') && 'halt_on_error=1:history_size=4' || '' }}
```

Cette approche liste explicitement les 6 combinaisons souhaitées. C'est plus verbeux qu'un produit cartésien, mais la matrice résultante est exactement celle voulue — pas de combinaisons inutiles à exclure, pas de logique d'enrichissement complexe à comprendre. Chaque entrée est auto-documentée par son champ `name`.

L'expression `${SANITIZER_FLAGS:+...}` dans le step CMake est une expansion conditionnelle bash : si `SANITIZER_FLAGS` est non-vide, l'option CMake est ajoutée ; sinon, rien n'est ajouté. Cela permet au même step de fonctionner pour les builds normaux et les builds avec sanitizers.

## Matrice sur GitLab CI

GitLab CI propose la directive `parallel:matrix` pour générer des jobs paramétrés, mais le mécanisme est moins riche que celui de GitHub Actions.

### Syntaxe de base

```yaml
build:
  stage: build
  parallel:
    matrix:
      - CXX: ["g++-15", "clang++-20"]
        BUILD_TYPE: ["Debug", "Release"]
  script:
    - cmake -B build
        -G Ninja
        -DCMAKE_CXX_COMPILER=${CXX}
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
    - cmake --build build --parallel $(nproc)
```

Cela génère 2 × 2 = 4 jobs nommés automatiquement :

```
build [g++-15, Debug]  
build [g++-15, Release]  
build [clang++-20, Debug]  
build [clang++-20, Release]  
```

### Limites de `parallel:matrix` sur GitLab

Contrairement à GitHub Actions, GitLab CI ne propose ni `include` ni `exclude` sur les matrices. Chaque variable de la matrice ne peut être qu'une liste de valeurs scalaires — pas d'objets avec des champs multiples. Cela limite la capacité à construire des matrices asymétriques.

**Contournement via `extends` et jobs multiples.** Pour une matrice asymétrique sur GitLab, la solution consiste à définir plusieurs jobs qui étendent le même template, chacun avec sa propre combinaison de variables :

```yaml
.build_template:
  stage: build
  script:
    - cmake -B ${BUILD_DIR}
        -G Ninja
        -DCMAKE_CXX_COMPILER=${CXX}
        -DCMAKE_C_COMPILER=${CC}
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
        -DCMAKE_CXX_STANDARD=${CPP_STANDARD}
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
        ${CMAKE_EXTRA_FLAGS}
    - cmake --build ${BUILD_DIR} --parallel $(nproc)
  cache:
    paths:
      - .ccache/
    policy: pull-push
  artifacts:
    paths:
      - ${BUILD_DIR}/bin/
      - ${BUILD_DIR}/tests/
      - ${BUILD_DIR}/CTestTestfile.cmake
    expire_in: 2 hours

# Matrice asymétrique : jobs listés explicitement
build-gcc15-cpp20-release:
  extends: .build_template
  variables:
    CXX: "g++-15"
    CC: "gcc-15"
    BUILD_TYPE: "Release"
    CPP_STANDARD: "20"
    BUILD_DIR: "build-gcc15-cpp20-rel"
    CMAKE_EXTRA_FLAGS: ""
  cache:
    key: "gcc15-cpp20-rel-${CI_COMMIT_REF_SLUG}"

build-gcc15-cpp17-release:
  extends: .build_template
  variables:
    CXX: "g++-15"
    CC: "gcc-15"
    BUILD_TYPE: "Release"
    CPP_STANDARD: "17"
    BUILD_DIR: "build-gcc15-cpp17-rel"
    CMAKE_EXTRA_FLAGS: ""
  cache:
    key: "gcc15-cpp17-rel-${CI_COMMIT_REF_SLUG}"

build-clang20-cpp20-release:
  extends: .build_template
  variables:
    CXX: "clang++-20"
    CC: "clang-20"
    BUILD_TYPE: "Release"
    CPP_STANDARD: "20"
    BUILD_DIR: "build-clang20-cpp20-rel"
    CMAKE_EXTRA_FLAGS: ""
  cache:
    key: "clang20-cpp20-rel-${CI_COMMIT_REF_SLUG}"

build-clang20-cpp20-asan:
  extends: .build_template
  variables:
    CXX: "clang++-20"
    CC: "clang-20"
    BUILD_TYPE: "Debug"
    CPP_STANDARD: "20"
    BUILD_DIR: "build-clang20-cpp20-asan"
    CMAKE_EXTRA_FLAGS: >-
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
  cache:
    key: "clang20-cpp20-asan-${CI_COMMIT_REF_SLUG}"
```

C'est plus verbeux que la syntaxe GitHub Actions mais fonctionnellement équivalent. Le template `.build_template` centralise toute la logique de build ; chaque job concret ne redéfinit que les variables qui changent.

### Matrice avec `parallel:matrix` et `rules` combinés

Pour adapter la matrice au contexte, combinez `parallel:matrix` avec `rules:` :

```yaml
# Matrice réduite sur les MR
build-mr:
  stage: build
  extends: .build_template
  parallel:
    matrix:
      - CXX: "g++-15"
        CC: "gcc-15"
        BUILD_TYPE: "Release"
        CPP_STANDARD: "20"
        BUILD_DIR: "build-gcc-rel"
      - CXX: "clang++-20"
        CC: "clang-20"
        BUILD_TYPE: "Release"
        CPP_STANDARD: "20"
        BUILD_DIR: "build-clang-rel"
  rules:
    - if: '$CI_PIPELINE_SOURCE == "merge_request_event"'

# Matrice complète sur main
build-full:
  stage: build
  extends: .build_template
  parallel:
    matrix:
      - CXX: ["g++-15", "clang++-20"]
        BUILD_TYPE: ["Debug", "Release"]
        CPP_STANDARD: ["17", "20", "23"]
  rules:
    - if: '$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH'
    - if: '$CI_PIPELINE_SOURCE == "schedule"'
```

Les merge requests exécutent 2 combinaisons (GCC Release C++20, Clang Release C++20). La branche principale et les builds nocturnes exécutent les 12 combinaisons du produit cartésien complet. Le feedback sur les MR reste rapide (<5 minutes) tandis que la couverture exhaustive est garantie sur `main`.

## Matrices conditionnelles : adapter au contexte

### Sur GitHub Actions : matrice dynamique

GitHub Actions permet de définir la matrice dans un job précédent et de la passer au job de build via les outputs. C'est la méthode la plus flexible pour adapter la matrice au contexte :

```yaml
jobs:
  configure-matrix:
    runs-on: ubuntu-latest
    outputs:
      matrix: ${{ steps.set-matrix.outputs.matrix }}
    steps:
      - id: set-matrix
        run: |
          if [[ "${{ github.event_name }}" == "pull_request" ]]; then
            # Matrice réduite pour les PR
            MATRIX='{"config":[
              {"name":"GCC 15 C++20 Release","cxx":"g++-15","cc":"gcc-15","std":"20","build_type":"Release","cache_key":"gcc15-20-rel","sanitizer":""},
              {"name":"Clang 20 C++20 Release","cxx":"clang++-20","cc":"clang-20","std":"20","build_type":"Release","cache_key":"clang20-20-rel","sanitizer":""},
              {"name":"ASan+UBSan","cxx":"clang++-20","cc":"clang-20","std":"20","build_type":"Debug","cache_key":"clang20-asan","sanitizer":"-fsanitize=address,undefined -fno-omit-frame-pointer"}
            ]}'
          elif [[ "${{ github.ref }}" == "refs/heads/main" ]]; then
            # Matrice complète pour main
            MATRIX='{"config":[
              {"name":"GCC 15 C++17 Release","cxx":"g++-15","cc":"gcc-15","std":"17","build_type":"Release","cache_key":"gcc15-17-rel","sanitizer":""},
              {"name":"GCC 15 C++20 Release","cxx":"g++-15","cc":"gcc-15","std":"20","build_type":"Release","cache_key":"gcc15-20-rel","sanitizer":""},
              {"name":"GCC 15 C++20 Debug","cxx":"g++-15","cc":"gcc-15","std":"20","build_type":"Debug","cache_key":"gcc15-20-dbg","sanitizer":""},
              {"name":"GCC 15 C++23 Release","cxx":"g++-15","cc":"gcc-15","std":"23","build_type":"Release","cache_key":"gcc15-23-rel","sanitizer":""},
              {"name":"Clang 20 C++20 Release","cxx":"clang++-20","cc":"clang-20","std":"20","build_type":"Release","cache_key":"clang20-20-rel","sanitizer":""},
              {"name":"Clang 20 C++23 Release","cxx":"clang++-20","cc":"clang-20","std":"23","build_type":"Release","cache_key":"clang20-23-rel","sanitizer":""},
              {"name":"ASan+UBSan","cxx":"clang++-20","cc":"clang-20","std":"20","build_type":"Debug","cache_key":"clang20-asan","sanitizer":"-fsanitize=address,undefined -fno-omit-frame-pointer"},
              {"name":"TSan","cxx":"clang++-20","cc":"clang-20","std":"20","build_type":"Debug","cache_key":"clang20-tsan","sanitizer":"-fsanitize=thread -fno-omit-frame-pointer"}
            ]}'
          fi
          echo "matrix=${MATRIX}" >> "$GITHUB_OUTPUT"

  build:
    needs: [configure-matrix]
    runs-on: ubuntu-latest
    strategy:
      fail-fast: false
      matrix: ${{ fromJson(needs.configure-matrix.outputs.matrix) }}
    name: ${{ matrix.config.name }}
    steps:
      # ... (identique à l'exemple include-only)
```

La fonction `fromJson()` convertit la chaîne JSON en objet matrice. Le job `configure-matrix` s'exécute en quelques secondes (pas de compilation, juste de la logique conditionnelle) et produit la matrice adaptée au contexte. Le job `build` la consomme et génère les combinaisons correspondantes.

Cette approche est puissante mais ajoute de la complexité. Elle se justifie pour les projets avec des matrices très différentes selon le contexte (3 combinaisons en PR, 15 sur `main`, 30 en nightly).

### Sur GitHub Actions : approche simplifiée avec `if` par job

Pour des cas plus simples, des jobs séparés avec des conditions `if` sont souvent plus lisibles :

```yaml
jobs:
  # Toujours exécuté (PR + main + tags)
  build-core:
    strategy:
      matrix:
        include:
          - {name: "GCC 15 C++20", cxx: g++-15, std: '20', build_type: Release}
          - {name: "Clang 20 C++20", cxx: clang++-20, std: '20', build_type: Release}
    name: ${{ matrix.name }}
    # ...

  # Uniquement sur main et tags
  build-extended:
    if: github.ref == 'refs/heads/main' || startsWith(github.ref, 'refs/tags/v')
    strategy:
      matrix:
        include:
          - {name: "GCC 15 C++17", cxx: g++-15, std: '17', build_type: Release}
          - {name: "GCC 15 C++23", cxx: g++-15, std: '23', build_type: Release}
          - {name: "Clang 20 C++23", cxx: clang++-20, std: '23', build_type: Release}
          - {name: "GCC 15 Debug", cxx: g++-15, std: '20', build_type: Debug}
    name: ${{ matrix.name }}
    # ...

  # Uniquement en nightly
  build-nightly:
    if: github.event_name == 'schedule'
    strategy:
      matrix:
        include:
          - {name: "Clang 20 C++17", cxx: clang++-20, std: '17', build_type: Release}
          - {name: "Clang 20 Debug", cxx: clang++-20, std: '20', build_type: Debug}
          - {name: "GCC 15 C++23 Debug", cxx: g++-15, std: '23', build_type: Debug}
    name: ${{ matrix.name }}
    # ...
```

Trois jobs (`build-core`, `build-extended`, `build-nightly`) couvrent les trois niveaux de la pyramide. Les conditions `if` déterminent lesquels s'exécutent selon le contexte. C'est moins flexible que la matrice dynamique mais nettement plus facile à comprendre et à maintenir.

## Optimisation du temps total de pipeline

### `fail-fast` : quand l'activer, quand le désactiver

```yaml
strategy:
  fail-fast: false     # Recommandé pour C++
```

La directive `fail-fast` (GitHub Actions) détermine si les jobs restants de la matrice sont annulés dès qu'un job échoue. Pour un projet C++, **`fail-fast: false` est presque toujours préférable** :

- Un échec de compilation GCC ne prédit pas un échec Clang — les deux compilateurs ont des messages d'erreur différents et le développeur a besoin des deux pour diagnostiquer le problème.  
- Un test échoué avec ASan n'implique pas un test échoué avec TSan — les deux sanitizers détectent des catégories de bugs indépendantes.  
- Annuler les jobs restants gaspille le travail déjà effectué (compilation en cours) et force un re-run complet pour obtenir les résultats manquants.

Le seul cas où `fail-fast: true` peut être judicieux est sur les merge requests avec une matrice large, où un échec rapide évite de consommer des minutes CI inutilement.

Sur GitLab CI, il n'y a pas d'équivalent direct de `fail-fast` — tous les jobs d'un même stage s'exécutent indépendamment, et l'échec d'un job n'annule pas les autres du même stage.

### Parallélisme et `needs` : réduire le chemin critique

Le temps total du pipeline n'est pas la somme des temps de tous les jobs — c'est la longueur du **chemin critique** (la séquence de jobs dépendants la plus longue). L'optimisation consiste à minimiser ce chemin critique en maximisant le parallélisme.

Sans `needs`, le pipeline sur GitHub Actions exécute tous les jobs en parallèle (pas de stages). Sur GitLab CI, les stages imposent un séquencement. Dans les deux cas, `needs` permet de créer un graphe de dépendances optimal :

```yaml
# GitHub Actions — graphe de dépendances optimal
jobs:
  lint:
    # ...

  build-gcc:
    needs: [lint]       # Attend lint, puis compile
    # ...

  build-clang:
    needs: [lint]       # Parallèle avec build-gcc
    # ...

  build-asan:
    needs: [lint]       # Parallèle avec build-gcc et build-clang
    # ...

  test-gcc:
    needs: [build-gcc]  # Démarre dès que build-gcc termine
    # ...

  test-clang:
    needs: [build-clang]  # Démarre dès que build-clang termine
    # ...

  test-asan:
    needs: [build-asan]
    # ...
```

Le chemin critique est : `lint → build (le plus long) → test`. Les trois builds s'exécutent en parallèle, et chaque test démarre dès que son build associé est terminé. Le temps total est `lint + max(build-gcc, build-clang, build-asan) + max(test-gcc, test-clang, test-asan)`, pas la somme de tous les jobs.

### Cache partagé entre combinaisons de la matrice

Certaines combinaisons de la matrice partagent une partie significative de leur compilation. Par exemple, `GCC 15 / C++20 / Release` et `GCC 15 / C++20 / Debug` compilent les mêmes fichiers source avec le même compilateur — seuls les flags d'optimisation changent. Avec ccache, si les deux jobs partagent le même cache (clé identique sauf le type de build), les fichiers qui produisent le même résultat indépendamment du type de build (headers précompilés, certains fichiers source simples) bénéficient du cache croisé.

En pratique, le gain est marginal car `-O0 -g` et `-O2 -DNDEBUG` produisent des fichiers objets très différents. La recommandation reste de séparer les clés de cache par configuration complète.

## Matrice de build recommandée

Voici une matrice de référence pour un projet C++ moderne en 2026, calibrée pour un bon équilibre couverture/temps :

### Merge request (3-4 combinaisons, <5 min)

| Compilateur | Standard | Build type | Sanitizer | Justification |
|-------------|----------|------------|-----------|---------------|
| GCC 15 | C++20 | Release | — | Build principal, détection des erreurs GCC |
| Clang 20 | C++20 | Release | — | Compatibilité multi-compilateur |
| Clang 20 | C++20 | Debug | ASan+UBSan | Détection des bugs mémoire et UB |

### Branche principale (7-8 combinaisons, <15 min)

| Compilateur | Standard | Build type | Sanitizer | Justification |
|-------------|----------|------------|-----------|---------------|
| GCC 15 | C++20 | Release | — | Build principal |
| GCC 15 | C++20 | Debug | — | Bugs sensibles aux optimisations |
| GCC 15 | C++17 | Release | — | Rétrocompatibilité standard minimum |
| Clang 20 | C++20 | Release | — | Compatibilité multi-compilateur |
| Clang 20 | C++23 | Release | — | Validation des fonctionnalités C++23 |
| Clang 20 | C++20 | Debug | ASan+UBSan | Erreurs mémoire et UB |
| Clang 20 | C++20 | Debug | TSan | Data races |

### Build nocturne (12-15 combinaisons)

Les 7 combinaisons de la branche principale, plus :

| Compilateur | Standard | Build type | Justification |
|-------------|----------|------------|---------------|
| GCC 15 | C++23 | Release | Validation C++23 sur GCC |
| GCC 15 | C++23 | Debug | Combinaison Debug + C++23 |
| Clang 20 | C++17 | Release | Rétrocompatibilité Clang |
| Clang 20 | C++20 | Debug | Debug Clang sans sanitizer |
| GCC 15 | C++20 | Debug | MSan (MemorySanitizer, Clang uniquement) |
| Cross ARM64 | C++20 | Release | Compatibilité architecturale |
| Cross RISC-V | C++20 | Release | Architecture émergente |

## Anti-patterns courants

### Matrice symétrique complète en permanence

```yaml
# ❌ Anti-pattern : produit cartésien complet sur chaque PR
matrix:
  compiler: [gcc-14, gcc-15, clang-19, clang-20]
  std: ['17', '20', '23']
  build_type: [Debug, Release]
# → 24 combinaisons, chacune à 5 min = 120 minutes de CI par PR
```

Quatre compilateurs × 3 standards × 2 build types à chaque merge request est excessif. Les différences entre GCC 14 et GCC 15 sont marginales pour la détection de bugs dans le code du projet — tester les deux sur chaque PR gaspille des ressources sans gain significatif. Réservez les tests multi-versions aux builds nocturnes.

### Matrice sans sanitizers

```yaml
# ❌ Anti-pattern : matrice large mais sans sanitizers
matrix:
  compiler: [gcc, clang]
  std: ['17', '20', '23']
  build_type: [Debug, Release]
# 12 combinaisons, mais aucun sanitizer
```

Douze combinaisons qui ne testent que la compilation et l'exécution classique. Un seul job ASan aurait plus de valeur que six de ces douze combinaisons pour la détection de bugs. La matrice doit inclure au moins ASan+UBSan — quitte à réduire le nombre de combinaisons compilateur/standard pour compenser.

### Duplication matrice build + matrice test

```yaml
# ❌ Anti-pattern : la matrice test duplique la matrice build
build:
  strategy:
    matrix:
      compiler: [gcc, clang]
      build_type: [Debug, Release]

test:
  needs: [build]
  strategy:
    matrix:
      compiler: [gcc, clang]
      build_type: [Debug, Release]
```

Sur GitHub Actions, le job `test` ne sait pas quel artifact downloader car la matrice du job `build` n'est pas directement liée à celle du job `test`. Chaque instance de `test` downloade un artifact par nom, et ce nom doit correspondre. La solution est soit de fusionner build et test dans le même job (plus simple mais pas de séparation), soit d'utiliser un nommage d'artifact qui inclut les paramètres de la matrice et de s'assurer que les deux matrices sont alignées.

## Monitoring de la matrice

À mesure que le projet évolue, la matrice doit être révisée périodiquement. Quelques signaux indiquent qu'un ajustement est nécessaire :

**Temps de pipeline en croissance constante** — Le projet grossit, la compilation prend plus de temps, les combinaisons s'accumulent. Réévaluez si chaque combinaison apporte encore de la valeur. Supprimez les combinaisons qui n'ont jamais détecté de bug en 6 mois.

**Combinaisons toujours vertes** — Une combinaison qui n'a jamais échoué en 6 mois ne détecte probablement rien que les autres combinaisons ne détectent déjà. Candidate à la suppression ou au déplacement vers le build nocturne.

**Combinaisons toujours rouges** — Une combinaison qui échoue systématiquement (par exemple C++23 avec une fonctionnalité pas encore supportée par le compilateur) pollue les résultats et désensibilise l'équipe aux échecs. Corrigez le problème ou désactivez temporairement la combinaison.

**Feedback trop lent sur les PR** — Si les développeurs attendent plus de 10 minutes pour le résultat de leur merge request, la matrice MR est trop large. Réduisez aux combinaisons essentielles et déplacez le reste vers `main`.

---

> 📎 *Ce chapitre a couvert l'intégralité du cycle CI/CD pour un projet C++ : structure des pipelines (38.1-38.2), accélération de la compilation (38.3), automatisation complète (38.4), gestion des releases (38.5), cross-compilation (38.6), et matrix builds (38.7). Les chapitres suivants couvrent le packaging et la distribution (chapitre 39) et l'observabilité (chapitre 40).*

⏭️ [Packaging et Distribution](/39-packaging/README.md)
