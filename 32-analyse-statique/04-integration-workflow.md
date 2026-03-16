🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 32.4 — Intégration dans le workflow de développement

## Introduction

Les sections précédentes ont présenté trois outils — clang-tidy, cppcheck et clang-format — individuellement. Cette section les assemble en une chaîne de qualité cohérente, intégrée aux gestes quotidiens du développeur : écrire du code, sauvegarder, committer, pousser. L'objectif est que l'analyse statique et le formatage soient des **filets de sécurité automatiques** qui fonctionnent sans effort conscient, pas des corvées manuelles que l'on oublie sous la pression d'une deadline.

L'intégration s'organise en trois couches, de la plus immédiate à la plus tardive :

1. **L'IDE** : feedback en temps réel pendant l'écriture du code.
2. **Les pre-commit hooks** : vérification automatique avant chaque commit.
3. **La CI** : validation systématique sur la branche partagée.

Chaque couche attrape ce qui a échappé à la précédente. Un développeur dont l'IDE est correctement configuré ne déclenchera presque jamais le pre-commit hook. Un développeur dont le pre-commit hook fonctionne ne déclenchera presque jamais le job CI. Mais si l'IDE n'est pas configuré (nouveau membre de l'équipe, environnement inhabituel), les couches suivantes garantissent que le code non conforme n'atteint jamais la branche principale.

---

## Couche 1 : Intégration dans l'IDE

### clangd : le socle unifié

`clangd`, le serveur LSP (*Language Server Protocol*) de LLVM, est le point d'intégration central. Il fournit simultanément l'auto-complétion, la navigation dans le code, les diagnostics du compilateur, **et** les diagnostics clang-tidy — le tout en temps réel pendant l'édition. Un seul outil remplace une demi-douzaine de plugins.

clangd lit automatiquement le fichier `.clang-tidy` du projet et exécute les checks configurés sur chaque fichier ouvert. Les diagnostics apparaissent sous forme de soulignements dans l'éditeur, avec des tooltips décrivant le problème et des *quick fixes* pour les checks qui proposent une correction automatique.

#### Configuration de clangd

Le fichier `.clangd` (distinct de `.clang-tidy`) configure le comportement du serveur LSP lui-même :

```yaml
# .clangd
CompileFlags:
  CompilationDatabase: build/

Diagnostics:
  ClangTidy:
    Add:
      - bugprone-*
      - performance-*
      - modernize-*
    Remove:
      - modernize-use-trailing-return-type
  UnusedIncludes: Strict

InlayHints:
  Enabled: true
  ParameterNames: true
  DeducedTypes: true
```

La clé `CompilationDatabase` indique à clangd où trouver le `compile_commands.json`. La section `Diagnostics.ClangTidy` peut compléter ou overrider les checks du fichier `.clang-tidy` — mais en pratique, il est préférable de centraliser la configuration dans `.clang-tidy` pour que la ligne de commande et l'IDE utilisent les mêmes règles.

La fonctionnalité `UnusedIncludes: Strict` est un ajout récent de clangd : elle signale les `#include` inutilisés directement dans l'éditeur, un diagnostic précieux pour la propreté du code.

#### VS Code

```json
// .vscode/settings.json
{
    // clangd comme serveur LSP (désactiver l'extension Microsoft C/C++ si installée)
    "clangd.path": "/usr/bin/clangd",
    "clangd.arguments": [
        "--background-index",
        "--clang-tidy",
        "--header-insertion=iwyu",
        "--completion-style=detailed"
    ],

    // Formatage automatique à la sauvegarde
    "editor.formatOnSave": true,
    "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd",

    // Formatage uniquement des lignes modifiées (évite les diffs massifs)
    "editor.formatOnSaveMode": "modifications"
}
```

L'option `"editor.formatOnSaveMode": "modifications"` est particulièrement importante sur un projet existant : seules les lignes modifiées par le développeur sont reformatées, ce qui évite de polluer les diffs avec des changements de formatage sur du code non touché.

#### CLion

CLion intègre nativement clang-tidy et clang-format. Dans `Settings → Editor → Inspections → C/C++ → General → Clang-Tidy`, activez les checks souhaités ou sélectionnez « Use .clang-tidy config ». Pour clang-format : `Settings → Editor → Code Style → C/C++ → Enable ClangFormat`.

CLion affiche les diagnostics clang-tidy comme des inspections intégrées, avec des quick fixes accessibles via `Alt+Enter`.

#### Neovim (avec nvim-lspconfig)

```lua
-- init.lua (extrait)
require('lspconfig').clangd.setup {
    cmd = {
        'clangd',
        '--background-index',
        '--clang-tidy',
        '--header-insertion=iwyu',
    },
    on_attach = function(client, bufnr)
        -- Formatage à la sauvegarde
        vim.api.nvim_create_autocmd('BufWritePre', {
            buffer = bufnr,
            callback = function()
                vim.lsp.buf.format({ async = false })
            end,
        })
    end,
}
```

---

## Couche 2 : Pre-commit hooks

Les pre-commit hooks sont des scripts exécutés automatiquement par Git avant chaque commit. Si le script retourne un code d'erreur non nul, le commit est bloqué. C'est le mécanisme idéal pour garantir que le code committé respecte les règles de formatage et passe les checks d'analyse statique de base.

### Le framework pre-commit

Le framework [pre-commit](https://pre-commit.com/) (couvert en détail au chapitre 47, sections 47.2–47.3) est l'outil standard pour gérer les hooks Git. Il se configure via un fichier `.pre-commit-config.yaml` à la racine du projet :

```yaml
# .pre-commit-config.yaml
repos:
  # clang-format : vérification du formatage
  - repo: https://github.com/pre-commit/mirrors-clang-format
    rev: v19.1.0
    hooks:
      - id: clang-format
        types_or: [c, c++]

  # clang-tidy : analyse statique (sur les fichiers modifiés)
  - repo: local
    hooks:
      - id: clang-tidy
        name: clang-tidy
        entry: bash -c 'clang-tidy -p build/ "$@"' --
        language: system
        types_or: [c, c++]
        # Ne s'exécute que si compile_commands.json existe
        require_serial: true

  # cppcheck : analyse complémentaire
  - repo: local
    hooks:
      - id: cppcheck
        name: cppcheck
        entry: cppcheck --enable=warning,performance --error-exitcode=1 --quiet --suppress=missingIncludeSystem
        language: system
        types_or: [c, c++]
```

Installation et activation :

```bash
pip install pre-commit  
pre-commit install  
```

Après cette configuration, chaque `git commit` exécute automatiquement :

1. **clang-format** sur les fichiers C++ modifiés. Si le formatage n'est pas conforme, le hook reformate les fichiers et bloque le commit. Le développeur n'a qu'à faire `git add` sur les fichiers reformatés puis recommiter.
2. **clang-tidy** sur les fichiers modifiés. Les diagnostics sont affichés et le commit est bloqué si des erreurs sont détectées.
3. **cppcheck** en complément sur les fichiers modifiés.

### Hooks légers vs exhaustifs

L'exécution des hooks doit rester rapide — idéalement sous 10 secondes — pour ne pas interrompre le flux de travail du développeur. Si les hooks sont trop lents, les développeurs les contournent avec `git commit --no-verify`, annulant leur utilité.

Stratégie recommandée :

| Hook | Exécution | Temps typique |
|---|---|---|
| clang-format | Sur les fichiers staged | < 1 seconde |
| clang-tidy (checks légers) | Sur les fichiers staged | 2–10 secondes |
| cppcheck | Sur les fichiers staged | 1–5 secondes |
| clang-tidy (clang-analyzer) | Réservé à la CI | Trop lent pour un hook |

Les checks `clang-analyzer-*` (analyse de chemin interprocédurale) sont trop lents pour un pre-commit hook. Réservez-les à la CI, où le temps d'exécution n'est pas un facteur bloquant.

Pour limiter les checks clang-tidy dans le hook à ceux qui sont rapides :

```yaml
  - repo: local
    hooks:
      - id: clang-tidy-fast
        name: clang-tidy (fast checks)
        entry: >
          bash -c 'clang-tidy -p build/
          -checks="-*,bugprone-*,performance-*,modernize-*,-clang-analyzer-*"
          "$@"' --
        language: system
        types_or: [c, c++]
```

### Hook clang-format avec correction automatique

Le hook clang-format peut être configuré pour corriger automatiquement les fichiers plutôt que simplement signaler les problèmes :

```yaml
  - repo: local
    hooks:
      - id: clang-format-fix
        name: clang-format (auto-fix)
        entry: bash -c 'clang-format -i "$@" && git add "$@"' --
        language: system
        types_or: [c, c++]
```

Ce hook formate les fichiers en place puis les ré-ajoute à l'index Git. Le commit inclut directement le code correctement formaté, sans intervention manuelle. C'est l'approche la plus fluide — le développeur ne voit jamais de rejet pour cause de formatage.

---

## Couche 3 : Pipeline CI

La CI est le gardien final. Même si l'IDE et les hooks couvrent la majorité des cas, la CI vérifie systématiquement chaque push sur la branche partagée. C'est la seule couche qui ne peut pas être contournée (contrairement aux hooks, désactivables avec `--no-verify`).

### Pipeline complet

```yaml
# .gitlab-ci.yml
stages:
  - quality
  - build
  - test

# ─── Formatage ────────────────────────────────────────────────
format:
  stage: quality
  image: ubuntu:24.04
  before_script:
    - apt-get update && apt-get install -y clang-format findutils
  script:
    - |
      ERRORS=$(find src/ include/ \( -name '*.cpp' -o -name '*.h' \) -print0 \
        | xargs -0 clang-format --dry-run --Werror 2>&1 || true)
      if [ -n "$ERRORS" ]; then
        echo "❌ Fichiers mal formatés détectés :"
        echo "$ERRORS"
        echo ""
        echo "Corrigez avec : find src/ include/ -name '*.cpp' -o -name '*.h' | xargs clang-format -i"
        exit 1
      fi
      echo "✅ Formatage OK"

# ─── Analyse statique clang-tidy ──────────────────────────────
clang-tidy:
  stage: quality
  image: ubuntu:24.04
  before_script:
    - apt-get update && apt-get install -y clang-tidy cmake g++ ninja-build
  script:
    - cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    - run-clang-tidy -p build/ -quiet 2>&1 | tee clang-tidy-report.txt
    - |
      if grep -q 'error:' clang-tidy-report.txt; then
        echo "❌ Erreurs clang-tidy détectées"
        exit 1
      fi
      echo "✅ clang-tidy OK"
  artifacts:
    when: always
    paths:
      - clang-tidy-report.txt
    expire_in: 7 days

# ─── Analyse statique cppcheck ────────────────────────────────
cppcheck:
  stage: quality
  image: ubuntu:24.04
  before_script:
    - apt-get update && apt-get install -y cppcheck cmake g++ ninja-build
  script:
    - cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    - cppcheck --project=build/compile_commands.json
        --enable=warning,performance
        --error-exitcode=1
        --suppress=missingIncludeSystem
        --xml 2> cppcheck-report.xml
  artifacts:
    when: always
    paths:
      - cppcheck-report.xml
    expire_in: 7 days

# ─── Build ────────────────────────────────────────────────────
build:
  stage: build
  needs: [format, clang-tidy, cppcheck]
  script:
    - cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    - cmake --build build

# ─── Tests ────────────────────────────────────────────────────
test:
  stage: test
  needs: [build]
  script:
    - ctest --test-dir build --output-on-failure
```

### Équivalent GitHub Actions

```yaml
# .github/workflows/quality.yml
name: Code Quality

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  format-check:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - name: Install clang-format
        run: sudo apt-get install -y clang-format
      - name: Check formatting
        run: |
          find src/ include/ \( -name '*.cpp' -o -name '*.h' \) -print0 \
            | xargs -0 clang-format --dry-run --Werror

  static-analysis:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - name: Install tools
        run: sudo apt-get install -y clang-tidy cppcheck cmake g++ ninja-build
      - name: Configure
        run: cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
      - name: clang-tidy
        run: run-clang-tidy -p build/ -quiet
      - name: cppcheck
        run: |
          cppcheck --project=build/compile_commands.json \
            --enable=warning,performance \
            --error-exitcode=1 \
            --suppress=missingIncludeSystem
```

### Stratégie de sévérité en CI

Tous les diagnostics ne méritent pas de bloquer le pipeline. Une stratégie en deux niveaux est recommandée :

**Bloquant (le pipeline échoue)** :
- Formatage non conforme (clang-format).
- Checks promus en `WarningsAsErrors` dans `.clang-tidy`.
- Erreurs cppcheck de niveau `error`.

**Non bloquant (le diagnostic est affiché mais le pipeline continue)** :
- Warnings clang-tidy non promus en erreur.
- Warnings et suggestions cppcheck de niveau `warning`/`performance`/`style`.

Cette séparation est gérée naturellement : les `WarningsAsErrors` dans `.clang-tidy` contrôlent quels checks bloquent, et `--error-exitcode=1` dans cppcheck ne déclenche le code d'erreur que pour les diagnostics de niveau `error`.

---

## Mise en place sur un projet existant

L'adoption de l'outillage complet sur un projet existant nécessite une stratégie progressive pour ne pas bloquer l'équipe.

### Étape 1 : clang-format (semaine 1)

Le formatage est le changement le moins risqué — il ne modifie pas la sémantique du code. C'est le point de départ idéal.

1. Choisissez un style de base (`Google`, `LLVM`, `Microsoft`) et créez le `.clang-format`.
2. Reformatez l'ensemble du projet en un seul commit dédié.
3. Ajoutez le hash de ce commit dans `.git-blame-ignore-revs` (section 32.3).
4. Activez le hook pre-commit clang-format.
5. Activez le job CI de vérification du formatage.

À partir de ce moment, tout nouveau code est automatiquement formaté.

### Étape 2 : clang-tidy minimal (semaines 2–3)

1. Créez le `.clang-tidy` avec le socle minimal (section 32.1.2) : `bugprone-*`, `clang-analyzer-core.*`, `performance-*` essentiels.
2. Exécutez `run-clang-tidy` sur le projet entier et corrigez les diagnostics — ils devraient être peu nombreux et chacun est probablement un vrai bug.
3. Promouvez ces checks en `WarningsAsErrors`.
4. Activez le job CI clang-tidy.

### Étape 3 : cppcheck (semaine 3)

1. Exécutez cppcheck sur le projet et corrigez les erreurs.
2. Créez le fichier de suppressions pour les faux positifs confirmés.
3. Activez le job CI cppcheck.

### Étape 4 : Enrichissement progressif (mois 2+)

Ajoutez progressivement les checks `modernize-*` et `readability-*` dans `.clang-tidy`, en corrigeant les diagnostics existants à chaque ajout (section 32.1.2, stratégie d'adoption). Chaque nouveau check est un petit commit : modification de `.clang-tidy` + corrections, facile à reviewer.

---

## Récapitulatif du chapitre 32

Ce chapitre a couvert les trois piliers de l'analyse statique et du formatage pour un projet C++ :

**clang-tidy** (section 32.1) est l'outil central d'analyse statique. Avec ses 400+ checks organisés en catégories, son intégration dans clangd pour le feedback temps réel, et sa capacité de correction automatique, il couvre le spectre complet de la détection de bugs, de la modernisation du code, et de la conformité aux bonnes pratiques. Le fichier `.clang-tidy` versionné dans le projet garantit la cohérence entre tous les développeurs et la CI.

**cppcheck** (section 32.2) complète clang-tidy avec son approche indépendante du compilateur et son taux de faux positifs minimal. Les deux outils détectent des catégories de bugs partiellement disjointes — les exécuter en tandem maximise la couverture de détection.

**clang-format** (section 32.3) élimine les discussions de style en automatisant le formatage. Configuré une fois dans le fichier `.clang-format`, il s'intègre dans l'IDE, les hooks Git et la CI pour garantir un code uniformément formaté sans effort.

**L'intégration dans le workflow** (cette section) assemble ces outils en trois couches de protection — IDE, pre-commit, CI — qui fonctionnent ensemble pour garantir que le code non conforme ne peut pas atteindre la branche principale. La stratégie d'adoption progressive permet d'intégrer ces outils sur un projet existant sans bloquer l'équipe.

Combinée avec les sanitizers (chapitre 29), l'analyse mémoire (chapitre 30) et le profiling de performance (chapitre 31), cette chaîne de qualité couvre les dimensions essentielles de la robustesse d'un programme C++ : correction, sécurité mémoire, performance et maintenabilité. Le chapitre suivant (chapitre 33) complète le tableau avec les tests unitaires, l'autre pilier indispensable de la qualité logicielle.

⏭️ [Module 11 : Tests et Qualité Logicielle](/module-11-tests-qualite.md)
