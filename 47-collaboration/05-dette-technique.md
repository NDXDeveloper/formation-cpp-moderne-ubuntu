🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 47.5 — Gestion de la dette technique

> **Chapitre 47 : Collaboration et Maintenance** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert  
> **Prérequis** : Section 47.4 (Code reviews efficaces), chapitres 32-35 (Analyse statique, tests, benchmarking), chapitre 38 (CI/CD)

---

## Introduction

Les sections précédentes ont mis en place des mécanismes pour produire du code de qualité : formatage automatique, analyse statique, code reviews structurées. Mais même avec ces garde-fous, tout projet logiciel accumule de la **dette technique** — du code qui fonctionne aujourd'hui mais qui ralentira le développement demain. Un prototype rapide transformé en production, un refactoring remis à plus tard, une dépendance non mise à jour, un module dont l'architecture ne correspond plus aux besoins actuels.

La dette technique n'est pas un échec. C'est un phénomène naturel et parfois un choix rationnel : livrer une fonctionnalité rapidement avec un design imparfait peut être la bonne décision *si* la dette est reconnue, mesurée et planifiée pour remboursement. Le problème commence quand la dette est invisible, non mesurée et ignorée jusqu'au jour où elle paralyse le projet.

Cette section traite la gestion de la dette technique comme une discipline : comment l'identifier, la catégoriser, la mesurer et la planifier dans un projet C++, où ses conséquences sont souvent plus sévères que dans d'autres écosystèmes.

---

## Pourquoi la dette technique est particulièrement coûteuse en C++

Tous les langages accumulent de la dette technique, mais en C++ ses conséquences sont amplifiées par plusieurs caractéristiques du langage et de son écosystème.

### Temps de compilation

Un module mal architecturé en C++ a un coût direct et mesurable : le temps de compilation. Un header monolithique inclus dans 200 fichiers `.cpp` signifie que chaque modification de ce header recompile 200 unités de traduction. Si ce header contient des templates lourds, la recompilation prend des minutes. Sur un projet de taille significative, une mauvaise architecture d'includes peut transformer un cycle edit-compile-test de 5 secondes en un cycle de 2 minutes — un facteur 24x qui s'accumule sur chaque développeur, chaque jour.

La dette technique liée aux includes se rembourse par le forward declaration, la séparation des headers en sous-modules, le passage aux modules C++20 (section 12.13), ou le pattern Pimpl (pointer to implementation).

### ABI compatibility

Dans une bibliothèque partagée, ajouter un membre de donnée à une classe, changer l'ordre d'héritage ou modifier la signature d'une fonction virtuelle casse l'ABI — les programmes compilés contre l'ancienne version crashent avec la nouvelle. Cette contrainte rend certains refactorings extrêmement coûteux, car ils imposent une recompilation de tous les consommateurs de la bibliothèque.

La dette technique accumulée dans une API publique est donc bien plus chère à rembourser que celle d'un code interne. Un mauvais choix d'interface publique fait en 2024 peut coûter une rupture de compatibilité en 2026 qui impacte tous les utilisateurs.

### Comportement indéfini latent

En C++, la dette technique peut se manifester de manière insidieuse : un `reinterpret_cast` non documenté, un accès à un `union` par le mauvais membre, une violation d'aliasing strict, un integer overflow signé. Ces constructions peuvent fonctionner pendant des années avec un compilateur et un niveau d'optimisation donnés, puis provoquer des bugs mystérieux après une montée de version du compilateur ou un passage de `-O0` à `-O2`. La dette technique liée au comportement indéfini est une bombe à retardement silencieuse.

### Dépendances et écosystème

L'écosystème C++ ne dispose pas d'un gestionnaire de paquets universel aussi mature que `npm`, `pip` ou `cargo`. Les dépendances sont souvent intégrées manuellement (vendoring, sous-modules Git, copie directe). Mettre à jour une dépendance dans ces conditions demande un effort considérable — ce qui pousse les équipes à repousser les mises à jour, accumulant une dette de sécurité et de compatibilité.

---

## Taxonomie de la dette technique en C++

Toute la dette technique n'est pas de même nature. La catégoriser aide à prioriser le remboursement.

### Dette de design

Le code fonctionne mais sa structure ne correspond pas (ou plus) aux besoins. C'est la forme la plus coûteuse car elle affecte chaque développement futur dans le module concerné.

**Exemples en C++ :**

- Une hiérarchie d'héritage profonde (5+ niveaux) là où une composition ou un polymorphisme statique (CRTP, concepts) serait plus adapté.  
- Un Singleton global utilisé pour passer des dépendances à travers le code, rendant les tests unitaires impossibles sans mock complexe.  
- Un module monolithique de 15 000 lignes qui mériterait d'être découpé en sous-modules avec des interfaces claires.  
- L'utilisation de `void*` et de C-style casts là où `std::variant`, `std::any` ou des templates fourniraient un typage sûr.

### Dette de code

Le code est fonctionnel mais ne respecte pas les standards de qualité du projet. Le coût est local — chaque fichier touché demande un effort supplémentaire de compréhension et de modification.

**Exemples en C++ :**

- Code pré-C++11 dans un projet qui cible C++20 : `NULL` au lieu de `nullptr`, boucles avec itérateurs explicites au lieu de range-based for, `new`/`delete` manuels au lieu de smart pointers.  
- Absence de `const`-correctness : paramètres non marqués `const` alors qu'ils ne sont pas modifiés, méthodes non marquées `const` alors qu'elles ne modifient pas l'état.  
- Fonctions de 300 lignes avec une complexité cyclomatique élevée.  
- Commentaires obsolètes qui décrivent un comportement qui a changé.  
- TODO sans ticket dispersés dans le code.

### Dette de test

Le code n'est pas suffisamment couvert par les tests, ou les tests existants sont fragiles, lents ou testent l'implémentation plutôt que le comportement.

**Exemples en C++ :**

- Un module critique (réseau, sérialisation, allocation mémoire) avec 10 % de couverture de code.  
- Tests qui dépendent de l'ordre d'exécution (état global partagé entre tests).  
- Absence de tests avec les sanitizers (ASan, UBSan, TSan) — les tests passent mais le code contient du comportement indéfini.  
- Pas de benchmarks de référence — impossible de détecter une régression de performance.

### Dette d'infrastructure

Le tooling, le build system et la CI ne sont pas à jour ou ne sont pas adaptés à la taille du projet.

**Exemples en C++ :**

- Build system basé sur des Makefiles manuels plutôt que CMake moderne.  
- Pas de cache de compilation (ccache/sccache) — la CI recompile tout à chaque pipeline.  
- Dépendances copiées manuellement dans le repo au lieu d'être gérées par Conan ou vcpkg.  
- CI sans sanitizers, sans analyse statique, sans vérification de formatage.  
- Version du compilateur en retard de 3+ générations (GCC 11 en 2026 quand GCC 15 est disponible).

### Dette de dépendances

Les bibliothèques externes ne sont pas à jour, exposant le projet à des vulnérabilités connues et à des incompatibilités croissantes.

**Exemples en C++ :**

- Boost 1.75 (2020) dans un projet en 2026, alors que plusieurs fonctionnalités utilisées sont maintenant dans le standard (C++20/23).  
- OpenSSL 1.1 (fin de vie) au lieu de OpenSSL 3.x.  
- Protobuf 3.x au lieu de Protobuf 5.x (editions), accumulant un écart de fonctionnalités et de performance.

---

## Identifier la dette technique

La première étape est de rendre la dette visible. Une dette invisible ne sera jamais remboursée.

### Marqueurs dans le code

**Les commentaires TODO/FIXME/HACK** sont le signal le plus direct. La convention introduite par le hook pre-commit (section 47.2.2) impose un format avec ticket :

```cpp
// TODO(PROJ-456): Remplacer par std::expected quand le support Clang sera complet
// FIXME(PROJ-789): Race condition possible si deux threads appellent init() simultanément
// HACK(PROJ-1011): Contournement d'un bug dans protobuf 4.25, à retirer après upgrade
```

Un script ou une commande CI peut extraire et compter ces marqueurs :

```bash
# Compter les TODO/FIXME/HACK par module
grep -rn "TODO\|FIXME\|HACK" src/ include/ \
    | awk -F: '{split($1,a,"/"); print a[2]}' \
    | sort | uniq -c | sort -rn
```

```
  42 network
  31 serialization
  18 core
   7 utils
   3 cli
```

Ce comptage donne une image instantanée de la distribution de la dette par module.

### Signaux de `clang-tidy`

L'exécution de `clang-tidy` avec la configuration complète (pas le sous-ensemble pre-commit) révèle la dette de modernisation. Les checks `modernize-*` identifient précisément le code qui pourrait être mis à jour :

```bash
# Exécuter clang-tidy en mode rapport (sans modifier le code)
run-clang-tidy -p build -checks='-*,modernize-*' -quiet 2>&1 \
    | grep "warning:" \
    | awk -F: '{print $4}' \
    | sort | uniq -c | sort -rn
```

```
 156 modernize-use-nullptr
  89 modernize-loop-convert
  67 modernize-use-override
  45 modernize-use-auto
  23 modernize-use-emplace
```

Ce rapport quantifie la dette de modernisation : 156 occurrences de `NULL` au lieu de `nullptr`, 89 boucles converties en range-based for, etc.

### Métriques de complexité

La complexité cyclomatique et la complexité cognitive sont des indicateurs de code difficile à maintenir. `clang-tidy` mesure la complexité cognitive via `readability-function-cognitive-complexity` :

```bash
# Identifier les fonctions trop complexes
run-clang-tidy -p build \
    -checks='-*,readability-function-cognitive-complexity' \
    -config='{CheckOptions: [{key: readability-function-cognitive-complexity.Threshold, value: 15}]}' \
    2>&1 | grep "warning:"
```

Les fonctions qui dépassent le seuil sont des candidates prioritaires pour un refactoring.

### Couverture de code

La couverture de code (chapitre 34) identifie les modules insuffisamment testés. Ce n'est pas un indicateur de qualité des tests (un code peut avoir 100 % de couverture avec des tests inutiles), mais un indicateur de risque : un module à 10 % de couverture est un risque de régression à chaque modification.

```bash
# Générer le rapport de couverture
cmake --build build --target coverage
# Identifier les modules sous le seuil
lcov --summary build/coverage.info | grep -B1 "lines\.\.\." | grep "src/"
```

### Temps de compilation par module

Un temps de compilation anormalement long pour un module est un signal de dette architecturale (headers trop larges, templates excessifs, dépendances inutiles) :

```bash
# Avec Ninja, afficher les 10 fichiers les plus lents à compiler
ninja -C build -j1 -d stats 2>&1 | sort -t= -k2 -rn | head -10

# Avec ClangBuildAnalyzer (outil dédié)
ClangBuildAnalyzer --all build/trace.json build/analysis.bin  
ClangBuildAnalyzer --analyze build/analysis.bin  
```

### Âge des dépendances

Un script simple peut vérifier l'âge des dépendances par rapport aux versions disponibles :

```bash
# Avec Conan
conan search --remote=conancenter "nlohmann_json/*" | tail -5
# Comparer avec la version dans conanfile.py
grep "nlohmann_json" conanfile.py
```

Des outils comme `dependabot` (GitHub) ou `renovate` automatisent cette veille et ouvrent des MR de mise à jour.

---

## Mesurer la dette technique

Identifier la dette est nécessaire mais insuffisant. Il faut la **quantifier** pour pouvoir la prioriser et communiquer son impact au management.

### Le registre de dette technique

Un registre de dette technique est un document vivant (fichier Markdown, page wiki, ou board dans le tracker de tâches) qui recense chaque élément de dette avec ses métadonnées :

```markdown
# Registre de dette technique

## DT-001 : Module network — Architecture monolithique
- **Type** : Design
- **Sévérité** : Haute
- **Impact** : Temps de compilation (+45s par rebuild), difficulté d'ajout de protocoles
- **Coût estimé de remboursement** : 3-5 jours développeur
- **Coût de non-remboursement** : +2h par feature impliquant le réseau
- **Tickets liés** : PROJ-456, PROJ-789
- **Date d'identification** : 2025-09-15
- **Responsable** : @alice

## DT-002 : Code legacy pré-C++17 dans src/serialization/
- **Type** : Code
- **Sévérité** : Moyenne
- **Impact** : 156 warnings modernize-*, lisibilité réduite, incompatible avec std::expected
- **Coût estimé de remboursement** : 2 jours développeur
- **Coût de non-remboursement** : friction lors de chaque modification (~30min/MR)
- **Tickets liés** : PROJ-1011
- **Date d'identification** : 2025-11-20
- **Responsable** : @bob
```

### Le ratio dette/feature

Une métrique pragmatique : quel pourcentage du temps de développement est consacré au combat contre la dette existante plutôt qu'au développement de nouvelles fonctionnalités ? Ce ratio se mesure par le temps passé sur les tickets de type "dette" vs "feature" dans le tracker :

| Ratio dette/feature | Interprétation |
|---|---|
| < 10 % | Dette probablement sous-estimée ou ignorée |
| 10-20 % | Zone saine — investissement régulier |
| 20-40 % | Dette significative, le projet ralentit |
| > 40 % | Dette critique — le projet est paralysé |

### Le coût du changement

Un indicateur indirect mais parlant : combien de temps faut-il pour implémenter un changement "simple" ? Si ajouter un nouveau champ à une structure de données nécessite de modifier 15 fichiers, de mettre à jour 3 formats de sérialisation et de corriger 20 tests cassés, c'est un signe de dette architecturale. Comparer le temps estimé d'un changement avec le temps réellement passé révèle la friction invisible.

---

## Planifier le remboursement

Identifier et mesurer la dette ne sert à rien si elle n'est jamais remboursée. La discipline de remboursement est la partie la plus difficile — elle demande de l'allocation de temps, donc du soutien managérial.

### La règle du Boy Scout

> *"Laissez le campement plus propre que vous ne l'avez trouvé."*

Appliquée au code : chaque MR devrait améliorer légèrement le code environnant, au-delà du changement fonctionnel. Remplacer un `NULL` par `nullptr` quand on touche une ligne adjacente, ajouter un `const` manquant, extraire une fonction trop longue. Ces micro-améliorations ne méritent pas un ticket dédié — elles se font naturellement dans le flux de développement.

**En pratique** : inclure dans la MR un petit commit de nettoyage séparé du changement fonctionnel :

```
feat(network): add HTTP/3 connection pooling

refactor(network): modernize adjacent code (nullptr, range-for)
```

Le commit de refactoring est séparé du commit fonctionnel pour faciliter le bisect et la review. Le reviewer peut se concentrer sur le changement fonctionnel en sachant que le refactoring est mécanique.

### Le budget de dette

Allouer un pourcentage fixe du temps de développement au remboursement de dette technique. La pratique courante est de réserver **15-20 % du sprint** (ou de l'itération) pour des tickets de dette.

Concrètement, sur une itération de 2 semaines avec 5 développeurs :

- 10 jours-développeur au total.  
- 8 jours sur les features et bugfixes.  
- 2 jours sur le remboursement de dette (refactoring, modernisation, mise à jour de dépendances).

Ce budget est prélevé *avant* la planification des features, pas après. S'il est traité comme un reste, il sera systématiquement sacrifié sous la pression des deadlines.

### Les sprints de dette

Pour les dettes majeures (refactoring architectural, migration de build system, montée de version majeure d'une dépendance), un budget de 15 % ne suffit pas. Certaines équipes planifient un **sprint de dette** tous les 3-4 sprints — une itération entièrement dédiée au remboursement. C'est le moment d'attaquer les items à haute sévérité du registre.

En C++, les sprints de dette sont particulièrement efficaces pour les migrations qui touchent beaucoup de fichiers (passage à `nullptr`, ajout de `override` sur toutes les méthodes virtuelles, migration vers un nouveau standard C++) car ces changements mécaniques sont difficiles à intercaler dans des sprints de feature sans provoquer des conflits de merge massifs.

### Priorisation

Tout ne peut pas être remboursé en même temps. La matrice impact/effort guide la priorisation :

```
          Effort faible         Effort élevé
        ┌─────────────────┬─────────────────┐
Impact  │   FAIRE VITE    │   PLANIFIER     │
élevé   │                 │                 │
        │ - Ajouter const │ - Refactoring   │
        │ - Modernize     │   architectural │
        │   nullptr       │ - Migration     │
        │ - Fix TODO      │   build system  │
        │   critiques     │ - Split module  │
        ├─────────────────┼─────────────────┤
Impact  │   OPPORTUNISTE  │   DIFFÉRER      │  
faible  │                 │                 │  
        │ - Renommer      │ - Réécriture    │
        │   variable      │   complète d'un │
        │ - Commentaire   │   module stable │
        │   obsolète      │ - Migration     │
        │                 │   vers modules  │
        │                 │   C++20 (si     │
        │                 │   support limité│
        └─────────────────┴─────────────────┘
```

**Faire vite** : impact élevé, effort faible. Ce sont les *quick wins* — souvent de la modernisation mécanique que `clang-tidy --fix` peut appliquer automatiquement.

**Planifier** : impact élevé, effort élevé. Nécessite un sprint dédié ou un projet de refactoring étalé sur plusieurs itérations.

**Opportuniste** : impact faible, effort faible. À traiter via la règle du Boy Scout, sans ticket dédié.

**Différer** : impact faible, effort élevé. Pas de raison de s'y attaquer tant que l'impact reste faible. Réévaluer périodiquement.

---

## Remboursement automatisé avec `clang-tidy --fix`

Une des forces de l'écosystème C++ est que certaines catégories de dette peuvent être remboursées **automatiquement**. `clang-tidy` avec l'option `--fix` applique les corrections suggérées :

```bash
# Remplacer tous les NULL par nullptr dans le projet
run-clang-tidy -p build -checks='-*,modernize-use-nullptr' -fix

# Ajouter override sur toutes les méthodes virtuelles
run-clang-tidy -p build -checks='-*,modernize-use-override' -fix

# Convertir les boucles en range-based for
run-clang-tidy -p build -checks='-*,modernize-loop-convert' -fix

# Remplacer push_back par emplace_back quand approprié
run-clang-tidy -p build -checks='-*,modernize-use-emplace' -fix
```

**Workflow recommandé pour un remboursement automatisé :**

```bash
# 1. Créer une branche dédiée
git checkout -b refactor/modernize-nullptr

# 2. Appliquer la correction automatique
run-clang-tidy -p build -checks='-*,modernize-use-nullptr' -fix

# 3. Vérifier le résultat (compilation + tests)
cmake --build build && ctest --test-dir build

# 4. Commiter et ouvrir une MR
git add -A  
git commit -m "refactor: replace NULL with nullptr (modernize-use-nullptr)  

Automated fix via clang-tidy --fix.
156 occurrences replaced across 43 files."

# 5. Ajouter à .git-blame-ignore-revs après merge
```

Le point crucial : **un seul type de correction par MR**. Mélanger `modernize-use-nullptr` et `modernize-loop-convert` dans la même MR rend la review impossible et le revert risqué. Chaque correction automatique est un commit atomique, reviewable et réversible indépendamment.

> **Attention** : les fixes automatiques de `clang-tidy` ne sont pas infaillibles. Certaines corrections peuvent changer la sémantique du code dans des cas limites (par exemple, `modernize-use-emplace` peut échouer si le constructeur est `explicit`). Toujours compiler et exécuter les tests après un fix automatique.

---

## Prévenir l'accumulation

Le meilleur remboursement est celui qui n'a pas besoin d'être fait. Plusieurs mécanismes présentés dans cette formation préviennent l'accumulation de nouvelle dette :

| Mécanisme | Type de dette prévenue | Référence |
|---|---|---|
| `clang-format` en pre-commit | Dette de style | Section 47.3.2 |
| `clang-tidy` en CI | Dette de modernisation, bugs latents | Section 47.3.3 |
| Code reviews | Dette de design | Section 47.4 |
| Tests unitaires systématiques | Dette de test | Chapitre 33 |
| Couverture de code en CI | Régression de couverture | Chapitre 34 |
| Gestion des dépendances (Conan/vcpkg) | Dette de dépendances | Chapitre 27 |
| CMake Presets | Dette d'infrastructure | Section 27.6 |
| Pre-commit hooks (TODO avec ticket) | Dette invisible (TODO orphelins) | Section 47.2.2 |
| Conventional Commits | Dette de traçabilité | Section 47.1 |
| Benchmarks de non-régression | Dette de performance | Chapitre 35 |

La combinaison de ces mécanismes crée un **effet de cliquet** : le projet peut s'améliorer progressivement, mais il ne peut pas régresser silencieusement en dessous d'un seuil de qualité. Chaque MR est au moins aussi propre que le code existant (grâce aux hooks et à la CI), et légèrement plus propre si le développeur applique la règle du Boy Scout.

---

## Communiquer sur la dette technique

La dette technique est un concept que les développeurs comprennent intuitivement mais qui reste souvent opaque pour les parties prenantes non techniques. Or, sans soutien managérial, le temps nécessaire au remboursement ne sera jamais alloué.

### La métaphore financière

La métaphore de la dette est puissante précisément parce qu'elle parle le langage du management :

- **Le principal** : le coût pour rembourser la dette (refactoring, réécriture, migration).  
- **Les intérêts** : le surcoût quotidien causé par la dette (temps supplémentaire par feature, bugs plus fréquents, onboarding plus long).  
- **La faillite technique** : le point où les intérêts dépassent la capacité de développement — le projet ne peut plus évoluer.

Un argument concret est toujours plus efficace qu'un discours abstrait. Plutôt que "le module réseau a de la dette technique", formulez ainsi : "chaque nouvelle fonctionnalité réseau prend 2 jours de plus que nécessaire à cause de l'architecture actuelle. En investissant 5 jours de refactoring, nous économisons 2 jours par feature pour les 12 prochains mois, soit un retour sur investissement de 40 jours."

### Le tableau de bord

Un tableau de bord de dette technique, mis à jour automatiquement par la CI, rend la dette visible en permanence :

```
═══════════════════════════════════════════════════
  Dashboard dette technique — MyProject — Mars 2026
═══════════════════════════════════════════════════
  TODO/FIXME/HACK dans le code    :   101 (↓12 vs février)
  Warnings clang-tidy (modernize) :   234 (↓45 vs février)
  Couverture de code globale      :   74% (↑2% vs février)
  Modules sous 50% de couverture  :     3 (= vs février)
  Dépendances obsolètes (>1 an)   :     2 (↓1 vs février)
  Complexité cognitive > seuil    :    17 fonctions (↓3)
═══════════════════════════════════════════════════
  Tendance globale : ✅ Amélioration
═══════════════════════════════════════════════════
```

Les flèches de tendance sont plus importantes que les valeurs absolues. Un projet avec 234 warnings `modernize` qui en avait 279 le mois précédent est sur la bonne trajectoire. Un projet avec 50 warnings qui en avait 30 le mois précédent accumule de la dette.

---

## Résumé

La dette technique en C++ est amplifiée par les temps de compilation, les contraintes d'ABI, le comportement indéfini latent et la gestion complexe des dépendances. La gérer efficacement repose sur un cycle continu : identifier (marqueurs dans le code, métriques `clang-tidy`, couverture, temps de compilation), mesurer (registre de dette, ratio dette/feature, coût du changement), planifier (règle du Boy Scout, budget de 15-20 %, sprints dédiés) et prévenir (outillage automatisé qui crée un effet de cliquet). L'automatisation avec `clang-tidy --fix` permet de rembourser mécaniquement des catégories entières de dette de modernisation. Et la communication en termes d'investissement et de retour sur investissement — pas en termes techniques — est ce qui débloque le temps nécessaire au remboursement.

---


⏭️ [Semantic Versioning et changelogs](/47-collaboration/06-semantic-versioning.md)
