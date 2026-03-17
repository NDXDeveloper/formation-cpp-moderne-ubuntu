🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 33. Unit Testing avec Google Test ⭐

## Introduction

Le test unitaire est la première ligne de défense contre les régressions dans un projet C++. Contrairement à un langage interprété où une erreur se manifeste à l'exécution, C++ combine des pièges à la compilation (templates mal instanciés, conversions implicites) et des comportements indéfinis silencieux à l'exécution (accès hors limites, use-after-free, data races). Sans suite de tests automatisés, un refactoring anodin peut introduire un bug qui ne se révélera qu'en production, parfois des semaines plus tard.

**Google Test** (souvent abrégé **GTest**) s'est imposé comme le framework de test unitaire de référence dans l'écosystème C++. Développé et maintenu par Google, il est utilisé en interne sur l'ensemble de leur codebase C++ — plusieurs centaines de millions de lignes de code — ainsi que par des projets majeurs comme Chromium, LLVM, Protocol Buffers, gRPC ou encore OpenCV. Cette adoption massive garantit une stabilité, une documentation et un écosystème d'outils (IDE, CI/CD, reporting) difficilement égalés par les alternatives.

## Pourquoi Google Test plutôt qu'un autre framework ?

L'écosystème C++ propose plusieurs frameworks de test : Catch2, Boost.Test, doctest, CTest. Chacun a ses mérites, mais GTest se distingue sur plusieurs axes qui expliquent sa domination dans l'industrie.

**Expressivité des assertions.** GTest fournit une bibliothèque d'assertions riche — `EXPECT_EQ`, `EXPECT_THROW`, `EXPECT_NEAR` pour les flottants, et bien d'autres — qui produit des messages d'erreur détaillés et lisibles sans effort supplémentaire de la part du développeur. Lorsqu'un test échoue, le rapport indique la valeur attendue, la valeur obtenue, le fichier et la ligne, ce qui réduit considérablement le temps de diagnostic.

**Fixtures et partage d'état.** Le mécanisme de *test fixtures* (`TEST_F`) permet de factoriser le setup et le teardown communs à un groupe de tests, en s'appuyant sur le modèle RAII familier aux développeurs C++. Chaque test reçoit une instance fraîche de la fixture, garantissant l'isolation entre les cas de test.

**Tests paramétrés.** Avec `TEST_P`, un même scénario de test peut être exécuté sur un ensemble de valeurs d'entrée sans duplication de code. C'est particulièrement utile pour valider des fonctions sur des plages de données, des cas limites ou des configurations multiples.

**Google Mock intégré.** Depuis la fusion des deux projets, GTest embarque directement **Google Mock**, un framework de mocking qui permet de créer des doublures de test pour les dépendances externes (réseau, base de données, système de fichiers). Cette intégration native évite d'avoir à assembler des outils disparates.

**Intégration avec l'écosystème.** GTest s'intègre naturellement avec CMake via `FetchContent` ou `find_package`, avec les principaux IDE (VS Code, CLion), avec les pipelines CI/CD (GitHub Actions, GitLab CI), et avec les outils de couverture de code (`gcov`, `lcov`) que nous aborderons au chapitre 34. Il produit des rapports au format XML compatible JUnit, directement exploitables par la plupart des plateformes d'intégration continue.

## Philosophie du test unitaire en C++

Tester du C++ présente des défis spécifiques qui méritent d'être explicités avant de plonger dans la syntaxe.

**Le coût du feedback loop.** En C++, chaque modification nécessite une recompilation. Un projet de taille moyenne peut prendre plusieurs dizaines de secondes à compiler, voire plusieurs minutes pour un grand projet. Structurer ses tests en bibliothèques indépendantes et tirer parti de la compilation incrémentale (voir section 26 sur CMake et section 2.3 sur ccache) est essentiel pour maintenir un cycle de feedback rapide.

**Comportements indéfinis.** Un test qui passe ne prouve pas l'absence de bug en C++. Un accès mémoire invalide peut ne provoquer aucun crash sur votre machine et exploser en production sur une architecture différente. C'est pourquoi les tests unitaires doivent être complétés par les sanitizers (section 29.4) : compiler et exécuter la suite de tests avec `-fsanitize=address,undefined` transforme les comportements indéfinis silencieux en erreurs détectables.

**Testabilité et conception.** Le code C++ fortement couplé — dépendances directes à des singletons, appels système bruts, état global — est difficile à tester. Écrire des tests unitaires pousse naturellement vers une meilleure architecture : injection de dépendances, interfaces abstraites, séparation des responsabilités. Le TDD (Test-Driven Development), que nous aborderons en section 33.5, formalise cette approche en faisant du test le moteur de la conception.

## Ce que vous apprendrez dans ce chapitre

Ce chapitre couvre l'ensemble du workflow de test unitaire avec Google Test, de l'installation à la pratique avancée :

La **section 33.1** détaille l'installation et la configuration de GTest dans un projet CMake, avec les deux approches recommandées (`FetchContent` et installation système).

La **section 33.2** couvre l'écriture de tests avec les trois macros fondamentales : `TEST` pour les cas simples, `TEST_F` pour les tests avec fixtures, et `TEST_P` pour les tests paramétrés.

La **section 33.3** explore la bibliothèque d'assertions et de matchers, en détaillant la distinction entre `EXPECT_*` (continue l'exécution en cas d'échec) et `ASSERT_*` (arrête le test immédiatement).

La **section 33.4** introduit Google Mock et ses capacités de mocking : création de mocks, définition d'attentes avec `EXPECT_CALL`, et matchers avancés pour vérifier les interactions entre composants.

La **section 33.5** aborde le Test-Driven Development appliqué au C++, avec un cycle Red-Green-Refactor concret sur un exemple de développement réel.

## Prérequis

Ce chapitre s'appuie sur plusieurs notions abordées précédemment dans la formation :

- **CMake** (chapitre 26) : les exemples utilisent CMake pour la configuration du projet et l'intégration de GTest. Une maîtrise de `add_executable`, `target_link_libraries` et `FetchContent` est attendue.
- **Classes et héritage** (chapitres 6-7) : les fixtures GTest reposent sur l'héritage de `::testing::Test`, et le mocking nécessite des classes avec des méthodes virtuelles.
- **Gestion de la mémoire** (chapitres 5 et 9) : plusieurs exemples de tests portent sur la détection de fuites mémoire et la vérification du comportement des smart pointers.
- **Gestion des erreurs** (chapitre 17) : les assertions sur les exceptions (`EXPECT_THROW`, `EXPECT_NO_THROW`) nécessitent une compréhension de `try`/`catch` et de `noexcept`.

---


⏭️ [Installation et configuration de GTest](/33-google-test/01-installation-gtest.md)
