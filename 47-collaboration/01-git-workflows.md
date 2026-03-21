🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 47.1 — Git et workflows (GitFlow, trunk-based)

> **Chapitre 47 : Collaboration et Maintenance** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert  
> **Prérequis** : Connaissance de base de Git (clone, commit, push, pull), chapitre 38 (CI/CD pour C++)

---

## Introduction

Git est omniprésent dans le développement logiciel, mais le choix d'un *workflow* — c'est-à-dire la manière dont une équipe organise ses branches, ses merges et ses releases — a un impact considérable sur la productivité, la stabilité et la vélocité d'un projet C++.

Ce choix n'est pas anodin en C++. Contrairement aux langages interprétés ou aux écosystèmes web où un déploiement prend quelques secondes, un projet C++ de taille significative implique des temps de compilation parfois longs, des contraintes d'ABI compatibility, des dépendances système à gérer et des cycles de validation plus exigeants (sanitizers, tests d'intégration, benchmarks). Le workflow Git doit tenir compte de ces réalités.

Cette section présente les trois principaux modèles de workflow, leurs forces et faiblesses dans le contexte spécifique du C++, et fournit les critères pour choisir celui qui convient à votre projet.

---

## Les fondamentaux Git en contexte C++

Avant de comparer les workflows, rappelons quelques particularités de Git appliqué au C++ qui influencent le choix du modèle.

### Le fichier `.gitignore` d'un projet C++

Un projet C++ génère de nombreux artefacts qui ne doivent jamais être versionnés. Un `.gitignore` rigoureux est la première brique de toute collaboration :

```gitignore
# Répertoires de build
build/  
cmake-build-*/  
out/  

# Artefacts de compilation
*.o
*.obj
*.a
*.so
*.so.*
*.dylib
*.lib
*.dll
*.exe

# Fichiers générés par CMake
CMakeCache.txt  
CMakeFiles/  
cmake_install.cmake  
Makefile  
compile_commands.json  

# Cache de compilation
.ccache/
.cache/

# IDE
.vscode/settings.json
.idea/
*.swp
*.swo
*~

# Conan / vcpkg
conanbuildinfo.*  
conan.lock  
graph_info.json  
vcpkg_installed/  

# Core dumps
core  
core.*  
vgcore.*  

# Coverage et profiling
*.gcda
*.gcno
*.gcov
lcov-report/
```

> **Bonne pratique** : versionnez `compile_commands.json` si toute l'équipe utilise le même environnement (DevContainers), ou ajoutez-le au `.gitignore` si les configurations divergent. Ce fichier est essentiel au bon fonctionnement de `clangd` et des outils d'analyse statique.

### Fichiers qui doivent être versionnés

Certains fichiers sont parfois oubliés ou exclus à tort :

- **`CMakeLists.txt`** et tous les fichiers CMake (`*.cmake`, presets) — c'est le cœur du build.  
- **`conanfile.py` / `conanfile.txt`** ou **`vcpkg.json`** — les dépendances doivent être reproductibles.  
- **`.clang-format`** et **`.clang-tidy`** — les règles de formatage et d'analyse sont partagées par l'équipe.  
- **`.pre-commit-config.yaml`** — la configuration des hooks (abordée en section 47.2).  
- **`CMakePresets.json`** — standardise les configurations de build entre développeurs (section 27.6).

### Commits et temps de compilation

En C++, un commit mal calibré peut avoir des conséquences en cascade. Modifier un header inclus dans 200 fichiers `.cpp` déclenche la recompilation de ces 200 unités de traduction. Cela a des implications directes sur le workflow :

- **Commits atomiques** : chaque commit devrait représenter une modification logique cohérente. Mélanger un refactoring de header et une correction de bug dans le même commit rend le bisect difficile et la CI inutilement lente.  
- **Séparation header/implémentation** : les modifications de fichiers `.h` ont un rayon d'impact beaucoup plus large que les modifications de fichiers `.cpp`. Un workflow qui encourage les petits commits fréquents (trunk-based) sera d'autant plus efficace si l'architecture du code sépare bien interface et implémentation.  
- **ccache et CI** : les workflows avec des merges fréquents vers la branche principale bénéficient davantage du cache de compilation (ccache/sccache), car le delta entre deux builds consécutifs reste faible.

---

## GitFlow : Le modèle classique structuré

### Principe

GitFlow, formalisé par Vincent Driessen en 2010, repose sur une structure de branches rigide avec des rôles bien définis :

```
main (ou master)     ← Code en production, chaque commit = une release taguée
  │
develop              ← Branche d'intégration, reflète l'état "prochaine release"
  │
  ├── feature/xxx    ← Branches de fonctionnalités, partent de develop
  ├── feature/yyy
  │
  ├── release/1.2.0  ← Préparation d'une release, stabilisation
  │
  └── hotfix/1.1.1   ← Correction urgente en production, part de main
```

### Cycle de vie typique

**1. Développement d'une fonctionnalité :**

```bash
# Créer la branche feature depuis develop
git checkout develop  
git pull origin develop  
git checkout -b feature/add-json-parser  

# ... développement, commits ...

# Merge dans develop (via merge request / pull request)
git checkout develop  
git merge --no-ff feature/add-json-parser  
git branch -d feature/add-json-parser  
```

**2. Préparation d'une release :**

```bash
# Créer la branche release depuis develop
git checkout develop  
git checkout -b release/2.4.0  

# Stabilisation : corrections de bugs uniquement, pas de nouvelles features
# Mise à jour du numéro de version dans CMakeLists.txt, CHANGELOG, etc.

# Finalisation
git checkout main  
git merge --no-ff release/2.4.0  
git tag -a v2.4.0 -m "Release 2.4.0"  

# Rétro-merge dans develop
git checkout develop  
git merge --no-ff release/2.4.0  
git branch -d release/2.4.0  
```

**3. Hotfix en production :**

```bash
# Créer le hotfix depuis main
git checkout main  
git checkout -b hotfix/2.4.1  

# Correction, test, bump version

git checkout main  
git merge --no-ff hotfix/2.4.1  
git tag -a v2.4.1 -m "Hotfix 2.4.1"  

# Rétro-merge dans develop
git checkout develop  
git merge --no-ff hotfix/2.4.1  
git branch -d hotfix/2.4.1  
```

### Forces de GitFlow pour le C++

**Stabilité des releases.** La branche `release/*` offre une phase de stabilisation dédiée, précieuse quand la validation inclut des tests lourds (benchmarks, fuzzing, tests d'intégration système) qui prennent du temps.

**Isolation des fonctionnalités.** Les branches `feature/*` longues sont courantes en C++ : ajouter un module de sérialisation Protobuf ou refactorer un sous-système mémoire peut prendre plusieurs semaines. GitFlow supporte bien ces développements longs.

**Support multi-version.** Si vous devez maintenir simultanément les versions 2.x et 3.x d'une bibliothèque (par exemple pour des raisons d'ABI compatibility), GitFlow s'adapte avec des branches de support additionnelles.

**Gestion de hotfix claire.** Le chemin hotfix → main → develop est bien défini et évite les oublis de rétro-merge.

### Faiblesses de GitFlow pour le C++

**Merges complexes et conflits fréquents.** Plus les branches feature vivent longtemps, plus les conflits de merge deviennent douloureux. En C++, les conflits dans les headers peuvent avoir des effets en cascade difficiles à résoudre.

**Feedback loop lente.** Le code n'atteint `develop` qu'au merge de la feature. Si la CI ne tourne que sur `develop`, les problèmes d'intégration sont découverts tard.

**Overhead pour les petites équipes.** Maintenir `main`, `develop`, `feature/*`, `release/*` et `hotfix/*` représente une charge cognitive significative pour une équipe de 2-3 personnes.

**Cache de compilation sous-exploité.** Les branches feature divergentes compilent chacune leur version du code. Les caches ccache/sccache sont moins efficaces car les builds divergent significativement de la branche principale.

### Quand choisir GitFlow

GitFlow convient bien aux projets C++ qui présentent ces caractéristiques :

- Releases planifiées avec un cycle long (mensuel ou trimestriel).  
- Nécessité de maintenir plusieurs versions en parallèle.  
- Équipe de taille moyenne à grande (5+ développeurs).  
- Projet avec des clients qui consomment des versions spécifiques (bibliothèques, SDK).  
- Contraintes réglementaires imposant une traçabilité stricte des releases.

---

## Trunk-Based Development : Le modèle véloce

### Principe

Le trunk-based development (TBD) repose sur une idée simple : tous les développeurs intègrent leur code dans une branche unique (`main` ou `trunk`) au moins une fois par jour. Les branches, quand elles existent, ont une durée de vie très courte (quelques heures à deux jours maximum).

```
main (trunk)  ──●──●──●──●──●──●──●──●──●──●──●──●── (flux continu)
                │        │     │              │
                └─ feat ─┘     └─── feat ────┘
              (quelques heures)  (1-2 jours max)
```

Les releases sont créées à partir du trunk via des tags ou des branches de release éphémères :

```
main  ──●──●──●──●──●──●──●──●──●──●──●──●──
              │                    │
              tag v2.4.0           tag v2.5.0
```

### Pratiques clés

**Short-lived feature branches :**

```bash
# Créer une branche courte depuis main
git checkout main  
git pull origin main  
git checkout -b feat/optimize-json-parser  

# Développement court et focalisé (quelques heures à 2 jours)
# Commits fréquents, scope réduit

# Rebase sur main avant le merge (pour un historique linéaire)
git fetch origin  
git rebase origin/main  

# Merge via pull/merge request (fast-forward ou squash)
git checkout main  
git merge --ff-only feat/optimize-json-parser  
```

**Feature flags pour les développements longs :**

Quand un développement prend plus de deux jours, le code est intégré dans le trunk derrière un *feature flag* :

```cpp
// config/feature_flags.h
namespace config {
    // Feature flags — désactivés par défaut en production
    inline constexpr bool enable_new_serializer = 
    #ifdef FEATURE_NEW_SERIALIZER
        true;
    #else
        false;
    #endif
}
```

```cpp
// Utilisation dans le code
#include "config/feature_flags.h"

void process_data(std::span<const std::byte> input) {
    if constexpr (config::enable_new_serializer) {
        // Nouveau chemin — intégré dans main, mais inactif par défaut
        new_serializer::process(input);
    } else {
        // Chemin actuel — en production
        legacy_serializer::process(input);
    }
}
```

L'utilisation de `if constexpr` est ici délibérée : le compilateur élimine la branche inactive, ce qui garantit un coût zéro en production et détecte les erreurs de compilation dans le nouveau code même s'il n'est pas encore activé.

Pour les feature flags à granularité runtime (activation progressive, A/B testing), on utilise plutôt une vérification dynamique :

```cpp
void process_request(const Request& req) {
    if (feature_flags::is_enabled("new_serializer")) {
        // Activable sans recompilation
        new_serializer::handle(req);
    } else {
        legacy_serializer::handle(req);
    }
}
```

**Intégration continue rigoureuse :**

Le TBD ne fonctionne que si la CI est rapide et fiable. Chaque push sur `main` déclenche immédiatement le pipeline complet. C'est ici que les investissements en ccache (section 2.3), en builds Ninja (section 28.3) et en CI parallélisée (section 38) deviennent critiques.

### Forces du trunk-based pour le C++

**Feedback rapide.** Le code est intégré et testé en continu. Les problèmes de compilation croisée (un changement de header qui casse un module distant) sont détectés en quelques heures, pas en quelques semaines.

**Cache de compilation efficace.** Tous les développeurs compilent à partir d'une base de code très proche. Les caches ccache/sccache ont un taux de hit élevé, ce qui réduit drastiquement les temps de build en CI et en local.

**Historique linéaire et bisect facile.** Un historique principalement linéaire (rebase + squash) rend `git bisect` redoutablement efficace pour traquer les régressions — un outil précieux quand un bug C++ peut se manifester par un segfault silencieux.

**Réduction des conflits de merge.** Les intégrations fréquentes réduisent mécaniquement la taille des conflits. Un conflit sur 20 lignes est trivial à résoudre ; un conflit sur 2000 lignes après trois semaines de divergence est un cauchemar.

### Faiblesses du trunk-based pour le C++

**Exige une CI rapide.** Si le pipeline CI prend 45 minutes (compilation Debug + Release, tests, sanitizers, benchmarks), le TBD devient frustrant. Il faut investir dans l'accélération du build (parallélisation, ccache, builds incrémentaux).

**Discipline de feature flags.** Les feature flags ajoutent de la complexité au code. En C++, un `if constexpr` non nettoyé après la stabilisation du feature crée de la dette technique. Il faut une discipline stricte de nettoyage.

**Inadapté au support multi-version long terme.** Si vous devez maintenir simultanément les branches 2.x, 3.x et 4.x d'une bibliothèque, le TBD pur devient difficile. Des branches de release longues sont alors nécessaires, ce qui rapproche le modèle d'un hybride.

**Risque sur la stabilité du trunk.** Un commit cassé sur `main` bloque potentiellement toute l'équipe. Les protections (CI obligatoire avant merge, reverts rapides) doivent être solides.

### Quand choisir le trunk-based

Le trunk-based development convient aux projets C++ qui présentent ces caractéristiques :

- Déploiement continu ou releases fréquentes (hebdomadaires ou plus).  
- Pipeline CI rapide (< 15-20 minutes pour le feedback initial).  
- Équipe de toute taille, à condition que la discipline CI soit forte.  
- Projets cloud-native où le binaire est déployé dans des conteneurs (pas de contrainte multi-version client).  
- Culture d'équipe orientée continuous delivery.

---

## GitHub Flow : Le compromis pragmatique

### Principe

GitHub Flow est un modèle simplifié qui ne conserve qu'une branche principale (`main`) et des branches de feature courtes à moyennes, sans branche `develop` ni branche `release` formelle :

```
main  ──●──●──────●──────────●──●──●──────●──
         │        │          │              │
         └─ feat ─┘          └──── feat ────┘
           (PR + review)       (PR + review)
```

Le cycle est simple :

1. Créer une branche depuis `main`.
2. Développer et commiter.
3. Ouvrir une Pull Request.
4. Review + CI verte.
5. Merge dans `main`.
6. Déployer depuis `main`.

### Positionnement par rapport aux deux autres

GitHub Flow se situe entre GitFlow et le TBD. Il partage avec le TBD l'idée d'une branche unique comme source de vérité, mais n'impose pas la contrainte de durée de vie courte des branches. Il est moins structuré que GitFlow (pas de `develop`, pas de `release/*`) mais plus accessible.

En pratique, c'est le workflow adopté par la majorité des projets open source C++ sur GitHub et par beaucoup d'équipes de taille petite à moyenne.

### Quand le choisir

- Projets open source avec des contributeurs externes.  
- Équipes petites à moyennes (2-10 développeurs).  
- Quand GitFlow est perçu comme trop lourd mais que le TBD strict est trop exigeant en infrastructure CI.

---

## Comparaison synthétique

| Critère | GitFlow | GitHub Flow | Trunk-Based |
|---|---|---|---|
| **Complexité du modèle** | Élevée | Faible | Faible (discipline élevée) |
| **Durée de vie des branches** | Longue (semaines) | Moyenne (jours) | Courte (heures à 2 jours) |
| **Fréquence d'intégration** | Au merge de feature | À la PR | Continue (≥ 1×/jour) |
| **Support multi-version** | Natif | Possible mais non prévu | Nécessite adaptation |
| **Efficacité du cache (ccache)** | Faible | Moyenne | Élevée |
| **Risque de conflits** | Élevé | Moyen | Faible |
| **Exigence CI** | Modérée | Moyenne | Très élevée |
| **Stabilité du trunk** | Garantie par `develop` | Garantie par CI + PR | Garantie par CI stricte |
| **Courbe d'apprentissage** | Raide | Douce | Douce (discipline stricte) |
| **Adapté à l'open source** | Peu | Très bien | Possible |
| **Taille d'équipe idéale** | 5-50+ | 2-15 | 2-100+ |

---

## Bonnes pratiques communes à tous les workflows

Quel que soit le modèle choisi, certaines pratiques sont universelles pour un projet C++ bien géré.

### Convention de nommage des branches

Adopter une convention cohérente facilite la navigation et l'automatisation CI :

```
feature/    → feat/add-protobuf-serializer  
bugfix/     → fix/memory-leak-connection-pool  
hotfix/     → hotfix/2.4.1-segfault-on-shutdown  
refactor/   → refactor/extract-network-module  
ci/         → ci/add-asan-pipeline  
docs/       → docs/update-api-reference  
```

> **Astuce** : préfixer par le numéro de ticket (`feat/PROJ-142-add-protobuf`) crée un lien automatique avec le tracker de tâches dans la plupart des forges (GitLab, GitHub, Jira).

### Convention de messages de commit

Les **Conventional Commits** sont devenus un standard largement adopté. Leur structure permet l'automatisation du changelog et du versioning (section 47.6) :

```
<type>(<scope>): <description>

[corps optionnel]

[footer optionnel]
```

Exemples concrets pour un projet C++ :

```
feat(serializer): add Protobuf support for Config objects

Implement serialization/deserialization using proto3.  
Config objects can now be persisted in binary format,  
reducing file size by ~60% compared to JSON.  

Refs: PROJ-142
```

```
fix(network): resolve use-after-free in ConnectionPool

The connection was being returned to the pool after the  
socket fd was closed. Reorder cleanup to close fd last.  

Detected by: AddressSanitizer  
Fixes: PROJ-287  
```

```
refactor(core)!: rename Endpoint to ServiceEndpoint

BREAKING CHANGE: All references to `Endpoint` class  
must be updated to `ServiceEndpoint`. This affects  
the public API of libnetwork.  
```

Les types courants : `feat`, `fix`, `refactor`, `perf`, `test`, `ci`, `docs`, `build`, `chore`.

Le `!` après le scope (ou le footer `BREAKING CHANGE:`) signale un changement incompatible — information cruciale pour le versioning sémantique (section 47.6).

### Protection de la branche principale

La branche `main` (ou `develop` dans GitFlow) doit être protégée par des règles strictes, configurables dans GitLab, GitHub ou Bitbucket :

- **CI obligatoire** : aucun merge sans pipeline vert.  
- **Review obligatoire** : au moins un (idéalement deux) approbateur(s).  
- **Branche à jour** : la branche source doit être rebasée sur la cible avant le merge, garantissant que la CI a validé le code *avec* les dernières modifications.  
- **Pas de push direct** : tout passe par une merge/pull request.  
- **Pas de force push** : l'historique de `main` est immuable.

### L'outil `git bisect` et l'importance d'un historique propre

`git bisect` est un outil de diagnostic puissant en C++ : il effectue une recherche binaire dans l'historique pour trouver le commit qui a introduit une régression. Son efficacité dépend directement de la qualité de l'historique :

```bash
# Lancer un bisect
git bisect start  
git bisect bad                  # Le commit actuel est cassé  
git bisect good v2.3.0          # Cette version fonctionnait  

# Git checkout un commit intermédiaire
# → Compiler, tester, puis indiquer le résultat
git bisect good   # ou git bisect bad

# Répéter jusqu'à trouver le commit fautif
# Sur 1000 commits, bisect trouve le coupable en ~10 étapes
```

Pour que `git bisect` fonctionne, chaque commit doit **compiler et passer les tests**. C'est un argument fort en faveur du squash merge (un seul commit par feature) ou du rebase interactif avant merge pour nettoyer l'historique.

### Stratégie de merge : merge commit, squash ou rebase

Le choix de la stratégie de merge a un impact concret sur le quotidien :

**Merge commit (`--no-ff`)** : conserve l'historique complet de la branche. Utile pour tracer le contexte d'un développement complexe, mais crée un historique non linéaire qui complique `git bisect` et `git log`.

```bash
git merge --no-ff feature/add-parser
# Crée un commit de merge explicite
```

**Squash merge** : compresse tous les commits de la branche en un seul sur la cible. Produit un historique linéaire et propre, mais perd le détail des commits intermédiaires.

```bash
git merge --squash feature/add-parser  
git commit -m "feat(parser): add JSON parser with validation"  
```

**Rebase + fast-forward** : réécrit l'historique de la branche sur la cible puis merge en fast-forward. Conserve les commits individuels tout en produisant un historique linéaire.

```bash
git rebase main feature/add-parser  
git checkout main  
git merge --ff-only feature/add-parser  
```

**Recommandation pour le C++** : le squash merge est généralement le meilleur compromis pour les feature branches. Il produit un historique lisible où chaque commit sur `main` correspond à une fonctionnalité complète, compilable et testée. Les détails de développement restent accessibles via la merge/pull request pour ceux qui ont besoin de contexte.

---

## Adapter le workflow à la taille du projet

### Projet personnel ou binôme

Un workflow minimaliste suffit : `main` + branches courtes, pas de formalisme excessif. L'important est de ne jamais commiter directement sur `main` et de garder l'habitude des commits atomiques :

```bash
# Même seul, travailler en branche
git checkout -b feat/add-logging
# ... développement ...
git checkout main  
git merge --squash feat/add-logging  
git commit -m "feat: add structured logging with spdlog"  
```

### Équipe de 3 à 10 développeurs

GitHub Flow est souvent le choix le plus pragmatique. Ajouter :

- CI obligatoire sur les PRs.  
- Un reviewer par PR minimum.  
- Pre-commit hooks (section 47.2) pour le formatage et le linting.  
- Tags pour les releases.

### Équipe de 10+ développeurs ou projet critique

Trunk-based avec CI rapide, ou GitFlow si le cycle de release est long et que le support multi-version est nécessaire. Ajouter :

- Deux reviewers par PR.  
- CI étendue (tests, sanitizers, benchmarks comparatifs).  
- CODEOWNERS pour router automatiquement les reviews.  
- Branch protection rules strictes.

### Projet open source

GitHub Flow est le standard de facto. Les contributeurs externes forkent le dépôt et soumettent des PRs. Ajouter :

- Templates de PR avec checklist (compilation, tests, documentation).  
- CI qui tourne sur les PRs des forks.  
- Un fichier `CONTRIBUTING.md` qui documente le workflow attendu.

---

## Fichier CONTRIBUTING.md : documenter le workflow

Quel que soit le workflow choisi, il doit être documenté dans un fichier `CONTRIBUTING.md` à la racine du projet. Ce fichier est le premier point d'entrée pour tout nouveau contributeur :

```markdown
# Contributing to MyProject

## Workflow

Ce projet utilise **GitHub Flow**. Toute modification passe par une  
Pull Request vers `main`.  

## Branches

- Nommage : `feat/description`, `fix/description`, `docs/description`
- Durée de vie maximale recommandée : 5 jours

## Avant de soumettre une PR

1. Rebasez votre branche sur `main` :
   `git rebase origin/main`
2. Vérifiez que le build passe :
   `cmake --preset release && cmake --build --preset release`
3. Lancez les tests :
   `ctest --preset release`
4. Vérifiez le formatage :
   `pre-commit run --all-files`

## Messages de commit

Ce projet suit les **Conventional Commits** :
- `feat:` pour les nouvelles fonctionnalités
- `fix:` pour les corrections de bugs
- `refactor:` pour les restructurations sans changement fonctionnel
- `perf:` pour les optimisations de performance
- `test:` pour l'ajout ou la modification de tests
- Ajoutez `!` ou un footer `BREAKING CHANGE:` pour les changements
  incompatibles

## Code review

- Un approbateur minimum requis
- La CI doit être verte avant le merge
- Merge par **squash** dans `main`
```

---

## Résumé

Le choix d'un workflow Git n'est pas dogmatique — il doit servir les contraintes réelles du projet. En C++, les temps de compilation, les exigences de stabilité et les contraintes d'ABI orientent ce choix davantage que dans d'autres écosystèmes. Retenez ces trois principes :

1. **Intégrez tôt et souvent** — plus une branche vit longtemps, plus le merge sera douloureux et plus le cache de compilation sera inefficace.
2. **Protégez la branche principale** — CI obligatoire, reviews, pas de push direct. Un `main` cassé en C++ peut bloquer toute l'équipe pour des heures.
3. **Documentez le workflow** — un `CONTRIBUTING.md` clair et un template de PR réduisent les frictions et accélèrent l'onboarding.

---


⏭️ [Pre-commit hooks : Automatisation de la qualité avant commit](/47-collaboration/02-pre-commit-hooks.md)
