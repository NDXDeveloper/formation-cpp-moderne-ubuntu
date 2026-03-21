🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 41 — Optimisation CPU et Mémoire ⭐

> **Module 14 : Optimisation de Performance** · Niveau Expert  
> **Partie VI : Sujets Avancés**

---

## Objectif du chapitre

Ce chapitre vous apprend à **penser comme le matériel**. En C++, écrire du code correct ne suffit pas : un algorithme théoriquement optimal peut se révéler catastrophiquement lent si son exécution ignore l'architecture réelle du processeur — hiérarchie de caches, prédiction de branchement, parallélisme d'instructions. L'objectif est de vous donner les connaissances et les outils pour transformer un code fonctionnel en code performant, en agissant sur les deux ressources les plus critiques d'un programme natif : **le temps CPU** et **la mémoire**.

---

## Pourquoi ce sujet est critique

Le C++ est choisi précisément dans les contextes où la performance compte : moteurs de jeu, systèmes embarqués, trading haute fréquence, infrastructure cloud, calcul scientifique. Pourtant, l'écart entre un code C++ "naïf" et un code optimisé peut atteindre **un ou deux ordres de grandeur** sur le même matériel, sans changer d'algorithme.

Les raisons principales de cet écart sont matérielles :

- **Le processeur est rapide, la mémoire est lente.** Un accès au cache L1 prend environ 1 ns ; un accès à la RAM principale en prend 50 à 100 ns. Un programme qui génère constamment des *cache misses* passe l'essentiel de son temps à attendre des données.  
- **Le processeur anticipe, et se trompe parfois.** La prédiction de branchement permet d'exécuter des instructions de manière spéculative. Lorsqu'un branchement est mal prédit, le pipeline est vidé et le coût se chiffre en dizaines de cycles perdus.  
- **Le processeur est parallèle par nature.** Les unités SIMD (*Single Instruction, Multiple Data*) présentes dans tous les CPU modernes peuvent traiter 4, 8 ou 16 valeurs en une seule instruction — mais seulement si le code et les données sont organisés pour l'exploiter.  
- **Le compilateur fait beaucoup, mais pas tout.** Les optimisations automatiques (`-O2`, `-O3`, LTO, PGO) sont puissantes, mais elles restent limitées par la structure du code source. Certaines transformations exigent une intervention explicite du développeur.

Ignorer ces réalités revient à piloter une voiture de course en première vitesse.

---

## Ce que vous allez apprendre

Ce chapitre couvre six axes complémentaires, chacun abordant un levier d'optimisation distinct :

### 41.1 — Comprendre le cache CPU et la localité des données

Le point de départ de toute optimisation sérieuse. Vous apprendrez comment fonctionnent les niveaux de cache (L1, L2, L3), ce qu'est une *cache line*, pourquoi le *false sharing* peut ruiner les performances en multi-thread, et comment le *data-oriented design* permet de repenser l'organisation des données pour maximiser la localité spatiale et temporelle.

### 41.2 — Branch prediction et optimisation des conditions

Les branchements conditionnels (`if`, `switch`, boucles) ont un coût caché lié au pipeline du processeur. Vous découvrirez le mécanisme de prédiction de branchement, les techniques pour écrire du code *branchless* quand c'est pertinent, et comment `[[likely]]` / `[[unlikely]]` (C++20) permettent de guider le compilateur.

### 41.3 — SIMD et vectorisation (SSE, AVX)

Les instructions SIMD permettent de paralléliser des calculs au niveau d'un seul cœur. Vous verrez comment utiliser les *intrinsics* pour un contrôle fin, et comment aider le compilateur à auto-vectoriser vos boucles sans recourir à l'assembleur.

### 41.4 — Profile-Guided Optimization (PGO)

Plutôt que de deviner les *hotpaths*, laissez le compilateur les mesurer. PGO utilise des données d'exécution réelles pour réordonner le code, optimiser les branchements et ajuster l'inlining. Vous apprendrez le workflow complet : instrumentation → exécution → recompilation optimisée.

### 41.5 — Link-Time Optimization (LTO)

Les frontières entre unités de compilation sont un obstacle pour l'optimiseur. LTO repousse l'optimisation à l'étape d'édition de liens, permettant l'inlining inter-fichiers, l'élimination de code mort global et des gains mesurables sur des projets réels — au prix d'un temps de build accru.

### 41.6 — `std::flat_map` / `std::flat_set` et performance cache

Application concrète des principes de localité mémoire aux conteneurs de la STL. Les conteneurs *flat* introduits en C++23 stockent leurs éléments dans un tableau contigu plutôt que dans un arbre de nœuds alloués séparément, ce qui transforme les performances de recherche pour les ensembles de petite et moyenne taille.

> 📎 *La présentation détaillée de `std::flat_map` et `std::flat_set` se trouve en **section 14.4**. La section 41.6 se concentre sur l'analyse de performance et les benchmarks comparatifs dans un contexte d'optimisation cache.*

---

## Prérequis

Pour tirer le meilleur parti de ce chapitre, vous devez maîtriser :

- **La gestion de la mémoire en C++** (chapitre 5) — distinction stack/heap, allocation dynamique, pointeurs.  
- **Les conteneurs de la STL** (chapitres 13–14) — `std::vector`, `std::map`, `std::unordered_map`, et leurs caractéristiques de performance.  
- **Les templates** (chapitre 16) — nécessaires pour comprendre certains patterns d'optimisation générique.  
- **La programmation concurrente** (chapitre 21) — indispensable pour les sections sur le false sharing et les atomiques.  
- **Les outils de profiling** (chapitre 31) — `perf`, flamegraphs. On ne peut pas optimiser ce qu'on ne mesure pas.

---

## Philosophie : Mesurer avant d'optimiser

Un principe traverse l'ensemble de ce chapitre :

> **Ne jamais optimiser à l'aveugle.**

L'optimisation prématurée reste l'un des pièges les plus courants en C++. Avant de réécrire une boucle en SIMD ou de réorganiser une structure de données, il faut **prouver par la mesure** que le code ciblé est réellement un goulot d'étranglement. Les outils vus au chapitre 31 — `perf record`, `perf stat`, flamegraphs — sont vos alliés indispensables.

La démarche systématique est la suivante :

1. **Profiler** — Identifier les *hotspots* réels avec `perf` ou un outil équivalent.
2. **Comprendre** — Analyser *pourquoi* le code est lent (cache misses ? branch mispredictions ? bande passante mémoire saturée ?).
3. **Modifier** — Appliquer l'optimisation ciblée.
4. **Mesurer à nouveau** — Vérifier que le gain est réel et quantifiable.
5. **Documenter** — Expliquer le *pourquoi* de l'optimisation dans le code, car un code optimisé est souvent moins lisible.

Ce cycle s'applique à chaque section du chapitre. Les exemples et benchmarks présentés suivront systématiquement cette discipline.

---

## Environnement et outils utilisés

Les exemples de ce chapitre sont testés avec :

| Outil | Version | Rôle |
|-------|---------|------|
| **GCC** | 15.x | Compilateur principal, support complet C++23/C++26 |
| **Clang** | 20.x | Compilateur alternatif, excellent pour les rapports de vectorisation |
| **perf** | Linux 6.x | Profiling CPU, compteurs matériels |
| **Google Benchmark** | 1.9+ | Micro-benchmarking reproductible |
| **Valgrind (cachegrind)** | 3.23+ | Simulation de cache pour analyse de localité |
| **Compiler Explorer** | godbolt.org | Inspection du code assembleur généré |

Les flags de compilation recommandés pour les benchmarks d'optimisation :

```bash
# Build optimisé pour benchmark
g++ -std=c++23 -O2 -march=native -DNDEBUG -o bench bench.cpp

# Build avec rapports de vectorisation (Clang)
clang++ -std=c++23 -O2 -march=native -Rpass=loop-vectorize \
        -Rpass-missed=loop-vectorize -o bench bench.cpp

# Build avec instrumentation PGO (phase 1)
g++ -std=c++23 -O2 -fprofile-generate -o bench_instrumented bench.cpp
```

> ⚠️ **Attention** : utilisez `-O2` (et non `-O3`) comme baseline de benchmark. `-O3` active des optimisations agressives (déroulement de boucles, vectorisation intensive) qui peuvent masquer l'impact de vos propres optimisations ou introduire des régressions inattendues. Passez à `-O3` uniquement après avoir mesuré son effet réel sur votre cas d'usage.

---

## Vue d'ensemble des sections

| Section | Thème | Levier principal |
|---------|-------|-----------------|
| 41.1 | Cache CPU et localité | Réduire les *cache misses* |
| 41.2 | Branch prediction | Réduire les *mispredictions* |
| 41.3 | SIMD et vectorisation | Exploiter le parallélisme d'instructions |
| 41.4 | PGO | Laisser le profiling guider le compilateur |
| 41.5 | LTO | Optimiser au-delà des frontières de fichiers |
| 41.6 | Conteneurs flat | Appliquer la localité aux structures de données |

---


⏭️ [Comprendre le cache CPU et la localité des données](/41-optimisation-cpu-memoire/01-cache-cpu.md)
