🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 11 — Tests et Qualité Logicielle

> 🎯 Niveau : Avancé

Ce module formalise la qualité du code par la preuve : tests unitaires avec Google Test, couverture de code avec gcov/lcov, et benchmarking avec Google Benchmark. Les outils du Module 10 (sanitizers, Valgrind, clang-tidy) détectent des bugs existants — les outils de ce module prouvent que le code fonctionne correctement, mesurent ce qui est effectivement testé, et quantifient la performance de manière reproductible. Un projet C++ professionnel sans tests automatisés est un projet qui régresse à chaque commit.

---

## Objectifs pédagogiques

1. **Écrire** des tests unitaires avec Google Test : tests simples (`TEST`), tests avec fixtures (`TEST_F`), tests paramétrés (`TEST_P`), assertions et matchers.
2. **Implémenter** des mocks avec Google Mock pour isoler les dépendances et tester le comportement des interfaces.
3. **Appliquer** le Test-Driven Development (TDD) en C++ : cycle red/green/refactor adapté au langage.
4. **Mesurer** la couverture de code avec gcov et lcov, générer des rapports HTML, et intégrer la mesure dans CMake.
5. **Écrire** des micro-benchmarks fiables avec Google Benchmark, en évitant les pièges de mesure (dead code elimination, warmup, bruit système).
6. **Interpréter** les résultats de benchmark (temps CPU, temps réel, throughput, complexité algorithmique) pour prendre des décisions d'optimisation fondées sur des données.

---

## Prérequis

- **Module 9, chapitre 26** : CMake — Google Test et Google Benchmark s'intègrent via `FetchContent` ou Conan, et la couverture de code nécessite des flags de compilation configurés dans CMake.
- **Module 9, chapitre 27** : gestion des dépendances (Conan/vcpkg) — pour installer GTest, GMock et Google Benchmark.
- **Module 3, chapitre 6** : classes et interfaces — les mocks de Google Mock s'appliquent sur des classes avec des méthodes virtuelles (interfaces).
- **Module 6** : gestion d'erreurs — tester la gestion des erreurs (exceptions, `std::expected`) nécessite des assertions spécifiques (`EXPECT_THROW`, `EXPECT_NO_THROW`).

---

## Chapitres

### Chapitre 33 — Unit Testing avec Google Test

Le framework de test unitaire dominant en C++. Ce chapitre couvre l'installation, l'écriture de tests, les assertions, le mocking avec Google Mock, et l'approche TDD.

- **Installation et configuration** : intégration via `FetchContent` ou Conan, configuration CMake (`enable_testing()`, `add_test()`, `gtest_discover_tests()`), exécution avec `ctest`.
- **`TEST`** : tests simples sans état partagé — un test = une fonction, isolation complète.
- **`TEST_F`** : tests avec fixtures — classe dérivée de `::testing::Test`, `SetUp()` et `TearDown()` pour l'initialisation/nettoyage d'état. Chaque test reçoit une instance fraîche de la fixture.
- **`TEST_P`** : tests paramétrés — exécution du même test avec des jeux de données différents via `INSTANTIATE_TEST_SUITE_P` et `::testing::Values`.
- **Assertions et matchers** : `EXPECT_EQ`, `EXPECT_NE`, `EXPECT_TRUE`, `EXPECT_THROW`, `EXPECT_NEAR` (flottants), `EXPECT_THAT` avec les matchers (`Contains`, `HasSubstr`, `ElementsAre`). Différence `EXPECT_*` (continue le test) vs `ASSERT_*` (arrête le test).
- **Google Mock** : création de mock classes (`MOCK_METHOD`), définition d'attentes (`EXPECT_CALL`, `ON_CALL`), matchers d'arguments (`_`, `Eq`, `Lt`, `HasSubstr`), actions (`Return`, `Throw`, `Invoke`), vérification de séquences (`InSequence`).
- **TDD en C++** : cycle red (test qui échoue) → green (implémentation minimale) → refactor (nettoyage). Adapté aux contraintes C++ : temps de compilation, séparation header/source, gestion des dépendances.

### Chapitre 34 — Couverture de Code

Mesure de ce qui est effectivement testé dans la base de code. gcov collecte les données, lcov les agrège, et les rapports HTML permettent de visualiser les lignes couvertes et non couvertes.

- **gcov et lcov** : compilation avec `--coverage` (ou `-fprofile-arcs -ftest-coverage`), exécution des tests, collecte avec `lcov --capture`, génération du rapport avec `genhtml`.
- **Rapports HTML** : visualisation par fichier, par fonction, et par ligne — identification des branches non couvertes.
- **Intégration CMake** : custom target pour automatiser la chaîne (build → test → collecte → rapport), exclusion des fichiers de test et des dépendances tierces.
- **Objectifs de couverture** : définition de seuils réalistes, distinction entre couverture de lignes (statement coverage) et couverture de branches (branch coverage), limites de la métrique.

### Chapitre 35 — Benchmarking

Mesure de performance reproductible avec Google Benchmark. Ce chapitre couvre l'écriture de benchmarks fiables, les pièges de mesure, et l'interprétation des résultats.

- **Google Benchmark** : macro `BENCHMARK`, registration de fonctions, `state.range()` pour les paramètres variables, `state.SetComplexityN()` pour mesurer la complexité algorithmique, `state.SetBytesProcessed()` pour le throughput.
- **Mesure fiable** : warmup, nombre d'itérations automatique, isolation du CPU (`taskset`), désactivation du frequency scaling (`cpupower frequency-set -g performance`), `benchmark::DoNotOptimize()` et `benchmark::ClobberMemory()` pour empêcher le compilateur d'éliminer le code mesuré.
- **Interprétation des résultats** : temps CPU vs temps réel (wall clock), comparaison de benchmarks (`--benchmark_compare`), régression de performance entre versions, prise de décision basée sur les données.

---

## Points de vigilance

- **Tests qui dépendent d'un ordre d'exécution.** Google Test ne garantit pas l'ordre d'exécution des tests entre les suites ni au sein d'une suite. Si un test modifie un état global (variable statique, fichier temporaire, base de données) et qu'un autre test en dépend, les résultats deviennent non déterministes. Chaque test doit être indépendant. Utilisez des fixtures (`TEST_F`) avec `SetUp()`/`TearDown()` pour initialiser l'état de chaque test. Activez `--gtest_shuffle` pour détecter les dépendances d'ordre cachées.

- **Mocks trop couplés à l'implémentation.** Un mock qui vérifie l'ordre exact des appels, le nombre précis d'invocations, et les arguments exacts de chaque méthode interne se casse dès que l'implémentation est refactorisée — même si le comportement observable ne change pas. C'est un test fragile qui coûte plus en maintenance qu'il n'apporte en confiance. Mockez les interfaces (comportement), pas les détails d'implémentation. Utilisez `EXPECT_CALL` avec `::testing::AtLeast(1)` plutôt que `::testing::Exactly(3)` quand le nombre exact d'appels n'est pas contractuel.

- **Couverture de code à 100% comme objectif.** Une couverture de 100% signifie que chaque ligne a été exécutée au moins une fois — pas que le code est correct. Les tests peuvent exécuter toutes les lignes sans vérifier les résultats (assertions manquantes), sans couvrir les cas limites, et sans tester les combinaisons d'état. À l'inverse, viser 100% pousse à écrire des tests triviaux sur du code getter/setter qui n'apportent aucune valeur. Un objectif raisonnable se situe entre 70% et 90% de couverture de lignes, avec un focus sur la couverture de branches des chemins critiques.

- **Micro-benchmark sans contrôle du dead code elimination.** Si le résultat d'un calcul n'est jamais utilisé, le compilateur en `-O2`/`-O3` peut éliminer le calcul entièrement — le benchmark mesure alors une boucle vide. Google Benchmark fournit `benchmark::DoNotOptimize(result)` pour marquer une valeur comme observable (empêche l'élimination) et `benchmark::ClobberMemory()` pour forcer un flush des caches du compilateur. Sans ces appels, les résultats de benchmark en mode optimisé sont potentiellement faux.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Écrire des tests unitaires avec Google Test (fixtures, tests paramétrés, assertions, matchers) et les exécuter via CTest.
- Isoler les dépendances avec Google Mock et écrire des mocks qui vérifient le comportement sans se coupler à l'implémentation.
- Mesurer la couverture de code avec gcov/lcov, générer des rapports HTML, et définir des objectifs de couverture réalistes.
- Écrire des micro-benchmarks fiables avec Google Benchmark en contrôlant le dead code elimination et l'environnement d'exécution.
- Intégrer tests, couverture et benchmarks dans CMake pour une exécution automatisée.

---


⏭️ [Unit Testing avec Google Test](/33-google-test/README.md)
