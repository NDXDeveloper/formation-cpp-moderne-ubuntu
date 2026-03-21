🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 47.3 — Configuration pre-commit : clang-format, clang-tidy, tests rapides 🔥

> **Chapitre 47 : Collaboration et Maintenance** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert  
> **Prérequis** : Section 47.2 complète (Pre-commit hooks, installation, hooks essentiels), chapitre 32 (Analyse statique et linting), section 26.5 (CMake et Ninja)

---

## Introduction

Les deux sections précédentes ont posé les fondations : le framework `pre-commit` est installé (47.2.1), et nous connaissons les hooks pertinents pour un projet C++ (47.2.2). Cette section passe à la mise en œuvre concrète. Elle assemble une configuration `.pre-commit-config.yaml` complète et opérationnelle, puis détaille les deux intégrations les plus critiques — `clang-format` et `clang-tidy` — avec les pièges spécifiques au C++ et les solutions éprouvées.

La difficulté n'est pas d'activer ces outils. C'est de les configurer pour qu'ils fonctionnent correctement dans tous les cas de figure : fichiers headers sans `compile_commands.json`, versions de `clang-format` hétérogènes entre développeurs, `clang-tidy` trop lent en pre-commit, faux positifs sur du code tiers. Cette section traite ces problèmes un par un.

---

## Le défi de l'intégration en C++

Dans un projet Python ou JavaScript, intégrer un linter dans les pre-commit hooks est trivial : l'outil opère fichier par fichier, sans contexte externe, et produit des résultats déterministes. En C++, la situation est fondamentalement différente, et c'est ce qui rend cette section nécessaire.

### `clang-format` : simple en apparence, piégeux en pratique

`clang-format` est le plus facile des deux à intégrer — il opère fichier par fichier sans avoir besoin de contexte de compilation. Mais les problèmes surgissent à l'échelle d'une équipe :

**Divergence de versions.** `clang-format` 18 et `clang-format` 19 peuvent produire un formatage différent pour le même fichier avec la même configuration `.clang-format`. Le résultat : deux développeurs avec des versions différentes se reformatent mutuellement le code en boucle. Les diffs deviennent illisibles, les reviews deviennent du bruit.

**Options de style entre versions.** Chaque nouvelle version de `clang-format` ajoute des options de style. Un `.clang-format` utilisant `PackConstructorInitializers` (introduit en clang-format 14) provoque une erreur sur `clang-format` 13. Inversement, un `.clang-format` conservateur n'exploite pas les améliorations des versions récentes.

**Macros et code conditionnel.** `clang-format` ne comprend pas la sémantique des macros préprocesseur. Un `#define` complexe ou un bloc `#ifdef` imbriqué peut être mal reformaté. Les fichiers contenant beaucoup de préprocesseur conditionnel (code multiplateforme, headers d'API publique) nécessitent parfois des `// clang-format off` / `// clang-format on` stratégiques.

### `clang-tidy` : puissant mais exigeant

`clang-tidy` est un outil d'analyse sémantique. Contrairement à `clang-format`, il ne se contente pas de lire le texte du fichier — il le compile réellement (partiellement) pour comprendre les types, les templates et les appels de fonctions. Cela implique des contraintes fortes :

**Dépendance à `compile_commands.json`.** Sans cette base de données de compilation, `clang-tidy` ne connaît pas les flags de compilation, les chemins d'include, les définitions de macros. Il produit alors des faux positifs massifs ou échoue silencieusement.

**Temps d'exécution variable.** L'analyse d'un fichier `.cpp` qui instancie des templates lourds (Boost, Eigen, Protobuf) peut prendre 10-30 secondes. Sur un commit touchant 5 fichiers de ce type, le hook atteint facilement la minute — inacceptable pour un workflow fluide.

**Headers vs sources.** Analyser un fichier `.h` isolé pose problème : `clang-tidy` doit le compiler, mais un header seul n'est pas une unité de traduction. Il manque les includes transitifs, les macros de configuration, les pragmas spécifiques. L'analyse produit souvent des erreurs qui n'existent pas en compilation réelle.

**Faux positifs sur code tiers.** Si un header modifié inclut indirectement du code de `third_party/`, `clang-tidy` peut rapporter des violations dans du code que vous ne contrôlez pas.

### L'objectif de cette section

Résoudre chacun de ces problèmes avec des configurations concrètes et testées. À la fin de cette section, vous disposerez :

1. D'un `.pre-commit-config.yaml` complet, prêt à copier et adapter.
2. D'une intégration `clang-format` qui garantit un formatage déterministe dans toute l'équipe.
3. D'une intégration `clang-tidy` qui reste rapide en pre-commit, avec une stratégie claire pour gérer `compile_commands.json`, le scope d'analyse et les checks activés.

---

## Philosophie de configuration : deux niveaux de rigueur

Un point clé sous-tend toute cette section : la configuration pre-commit locale et la configuration CI ne doivent pas être identiques. Elles servent des objectifs différents et opèrent dans des contextes différents.

### Pre-commit : rapide, focalisé, tolérant

Le hook pre-commit s'exécute sur la machine du développeur, dans le flux de travail. Il doit :

- Terminer en **moins de 15 secondes** dans le cas typique.  
- Opérer uniquement sur les **fichiers stagés** (pas sur tout le projet).  
- Se limiter aux vérifications dont le résultat est **déterministe** et **sans faux positif** — un hook qui bloque un commit à tort érode la confiance et pousse les développeurs vers `--no-verify`.  
- Corriger automatiquement ce qui peut l'être (formatage) et signaler ce qui ne peut pas l'être (bugs détectés par `clang-tidy`).

### CI : exhaustive, stricte, lente si nécessaire

La CI s'exécute sur un serveur, en arrière-plan. Elle peut se permettre :

- D'exécuter `clang-tidy` sur **tous les fichiers** du projet, pas seulement les fichiers modifiés.  
- D'activer **tous les checks** configurés dans `.clang-tidy`, y compris les checks coûteux.  
- De vérifier la cohérence du formatage sur l'**ensemble du projet** (`pre-commit run --all-files`).  
- De lancer les **sanitizers**, les **tests complets** et les **benchmarks de non-régression**.  
- De prendre **5, 10, 30 minutes** si nécessaire.

### Conséquence sur la configuration

Cette dualité se traduit par une configuration en deux couches :

| Aspect | Pre-commit (local) | CI (serveur) |
|---|---|---|
| **`clang-format`** | Sur les fichiers stagés, auto-correction | Sur tout le projet, vérification only |
| **`clang-tidy` — checks** | Sous-ensemble critique (bugs, UB, memory) | Ensemble complet (+ modernize, readability, perf) |
| **`clang-tidy` — scope** | Fichiers `.cpp` modifiés uniquement | Tous les fichiers `.cpp` du projet |
| **Temps cible** | < 15 secondes | < 30 minutes |
| **Comportement en cas d'échec** | Bloque le commit | Bloque le merge |

Le `.clang-format` est unique (même fichier localement et en CI). Le `.clang-tidy` est également unique, mais les checks activés peuvent être restreints localement via les arguments du hook — sans dupliquer le fichier de configuration. Les sous-sections 47.3.2 et 47.3.3 détaillent cette mécanique.

---

## Architecture des fichiers de configuration

Avant d'entrer dans le détail de chaque sous-section, voici la vue d'ensemble des fichiers impliqués et leur rôle :

```
mon-projet/
├── .pre-commit-config.yaml     ← Configuration des hooks (section 47.3.1)
├── .clang-format               ← Règles de formatage C++ (section 47.3.2)
├── .clang-tidy                 ← Checks d'analyse statique (section 47.3.3)
├── .cmake-format.yaml          ← Règles de formatage CMake
├── .cz.toml                    ← Configuration commitizen
├── CMakeLists.txt              ← Génère compile_commands.json
├── CMakePresets.json            ← Standardise -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
├── build/
│   └── compile_commands.json   ← Base de compilation pour clang-tidy
├── scripts/
│   └── setup-dev.sh            ← Onboarding (section 47.2.1)
└── CONTRIBUTING.md             ← Documentation du workflow
```

**Tous ces fichiers sauf `build/`** sont versionnés dans Git. C'est la garantie que chaque développeur, quel que soit le moment où il clone le projet, dispose de la configuration exacte attendue.

Le `compile_commands.json` mérite une attention particulière. Il est généré par CMake et vit dans le répertoire de build (non versionné). Cela pose un problème : comment `clang-tidy` le trouve-t-il quand il est invoqué par le hook pre-commit ? Deux approches existent — un symlink à la racine du projet ou l'argument `-p` — et la section 47.3.3 détaille les deux.

---

## Garantir l'homogénéité des versions d'outils

Le problème de divergence de versions de `clang-format` et `clang-tidy` entre développeurs est suffisamment fréquent pour mériter une discussion dédiée avant d'entrer dans les sous-sections.

### Stratégie 1 : DevContainers (recommandée)

La solution la plus robuste est d'utiliser un DevContainer (section 2.4.3) qui fige la version des outils :

```dockerfile
# .devcontainer/Dockerfile
FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04

RUN apt-get update && apt-get install -y \
    clang-format-19 \
    clang-tidy-19 \
    && update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-19 100 \
    && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-19 100
```

Chaque développeur qui ouvre le projet dans VS Code ou un autre IDE compatible utilise automatiquement les mêmes versions. Zéro divergence possible.

### Stratégie 2 : Documentation et vérification

Si les DevContainers ne sont pas une option, documentez la version requise dans le `CONTRIBUTING.md` et ajoutez un hook local de vérification :

```yaml
# Dans .pre-commit-config.yaml
- repo: local
  hooks:
    - id: check-clang-format-version
      name: "Verify clang-format version"
      entry: bash -c 'VERSION=$(clang-format --version | grep -oP "\d+\.\d+\.\d+"); MAJOR=$(echo $VERSION | cut -d. -f1); if [ "$MAJOR" -lt 19 ]; then echo "❌ clang-format $VERSION détecté, version 19+ requise"; echo "   sudo apt install clang-format-19"; exit 1; fi'
      language: system
      always_run: true
      pass_filenames: false
```

Ce hook vérifie la version à chaque commit et bloque si elle est trop ancienne. C'est moins robuste qu'un DevContainer (le développeur peut toujours avoir une mauvaise version dans le `PATH`), mais c'est mieux que rien.

### Stratégie 3 : `update-alternatives` standardisé

Si l'équipe est sur Ubuntu, un script d'installation standardisé peut configurer `update-alternatives` pour que `clang-format` pointe vers la bonne version (section 2.1.1) :

```bash
# Dans scripts/setup-dev.sh
sudo apt-get install -y clang-format-19 clang-tidy-19  
sudo update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-19 100  
sudo update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-19 100  
```

---

## Vue d'ensemble des sous-sections

| Sous-section | Contenu |
|---|---|
| **47.3.1 — `.pre-commit-config.yaml`** | Configuration complète assemblée, commentée ligne par ligne, prête à utiliser |
| **47.3.2 — Intégration `clang-format`** | Configuration `.clang-format`, gestion des versions, exclusions, `// clang-format off`, commit initial de reformatage |
| **47.3.3 — Intégration `clang-tidy`** | Configuration `.clang-tidy` en deux niveaux, gestion de `compile_commands.json`, scope d'analyse, checks pre-commit vs CI |

---


⏭️ [.pre-commit-config.yaml](/47-collaboration/03.1-pre-commit-config.md)
