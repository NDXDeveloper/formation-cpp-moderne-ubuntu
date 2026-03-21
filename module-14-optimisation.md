🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 14 — Optimisation de Performance

> 🎯 Niveau : Expert

Ce module traite de l'optimisation guidée par la compréhension du matériel. Cache CPU, false sharing, branch prediction, SIMD, PGO, LTO, puis descente au niveau le plus bas : inline assembly, manipulation de bits, memory ordering, lock-free programming. Ce ne sont pas des techniques à appliquer partout — elles ne se justifient qu'après profilage, sur les hotspots identifiés. Mais quand elles se justifient, elles produisent des gains d'un ordre de grandeur que les optimisations de haut niveau ne peuvent pas atteindre.

---

## Objectifs pédagogiques

1. **Comprendre** la hiérarchie de cache CPU (L1/L2/L3), les cache lines, le false sharing, et appliquer les principes du data-oriented design pour maximiser la localité des données.
2. **Exploiter** la vectorisation SIMD (SSE, AVX) via les intrinsics et l'auto-vectorisation du compilateur, et vérifier que le compilateur vectorise effectivement les boucles critiques.
3. **Appliquer** Profile-Guided Optimization (PGO) et Link-Time Optimization (LTO) sur des binaires de production.
4. **Implémenter** des structures de données lock-free basées sur compare-and-swap (CAS) avec une compréhension correcte du memory ordering.
5. **Utiliser** les barrières mémoire (`memory_order_acquire`, `release`, `seq_cst`, `relaxed`) en sachant quand chaque niveau est approprié.
6. **Manipuler** les bits et les bitfields pour le code proche du hardware, et lire/écrire de l'inline assembly quand c'est la seule option.

---

## Prérequis

- **Module 10, chapitre 31** : profiling avec `perf` (sampling, compteurs matériels, flamegraphs) — toute optimisation commence par une mesure. Sans profil, vous optimisez à l'aveugle.
- **Module 11, chapitre 35** : benchmarking avec Google Benchmark — les micro-benchmarks sont le moyen de valider que les optimisations de ce module produisent un gain réel.
- **Module 7, chapitre 21** : threads, atomiques, `std::atomic` — le chapitre 42 (lock-free) étend directement ces concepts avec le memory ordering avancé.
- **Module 5, chapitre 14** : conteneurs associatifs — la section 41.6 compare les performances cache de `std::flat_map`/`std::flat_set` vs `std::map` dans un contexte d'optimisation.

---

## Chapitres

### Chapitre 41 — Optimisation CPU et Mémoire

Optimisations guidées par l'architecture matérielle. Ce chapitre part du cache CPU (le facteur de performance le plus sous-estimé), passe par la vectorisation SIMD, et se termine par les optimisations du compilateur (PGO, LTO).

- **Cache CPU et localité des données** : hiérarchie L1 (4 cycles), L2 (~10 cycles), L3 (~40 cycles), RAM (~200 cycles). Cache lines (typiquement 64 octets), impact de la disposition des données sur le cache hit rate. False sharing : deux threads qui écrivent sur des variables situées sur la même cache line forcent des invalidations croisées — diagnostic avec `perf c2c`.
- **Data-oriented design** : organiser les données par accès (Structure of Arrays vs Array of Structures), aligner les structures sur les cache lines (`alignas(64)`), réduire la taille des structures pour maximiser la densité dans le cache.
- **Branch prediction** : comment le CPU prédit les branchements, coût d'un misprediction (~15-20 cycles), techniques pour aider le prédicteur (`[[likely]]`/`[[unlikely]]` C++20, tri des conditions par probabilité, branchless programming).
- **SIMD et vectorisation** : instructions SSE (128 bits, 4 floats), AVX (256 bits, 8 floats), AVX-512 (512 bits). Intrinsics (`_mm256_add_ps`, `_mm256_mul_ps`) pour le contrôle direct, auto-vectorisation du compilateur pour les boucles simples.
- **Auto-vectorisation** : rapports du compilateur (`-fopt-info-vec` GCC, `-Rpass=loop-vectorize` Clang) pour vérifier quelles boucles sont vectorisées et pourquoi les autres ne le sont pas.
- **Profile-Guided Optimization (PGO)** : compilation initiale avec instrumentation (`-fprofile-generate`), exécution avec des données représentatives, recompilation guidée par le profil (`-fprofile-use`). Gains typiques : 10-20% sur les chemins chauds.
- **Link-Time Optimization (LTO)** : optimisation inter-modules à l'édition de liens (`-flto`), inlining cross-module, dead code elimination globale. Compatible avec PGO pour des gains cumulés.
- **`std::flat_map`/`std::flat_set` et performance cache** : benchmark comparatif avec `std::map` montrant l'impact de la localité mémoire contiguë sur les lectures fréquentes.

> 📎 Pour la couverture fonctionnelle de `std::flat_map`/`std::flat_set`, voir section 14.4.

### Chapitre 42 — Programmation Bas Niveau

Le niveau le plus bas accessible en C++ sans écrire de l'assembleur pur. Ce chapitre est réservé aux cas où les abstractions du langage ne suffisent plus — drivers, moteurs de jeu, systèmes temps réel, structures concurrentes haute performance.

- **Inline assembly** : syntaxe GCC/Clang (`asm volatile("..." : outputs : inputs : clobbers)`), contraintes de registres, lecture de registres spéciaux (TSC pour le timestamping, CPUID). À utiliser en dernier recours — le compilateur optimise souvent mieux que l'humain.
- **Manipulation de bits et bitfields** : opérations bit à bit (`&`, `|`, `^`, `<<`, `>>`), bitfields dans les structs, applications (flags compacts, protocoles réseau, registres hardware). `std::bit_cast` (C++20) pour les réinterprétations sûres.
- **Memory ordering et barrières mémoire** : les six ordres de mémoire C++ (`relaxed`, `consume`, `acquire`, `release`, `acq_rel`, `seq_cst`), ce que chacun garantit et ce qu'il ne garantit pas. Relation happens-before, synchronizes-with. Impact sur les architectures faiblement ordonnées (ARM, RISC-V) vs fortement ordonnées (x86).
- **Lock-free programming** : structures de données sans mutex — stacks, queues, compteurs lock-free. Compare-and-swap (CAS) : `std::atomic::compare_exchange_weak` et `compare_exchange_strong`, spurious failures, boucle CAS. ABA problem et solutions (tagged pointers, hazard pointers).

---

## Points de vigilance

- **Optimiser avant de profiler.** L'anti-pattern le plus courant en optimisation : réécrire du code "pour la performance" sans avoir mesuré où le temps est réellement passé. Dans la majorité des cas, le hotspot n'est pas où on le pense. Règle : profilez d'abord avec `perf` et des flamegraphs (Module 10), identifiez les fonctions qui consomment le plus de CPU, puis optimisez ces fonctions uniquement. Mesurez après chaque modification avec Google Benchmark pour confirmer le gain.

- **False sharing non détecté sans outils.** Deux threads qui modifient des variables adjacentes en mémoire (même cache line) provoquent des invalidations croisées permanentes entre les caches L1 des cœurs. Le programme est correct — il est juste lent, sans explication évidente dans le code. `perf c2c` (cache-to-cache) détecte les lignes de cache disputées entre cœurs. Solution : aligner les données par thread sur des cache lines séparées (`alignas(std::hardware_destructive_interference_size)` ou `alignas(64)`).

- **Auto-vectorisation échouée silencieusement.** Le compilateur peut décider de ne pas vectoriser une boucle pour de nombreuses raisons : dépendances de données, aliasing possible, types non vectorisables, boucle trop complexe. Sans activer les rapports de vectorisation (`-fopt-info-vec-missed` GCC, `-Rpass-missed=loop-vectorize` Clang), vous ne savez pas que la boucle n'est pas vectorisée. Activez ces rapports systématiquement sur les boucles critiques identifiées par le profiler, et adaptez le code si nécessaire (`__restrict__`, simplification de la boucle, `#pragma omp simd`).

- **`memory_order_relaxed` mal compris dans le lock-free programming.** `relaxed` ne fournit aucune garantie d'ordre entre les opérations atomiques — il garantit seulement l'atomicité de chaque opération individuelle. Sur x86, le code avec `relaxed` semble fonctionner (car x86 est fortement ordonné), mais il casse sur ARM ou RISC-V (faiblement ordonnés). Règle : commencez par `memory_order_seq_cst` (le plus strict, le plus sûr), puis relâchez vers `acquire`/`release` ou `relaxed` uniquement après avoir prouvé la correction et mesuré un gain de performance. Utilisez TSan pour valider la correction des chemins concurrents.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Diagnostiquer les problèmes de localité cache avec `perf stat` (cache misses) et `perf c2c` (false sharing), et restructurer les données en conséquence.
- Vérifier que le compilateur vectorise les boucles critiques et intervenir (intrinsics, pragmas, restructuration) quand ce n'est pas le cas.
- Appliquer PGO et LTO pour obtenir des gains mesurables sur des binaires de production.
- Implémenter des structures lock-free avec CAS en utilisant le memory ordering correct, et valider la correction avec TSan.
- Lire et écrire de l'inline assembly quand les abstractions C++ ne suffisent pas.
- Prendre des décisions d'optimisation fondées sur des mesures, pas sur des intuitions.

---


⏭️ [Optimisation CPU et Mémoire](/41-optimisation-cpu-memoire/README.md)
