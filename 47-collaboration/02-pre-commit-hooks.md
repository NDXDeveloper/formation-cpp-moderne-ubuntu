🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 47.2 — Pre-commit hooks : Automatisation de la qualité avant commit 🔥

> **Chapitre 47 : Collaboration et Maintenance** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert  
> **Prérequis** : Section 47.1 (Git et workflows), chapitre 32 (Analyse statique et linting), section 2.3 (ccache)

---

## Introduction

La section précédente a établi les workflows Git qui structurent la collaboration. Mais un workflow, aussi bien conçu soit-il, ne garantit rien si le code qui atteint la branche partagée contient des erreurs de formatage, des warnings ignorés ou des violations des standards de codage. Les code reviews (section 47.4) et la CI (chapitre 38) détectent ces problèmes — mais tard, après que le développeur a perdu le contexte et que l'équipe a dépensé du temps à les signaler.

Les **pre-commit hooks** résolvent ce problème en interceptant les défauts **au moment du commit**, avant même que le code ne quitte la machine du développeur. C'est l'incarnation la plus directe du principe *shift-left* introduit dans le README de ce chapitre : déplacer la détection d'erreurs le plus tôt possible dans le cycle.

Cette section présente le concept, le framework `pre-commit` qui l'implémente, et les raisons pour lesquelles il est devenu un outil incontournable dans les projets C++ professionnels en 2026.

---

## Le problème : la friction humaine dans l'application des standards

Considérons un scénario courant. Votre projet a défini des règles claires (chapitre 46) :

- Formatage imposé par `.clang-format` (style défini, largeur de ligne, placement des accolades).  
- Analyse statique par `.clang-tidy` (modernisation C++, détection de bugs, respect des core guidelines).  
- Interdiction de commiter des fichiers binaires, des artefacts de build ou des fichiers de grande taille.  
- Messages de commit au format Conventional Commits (section 47.1).

En théorie, chaque développeur lance `clang-format` et `clang-tidy` avant de commiter. En pratique :

- **L'oubli est inévitable.** Même les développeurs les plus rigoureux oublient de lancer le formateur après une session de débogage intense à 18h.  
- **La friction tue l'adoption.** Si respecter les standards demande trois commandes manuelles avant chaque commit, la conformité chute progressivement.  
- **Les reviews deviennent du bruit.** Quand 40 % des commentaires de code review portent sur le formatage ou des warnings triviaux, les reviewers perdent du temps sur des problèmes mécaniques au lieu de se concentrer sur la logique et l'architecture.  
- **La CI échoue pour des raisons évitables.** Attendre 10 à 20 minutes qu'un pipeline CI échoue pour un espace manquant est un gaspillage pur.

Les pre-commit hooks éliminent cette catégorie entière de problèmes en automatisant les vérifications que les humains oublient.

---

## Les hooks Git natifs

Git intègre nativement un mécanisme de hooks : des scripts placés dans `.git/hooks/` qui s'exécutent automatiquement à des moments clés du workflow Git. Les principaux hooks pertinents pour la qualité de code sont :

| Hook | Déclenchement | Usage typique |
|---|---|---|
| `pre-commit` | Avant la création du commit | Formatage, linting, vérifications rapides |
| `commit-msg` | Après la saisie du message | Validation du format de message (Conventional Commits) |
| `pre-push` | Avant l'envoi vers le remote | Tests unitaires, vérifications plus lourdes |
| `prepare-commit-msg` | Avant l'ouverture de l'éditeur | Pré-remplissage du message (numéro de ticket) |

Un hook natif est un simple script exécutable. Par exemple, un `pre-commit` minimaliste en Bash :

```bash
#!/usr/bin/env bash
# .git/hooks/pre-commit — Vérification basique du formatage

# Récupérer les fichiers C++ stagés (ajoutés avec git add)
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACM \
    | grep -E '\.(cpp|hpp|h|cc|cxx)$')

if [ -z "$STAGED_FILES" ]; then
    exit 0  # Aucun fichier C++ modifié, rien à vérifier
fi

# Vérifier le formatage avec clang-format
UNFORMATTED=""  
for file in $STAGED_FILES; do  
    # --dry-run + --Werror : échoue si le fichier n'est pas formaté
    if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
        UNFORMATTED="$UNFORMATTED\n  $file"
    fi
done

if [ -n "$UNFORMATTED" ]; then
    echo "❌ Fichiers mal formatés :$UNFORMATTED"
    echo ""
    echo "Corrigez avec : clang-format -i <fichier>"
    echo "Ou globalement : git diff --cached --name-only | xargs clang-format -i"
    exit 1  # Bloque le commit
fi

echo "✅ Formatage OK"  
exit 0  
```

### Limites des hooks natifs

Ce mécanisme fonctionne, mais présente plusieurs problèmes qui le rendent inadapté à un usage professionnel :

**Non versionné.** Le répertoire `.git/hooks/` n'est pas suivi par Git. Chaque développeur doit installer les hooks manuellement, et rien ne garantit que tous utilisent la même version.

**Pas de gestion des dépendances.** Le script ci-dessus suppose que `clang-format` est installé et accessible dans le `PATH`. Il ne gère ni la version requise, ni l'installation automatique.

**Maintenance pénible.** Quand le nombre de vérifications augmente (formatage, linting, taille des fichiers, secrets, message de commit), le script monolithique devient difficile à maintenir.

**Pas de gestion des fichiers stagés.** Le script naïf ci-dessus opère sur les fichiers complets, pas sur la version stagée (`git add`). Si un développeur a formaté une partie du fichier mais pas l'autre, le hook peut donner un faux positif ou un faux négatif.

**Portabilité.** Un script Bash ne fonctionne pas nativement sous Windows. Les équipes multi-plateformes doivent maintenir des scripts en parallèle.

C'est précisément pour résoudre ces problèmes que le framework `pre-commit` a été créé.

---

## Le framework `pre-commit` : l'outil de référence

Le framework [pre-commit](https://pre-commit.com) est un outil écrit en Python qui standardise l'installation, la configuration et l'exécution des hooks Git. Il est devenu le standard de facto dans l'écosystème C++ (et bien au-delà) pour plusieurs raisons :

### Configuration déclarative et versionnée

Toute la configuration est décrite dans un fichier `.pre-commit-config.yaml` à la racine du projet — un fichier versionné dans Git, partagé par toute l'équipe :

```yaml
# .pre-commit-config.yaml — Aperçu (détaillé en section 47.3)
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.6.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-added-large-files
        args: ['--maxkb=500']

  - repo: https://github.com/pocc/pre-commit-hooks
    rev: v1.3.5
    hooks:
      - id: clang-format
        args: ['--style=file']
      - id: clang-tidy
        args: ['--config-file=.clang-tidy']
```

Quand un développeur clone le projet et lance `pre-commit install`, les hooks sont automatiquement installés dans `.git/hooks/` avec la configuration exacte du projet. Plus de script à copier manuellement, plus de divergence entre développeurs.

### Gestion automatique des environnements

`pre-commit` télécharge et isole automatiquement les outils nécessaires dans des environnements virtuels dédiés. Un hook écrit en Python, Node.js, Ruby ou Go est installé dans son propre sandbox, sans polluer le système du développeur. Pour les hooks basés sur des outils système comme `clang-format` ou `clang-tidy`, le framework utilise les binaires déjà présents sur la machine (avec vérification de leur disponibilité).

### Opération sur les fichiers stagés uniquement

Le framework ne passe aux hooks que les fichiers effectivement stagés (`git add`), et dans leur version stagée — pas la version sur disque. Cela élimine les faux positifs liés aux modifications locales non commitées.

### Écosystème de hooks prêts à l'emploi

Des centaines de hooks sont disponibles pour l'écosystème C++ :

| Hook | Rôle | Source |
|---|---|---|
| `clang-format` | Formatage du code | `pocc/pre-commit-hooks` |
| `clang-tidy` | Analyse statique | `pocc/pre-commit-hooks` |
| `cmake-format` | Formatage des CMakeLists.txt | `cheshirekow/cmake-format-precommit` |
| `cmake-lint` | Linting CMake | `cheshirekow/cmake-format-precommit` |
| `trailing-whitespace` | Suppression des espaces en fin de ligne | `pre-commit/pre-commit-hooks` |
| `end-of-file-fixer` | Ajout du newline final | `pre-commit/pre-commit-hooks` |
| `check-added-large-files` | Blocage des gros fichiers binaires | `pre-commit/pre-commit-hooks` |
| `check-merge-conflict` | Détection de marqueurs de conflit oubliés | `pre-commit/pre-commit-hooks` |
| `check-yaml` | Validation syntaxique YAML | `pre-commit/pre-commit-hooks` |
| `commitizen` | Validation du message de commit (Conventional Commits) | `commitizen-tools/commitizen` |
| `codespell` | Détection de fautes de frappe dans le code et les commentaires | `codespell-project/codespell` |

Cette approche modulaire permet de composer un pipeline de vérification adapté à chaque projet, sans écrire une seule ligne de script.

---

## Architecture : pre-commit, CI et code review en synergie

Les pre-commit hooks ne remplacent ni la CI ni la code review. Ces trois mécanismes forment des couches complémentaires avec des rôles distincts :

```
                    ┌──────────────────────────────────────────┐
                    │            Code review (humain)          │
                    │  Architecture, logique, maintenabilité   │
                    │  Détecte : erreurs de conception,        │
                    │  dette technique, cas limites            │
                    ├──────────────────────────────────────────┤
                    │          CI pipeline (serveur)           │
                    │  Build complet, tests, sanitizers,       │
                    │  benchmarks, cross-compilation           │
                    │  Détecte : régressions, erreurs de link, │
                    │  bugs mémoire, dégradations perf         │
                    ├──────────────────────────────────────────┤
                    │      Pre-commit hooks (local, rapide)    │
                    │  Formatage, linting rapide, hygiène      │
                    │  Détecte : style, whitespace, fichiers   │
                    │  interdits, messages mal formés          │
                    └──────────────────────────────────────────┘
                         ▲                                  
                         │  Le développeur est ici            
                    Plus tôt = moins cher                    
```

**Les pre-commit hooks** filtrent les problèmes triviaux et mécaniques. Ils s'exécutent en quelques secondes et agissent comme un premier filet de sécurité. Leur scope est volontairement limité : formatage, whitespace, taille de fichiers, validation syntaxique, linting léger.

**La CI** prend le relais avec des vérifications qui nécessitent un build complet : compilation multi-compilateur, exécution de la suite de tests, sanitizers (ASan, UBSan, TSan), benchmarks de non-régression. Ces vérifications prennent des minutes et nécessitent un environnement contrôlé.

**La code review** se concentre sur ce que les outils ne peuvent pas vérifier : la pertinence des choix d'architecture, la clarté du code, les cas limites non couverts, la cohérence avec le reste du projet.

L'objectif des pre-commit hooks est de **libérer la CI et les reviewers** de tout ce qui peut être vérifié automatiquement et instantanément. Quand un développeur ouvre une merge request, le formatage est déjà correct, les warnings triviaux sont déjà résolus, et le reviewer peut se concentrer sur le fond.

---

## Ce qui doit être dans un hook — et ce qui ne doit pas

### Critère : temps d'exécution

La règle d'or est la rapidité. Un hook `pre-commit` qui prend plus de 10-15 secondes devient une nuisance que les développeurs finiront par contourner (`git commit --no-verify`). C'est le piège à éviter absolument.

**Adapté aux pre-commit hooks** (quelques secondes) :

- `clang-format` — formater les fichiers stagés est quasi instantané.  
- `trailing-whitespace`, `end-of-file-fixer` — opérations triviales.  
- `check-added-large-files` — vérification rapide de la taille.  
- `check-merge-conflict` — recherche de marqueurs de conflit.  
- `codespell` — détection de typos, très rapide.  
- `cmake-format` — formatage des fichiers CMake.  
- `commitizen` — validation du message de commit.

**Limite acceptable** (si scope réduit aux fichiers modifiés) :

- `clang-tidy` sur les fichiers stagés uniquement — peut prendre 5-15 secondes selon le nombre de fichiers et les checks activés. Acceptable si le nombre de fichiers modifiés par commit reste raisonnable.

**À exclure des pre-commit hooks** (trop lent, à réserver à la CI) :

- Compilation complète du projet.  
- Exécution de la suite de tests.  
- Sanitizers (ASan, UBSan, TSan) — nécessitent un build dédié.  
- Benchmarks de performance.  
- `clang-tidy` sur l'ensemble du projet.  
- Génération de documentation Doxygen.

### Critère : déterminisme

Un hook doit donner le même résultat quel que soit l'environnement du développeur. C'est pourquoi les hooks doivent :

- Utiliser la configuration versionnée (`.clang-format`, `.clang-tidy`) plutôt que des paramètres hardcodés.  
- Spécifier les versions des outils utilisés (via `rev` dans `.pre-commit-config.yaml`).  
- Opérer uniquement sur les fichiers stagés, pas sur l'ensemble du projet.

### Critère : pas de modification silencieuse non contrôlée

Certains hooks *modifient* les fichiers (auto-formatage). C'est souhaitable pour `clang-format` (le développeur voit le diff et peut re-stager), mais potentiellement dangereux pour des outils qui réécriraient du code de manière non prédictible. La règle : seuls les outils de formatage déterministe devraient modifier les fichiers. Les outils d'analyse (clang-tidy, cppcheck) doivent uniquement *rapporter* les problèmes.

---

## Gestion de l'option `--no-verify`

Git permet de contourner les hooks avec `git commit --no-verify` (ou `-n`). C'est un échappatoire nécessaire (par exemple pour un WIP commit rapide sur une branche personnelle), mais qui peut devenir un réflexe si les hooks sont trop lents ou trop contraignants.

Stratégies pour limiter les abus :

**Garder les hooks rapides.** C'est la mesure la plus efficace. Si les hooks prennent 2-3 secondes, personne ne les contourne.

**Dupliquer les vérifications critiques en CI.** La CI doit vérifier le formatage indépendamment des hooks. Ainsi, même si un développeur utilise `--no-verify`, la merge request échouera. Le pre-commit hook est un confort, la CI est le filet de sécurité.

**Ne pas fliquer les développeurs.** L'objectif des hooks est d'aider, pas de punir. Si l'équipe contourne systématiquement les hooks, c'est un signal que la configuration est trop lourde — il faut alléger, pas surveiller.

---

## Impact sur le workflow quotidien

Un workflow typique avec pre-commit hooks installés ressemble à ceci :

```bash
# Le développeur travaille normalement
vim src/network/connection_pool.cpp

# Il stage ses modifications
git add src/network/connection_pool.cpp

# Il commit — les hooks s'exécutent automatiquement
git commit -m "fix(network): handle timeout in connection pool"

# ═══════════════════════════════════════════════════
# Les hooks s'exécutent (2-5 secondes) :
#   ✅ trailing-whitespace ........... Passed
#   ✅ end-of-file-fixer ............ Passed
#   ✅ check-added-large-files ...... Passed
#   ✅ clang-format ................. Passed
#   ✅ clang-tidy ................... Passed
#   ✅ cmake-format ................. Passed
#   ✅ commitizen (conventional) .... Passed
# ═══════════════════════════════════════════════════

# Commit accepté, le développeur continue
git push origin fix/network-timeout
```

Si un hook échoue :

```bash
git commit -m "fix(network): handle timeout in connection pool"

# ═══════════════════════════════════════════════════
#   ✅ trailing-whitespace ........... Passed
#   ✅ end-of-file-fixer ............ Passed
#   ✅ check-added-large-files ...... Passed
#   ❌ clang-format ................. Failed
#     - src/network/connection_pool.cpp
#       (fichier reformaté automatiquement)
#   ── Commit bloqué ──
# ═══════════════════════════════════════════════════

# clang-format a corrigé le fichier automatiquement
# Il suffit de re-stager et re-commiter :
git add src/network/connection_pool.cpp  
git commit -m "fix(network): handle timeout in connection pool"  

# Cette fois, tous les hooks passent ✅
```

Le cycle correction-restage prend quelques secondes. C'est incomparablement moins coûteux que de découvrir le problème 20 minutes plus tard dans la CI, ou lors de la code review.

---

## Vue d'ensemble des sous-sections

Les deux sous-sections suivantes détaillent la mise en œuvre concrète :

| Sous-section | Contenu |
|---|---|
| **47.2.1 — Installation du framework pre-commit** | Installation sur Ubuntu, `pre-commit install`, intégration dans le workflow d'onboarding, gestion des mises à jour |
| **47.2.2 — Hooks essentiels pour C++** | Sélection et configuration des hooks pertinents pour un projet C++ : formatage, linting, hygiène, validation des messages de commit |

La section 47.3 complètera en détaillant la configuration `.pre-commit-config.yaml` complète avec l'intégration spécifique de `clang-format` et `clang-tidy`.

---


⏭️ [Installation du framework pre-commit](/47-collaboration/02.1-installation-pre-commit.md)
