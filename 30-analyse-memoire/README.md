🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 30 : Analyse Mémoire

## Module 10 : Débogage, Profiling et Qualité Code — Niveau Avancé

---

## Vue d'ensemble

En C++, la gestion manuelle de la mémoire offre un contrôle inégalé sur les ressources système, mais elle constitue aussi l'une des sources les plus fréquentes de bugs critiques. Les fuites mémoire (*memory leaks*), les accès hors limites (*buffer overflows*), les lectures de mémoire non initialisée ou les doubles libérations (*double free*) sont des défauts qui partagent une caractéristique commune : ils sont souvent silencieux. Un programme peut fonctionner apparemment sans problème pendant des jours, voire des semaines, avant qu'une fuite mémoire ne provoque un crash en production ou qu'un accès invalide ne soit exploité comme faille de sécurité.

Les techniques de débogage classiques — `std::print`, breakpoints GDB, revue de code — atteignent rapidement leurs limites face à ces problèmes. Un pointeur qui référence une zone mémoire libérée peut retourner des valeurs cohérentes pendant la majorité des exécutions et ne provoquer un comportement indéfini (*undefined behavior*) que dans des conditions spécifiques de charge ou de timing. C'est précisément pour cette raison que l'analyse mémoire nécessite des outils spécialisés, capables d'instrumenter l'exécution d'un programme pour traquer chaque allocation, chaque libération et chaque accès.

Ce chapitre se concentre sur les outils et méthodologies d'analyse mémoire disponibles sur Linux, en complément des sanitizers abordés au chapitre 29. Là où les sanitizers fonctionnent par instrumentation à la compilation, les outils présentés ici — Valgrind en tête — opèrent à un niveau différent : ils interceptent dynamiquement les appels mémoire au moment de l'exécution, sans nécessiter de recompilation du code source.

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez en mesure de :

- Utiliser **Valgrind** et son outil **Memcheck** pour détecter les fuites mémoire, les accès invalides et les utilisations de mémoire non initialisée dans un programme C++.
- Lire et interpréter les rapports générés par Valgrind afin d'identifier précisément l'origine des défauts dans le code source.
- Profiler la consommation mémoire d'un programme dans le temps avec **Massif**, l'outil de *heap profiling* de Valgrind, et analyser les snapshots produits.
- Mettre en place une stratégie systématique de détection et de résolution des fuites mémoire, depuis l'identification du symptôme jusqu'au correctif.

---

## Plan du chapitre

- **30.1 — Valgrind : Détection de fuites et erreurs mémoire**
  Présentation de Valgrind et de son outil principal Memcheck. Installation sur Ubuntu, première exécution, détection de fuites et d'accès mémoire invalides, puis lecture détaillée des rapports d'erreurs.

- **30.2 — Heap profiling avec Massif**
  Analyse de la consommation mémoire au fil du temps. Utilisation de Massif pour identifier les pics d'allocation, les fonctions les plus consommatrices et les opportunités d'optimisation.

- **30.3 — Memory leaks : Détection et résolution**
  Méthodologie complète pour traquer les fuites mémoire dans un projet réel. Patterns courants de fuites, stratégies de correction, et articulation avec les smart pointers et le RAII pour les prévenir à la source.

---

## Prérequis

Ce chapitre s'appuie sur plusieurs notions abordées précédemment dans la formation :

- **Chapitre 5 — Gestion de la Mémoire** : compréhension du modèle mémoire d'un processus (stack vs heap), de l'allocation dynamique avec `new`/`delete`, et des dangers classiques (dangling pointers, double free). Les concepts de ce chapitre fondateur sont ici mis en pratique à travers l'outillage.
- **Chapitre 9 — Smart Pointers** : `std::unique_ptr`, `std::shared_ptr` et `std::weak_ptr` constituent la réponse du C++ moderne aux problèmes de fuites mémoire. L'analyse mémoire révèle souvent des situations où l'adoption de smart pointers aurait évité le défaut.
- **Chapitre 29 — Débogage Avancé** : les sanitizers (AddressSanitizer, MemorySanitizer) présentés en section 29.4 sont complémentaires aux outils de ce chapitre. Comprendre leurs différences d'approche — instrumentation à la compilation vs interception dynamique — permet de choisir le bon outil selon le contexte.

Une familiarité avec la ligne de commande Linux et la compilation avec `g++` ou `clang++` est également nécessaire.

---

## Positionnement : Sanitizers vs Valgrind

Le chapitre 29 a introduit les sanitizers (`-fsanitize=address`, `-fsanitize=memory`), qui détectent les erreurs mémoire par instrumentation du binaire à la compilation. Ce chapitre aborde une approche complémentaire avec Valgrind, qui fonctionne sans recompilation en émulant l'exécution du programme.

Chaque approche a ses forces :

| Critère | Sanitizers (ASan, MSan) | Valgrind (Memcheck) |
|---|---|---|
| **Recompilation requise** | Oui (`-fsanitize=...`) | Non |
| **Surcoût à l'exécution** | ~2× | ~10–20× |
| **Précision des rapports** | Très élevée | Très élevée |
| **Détection de fuites** | Basique (LeakSanitizer) | Détaillée (catégorisée) |
| **Heap profiling** | Non | Oui (Massif) |
| **Compatibilité binaires tiers** | Non (source requise) | Oui |
| **Intégration CI** | Rapide (peu de surcoût) | Plus lent |

En pratique, une stratégie robuste combine les deux : les sanitizers en intégration continue pour leur rapidité, et Valgrind pour les analyses approfondies, le profiling mémoire, ou l'inspection de binaires tiers dont on ne dispose pas du code source.

---

## Conventions

Tout au long de ce chapitre, les exemples sont compilés sur Ubuntu avec GCC 15 (`g++`) en mode debug :

```bash
g++ -std=c++23 -g -O0 -o mon_programme mon_programme.cpp
```

L'option `-g` génère les informations de débogage nécessaires pour que Valgrind puisse associer les erreurs aux lignes du code source. L'option `-O0` désactive les optimisations du compilateur, qui peuvent rendre les rapports difficiles à interpréter en réorganisant ou en éliminant du code.

> ⚠️ **Important** : Valgrind et AddressSanitizer ne peuvent pas être utilisés simultanément sur le même binaire. Si votre binaire a été compilé avec `-fsanitize=address`, recompilez-le sans ce flag avant de l'analyser avec Valgrind.

⏭️ [Valgrind : Détection de fuites et erreurs mémoire](/30-analyse-memoire/01-valgrind.md)
