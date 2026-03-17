🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 33.1 Installation et configuration de GTest

## Présentation de Google Test

Google Test est distribué sous forme de code source C++ hébergé sur GitHub (`google/googletest`). Le dépôt contient deux composants principaux :

- **Google Test** (`gtest`) : le framework de test unitaire proprement dit, avec les macros `TEST`, `TEST_F`, `TEST_P` et la bibliothèque d'assertions.
- **Google Mock** (`gmock`) : le framework de mocking, intégré au même dépôt depuis 2015. Lier `gmock` inclut automatiquement `gtest`.

Le projet suit un modèle de release régulier. Au moment de la rédaction, la branche `main` est activement développée et les releases stables sont taguées sur GitHub. À partir de la version 1.17.0, GTest requiert **C++17 minimum** (les versions antérieures supportaient C++14). La version minimale de CMake requise par GTest est 3.13, ce qui ne pose aucun problème avec les versions CMake 3.28+ disponibles sur Ubuntu 24.04 LTS et ultérieures.

> ⚠️ **Convention importante.** Google recommande officiellement de compiler GTest avec le même compilateur, les mêmes flags et le même standard C++ que le projet qui l'utilise. Installer une version pré-compilée système peut fonctionner, mais expose à des incompatibilités subtiles d'ABI, notamment lors du passage entre `-std=c++17` et `-std=c++20` ou entre `libstdc++` et `libc++`. C'est pourquoi l'approche **FetchContent** est recommandée comme méthode principale.

## Méthode 1 : FetchContent (recommandée)

`FetchContent` est le mécanisme natif de CMake pour télécharger et intégrer des dépendances au moment de la configuration. C'est l'approche recommandée par Google dans la documentation officielle de GTest, et celle qui offre le meilleur contrôle.

### CMakeLists.txt racine

Voici la structure minimale d'un projet intégrant GTest via `FetchContent` :

```cmake
cmake_minimum_required(VERSION 3.20)  
project(mon_projet  
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
set(CMAKE_CXX_EXTENSIONS OFF)  

# ── Source principale ──────────────────────────────────────
add_subdirectory(src)

# ── Tests ──────────────────────────────────────────────────
option(BUILD_TESTS "Compiler les tests unitaires" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

L'option `BUILD_TESTS` permet de désactiver la compilation des tests lors d'un build de production (`cmake -DBUILD_TESTS=OFF ..`), ce qui accélère les builds en CI quand seul le packaging est nécessaire.

### tests/CMakeLists.txt

C'est dans ce fichier que l'on récupère et configure GTest :

```cmake
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0  # Figer sur une release stable
    GIT_SHALLOW    ON       # Ne pas télécharger tout l'historique
)

# Empêche GTest d'installer ses propres cibles lors de 'make install'
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

# Sur Windows, cette option évite des conflits de runtime.
# Sur Linux, elle est sans effet mais ne gêne pas.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# ── Définition d'un exécutable de test ─────────────────────
add_executable(mon_projet_tests
    test_math.cpp
    test_string_utils.cpp
)

target_link_libraries(mon_projet_tests
    PRIVATE
        GTest::gtest_main   # Fournit le main() automatiquement
        GTest::gmock         # Google Mock (inclut gtest)
        mon_projet_lib       # La librairie à tester
)

# Intégration avec CTest
include(GoogleTest)  
gtest_discover_tests(mon_projet_tests)  
```

Plusieurs points méritent une explication détaillée.

**`GIT_TAG v1.17.0`.** Figer la version est fondamental pour la reproductibilité des builds. Ne jamais pointer vers `main` — une mise à jour de GTest pourrait casser vos tests sans lien avec votre code. Lors d'une montée de version, changez le tag explicitement et validez que la suite passe.

**`GIT_SHALLOW ON`.** Cette option demande à CMake de faire un clone superficiel (sans historique). Le téléchargement passe de plusieurs dizaines de mégaoctets à quelques mégaoctets, ce qui est appréciable en CI où le dépôt est cloné à chaque pipeline.

**`GTest::gtest_main`.** GTest fournit deux cibles : `GTest::gtest` (la bibliothèque sans point d'entrée) et `GTest::gtest_main` (qui inclut une implémentation de `main()`). En liant `gtest_main`, vous n'avez pas besoin d'écrire de fonction `main()` dans vos fichiers de test — GTest s'en charge. Si vous avez besoin d'un `main()` personnalisé (par exemple pour initialiser un environnement global), liez `GTest::gtest` à la place et écrivez votre propre point d'entrée.

**`gtest_discover_tests`.** Cette commande CMake (fournie par le module `GoogleTest`) introspect l'exécutable de test après compilation pour découvrir automatiquement tous les cas de test enregistrés. Chaque `TEST`, `TEST_F` et `TEST_P` apparaît alors comme un test CTest individuel, ce qui permet de les exécuter sélectivement et de voir le détail dans les rapports CI.

### Premier build et exécution

```bash
# Configuration avec Ninja (recommandé, voir section 26.5)
cmake -B build -G Ninja

# Compilation
cmake --build build

# Exécution via CTest
cd build && ctest --output-on-failure
```

L'option `--output-on-failure` est quasi indispensable : par défaut, CTest masque la sortie standard des tests qui réussissent, mais affiche le détail complet lorsqu'un test échoue. C'est le comportement idéal en CI.

Vous pouvez aussi exécuter le binaire de test directement pour un contrôle plus fin :

```bash
./build/tests/mon_projet_tests
```

La sortie typique ressemble à ceci :

```
[==========] Running 12 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 5 tests from MathTest
[ RUN      ] MathTest.AdditionPositive
[       OK ] MathTest.AdditionPositive (0 ms)
[ RUN      ] MathTest.AdditionNegative
[       OK ] MathTest.AdditionNegative (0 ms)
[ RUN      ] MathTest.DivisionByZero
[       OK ] MathTest.DivisionByZero (0 ms)
...
[==========] 12 tests from 3 test suites ran. (2 ms total)
[  PASSED  ] 12 tests.
```

## Méthode 2 : Installation système (apt)

Ubuntu fournit GTest dans ses dépôts officiels. Cette approche est plus rapide à mettre en place mais offre moins de contrôle sur la version utilisée.

```bash
sudo apt update  
sudo apt install libgtest-dev libgmock-dev  
```

> 📝 **Note historique.** Pendant longtemps, le paquet `libgtest-dev` ne fournissait que les sources et il fallait compiler manuellement la bibliothèque. Sur Ubuntu 22.04+ et versions ultérieures, le paquet inclut désormais les bibliothèques pré-compilées, ce qui simplifie considérablement l'installation.

Le `CMakeLists.txt` de test se simplifie alors — plus besoin de `FetchContent` :

```cmake
find_package(GTest REQUIRED)

add_executable(mon_projet_tests
    test_math.cpp
    test_string_utils.cpp
)

target_link_libraries(mon_projet_tests
    PRIVATE
        GTest::gtest_main
        GTest::gmock
        mon_projet_lib
)

include(GoogleTest)  
gtest_discover_tests(mon_projet_tests)  
```

`find_package(GTest REQUIRED)` localise les fichiers CMake config installés par le paquet système et rend disponibles les cibles importées `GTest::gtest`, `GTest::gtest_main` et `GTest::gmock`.

### Quand préférer l'installation système

L'installation via `apt` reste pertinente dans certains contextes :

- **Prototypage rapide** : vous voulez tester une idée sans configurer un projet CMake complet.
- **Contraintes réseau** : l'environnement de build n'a pas accès à GitHub (certains environnements d'entreprise ou CI air-gapped).
- **Cohérence distribution** : vous construisez un paquet DEB (section 39.1) et souhaitez dépendre d'une version fournie par la distribution.

Dans tous les autres cas, préférez FetchContent pour la reproductibilité et le contrôle de version.

## Méthode 3 : Conan 2

Si votre projet utilise déjà Conan comme gestionnaire de dépendances (voir section 27.2), GTest s'intègre naturellement dans votre `conanfile.py` :

```python
from conan import ConanFile  
from conan.tools.cmake import cmake_layout  

class MonProjetConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("gtest/1.17.0")

    def layout(self):
        cmake_layout(self)
```

Après un `conan install .`, les cibles CMake `GTest::gtest_main` et `GTest::gmock` sont disponibles via le générateur `CMakeDeps`, exactement comme avec les deux autres méthodes. Le `CMakeLists.txt` de test reste identique à celui de la méthode `find_package`.

## Structure de projet recommandée

Quelle que soit la méthode d'installation choisie, la structure de fichiers suivante offre une organisation claire et scalable :

```
mon_projet/
├── CMakeLists.txt              # Racine du projet
├── CMakePresets.json            # Presets (section 27.6)
├── src/
│   ├── CMakeLists.txt
│   ├── math.cpp
│   └── string_utils.cpp
├── include/
│   └── mon_projet/
│       ├── math.hpp
│       └── string_utils.hpp
└── tests/
    ├── CMakeLists.txt           # Configuration GTest
    ├── test_math.cpp
    └── test_string_utils.cpp
```

Les points clés de cette organisation sont les suivants.

**Séparation `src/` et `tests/`.** Le code de production et le code de test vivent dans des répertoires distincts. Le code source est compilé sous forme de bibliothèque (`mon_projet_lib`) que les tests lient comme dépendance. Cette séparation garantit que le code de test n'est jamais inclus dans le livrable final.

**Un fichier de test par unité.** Chaque fichier source a un fichier de test correspondant (`math.cpp` → `test_math.cpp`). Cette convention facilite la navigation et garantit que chaque module est couvert.

**Bibliothèque intermédiaire.** Le `CMakeLists.txt` de `src/` crée une cible bibliothèque :

```cmake
add_library(mon_projet_lib
    math.cpp
    string_utils.cpp
)

target_include_directories(mon_projet_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# L'exécutable principal lie la bibliothèque
add_executable(mon_projet main.cpp)  
target_link_libraries(mon_projet PRIVATE mon_projet_lib)  
```

Cette approche évite de compiler les sources deux fois (une fois pour l'exécutable, une fois pour les tests). Les tests lient directement `mon_projet_lib` et ont accès à toutes les fonctions et classes publiques.

## Vérification de l'installation

Un test minimal permet de valider que l'intégration fonctionne correctement. Créez `tests/test_smoke.cpp` :

```cpp
#include <gtest/gtest.h>

TEST(SmokeTest, GTestFonctionne) {
    EXPECT_TRUE(true);
    EXPECT_EQ(2 + 2, 4);
}
```

Compilez et exécutez :

```bash
cmake -B build -G Ninja  
cmake --build build  
cd build && ctest --output-on-failure  
```

Si le test passe, votre environnement est opérationnel. Si la compilation échoue, les causes les plus fréquentes sont :

- **`gtest/gtest.h` introuvable** : vérifiez que `target_link_libraries` inclut bien `GTest::gtest_main` — CMake propage automatiquement les include directories via les cibles importées.
- **Erreurs de liaison** : assurez-vous que la version de GTest a été compilée avec le même standard C++ que votre projet. C'est l'avantage principal de FetchContent : GTest hérite automatiquement de vos settings de compilation.
- **`gtest_discover_tests` ne trouve rien** : le binaire de test doit être compilé avant que CTest puisse l'introspecter. Lancez `cmake --build build` avant `ctest`.

## Options de ligne de commande utiles

L'exécutable de test GTest accepte de nombreux flags qui s'avèrent précieux au quotidien.

**Filtrage des tests.** Le flag `--gtest_filter` permet d'exécuter un sous-ensemble de tests à l'aide de patterns avec wildcards :

```bash
# Exécuter uniquement les tests du suite MathTest
./mon_projet_tests --gtest_filter="MathTest.*"

# Exécuter un test spécifique
./mon_projet_tests --gtest_filter="MathTest.DivisionByZero"

# Exclure un test lent (le '-' indique l'exclusion)
./mon_projet_tests --gtest_filter="*:-SlowTest.*"
```

**Répétition.** Le flag `--gtest_repeat=N` exécute la suite N fois. Combiné avec `--gtest_shuffle`, il permet de détecter les dépendances cachées entre tests (un test qui passe uniquement parce qu'un autre a modifié un état global) :

```bash
# 10 exécutions avec ordre aléatoire
./mon_projet_tests --gtest_repeat=10 --gtest_shuffle
```

**Sortie XML pour la CI.** Le flag `--gtest_output` génère un rapport au format JUnit XML, directement exploitable par GitHub Actions, GitLab CI et la plupart des plateformes CI :

```bash
./mon_projet_tests --gtest_output=xml:report.xml
```

**Affichage verbeux.** Le flag `--gtest_print_time=0` masque les durées si elles polluent la sortie, tandis que `--gtest_color=yes` force la coloration même quand la sortie est redirigée.

## Intégration avec les sanitizers

Comme mentionné dans l'introduction du chapitre, les tests unitaires atteignent leur pleine efficacité lorsqu'ils sont exécutés sous les sanitizers. La configuration CMake recommandée consiste à créer un preset ou un build type dédié :

```cmake
# Option pour activer les sanitizers en mode test
option(ENABLE_SANITIZERS "Activer AddressSanitizer et UBSan" OFF)

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()
```

L'activation se fait alors au moment de la configuration :

```bash
cmake -B build-san -G Ninja -DENABLE_SANITIZERS=ON  
cmake --build build-san  
cd build-san && ctest --output-on-failure  
```

Avec cette configuration, tout accès mémoire invalide, dépassement de tampon, utilisation après libération ou comportement indéfini déclenché par un test sera immédiatement détecté et rapporté avec une trace d'exécution complète. La section 29.4 détaille le fonctionnement et la configuration avancée des sanitizers.

---


⏭️ [Écriture de tests : TEST, TEST_F, fixtures](/33-google-test/02-ecriture-tests.md)
