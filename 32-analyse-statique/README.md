🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 32 : Analyse Statique et Linting

## Module 10 : Débogage, Profiling et Qualité Code — Niveau Avancé

---

## Vue d'ensemble

Les chapitres précédents ont couvert les outils d'analyse *dynamique* : Valgrind et les sanitizers détectent les erreurs mémoire pendant l'exécution (chapitre 29–30), `perf` et `gprof` profilent la performance en temps réel (chapitre 31). Ces outils partagent une caractéristique commune — ils nécessitent que le programme soit exécuté, avec des données d'entrée suffisamment variées pour exercer les chemins de code problématiques. Un bug qui se manifeste uniquement sur un chemin d'exécution rare peut passer à travers des milliers d'exécutions de tests sans être détecté.

L'**analyse statique** adopte l'approche inverse : elle examine le code source *sans l'exécuter* et détecte des catégories entières de bugs par raisonnement sur la structure du programme. Un analyseur statique parcourt l'arbre syntaxique abstrait (AST), le graphe de flot de contrôle et le graphe de flot de données pour identifier des patterns problématiques : variables non initialisées, pointeurs potentiellement nuls déréférencés, branches mortes, violations de bonnes pratiques, conversions de types dangereuses, ou encore violations des C++ Core Guidelines.

Le **linting** est une forme légère d'analyse statique centrée sur le style et les conventions. Un linter vérifie le formatage du code (indentation, espaces, longueur des lignes), la cohérence des noms de variables, et les conventions spécifiques au projet. En C++, la frontière entre analyse statique et linting est floue : un outil comme `clang-tidy` combine la détection de bugs réels (analyse statique profonde) et la vérification de conventions de codage (linting), tandis que `clang-format` est un outil de formatage pur.

L'intérêt de ces outils est qu'ils s'exécutent en amont du cycle de développement — avant la compilation dans le cas du formatage, avant les tests dans le cas de l'analyse statique. Un bug détecté par l'analyseur statique avant même le premier `make` est un bug qui ne coûte presque rien à corriger. Le même bug découvert en production après un incident coûte des ordres de grandeur de plus en temps, en stress et en confiance des utilisateurs. C'est cette asymétrie qui justifie l'investissement dans une chaîne d'analyse statique intégrée au workflow de développement.

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez en mesure de :

- Configurer et utiliser **clang-tidy** pour détecter automatiquement les bugs, les anti-patterns et les violations des bonnes pratiques dans du code C++ moderne.
- Sélectionner les **checks** adaptés à votre projet parmi les centaines disponibles dans clang-tidy, et les organiser dans un fichier `.clang-tidy` versionné avec le projet.
- Utiliser **cppcheck** comme outil complémentaire de détection d'erreurs, en comprenant ses forces et ses limites par rapport à clang-tidy.
- Configurer **clang-format** pour imposer un formatage cohérent et automatique sur l'ensemble d'un projet, en s'appuyant sur un fichier `.clang-format` partagé par l'équipe.
- Intégrer ces outils dans le **workflow quotidien** — IDE, pre-commit hooks, pipeline CI — pour que l'analyse statique et le formatage soient des filets de sécurité automatiques et non des corvées manuelles.

---

## Plan du chapitre

- **32.1 — clang-tidy : Analyse statique moderne**
  L'outil central de l'analyse statique en C++. Configuration du fichier `.clang-tidy`, sélection des checks par catégorie (bugprone, performance, modernize, cppcoreguidelines), exécution en ligne de commande et via l'IDE, correction automatique avec `--fix`.

- **32.2 — cppcheck : Détection d'erreurs**
  Un analyseur statique complémentaire, indépendant du compilateur. Installation, utilisation, catégories d'erreurs détectées (fuites de ressources, buffer overflows, variables non initialisées), et intégration dans CMake.

- **32.3 — clang-format : Formatage automatique**
  Le formateur de code standard de l'écosystème LLVM. Configuration du fichier `.clang-format`, choix d'un style de base (Google, LLVM, Mozilla), personnalisation des règles, formatage à la sauvegarde dans l'IDE.

- **32.4 — Intégration dans le workflow de développement**
  Mise en place d'une chaîne complète : analyse statique et formatage automatiques dans l'IDE, pre-commit hooks pour valider avant chaque commit, jobs CI pour garantir la qualité sur la branche principale.

---

## Prérequis

Ce chapitre s'appuie sur les connaissances suivantes :

- **Chapitre 2 — Toolchain sur Ubuntu** : installation des compilateurs GCC et Clang, familiarité avec les options de compilation. clang-tidy et clang-format font partie de l'écosystème LLVM/Clang.
- **Chapitre 26 — CMake** : les outils d'analyse statique s'intègrent dans le build system via CMake. La propriété `CMAKE_CXX_CLANG_TIDY` permet d'exécuter clang-tidy à chaque compilation.
- **Chapitre 29 — Débogage Avancé** : les sanitizers (AddressSanitizer, UBSan) détectent des bugs à l'exécution que l'analyse statique peut aussi identifier à la compilation. Comprendre la complémentarité entre les deux approches est essentiel.

Une familiarité avec un éditeur de code configuré pour C++ (VS Code avec clangd, CLion, ou Vim/Neovim avec un LSP) est recommandée, car l'intégration dans l'IDE est l'un des points forts de ces outils.

---

## Analyse statique vs analyse dynamique : complémentarité

L'analyse statique et l'analyse dynamique ne sont pas des approches concurrentes — elles sont complémentaires, chacune détectant des catégories de bugs que l'autre manque.

### Ce que l'analyse statique détecte bien

L'analyse statique excelle pour les bugs qui relèvent de la structure du code plutôt que de son comportement à l'exécution :

- **Bugs de logique locale** : variable non initialisée avant utilisation, branche conditionnelle toujours vraie ou toujours fausse, division par zéro quand le diviseur provient d'une constante.
- **Violations de type-safety** : conversions implicites dangereuses (`int` vers `short` avec troncature possible), utilisation de `reinterpret_cast` sans justification, comparaisons signées/non signées.
- **Anti-patterns et code modernisable** : utilisation de `new`/`delete` au lieu de smart pointers, boucles `for` avec index au lieu de range-based for, `NULL` au lieu de `nullptr`.
- **Violations de conventions** : non-respect des C++ Core Guidelines, utilisation de fonctions dépréciées, nommage incohérent.
- **Complexité et maintenabilité** : fonctions trop longues, trop de paramètres, imbrication excessive, code dupliqué.

### Ce que l'analyse statique manque

Certains bugs sont fondamentalement hors de portée de l'analyse statique, car ils dépendent de l'état du programme à l'exécution :

- **Bugs dépendant des données d'entrée** : un buffer overflow qui ne se produit que pour une taille d'entrée spécifique, un comportement indéfini déclenché uniquement par certaines combinaisons de paramètres.
- **Data races** : la détection de conditions de course entre threads nécessite de modéliser tous les entrelacements possibles, ce qui est en théorie un problème indécidable. ThreadSanitizer (section 29.4.3) est bien plus efficace pour cette catégorie.
- **Fuites mémoire complexes** : une fuite qui dépend du chemin d'exécution à travers plusieurs fonctions et modules est difficile à détecter par analyse statique seule. Memcheck (chapitre 30) reste l'outil de référence.
- **Problèmes de performance** : l'analyse statique peut signaler des patterns connus pour être lents (copie inutile, allocation dans une boucle), mais elle ne peut pas quantifier l'impact réel. `perf` (chapitre 31) est irremplaçable pour cela.

### Stratégie combinée

Une chaîne de qualité robuste combine les deux approches :

| Étape | Outil | Ce qu'il attrape |
|---|---|---|
| Édition (temps réel) | clangd + clang-tidy dans l'IDE | Bugs locaux, anti-patterns, style |
| Pre-commit | clang-format + clang-tidy | Formatage, violations de conventions |
| Compilation | clang-tidy via CMake | Analyse plus profonde (cross-function) |
| Tests | Sanitizers (ASan, UBSan, TSan) | Bugs d'exécution, UB, data races |
| Nightly | Valgrind, cppcheck (analyse complète) | Fuites mémoire, analyse exhaustive |

Chaque couche attrape des bugs que les couches précédentes ont laissé passer. L'objectif n'est pas de choisir entre analyse statique et dynamique, mais de les superposer pour réduire la surface de bugs qui atteignent la production.

---

## L'écosystème des outils d'analyse statique C++ en 2026

L'écosystème d'analyse statique pour C++ est riche. Ce chapitre se concentre sur les outils open source les plus utilisés et les plus pertinents pour un développeur sur Linux :

| Outil | Type | Licence | Intégration IDE | Points forts |
|---|---|---|---|---|
| **clang-tidy** | Analyse statique + linting | Apache 2.0 (LLVM) | Excellente (clangd) | 400+ checks, fix automatique, C++ Core Guidelines |
| **cppcheck** | Analyse statique | GPL | Bonne (plugins) | Indépendant du compilateur, faible taux de faux positifs |
| **clang-format** | Formatage | Apache 2.0 (LLVM) | Excellente | Standard de facto, hautement configurable |
| clangd | Serveur LSP | Apache 2.0 (LLVM) | Native | Diagnostics temps réel, intègre clang-tidy |
| PVS-Studio | Analyse statique | Propriétaire | Bonne | Analyse interprocédurale poussée |
| SonarQube (C++) | Analyse statique + métriques | Propriétaire | CI/CD | Tableau de bord projet, dette technique |
| Coverity | Analyse statique | Propriétaire | CI/CD | Référence industrielle, très faible taux de faux positifs |

Les outils propriétaires (PVS-Studio, SonarQube, Coverity) offrent une analyse interprocédurale plus poussée et des tableaux de bord projet, mais leur coût de licence les réserve généralement aux grandes organisations. Pour un développeur individuel ou une équipe de taille moyenne, la combinaison **clang-tidy + cppcheck + clang-format** couvre l'essentiel des besoins avec un outillage entièrement gratuit et open source.

---

## Conventions

Les exemples de ce chapitre utilisent les versions d'outils disponibles sur Ubuntu en 2026 :

```bash
# Versions de référence
clang-tidy --version   # LLVM 19/20  
cppcheck --version     # Cppcheck 2.x  
clang-format --version # LLVM 19/20  
```

Les outils LLVM (clang-tidy, clang-format, clangd) partagent le même numéro de version que Clang. Ils sont installés ensemble via le paquet `clang-tools` ou individuellement. Les sections qui suivent détaillent l'installation de chaque outil.

Les fichiers de configuration (`.clang-tidy`, `.clang-format`) sont placés à la racine du projet et versionnés avec le code source dans Git. Cela garantit que tous les développeurs et le pipeline CI utilisent exactement les mêmes règles.

⏭️ [clang-tidy : Analyse statique moderne](/32-analyse-statique/01-clang-tidy.md)
