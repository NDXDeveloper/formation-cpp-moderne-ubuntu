# Exemples du Chapitre 33 — Unit Testing avec Google Test

Projet CMake intégrant GTest via FetchContent, avec des tests unitaires illustrant les concepts du chapitre.

## Prérequis

```bash
g++-15 --version    # GCC 15  
cmake --version     # CMake 3.20+  
ninja --version     # Ninja  
```

> GTest est téléchargé automatiquement par FetchContent — aucune installation préalable nécessaire.

---

## Structure

```
exemples/
├── CMakeLists.txt              # Projet CMake avec FetchContent(googletest)
├── include/mp/math.hpp         # Bibliothèque mathématique à tester
├── tests/
│   ├── test_math.cpp           # Tests unitaires (section 33.2.1)
│   └── test_assertions.cpp     # Assertions et matchers (section 33.3)
└── README.md
```

---

## Compilation et exécution

```bash
cd exemples  
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build  
cd build && ctest --output-on-failure  
```

## Tests inclus

### test\_math (section 33.2.1)

| | |
|---|---|
| **Section** | 33.2.1 |
| **Fichier .md** | `02.1-test-simple.md` |
| **Description** | Tests `TEST` sur les fonctions `add`, `divide`, `factorial`, `average` — cas nominaux, cas limites, exceptions. |

**19 tests au total, dont :**
- `Add.ReturnsSumOfTwoPositiveIntegers`, `Add.HandlesNegativeNumbers`, `Add.HandlesZero`
- `Divide.ReturnsIntegerQuotient`, `Divide.ThrowsOnZeroDivisor`
- `Factorial.ReturnsOneForZero`, `Factorial.ComputesCorrectly`, `Factorial.ThrowsOnNegativeInput`
- `Average.ComputesMean`, `Average.ThrowsOnEmptyVector`

### test\_assertions (section 33.3)

| | |
|---|---|
| **Section** | 33.3 |
| **Fichier .md** | `03-assertions-matchers.md` |
| **Description** | Démonstration des assertions GTest (`EXPECT_EQ`, `EXPECT_NEAR`, `EXPECT_THROW`) et matchers GMock (`HasSubstr`, `ContainsRegex`, `Contains`, `Each`, `ElementsAre`). |

**8 tests, dont :**
- `Assertions.Equality`, `Assertions.Ordering`, `Assertions.FloatingPoint`
- `Assertions.Strings`, `Assertions.Exceptions`
- `Matchers.Scalars`, `Matchers.Strings`, `Matchers.Containers`

---

## Nettoyage

```bash
rm -rf build
```
