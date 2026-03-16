🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Partie IV — Tooling et Build Systems

Cette partie traite de tout ce qui entoure le code. Savoir écrire du C++ correct ne suffit pas en contexte professionnel — il faut savoir le compiler efficacement (CMake, Ninja, Conan), le déboguer (GDB, sanitizers, core dumps), le profiler (perf, flamegraphs, Valgrind), l'analyser statiquement (clang-tidy, cppcheck), le tester (Google Test, Google Mock) et en mesurer la performance (Google Benchmark). Un développeur C++ qui ne maîtrise pas son outillage produit du code qu'il ne peut ni diagnostiquer ni maintenir à l'échelle.

---

## Ce que vous allez maîtriser

- Vous serez capable de structurer un projet CMake moderne basé sur les targets (`target_link_libraries`, `PUBLIC`/`PRIVATE`/`INTERFACE`), avec génération Ninja et CMake Presets.
- Vous serez capable de gérer les dépendances externes avec Conan 2.0 (`conanfile.py`, profils, intégration CMake) et de choisir entre linkage statique (`.a`) et dynamique (`.so`).
- Vous serez capable de naviguer entre Make, Ninja et Meson, et de justifier le choix du build system selon le contexte du projet.
- Vous serez capable de déboguer un programme C++ avec GDB (breakpoints conditionnels, watchpoints, core dumps) et via l'intégration IDE (VS Code, CLion).
- Vous serez capable d'instrumenter votre code avec les quatre sanitizers (Address, UndefinedBehavior, Thread, Memory) et d'interpréter leurs rapports.
- Vous serez capable de détecter et résoudre des fuites mémoire avec Valgrind (memcheck, Massif) et de profiler les allocations heap.
- Vous serez capable de profiler la performance CPU avec `perf` (sampling, compteurs matériels) et de générer des flamegraphs pour identifier les hotspots.
- Vous serez capable de configurer clang-tidy et clang-format sur un projet et de les intégrer dans le workflow de développement.
- Vous serez capable d'écrire des tests unitaires avec Google Test (fixtures, tests paramétrés) et des mocks avec Google Mock, en suivant une approche TDD.
- Vous serez capable de mesurer la couverture de code avec gcov/lcov et de produire des rapports HTML intégrés à CMake.
- Vous serez capable d'écrire des micro-benchmarks fiables avec Google Benchmark et d'interpréter correctement les résultats.

---

## Prérequis

- **Partie I — Fondations** : toolchain de base (chapitre 2), gestion mémoire manuelle (chapitre 5) — indispensable pour comprendre ce que les sanitizers et Valgrind détectent.
- **Partie II — C++ Moderne** : smart pointers (chapitre 9), templates (chapitre 16), gestion d'erreurs (chapitre 17) — le code que vous allez compiler, tester et profiler utilise ces patterns.
- **Partie III — Programmation Système Linux** : threads (chapitre 21) — nécessaire pour exploiter ThreadSanitizer et le profiling de programmes concurrents.

---

## Modules de cette partie

| # | Titre | Niveau | Chapitres | Lien |
|---|-------|--------|-----------|------|
| Module 9 | Build Systems et Gestion de Projet | Avancé | 26, 27, 28 | [module-09-build-systems.md](/module-09-build-systems.md) |
| Module 10 | Débogage, Profiling et Qualité Code | Avancé | 29, 30, 31, 32 | [module-10-debogage-profiling.md](/module-10-debogage-profiling.md) |
| Module 11 | Tests et Qualité Logicielle | Avancé | 33, 34, 35 | [module-11-tests-qualite.md](/module-11-tests-qualite.md) |

---

## Fil conducteur

Le Module 9 pose l'infrastructure de build : CMake comme générateur central, Conan 2.0 et vcpkg pour les dépendances, Ninja comme backend d'exécution rapide, et Meson comme alternative à connaître. Sans cette base, les outils des modules suivants n'ont pas de projet structuré sur lequel s'appliquer. Le Module 10 s'attaque au diagnostic : débogage avec GDB et sanitizers (chapitre 29), analyse mémoire avec Valgrind (chapitre 30), profiling CPU avec `perf` et flamegraphs (chapitre 31), puis analyse statique avec clang-tidy et cppcheck (chapitre 32). La progression va du runtime (bugs, fuites, performances) vers le statique (détection sans exécution). Le Module 11 ferme la boucle en formalisant la qualité : tests unitaires avec Google Test (chapitre 33), couverture de code avec gcov/lcov intégrée à CMake (chapitre 34), et benchmarking avec Google Benchmark (chapitre 35). À la sortie de cette partie, vous disposez d'un pipeline complet — build, diagnostic, test, mesure — prêt à être automatisé dans un workflow CI/CD (Partie V).

---


⏭️ [Module 9 : Build Systems et Gestion de Projet](/module-09-build-systems.md)
