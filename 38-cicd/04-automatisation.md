🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 38.4 Automatisation : Tests, analyse statique, déploiement

## Introduction

Les sections précédentes ont détaillé la structure des pipelines (38.1, 38.2) et l'accélération de la compilation (38.3). Ce socle technique est nécessaire mais pas suffisant : un pipeline CI/CD mature ne se contente pas de compiler et de lancer les tests — il orchestre un ensemble de vérifications automatisées qui, collectivement, garantissent que chaque changement poussé dans le dépôt respecte les standards de qualité du projet.

Cette section adopte une perspective transversale. Plutôt que de détailler à nouveau la syntaxe des jobs (couverte en 38.1.2 et 38.2.1), elle se concentre sur les **stratégies d'automatisation** : quelles vérifications déclencher, à quel moment du cycle de développement, avec quelle granularité, et comment orchestrer le feedback vers les développeurs. L'objectif est de construire un système où la qualité est vérifiée automatiquement à chaque étape, sans intervention humaine et sans ralentir le flux de développement.

## Les trois piliers de l'automatisation CI/CD en C++

Un pipeline C++ complet automatise trois catégories de vérifications, chacune avec ses propres contraintes de temps et de ressources :

```
                    Coût         Fréquence         Feedback
                    ────         ─────────         ────────
 Analyse statique   Faible       Chaque commit     Immédiat (secondes)
 Tests              Moyen        Chaque commit     Rapide (minutes)
 Déploiement        Élevé        Tags / main       Différé (minutes)

 ┌─────────────────────────────────────────────────────────────────┐
 │                    CHAQUE COMMIT / MR                           │
 │  ┌──────────────┐  ┌───────────────┐  ┌──────────────────────┐  │
 │  │  Formatage   │  │ Analyse       │  │ Tests unitaires      │  │
 │  │  clang-format│  │ statique      │  │ Tests d'intégration  │  │
 │  │              │  │ clang-tidy    │  │ Sanitizers           │  │
 │  └──────────────┘  └───────────────┘  └──────────────────────┘  │
 └─────────────────────────────────────────────────────────────────┘
 ┌─────────────────────────────────────────────────────────────────┐
 │                    BRANCHE PRINCIPALE                           │
 │  ┌──────────────┐  ┌───────────────┐  ┌──────────────────────┐  │
 │  │  Matrice     │  │ Couverture    │  │ Benchmarks           │  │
 │  │  complète    │  │ de code       │  │ de performance       │  │
 │  └──────────────┘  └───────────────┘  └──────────────────────┘  │
 └─────────────────────────────────────────────────────────────────┘
 ┌─────────────────────────────────────────────────────────────────┐
 │                    TAGS DE VERSION                              │
 │  ┌──────────────┐  ┌───────────────┐  ┌──────────────────────┐  │
 │  │  Packaging   │  │ Publication   │  │ Déploiement          │  │
 │  │  DEB/RPM     │  │ Docker/       │  │ Documentation        │  │
 │  │              │  │ Release       │  │                      │  │
 │  └──────────────┘  └───────────────┘  └──────────────────────┘  │
 └─────────────────────────────────────────────────────────────────┘
```

La clé est de **calibrer le niveau de vérification au contexte** : un commit sur une branche de feature ne nécessite pas la matrice complète de tests, tandis qu'un tag de version doit passer par toutes les étapes avant de produire un livrable.

## Automatisation des tests

### Stratégie de tests en couches

Un projet C++ mature possède plusieurs niveaux de tests, chacun avec un coût et une couverture différents. L'automatisation CI les organise en couches, des plus rapides aux plus coûteux :

**Couche 1 — Tests unitaires** (quelques secondes à 2 minutes). Ce sont les tests Google Test classiques qui vérifient des fonctions et des classes isolément. Ils s'exécutent sur chaque commit et chaque merge request, sur toutes les combinaisons de la matrice. Leur rapidité permet un feedback quasi-immédiat.

**Couche 2 — Tests d'intégration** (2 à 10 minutes). Ces tests vérifient l'interaction entre composants : lecture/écriture de fichiers réels, communication réseau locale, intégration avec des bases de données. Ils sont plus lents et peuvent nécessiter des services externes (un conteneur PostgreSQL, un serveur Redis). Sur les merge requests, seuls les tests d'intégration liés aux fichiers modifiés sont exécutés. Sur `main`, tous sont exécutés.

**Couche 3 — Tests avec sanitizers** (5 à 30 minutes). Les builds ASan, TSan et UBSan détectent des bugs invisibles aux tests classiques. Le surcoût d'instrumentation ralentit l'exécution de 2x à 10x. Sur les merge requests, un seul build sanitizer (ASan+UBSan) est suffisant. Sur `main` et les tags, les trois sanitizers sont exécutés.

**Couche 4 — Tests de performance / Benchmarks** (10 à 30 minutes). Les benchmarks Google Benchmark détectent les régressions de performance. Ils nécessitent un environnement stable (pas de co-location avec d'autres jobs) et une baseline de référence. Ils s'exécutent uniquement sur `main` ou sur demande manuelle.

### Exécution conditionnelle par contexte

La mise en œuvre de cette stratégie en couches repose sur l'exécution conditionnelle. Voici le pattern sur GitHub Actions :

```yaml
jobs:
  unit-tests:
    # Couche 1 : toujours
    if: always()
    needs: [build]
    strategy:
      matrix:
        compiler: [gcc, clang]
    steps:
      - # ... exécution CTest

  integration-tests:
    # Couche 2 : MR (si fichiers pertinents changés) + main
    if: |
      github.event_name == 'push' && github.ref == 'refs/heads/main' ||
      github.event_name == 'pull_request'
    needs: [build]
    services:
      postgres:
        image: postgres:16
        env:
          POSTGRES_PASSWORD: test
        ports: ['5432:5432']
    steps:
      - # ... exécution des tests d'intégration

  sanitizer-tests:
    # Couche 3 : ASan sur les MR, tous les sanitizers sur main
    needs: [build-asan]
    steps:
      - # ... exécution avec ASAN_OPTIONS

  benchmarks:
    # Couche 4 : uniquement sur main ou déclenchement manuel
    if: |
      github.ref == 'refs/heads/main' ||
      github.event_name == 'workflow_dispatch'
    needs: [build]
    runs-on: [self-hosted, benchmark]    # Runner dédié, isolé
    steps:
      - # ... exécution Google Benchmark
```

Sur GitLab CI, le même pattern utilise les directives `rules:` :

```yaml
unit-tests:
  stage: test
  rules:
    - if: '$CI_PIPELINE_SOURCE == "merge_request_event"'
    - if: '$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH'
    - if: '$CI_COMMIT_TAG'

benchmarks:
  stage: test
  rules:
    - if: '$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH'
    - if: '$CI_PIPELINE_SOURCE == "schedule"'
    - when: manual
      allow_failure: true
  tags:
    - benchmark-runner
```

La directive `when: manual` sur GitLab CI ajoute un bouton dans l'interface du pipeline, permettant de déclencher les benchmarks à la demande sur n'importe quelle branche. `allow_failure: true` empêche le job manuel de bloquer le pipeline s'il n'est pas exécuté.

### Tests d'intégration avec services

Les tests d'intégration C++ nécessitent souvent des services externes. Les deux plateformes supportent le lancement de conteneurs de services aux côtés du job :

```yaml
# GitHub Actions
jobs:
  integration-tests:
    runs-on: ubuntu-latest
    services:
      redis:
        image: redis:7
        ports: ['6379:6379']
        options: >-
          --health-cmd "redis-cli ping"
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5
      postgres:
        image: postgres:16
        env:
          POSTGRES_DB: testdb
          POSTGRES_USER: test
          POSTGRES_PASSWORD: test
        ports: ['5432:5432']
        options: >-
          --health-cmd pg_isready
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5
    env:
      REDIS_HOST: localhost
      POSTGRES_HOST: localhost
      POSTGRES_PORT: 5432
    steps:
      - uses: actions/checkout@v4
      - uses: actions/download-artifact@v4
        with:
          name: build-gcc-release
          path: build/
      - run: chmod +x build/tests/*
      - name: Run integration tests
        run: |
          cd build
          ctest --parallel $(nproc) --timeout 120 \
            -L integration                          # Label CTest
```

```yaml
# GitLab CI
integration-tests:
  stage: test
  services:
    - name: redis:7
      alias: redis
    - name: postgres:16
      alias: postgres
      variables:
        POSTGRES_DB: testdb
        POSTGRES_USER: test
        POSTGRES_PASSWORD: test
  variables:
    REDIS_HOST: redis
    POSTGRES_HOST: postgres
  script:
    - cd ${BUILD_DIR}
    - ctest --parallel $(nproc) --timeout 120 -L integration
```

**Le label CTest `-L integration`** filtre les tests par catégorie. Dans le `CMakeLists.txt`, les tests sont tagués lors de leur déclaration :

```cmake
add_test(NAME test_db_connection COMMAND test_integration_db)  
set_tests_properties(test_db_connection PROPERTIES LABELS "integration")  

add_test(NAME test_core_logic COMMAND test_unit_core)  
set_tests_properties(test_core_logic PROPERTIES LABELS "unit")  
```

Cette catégorisation permet au pipeline d'exécuter `ctest -L unit` pour les tests rapides et `ctest -L integration` pour les tests nécessitant des services, sans modifier le code de test.

### Gestion des tests flaky

Les tests intermittents (*flaky tests*) sont un fléau en CI, particulièrement pour les projets C++ avec du code concurrent. Un test qui échoue aléatoirement dans 5% des exécutions érode la confiance dans le pipeline et pousse les développeurs à ignorer les échecs.

**Détection.** CTest peut relancer automatiquement les tests échoués :

```yaml
script:
  - cd build
  - ctest --parallel $(nproc) --timeout 120 --repeat until-pass:3
```

`--repeat until-pass:3` relance un test échoué jusqu'à 3 fois. Si le test finit par passer, il est marqué comme réussi mais CTest note la relance. C'est un filet de sécurité à court terme, pas une solution — un test qui nécessite des retries doit être investigué et corrigé.

**Quarantaine.** Une approche plus rigoureuse consiste à isoler les tests flaky identifiés dans un label CTest dédié et à les exécuter dans un job séparé avec `allow_failure: true` :

```yaml
flaky-tests:
  stage: test
  allow_failure: true
  script:
    - cd ${BUILD_DIR}
    - ctest -L flaky --repeat until-pass:3 --timeout 120
  rules:
    - if: '$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH'
```

Le job n'est pas bloquant, mais sa présence dans le pipeline rappelle que des tests instables existent et doivent être corrigés.

## Automatisation de l'analyse statique

### Analyse incrémentale vs analyse complète

L'analyse statique avec clang-tidy est potentiellement coûteuse : sur un projet de 200 fichiers, une analyse complète peut prendre 5 à 15 minutes. Deux stratégies d'automatisation coexistent :

**Analyse incrémentale sur les merge requests.** Seuls les fichiers modifiés (et éventuellement les fichiers qui les incluent) sont analysés. Le feedback est rapide et ciblé. C'est la stratégie recommandée pour le workflow quotidien :

```yaml
# GitLab CI
clang-tidy-diff:
  stage: lint
  script:
    - cmake -B build -G Ninja
        -DCMAKE_CXX_COMPILER=clang++-20
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    - |
      git fetch origin ${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}
      CHANGED=$(git diff --name-only --diff-filter=ACMR \
        origin/${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}...HEAD \
        -- '*.cpp' '*.hpp' '*.h')
      [ -z "$CHANGED" ] && { echo "Aucun fichier C++ modifié."; exit 0; }
      echo "$CHANGED" | xargs -P $(nproc) clang-tidy-20 \
        -p build/compile_commands.json \
        --warnings-as-errors='*'
  rules:
    - if: '$CI_PIPELINE_SOURCE == "merge_request_event"'
```

**Analyse complète sur la branche principale et en nightly.** L'intégralité du code source est analysée. Cela détecte les problèmes dans les fichiers qui ne sont jamais modifiés (legacy code) et les problèmes inter-fichiers. Cette analyse est plus lente et s'exécute en arrière-plan :

```yaml
clang-tidy-full:
  stage: lint
  script:
    - cmake -B build -G Ninja
        -DCMAKE_CXX_COMPILER=clang++-20
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    - run-clang-tidy-20 -p build -j $(nproc) -quiet
  rules:
    - if: '$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH'
    - if: '$CI_PIPELINE_SOURCE == "schedule"'
  allow_failure: true
```

L'outil `run-clang-tidy` (fourni avec LLVM) orchestre l'exécution de clang-tidy sur tous les fichiers listés dans la compilation database, en parallèle. C'est plus efficace qu'un `find | xargs` car il gère nativement le parallélisme et les erreurs.

`allow_failure: true` sur l'analyse complète est un choix pragmatique pour les projets avec une base de code existante qui n'est pas encore entièrement conforme. L'analyse complète sert alors d'indicateur de progrès plutôt que de gate bloquante.

### Automatisation du formatage

Deux philosophies s'opposent pour le formatage automatique :

**Vérification stricte (gate).** Le pipeline vérifie le formatage et échoue si des fichiers ne sont pas conformes. Le développeur doit corriger localement avant de repousser :

```yaml
format-check:
  stage: lint
  script:
    - find src include tests -type f \( -name '*.cpp' -o -name '*.hpp' \) \
      | xargs clang-format-20 --dry-run --Werror
```

**Correction automatique (auto-fix).** Le pipeline reformate les fichiers et pousse un commit de correction automatiquement. Cette approche est plus confortable pour les développeurs mais introduit des commits automatiques dans l'historique :

```yaml
# GitHub Actions — auto-format et commit
format-fix:
  runs-on: ubuntu-latest
  if: github.event_name == 'pull_request'
  permissions:
    contents: write
  steps:
    - uses: actions/checkout@v4
      with:
        ref: ${{ github.head_ref }}
        token: ${{ secrets.GITHUB_TOKEN }}

    - name: Run clang-format
      run: |
        find src include tests -type f \( -name '*.cpp' -o -name '*.hpp' \) \
          | xargs clang-format-20 -i

    - name: Commit changes
      run: |
        git config user.name "CI Bot"
        git config user.email "ci@exemple.com"
        git diff --quiet && exit 0    # Rien à committer
        git add -A
        git commit -m "style: auto-format with clang-format"
        git push
```

> ⚠️ **Attention avec l'auto-fix.** Un commit automatique déclenche un nouveau run du pipeline, créant potentiellement une boucle. La plupart des plateformes détectent les commits poussés par le token CI et ne redéclenchent pas le workflow, mais vérifiez ce comportement sur votre configuration.

**Recommandation** : la vérification stricte est préférable. Combinée avec des pre-commit hooks (chapitre 47), elle garantit que le code est correctement formaté avant même d'atteindre le pipeline CI, éliminant le besoin de corrections automatiques.

### Couverture de code automatisée

La mesure automatique de la couverture de code complète l'analyse statique en identifiant les parties du code qui ne sont pas exercées par les tests. L'intégration dans le pipeline suit un pattern standard :

```yaml
# GitLab CI
coverage:
  stage: test
  variables:
    CXX: "g++-15"
    CC: "gcc-15"
  script:
    - cmake -B build-cov
        -G Ninja
        -DCMAKE_BUILD_TYPE=Debug
        -DCMAKE_CXX_COMPILER=${CXX}
        -DCMAKE_CXX_FLAGS="--coverage"
        -DCMAKE_EXE_LINKER_FLAGS="--coverage"
    - cmake --build build-cov --parallel $(nproc)
    - cd build-cov && ctest --parallel $(nproc)
    - cd ..
    - gcovr --xml-pretty
        --exclude 'tests/'
        --exclude 'build-cov/_deps/'
        -r . --object-directory build-cov
        -o coverage.xml
    - gcovr --html-details
        --exclude 'tests/'
        --exclude 'build-cov/_deps/'
        -r . --object-directory build-cov
        -o coverage-report/index.html
  coverage: '/^TOTAL.*\s+(\d+(?:\.\d+)?)%$/'
  artifacts:
    reports:
      coverage_report:
        coverage_format: cobertura
        path: coverage.xml
    paths:
      - coverage-report/
    expire_in: 7 days
  rules:
    - if: '$CI_PIPELINE_SOURCE == "merge_request_event"'
    - if: '$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH'
```

Plusieurs éléments méritent attention :

**`coverage: '/regex/'`** est une directive GitLab CI qui extrait le pourcentage de couverture des logs du job et l'affiche dans la merge request sous forme de badge. La regex doit correspondre à la sortie de gcovr.

**`artifacts: reports: coverage_report:`** au format Cobertura permet à GitLab d'afficher la couverture ligne par ligne directement dans le diff de la merge request. Chaque ligne modifiée est annotée en vert (couverte) ou en rouge (non couverte).

**Le rapport HTML** (`coverage-report/`) est archivé comme artifact téléchargeable pour une inspection détaillée.

**Build séparé pour la couverture.** Le build de couverture (`build-cov`) est distinct des builds standard car les flags `--coverage` modifient le code généré et ralentissent l'exécution. Ne pas mélanger le build de couverture avec les builds normaux — ils ont des finalités différentes.

### Seuils de couverture et quality gates

Un pipeline peut échouer si la couverture descend en dessous d'un seuil défini :

```yaml
    - name: Check coverage threshold
      run: |
        COVERAGE=$(gcovr --print-summary -r . --object-directory build-cov \
          | grep 'lines:' | awk '{print $2}' | tr -d '%')
        echo "Couverture : ${COVERAGE}%"
        if (( $(echo "$COVERAGE < 80.0" | bc -l) )); then
          echo "ERREUR : Couverture (${COVERAGE}%) inférieure au seuil de 80%"
          exit 1
        fi
```

Ce type de *quality gate* est un outil puissant mais à utiliser avec discernement. Un seuil trop élevé (95%) pousse les développeurs à écrire des tests sans valeur pour satisfaire la métrique. Un seuil raisonnable (70-80%) pour les nouvelles lignes de code est plus pragmatique. Certaines équipes préfèrent un seuil sur le diff (la couverture des nouvelles lignes doit être ≥ 80%) plutôt qu'un seuil global, ce qui évite de pénaliser un projet qui hérite d'une base de code peu testée.

## Automatisation du déploiement

### Déploiement déclenché par les tags

Le pattern le plus courant pour les projets C++ est le déploiement sur tag de version. Un push de tag `v1.2.3` déclenche une chaîne complète : build Release → tests complets → packaging → publication :

```yaml
# GitHub Actions — workflow dédié release
name: Release

on:
  push:
    tags: ['v[0-9]+.[0-9]+.[0-9]+*']    # v1.2.3, v1.2.3-rc1

env:
  BUILD_DIR: build
  BUILD_TYPE: Release

permissions:
  contents: write
  packages: write

jobs:
  build-release:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0                 # Historique complet pour git describe

      - name: Extract version from tag
        id: version
        run: |
          VERSION="${GITHUB_REF_NAME#v}"
          echo "version=${VERSION}" >> "$GITHUB_OUTPUT"
          echo "Building version ${VERSION}"

      - name: Setup toolchain
        uses: aminya/setup-cpp@v1
        with:
          compiler: gcc-15
          cmake: true
          ninja: true
          ccache: true

      - name: Setup ccache
        uses: hendrikmuhs/ccache-action@v1
        with:
          key: release
          max-size: 2G

      - name: Build
        run: |
          cmake -B ${{ env.BUILD_DIR }} \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=${{ env.BUILD_TYPE }} \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
            -DPROJECT_VERSION=${{ steps.version.outputs.version }}
          cmake --build ${{ env.BUILD_DIR }} --parallel $(nproc)

      - name: Run full test suite
        working-directory: ${{ env.BUILD_DIR }}
        run: ctest --parallel $(nproc) --timeout 120 --output-on-failure

      - name: Upload build
        uses: actions/upload-artifact@v4
        with:
          name: release-binaries
          path: ${{ env.BUILD_DIR }}/bin/

  package-deb:
    needs: [build-release]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/download-artifact@v4
        with:
          name: release-binaries
          path: build/bin/
      - name: Build DEB package
        run: |
          VERSION="${GITHUB_REF_NAME#v}"
          # ... (logique de packaging DEB — voir section 38.1.2)
      - uses: actions/upload-artifact@v4
        with:
          name: deb-package
          path: '*.deb'

  docker-image:
    needs: [build-release]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/download-artifact@v4
        with:
          name: release-binaries
          path: build/bin/
      - uses: docker/login-action@v3
        with:
          registry: ghcr.io
          username: ${{ github.actor }}
          password: ${{ secrets.GITHUB_TOKEN }}
      - uses: docker/build-push-action@v6
        with:
          context: .
          file: Dockerfile.runtime
          push: true
          tags: |
            ghcr.io/${{ github.repository }}:${{ github.ref_name }}
            ghcr.io/${{ github.repository }}:latest

  publish-release:
    needs: [package-deb, docker-image]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/download-artifact@v4
        with:
          name: deb-package
      - uses: softprops/action-gh-release@v2
        with:
          files: '*.deb'
          generate_release_notes: true
          prerelease: ${{ contains(github.ref_name, 'rc') }}
```

Ce workflow illustre un **pipeline de release en 4 étapes** :

1. **Build + tests complets** — Le code est compilé en Release et la suite de tests complète est exécutée. Si un test échoue, aucun artifact n'est produit.
2. **Packaging DEB** — Le paquet Debian est construit à partir des binaires validés.
3. **Image Docker** — L'image de production est construite et poussée sur le registry.
4. **Publication** — La release GitHub est créée avec le paquet DEB et les notes de release auto-générées.

Les étapes 2 et 3 s'exécutent en parallèle (elles dépendent toutes deux de l'étape 1 mais pas l'une de l'autre). L'étape 4 attend la fin des deux.

### Déploiement continu de la documentation

La documentation d'un projet C++ (générée par Doxygen, mdBook, ou Sphinx) peut être automatiquement publiée à chaque merge sur `main` :

```yaml
# GitLab CI — déploiement GitLab Pages
pages:
  stage: deploy
  script:
    - apt-get update && apt-get install -y doxygen graphviz
    - cmake -B build-docs -G Ninja -DBUILD_DOCS=ON
    - cmake --build build-docs --target docs
    - mv build-docs/docs/html public
  artifacts:
    paths:
      - public
  rules:
    - if: '$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH'
      changes:
        - 'src/**'
        - 'include/**'
        - 'docs/**'
        - 'Doxyfile'
```

```yaml
# GitHub Actions — déploiement GitHub Pages
deploy-docs:
  runs-on: ubuntu-latest
  if: github.ref == 'refs/heads/main'
  permissions:
    pages: write
    id-token: write
  environment:
    name: github-pages
    url: ${{ steps.deployment.outputs.page_url }}
  steps:
    - uses: actions/checkout@v4
    - name: Install Doxygen
      run: sudo apt-get install -y doxygen graphviz
    - name: Generate docs
      run: |
        cmake -B build-docs -G Ninja -DBUILD_DOCS=ON
        cmake --build build-docs --target docs
    - uses: actions/upload-pages-artifact@v3
      with:
        path: build-docs/docs/html
    - id: deployment
      uses: actions/deploy-pages@v4
```

La directive `changes:` sur GitLab CI limite le redéploiement aux commits qui modifient effectivement le code source ou la documentation. Un commit qui ne touche que le CI ou le README ne déclenche pas de rebuild Doxygen inutile.

## Orchestration et notifications

### Feedback sur les merge/pull requests

Le pipeline CI est d'autant plus efficace que son feedback est visible et exploitable. Les deux plateformes offrent des mécanismes pour enrichir les merge/pull requests avec les résultats du pipeline :

**Rapports de test.** Les rapports JUnit intégrés affichent directement dans la MR quels tests ont échoué, avec le message d'erreur et la durée (voir section 38.1.2 pour GitLab et 38.2.2 pour GitHub).

**Couverture de code.** L'annotation Cobertura dans le diff de la MR montre la couverture ligne par ligne sur le code modifié.

**Annotations clang-tidy.** L'action `cpp-linter/cpp-linter-action` (GitHub) ou l'intégration Code Quality de GitLab postent les avertissements clang-tidy comme annotations sur les lignes de code concernées.

**Résumé du pipeline.** Un commentaire automatique peut synthétiser les résultats :

```yaml
# GitHub Actions
- name: Post pipeline summary
  if: github.event_name == 'pull_request' && always()
  uses: actions/github-script@v7
  with:
    script: |
      const summary = `## Résumé CI
      | Vérification | Statut |
      |-------------|--------|
      | Formatage | ${{ needs.lint.result == 'success' && '✅' || '❌' }} |
      | Analyse statique | ${{ needs.lint.result == 'success' && '✅' || '❌' }} |
      | Build GCC | ${{ needs.build.result == 'success' && '✅' || '❌' }} |
      | Tests | ${{ needs.test.result == 'success' && '✅' || '❌' }} |
      | Sanitizers | ${{ needs.sanitizers.result == 'success' && '✅' || '❌' }} |`;
      github.rest.issues.createComment({
        owner: context.repo.owner,
        repo: context.repo.repo,
        issue_number: context.issue.number,
        body: summary
      });
```

### Notifications sur échec

Pour les branches protégées (`main`, `release`), un échec de pipeline doit être signalé immédiatement à l'équipe. Les deux plateformes supportent les notifications par email nativement, mais les intégrations Slack ou Microsoft Teams sont plus réactives :

```yaml
# GitLab CI — notification Slack sur échec de main
notify-failure:
  stage: deploy
  script:
    - |
      curl -X POST "$SLACK_WEBHOOK_URL" \
        -H 'Content-type: application/json' \
        -d "{
          \"text\": \"🔴 Pipeline échoué sur ${CI_COMMIT_BRANCH}\",
          \"blocks\": [{
            \"type\": \"section\",
            \"text\": {
              \"type\": \"mrkdwn\",
              \"text\": \"*Pipeline échoué* sur \`${CI_COMMIT_BRANCH}\`\n\
              Commit: ${CI_COMMIT_SHORT_SHA} par ${GITLAB_USER_NAME}\n\
              <${CI_PIPELINE_URL}|Voir le pipeline>\"
            }
          }]
        }"
  rules:
    - if: '$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH'
      when: on_failure
```

La directive `when: on_failure` garantit que ce job ne s'exécute que lorsque le pipeline échoue — pas sur les exécutions réussies.

### Builds nocturnes (nightly)

Un build nocturne exécute l'intégralité des vérifications sans contrainte de temps, servant de filet de sécurité complet :

```yaml
# GitLab CI — pipeline nocturne complet
# Configuré dans Settings > CI/CD > Schedules
nightly-full-matrix:
  stage: build
  extends: .cmake_build
  parallel:
    matrix:
      - CXX: ["g++-15", "clang++-20"]
        BUILD_TYPE: ["Debug", "Release"]
        CPP_STANDARD: ["17", "20", "23"]
  rules:
    - if: '$CI_PIPELINE_SOURCE == "schedule"'
```

```yaml
# GitHub Actions
name: Nightly  
on:  
  schedule:
    - cron: '0 3 * * *'    # Chaque nuit à 3h

jobs:
  full-matrix:
    strategy:
      fail-fast: false
      matrix:
        compiler: [gcc, clang]
        build_type: [Debug, Release]
        cpp_standard: ['17', '20', '23']
        include:
          - compiler: gcc
            cxx: g++-15
          - compiler: clang
            cxx: clang++-20
    # ... (build + test complet)
```

Le build nocturne couvre les combinaisons que le CI quotidien omet pour rester rapide : standards C++ anciens (C++17), builds Debug Clang, sanitizers complets (TSan en plus d'ASan). Si le nightly échoue, l'équipe est notifiée le matin et peut corriger avant que le problème n'affecte le workflow quotidien.

## Résumé : matrice de vérifications par contexte

| Vérification | MR / PR | Push sur `main` | Tag de version | Nightly |
|-------------|---------|-----------------|----------------|---------|
| clang-format | ✅ (strict) | ✅ | ✅ | ✅ |
| clang-tidy (diff) | ✅ | — | — | — |
| clang-tidy (complet) | — | ✅ | ✅ | ✅ |
| Build GCC Release | ✅ | ✅ | ✅ | ✅ |
| Build Clang Release | ✅ | ✅ | ✅ | ✅ |
| Build GCC Debug | ❌ | ✅ | ✅ | ✅ |
| Build Clang Debug | ❌ | ❌ | ❌ | ✅ |
| Tests unitaires | ✅ | ✅ | ✅ | ✅ |
| Tests d'intégration | ✅ (ciblés) | ✅ (complets) | ✅ (complets) | ✅ |
| ASan + UBSan | ✅ | ✅ | ✅ | ✅ |
| TSan | ❌ | ✅ | ✅ | ✅ |
| Couverture de code | ✅ | ✅ | ❌ | ✅ |
| Benchmarks | ❌ | ❌ | ❌ | ✅ |
| Build C++17/23 | ❌ | ❌ | ❌ | ✅ |
| Packaging DEB | ❌ | ❌ | ✅ | ❌ |
| Image Docker | ❌ | ❌ | ✅ | ❌ |
| Release / Publication | ❌ | ❌ | ✅ | ❌ |
| Documentation | ❌ | ✅ | ✅ | ❌ |

Ce tableau est un point de départ. Chaque projet doit calibrer cette matrice en fonction de sa taille, de ses contraintes de temps, et de ses ressources CI. Le principe directeur est le **feedback proportionnel** : plus le changement est proche de la production, plus les vérifications sont exhaustives.

---

> **Section suivante** : 38.5 Artifacts et gestion des releases — Génération, versioning et publication des livrables : paquets DEB/RPM, binaires statiques, images Docker, et releases GitHub/GitLab.

⏭️ [Artifacts et gestion des releases](/38-cicd/05-artifacts-releases.md)
