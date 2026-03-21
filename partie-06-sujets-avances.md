🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Partie VI — Sujets Avancés

Cette partie s'adresse aux développeurs qui veulent exploiter C++ à la limite de ses capacités. Optimisation cache-aware, SIMD, PGO, LTO, lock-free programming, interopérabilité avec Python/Rust/WebAssembly, design patterns idiomatiques, safety profiles, fuzzing — ce sont les compétences qui séparent un développeur senior d'un architecte système ou d'un expert performance. Le niveau est exigeant, mais les sujets traités ici sont ceux qui font la différence dans les rôles de system programmer, développeur infrastructure, ou lead technique sur des projets critiques.

---

## Ce que vous allez maîtriser

- Vous serez capable d'optimiser la localité mémoire d'un programme en comprenant la hiérarchie de cache CPU (L1/L2/L3), les cache lines, le false sharing, et en appliquant les principes du data-oriented design.
- Vous serez capable d'exploiter la vectorisation SIMD (SSE, AVX) via les intrinsics et l'auto-vectorisation du compilateur, et de vérifier que le compilateur vectorise effectivement vos boucles.
- Vous serez capable d'appliquer Profile-Guided Optimization (PGO) et Link-Time Optimization (LTO) pour obtenir des gains de performance mesurables sur des binaires de production.
- Vous serez capable d'implémenter des structures de données lock-free basées sur les opérations compare-and-swap (CAS), avec une compréhension correcte du memory ordering et des barrières mémoire.
- Vous serez capable d'exposer du code C++ à Python via pybind11 ou nanobind, avec gestion des types, des conversions et du GIL.
- Vous serez capable de mettre en place une interopérabilité C++↔Rust via `cxx` ou `autocxx`, et d'évaluer une stratégie de migration progressive vers Rust sur un codebase existant.
- Vous serez capable de compiler du C++ vers WebAssembly avec Emscripten et de l'intégrer dans une application JavaScript.
- Vous serez capable d'appliquer les design patterns adaptés au C++ moderne : Singleton thread-safe, Factory, CRTP, type erasure, dependency injection.
- Vous serez capable d'identifier et de prévenir les vulnérabilités classiques en C++ (buffer overflow, integer overflow, use-after-free) via les flags de compilation (`-fstack-protector`, `-D_FORTIFY_SOURCE`, PIE/ASLR).
- Vous serez capable de mettre en place du fuzzing avec AFL++ et LibFuzzer pour détecter des bugs et vulnérabilités automatiquement.
- Vous serez capable de positionner votre projet par rapport aux safety profiles C++26 et au contexte réglementaire 2026 (NSA, CISA, Cyber Resilience Act), et de définir une stratégie de hardening adaptée.

---

## Prérequis

- **Partie II — C++ Moderne** : templates et concepts (chapitre 16), move semantics (chapitre 10), `std::variant`/`std::any` (chapitre 12) — nécessaires pour les design patterns, le type erasure et le CRTP.
- **Partie III — Programmation Système Linux** : threads et atomiques (chapitre 21), networking (chapitre 22) — indispensables pour le lock-free programming et les optimisations concurrentes.
- **Partie IV — Tooling et Build Systems** : profiling avec `perf` (chapitre 31), sanitizers (chapitre 29), CMake (chapitre 26) — les outils de mesure et de build sont utilisés intensivement dans cette partie.

---

## Modules de cette partie

| # | Titre | Niveau | Chapitres | Lien |
|---|-------|--------|-----------|------|
| Module 14 | Optimisation de Performance | Expert | 41, 42 | [module-14-optimisation.md](/module-14-optimisation.md) |
| Module 15 | Interopérabilité | Expert | 43 | [module-15-interoperabilite.md](/module-15-interoperabilite.md) |
| Module 16 | Patterns et Architecture | Expert | 44, 45 | [module-16-patterns-architecture.md](/module-16-patterns-architecture.md) |

---

## Fil conducteur

Le Module 14 pousse la performance au maximum : d'abord la compréhension du matériel (cache CPU, branch prediction, SIMD) au chapitre 41, puis la descente au niveau le plus bas (inline assembly, manipulation de bits, memory ordering, lock-free) au chapitre 42. Cette maîtrise du hardware est ce qui permet d'écrire du code que les langages managés ne peuvent pas concurrencer. Le Module 15 traite la question inverse : comment faire coexister C++ avec d'autres langages. Python via pybind11/nanobind pour le scripting et le machine learning, Rust via `cxx`/`autocxx` pour la sécurité mémoire et la migration progressive, WebAssembly via Emscripten pour le web. Le Module 16 consolide l'ensemble avec les patterns de conception adaptés au C++ moderne (chapitre 44) et la sécurité (chapitre 45) — buffer overflows, fuzzing avec AFL++/LibFuzzer, et surtout le positionnement vis-à-vis des safety profiles et du contexte réglementaire 2026. À la sortie de cette partie, vous avez les compétences d'un expert capable d'optimiser, d'intégrer et de sécuriser du C++ dans un environnement de production exigeant.

---


⏭️ [Module 14 : Optimisation de Performance](/module-14-optimisation.md)
