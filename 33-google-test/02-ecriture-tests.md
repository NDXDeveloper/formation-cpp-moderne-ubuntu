🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 33.2 Écriture de tests : TEST, TEST_F, fixtures

## Les trois macros fondamentales

Google Test propose trois macros principales pour déclarer des cas de test. Chacune répond à un besoin différent en termes de complexité et de partage d'état :

- **`TEST(TestSuiteName, TestName)`** — Le cas le plus simple. Chaque test est une fonction autonome, sans état partagé ni initialisation préalable. C'est le point de départ pour tester des fonctions pures, des calculs isolés ou des comportements élémentaires.

- **`TEST_F(TestFixtureName, TestName)`** — Le "F" signifie *fixture*. Le test s'exécute dans le contexte d'une classe fixture qui fournit un setup et un teardown communs. Chaque cas de test reçoit une **instance fraîche** de la fixture, garantissant l'isolation. C'est la macro à privilégier dès que plusieurs tests partagent des données d'initialisation ou des ressources.

- **`TEST_P(TestFixtureName, TestName)`** — Le "P" signifie *paramétré*. Le test est exécuté plusieurs fois avec des jeux de données différents, fournis par un générateur. C'est l'outil idéal pour valider un même comportement sur une plage de valeurs d'entrée sans dupliquer le code du test.

## Anatomie d'un fichier de test

Un fichier de test GTest suit une structure conventionnelle qui mérite d'être comprise avant d'entrer dans le détail de chaque macro :

```cpp
// ── 1. Includes ───────────────────────────────────────────
#include <gtest/gtest.h>       // Macros TEST, TEST_F, assertions
#include <gmock/gmock.h>       // Si mocking nécessaire (section 33.4)

#include "mon_projet/math.hpp" // Le code à tester

// ── 2. Helpers et fixtures ────────────────────────────────
// Classes fixture, fonctions utilitaires locales au fichier
// de test, constantes de test...

// ── 3. Cas de test ────────────────────────────────────────
// TEST, TEST_F, TEST_P dans un ordre logique

// ── 4. Pas de main() ─────────────────────────────────────
// gtest_main fournit le point d'entrée (voir section 33.1)
```

L'absence de `main()` est un point important. En liant `GTest::gtest_main` dans CMake, le framework fournit automatiquement un point d'entrée qui initialise GTest, découvre tous les tests enregistrés via les macros, les exécute et produit le rapport. Cela signifie que chaque macro `TEST`, `TEST_F` ou `TEST_P` enregistre silencieusement le cas de test dans un registre global au moment du chargement du programme — vous n'avez rien à câbler manuellement.

## Nommage : test suites et test cases

GTest organise les tests selon une hiérarchie à deux niveaux. Le premier argument de chaque macro désigne la **test suite** (anciennement appelée *test case* avant GTest 1.10 — attention à la terminologie dans les anciennes ressources). Le second argument est le **nom du test** au sein de cette suite.

```cpp
TEST(MathUtils, AdditionReturnsCorrectSum)  
//   ^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^^^^
//   suite      nom du test
```

Cette hiérarchie structure la sortie console et les rapports XML :

```
[----------] 3 tests from MathUtils
[ RUN      ] MathUtils.AdditionReturnsCorrectSum
[       OK ] MathUtils.AdditionReturnsCorrectSum (0 ms)
[ RUN      ] MathUtils.MultiplicationHandlesZero
[       OK ] MathUtils.MultiplicationHandlesZero (0 ms)
[ RUN      ] MathUtils.DivisionThrowsOnZeroDivisor
[       OK ] MathUtils.DivisionThrowsOnZeroDivisor (0 ms)
```

Elle permet aussi le filtrage ciblé depuis la ligne de commande :

```bash
# Tous les tests de la suite MathUtils
./mon_projet_tests --gtest_filter="MathUtils.*"

# Un test précis
./mon_projet_tests --gtest_filter="MathUtils.DivisionThrowsOnZeroDivisor"
```

### Conventions de nommage

GTest impose une contrainte technique : les noms de suite et de test ne doivent pas contenir de caractère underscore `_` en début de nom (réservé par le C++), et deux suites différentes ne peuvent pas partager le même nom dans un même binaire. Au-delà de ces contraintes, plusieurs conventions de nommage coexistent dans l'industrie.

**Convention descriptive (recommandée dans cette formation).** Le nom de la suite correspond à la classe ou au module testé, et le nom du test décrit le comportement attendu de façon lisible :

```cpp
TEST(JsonParser, ReturnsErrorOnMalformedInput)  
TEST(JsonParser, ParsesNestedObjectsCorrectly)  
TEST(ConnectionPool, ReusesIdleConnections)  
```

Cette convention produit des rapports auto-documentés — quand un test échoue en CI, le nom seul suffit souvent à comprendre ce qui a cassé, sans même lire le corps du test.

**Convention Given/When/Then.** Certaines équipes adoptent un nommage inspiré du BDD :

```cpp
TEST(Account, GivenPositiveBalance_WhenWithdrawingLess_ThenSucceeds)
```

Le principal inconvénient est la longueur des noms, mais la lisibilité des rapports peut compenser dans les projets où les tests servent de documentation vivante.

## Le modèle d'exécution de GTest

Comprendre le modèle d'exécution est essentiel pour écrire des tests fiables, en particulier avec les fixtures.

**Isolation par défaut.** Chaque cas de test (`TEST` ou `TEST_F`) s'exécute de manière indépendante. GTest ne garantit aucun ordre d'exécution entre les tests — et le flag `--gtest_shuffle` randomise explicitement cet ordre. Un test ne doit jamais dépendre de l'état laissé par un autre test. Si cela arrive, c'est un bug dans les tests, pas dans le code.

**Cycle de vie d'une fixture.** Pour chaque `TEST_F`, GTest effectue la séquence suivante :

1. **Construction** — Une nouvelle instance de la classe fixture est créée (appel du constructeur).
2. **`SetUp()`** — La méthode `SetUp()` est appelée si elle est définie (override).
3. **Corps du test** — Le code entre les accolades du `TEST_F` s'exécute.
4. **`TearDown()`** — La méthode `TearDown()` est appelée si elle est définie.
5. **Destruction** — L'instance de la fixture est détruite (appel du destructeur).

Ce cycle se répète intégralement pour chaque test de la suite. Deux tests de la même fixture ne partagent jamais la même instance. Ce modèle s'aligne naturellement avec le RAII (section 6.3) : les ressources acquises dans le constructeur ou `SetUp()` sont libérées dans le destructeur ou `TearDown()`, et chaque test part d'un état propre.

**`SetUp()` vs constructeur.** Les deux sont des points d'initialisation valides, mais ils diffèrent sur un point important : si une assertion fatale (`ASSERT_*`) échoue dans `SetUp()`, GTest saute le corps du test et passe directement à `TearDown()` puis au destructeur. Dans un constructeur, une assertion fatale ne peut pas interrompre proprement l'exécution — le comportement est indéfini car l'objet est en cours de construction. La règle pratique : utilisez le constructeur pour les initialisations qui ne peuvent pas échouer (assignation de valeurs, allocation de mémoire via smart pointers) et `SetUp()` pour celles qui peuvent échouer et nécessitent une assertion (ouverture de fichier, connexion réseau, validation de préconditions).

## EXPECT vs ASSERT : deux philosophies d'assertion

Avant de détailler chaque macro dans les sous-sections suivantes, il est important de comprendre la distinction fondamentale entre les deux familles d'assertions de GTest, car elle influence la manière dont on structure le corps de chaque test.

**`EXPECT_*`** — Assertion non fatale. En cas d'échec, l'erreur est enregistrée mais l'exécution du test continue. Cela permet de vérifier plusieurs propriétés dans un même test et de voir toutes les assertions qui échouent en une seule exécution.

**`ASSERT_*`** — Assertion fatale. En cas d'échec, le test s'arrête immédiatement (retour de la fonction en cours). C'est nécessaire quand la suite du test n'a pas de sens si l'assertion échoue — typiquement, vérifier qu'un pointeur n'est pas `nullptr` avant de le déréférencer.

```cpp
TEST(Example, DemonstratesExpectVsAssert) {
    auto* ptr = createWidget();

    // ASSERT : si ptr est null, la suite crasherait
    ASSERT_NE(ptr, nullptr);

    // EXPECT : on veut voir toutes les propriétés qui échouent
    EXPECT_EQ(ptr->width(), 100);
    EXPECT_EQ(ptr->height(), 200);
    EXPECT_TRUE(ptr->isVisible());

    delete ptr;
}
```

La règle d'usage est simple : **préférez `EXPECT_*` par défaut**, et réservez `ASSERT_*` aux vérifications dont dépend la validité du reste du test. Un test qui rapporte trois assertions échouées est bien plus informatif qu'un test qui s'arrête à la première — il permet de diagnostiquer un problème systémique plutôt que de le découvrir assertion par assertion sur trois exécutions successives.

## Ce que couvrent les sous-sections

Les trois sous-sections suivantes détaillent chaque macro avec des exemples concrets :

- **Section 33.2.1** explore `TEST` pour les cas simples : tests de fonctions libres, assertions de base et bonnes pratiques de structuration.
- **Section 33.2.2** couvre `TEST_F` et le mécanisme de fixtures : création de classes fixture, gestion du setup/teardown et patterns courants.
- **Section 33.2.3** présente `TEST_P` et les tests paramétrés : générateurs de valeurs, combinaisons de paramètres et cas d'usage typiques.

---


⏭️ [TEST : Tests simples](/33-google-test/02.1-test-simple.md)
