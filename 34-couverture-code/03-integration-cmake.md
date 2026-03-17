🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 34.3 Intégration dans CMake

## Objectif : une seule commande

Les sections 34.1 et 34.2 ont détaillé un workflow en dix commandes manuelles — de la compilation instrumentée au rapport HTML. Ce workflow fonctionne, mais il est trop verbeux pour un usage quotidien et trop fragile pour la CI. L'objectif de cette section est de l'encapsuler dans CMake, de sorte qu'une seule commande produise le rapport complet :

```bash
cmake --build build-cov --target coverage
```

Cette cible `coverage` orchestre automatiquement la réinitialisation des compteurs, l'exécution des tests, la capture des données, le filtrage et la génération du rapport HTML. Le développeur n'a plus qu'à ouvrir le rapport.

## Stratégie d'intégration

L'intégration dans CMake repose sur trois éléments :

1. **Un build type ou une option dédiée** qui active les flags d'instrumentation sans polluer les builds de production.
2. **La détection des outils** (`gcov`, `lcov`, `genhtml`) au moment de la configuration CMake.
3. **Une cible custom** (`coverage`) qui enchaîne les étapes du pipeline via `add_custom_target`.

## Étape 1 : option de compilation

La première décision est de savoir comment activer l'instrumentation. Deux approches coexistent dans l'écosystème.

### Approche par option CMake (recommandée)

```cmake
option(ENABLE_COVERAGE "Activer la mesure de couverture de code" OFF)

if(ENABLE_COVERAGE)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING 
            "La couverture de code est plus fiable avec CMAKE_BUILD_TYPE=Debug")
    endif()

    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()
```

L'activation se fait au moment de la configuration :

```bash
cmake -B build-cov -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_COVERAGE=ON
```

Cette approche est explicite — la couverture est un choix conscient, pas un effet de bord du build type. Elle permet aussi de combiner la couverture avec d'autres options (sanitizers, assertions activées).

Le `-O0` est critique : les optimisations réorganisent le code machine et produisent des résultats de couverture trompeurs (section 34.1). Le `add_link_options(--coverage)` est nécessaire car GCC doit aussi lier les bibliothèques de profiling (`libgcov`).

### Approche par build type dédié

Certains projets définissent un build type `Coverage` :

```cmake
set(CMAKE_CXX_FLAGS_COVERAGE "--coverage -O0 -g" CACHE STRING "")  
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage" CACHE STRING "")  
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "--coverage" CACHE STRING "")  
```

L'activation se fait via :

```bash
cmake -B build-cov -G Ninja -DCMAKE_BUILD_TYPE=Coverage
```

Cette approche est plus conventionnelle dans l'écosystème CMake (alignée sur les build types `Debug`, `Release`, `RelWithDebInfo`) mais moins flexible — elle est mutuellement exclusive avec les autres build types.

Les deux approches sont valides. Cette formation utilise l'approche par option pour sa flexibilité.

## Étape 2 : détection des outils

Avant de créer la cible `coverage`, il faut s'assurer que les outils nécessaires sont disponibles sur le système :

```cmake
if(ENABLE_COVERAGE)
    find_program(GCOV_PATH gcov REQUIRED)
    find_program(LCOV_PATH lcov REQUIRED)
    find_program(GENHTML_PATH genhtml REQUIRED)

    message(STATUS "gcov trouvé : ${GCOV_PATH}")
    message(STATUS "lcov trouvé : ${LCOV_PATH}")
    message(STATUS "genhtml trouvé : ${GENHTML_PATH}")
endif()
```

`find_program` avec `REQUIRED` arrête la configuration CMake si l'outil n'est pas trouvé, avec un message d'erreur clair. C'est préférable à une erreur cryptique au moment du build, quand l'utilisateur a oublié d'installer lcov.

### Vérifier la version de lcov

lcov 2.x et 1.x diffèrent sur certaines options. Pour les projets qui utilisent des fonctionnalités spécifiques à la version 2, une vérification est possible :

```cmake
if(ENABLE_COVERAGE)
    execute_process(
        COMMAND ${LCOV_PATH} --version
        OUTPUT_VARIABLE LCOV_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX MATCH "[0-9]+\\.[0-9]+" LCOV_VERSION "${LCOV_VERSION_OUTPUT}")
    message(STATUS "lcov version : ${LCOV_VERSION}")

    if(LCOV_VERSION VERSION_LESS "2.0")
        message(WARNING 
            "lcov ${LCOV_VERSION} détecté. Version 2.0+ recommandée.")
    endif()
endif()
```

## Étape 3 : la cible coverage

Voici le cœur de l'intégration — une cible custom qui orchestre le pipeline complet :

```cmake
if(ENABLE_COVERAGE)
    set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage_report")
    set(COVERAGE_INFO_FILE "${CMAKE_BINARY_DIR}/coverage.info")
    set(COVERAGE_INFO_FILTERED "${CMAKE_BINARY_DIR}/coverage_filtered.info")

    add_custom_target(coverage
        COMMENT "Génération du rapport de couverture..."

        # 1. Réinitialiser les compteurs
        COMMAND ${LCOV_PATH} --zerocounters 
                --directory ${CMAKE_BINARY_DIR}

        # 2. Capturer la baseline (compteurs à zéro)
        COMMAND ${LCOV_PATH} --capture --initial
                --directory ${CMAKE_BINARY_DIR}
                --output-file ${COVERAGE_INFO_FILE}.base
                --gcov-tool ${GCOV_PATH}

        # 3. Exécuter les tests
        COMMAND ${CMAKE_CTEST_COMMAND} 
                --test-dir ${CMAKE_BINARY_DIR}
                --output-on-failure

        # 4. Capturer les résultats
        COMMAND ${LCOV_PATH} --capture
                --directory ${CMAKE_BINARY_DIR}
                --output-file ${COVERAGE_INFO_FILE}.test
                --gcov-tool ${GCOV_PATH}

        # 5. Combiner baseline et résultats
        COMMAND ${LCOV_PATH} 
                --add-tracefile ${COVERAGE_INFO_FILE}.base
                --add-tracefile ${COVERAGE_INFO_FILE}.test
                --output-file ${COVERAGE_INFO_FILE}

        # 6. Filtrer les fichiers non pertinents
        COMMAND ${LCOV_PATH} --remove ${COVERAGE_INFO_FILE}
                '/usr/*'
                '${CMAKE_BINARY_DIR}/_deps/*'
                '*/tests/*'
                --output-file ${COVERAGE_INFO_FILTERED}

        # 7. Générer le rapport HTML
        COMMAND ${GENHTML_PATH} ${COVERAGE_INFO_FILTERED}
                --output-directory ${COVERAGE_OUTPUT_DIR}
                --title "${PROJECT_NAME} — Couverture"
                --legend
                --branch-coverage
                --demangle-cpp
                --sort

        # 8. Afficher le résumé
        COMMAND ${LCOV_PATH} --summary ${COVERAGE_INFO_FILTERED}

        COMMAND ${CMAKE_COMMAND} -E echo 
                "Rapport HTML : ${COVERAGE_OUTPUT_DIR}/index.html"

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${PROJECT_NAME}_tests   # Rebuild les tests si nécessaire
    )
endif()
```

Plusieurs points méritent attention.

**`DEPENDS`** — La clause `DEPENDS` déclare une dépendance sur la cible de test. CMake recompile automatiquement les tests si le code source a changé avant d'exécuter la cible `coverage`. Adaptez le nom de la cible (`${PROJECT_NAME}_tests`) à votre projet — si vous avez plusieurs binaires de test, listez-les tous.

**`--gcov-tool`** — Spécifie explicitement le chemin vers `gcov`. C'est important quand plusieurs versions de GCC coexistent sur le système (par exemple GCC 14 et GCC 15 installés via `update-alternatives`, section 2.1.1). Le `gcov` doit correspondre à la version de `g++` utilisée pour la compilation — un mismatch produit des erreurs de format de fichier.

**`${CMAKE_CTEST_COMMAND}`** — Utilise le CTest intégré à CMake plutôt qu'un chemin codé en dur. Cela garantit la portabilité entre les systèmes.

**Filtrage des chemins** — Les patterns de `--remove` utilisent `${CMAKE_BINARY_DIR}/_deps/*` pour exclure les dépendances FetchContent. Ajustez les patterns selon la structure de votre projet. Si vous utilisez Conan, ajoutez le chemin du cache Conan.

### Utilisation

```bash
# Configuration une seule fois
cmake -B build-cov -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_COVERAGE=ON

# Génération du rapport (compile, teste, mesure, génère HTML)
cmake --build build-cov --target coverage

# Ouvrir le rapport
xdg-open build-cov/coverage_report/index.html
```

Une seule commande après la configuration initiale. C'est l'expérience développeur visée.

## Module CMake réutilisable

Pour les projets multi-modules ou les organisations qui veulent standardiser l'approche, l'intégration de la couverture peut être encapsulée dans un module CMake réutilisable :

```cmake
# cmake/CodeCoverage.cmake

function(add_coverage_target)
    cmake_parse_arguments(COV "" "TARGET;OUTPUT_DIR" "EXCLUDE" ${ARGN})

    if(NOT ENABLE_COVERAGE)
        return()
    endif()

    find_program(LCOV_PATH lcov REQUIRED)
    find_program(GENHTML_PATH genhtml REQUIRED)
    find_program(GCOV_PATH gcov REQUIRED)

    if(NOT COV_OUTPUT_DIR)
        set(COV_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage_report")
    endif()

    set(_info "${CMAKE_BINARY_DIR}/coverage.info")
    set(_info_filtered "${CMAKE_BINARY_DIR}/coverage_filtered.info")

    # Construire la liste de patterns d'exclusion
    set(_exclude_patterns '/usr/*' '${CMAKE_BINARY_DIR}/_deps/*')
    if(COV_EXCLUDE)
        list(APPEND _exclude_patterns ${COV_EXCLUDE})
    endif()

    add_custom_target(coverage
        COMMENT "Génération du rapport de couverture..."

        COMMAND ${LCOV_PATH} --zerocounters 
                --directory ${CMAKE_BINARY_DIR}

        COMMAND ${LCOV_PATH} --capture --initial
                --directory ${CMAKE_BINARY_DIR}
                --output-file ${_info}.base
                --gcov-tool ${GCOV_PATH}

        COMMAND ${CMAKE_CTEST_COMMAND}
                --test-dir ${CMAKE_BINARY_DIR}
                --output-on-failure

        COMMAND ${LCOV_PATH} --capture
                --directory ${CMAKE_BINARY_DIR}
                --output-file ${_info}.test
                --gcov-tool ${GCOV_PATH}

        COMMAND ${LCOV_PATH}
                --add-tracefile ${_info}.base
                --add-tracefile ${_info}.test
                --output-file ${_info}

        COMMAND ${LCOV_PATH} --remove ${_info}
                ${_exclude_patterns}
                --output-file ${_info_filtered}

        COMMAND ${GENHTML_PATH} ${_info_filtered}
                --output-directory ${COV_OUTPUT_DIR}
                --title "${PROJECT_NAME} — Couverture"
                --legend --branch-coverage --demangle-cpp --sort

        COMMAND ${LCOV_PATH} --summary ${_info_filtered}

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    if(COV_TARGET)
        add_dependencies(coverage ${COV_TARGET})
    endif()
endfunction()
```

L'utilisation dans le `CMakeLists.txt` racine devient minimale :

```cmake
cmake_minimum_required(VERSION 3.20)  
project(mon_projet VERSION 1.0.0 LANGUAGES CXX)  

option(ENABLE_COVERAGE "Activer la couverture de code" OFF)

if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

add_subdirectory(src)  
add_subdirectory(tests)  

# ── Couverture ─────────────────────────────────────────────
include(cmake/CodeCoverage.cmake)  
add_coverage_target(  
    TARGET mon_projet_tests
    EXCLUDE '*/tests/*' '*/third_party/*'
)
```

Le module est réutilisable entre projets : il suffit de copier `cmake/CodeCoverage.cmake` et d'appeler `add_coverage_target` avec les paramètres spécifiques au projet.

## Intégration avec CMake Presets

Les CMake Presets (section 27.6) offrent un moyen élégant de standardiser la configuration de couverture pour toute l'équipe :

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "coverage",
            "displayName": "Coverage (GCC, Debug)",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build-cov",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_CXX_COMPILER": "g++",
                "ENABLE_COVERAGE": "ON",
                "BUILD_TESTS": "ON"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "coverage-report",
            "configurePreset": "coverage",
            "targets": ["coverage"]
        }
    ]
}
```

Le workflow devient alors parfaitement standardisé :

```bash
# Configuration
cmake --preset coverage

# Génération complète du rapport
cmake --build --preset coverage-report
```

Le fichier `CMakePresets.json` est versionné avec le projet. Chaque développeur et chaque pipeline CI utilisent exactement les mêmes options — pas de divergence entre les environnements.

## Cible de vérification du seuil

Au-delà de la génération du rapport, la CI a souvent besoin de **vérifier** que la couverture ne descend pas sous un seuil minimal. Un script compagnon peut être invoqué comme cible supplémentaire :

```cmake
# Seuil configurable
set(COVERAGE_MINIMUM_LINE 80 CACHE STRING "Seuil minimal de couverture de lignes (%)")  
set(COVERAGE_MINIMUM_BRANCH 60 CACHE STRING "Seuil minimal de couverture de branches (%)")  

if(ENABLE_COVERAGE)
    # Script de vérification
    file(WRITE ${CMAKE_BINARY_DIR}/check_coverage.sh [=[
#!/bin/bash
set -e  
INFO_FILE="$1"  
MIN_LINE="$2"  
MIN_BRANCH="$3"  

LINE_COV=$(lcov --summary "$INFO_FILE" 2>&1 | grep "lines" | grep -oP '[\d.]+(?=%)')  
BRANCH_COV=$(lcov --summary "$INFO_FILE" 2>&1 | grep "branches" | grep -oP '[\d.]+(?=%)')  

echo "Couverture de lignes   : ${LINE_COV}% (seuil : ${MIN_LINE}%)"  
echo "Couverture de branches : ${BRANCH_COV}% (seuil : ${MIN_BRANCH}%)"  

PASS=true  
if (( $(echo "$LINE_COV < $MIN_LINE" | bc -l) )); then  
    echo "❌ Couverture de lignes insuffisante"
    PASS=false
fi  
if (( $(echo "$BRANCH_COV < $MIN_BRANCH" | bc -l) )); then  
    echo "❌ Couverture de branches insuffisante"
    PASS=false
fi

if [ "$PASS" = false ]; then
    exit 1
fi  
echo "✅ Seuils de couverture respectés"  
]=])

    add_custom_target(coverage-check
        COMMAND bash ${CMAKE_BINARY_DIR}/check_coverage.sh
                ${COVERAGE_INFO_FILTERED}
                ${COVERAGE_MINIMUM_LINE}
                ${COVERAGE_MINIMUM_BRANCH}
        DEPENDS coverage
        COMMENT "Vérification des seuils de couverture..."
    )
endif()
```

La cible `coverage-check` dépend de `coverage` (elle déclenche donc la génération complète) puis vérifie les seuils. En CI, le pipeline appelle cette cible au lieu de `coverage` :

```bash
cmake --build build-cov --target coverage-check
```

Si la couverture est sous le seuil, la commande retourne un code d'erreur non nul et le pipeline échoue. La section 34.4 discute du choix des seuils eux-mêmes.

## Intégration CI : aperçu

La cible CMake s'intègre directement dans les pipelines. Voici la structure minimale pour les deux plateformes principales — la configuration détaillée est couverte en section 38.

### GitHub Actions (extrait)

```yaml
- name: Configure (coverage)
  run: cmake -B build-cov -G Ninja
       -DCMAKE_BUILD_TYPE=Debug
       -DENABLE_COVERAGE=ON

- name: Couverture
  run: cmake --build build-cov --target coverage-check

- name: Publier le rapport
  uses: actions/upload-artifact@v4
  with:
    name: coverage-report
    path: build-cov/coverage_report/
```

### GitLab CI (extrait)

```yaml
coverage:
  stage: test
  script:
    - cmake -B build-cov -G Ninja
      -DCMAKE_BUILD_TYPE=Debug
      -DENABLE_COVERAGE=ON
    - cmake --build build-cov --target coverage-check
  artifacts:
    paths:
      - build-cov/coverage_report/
    reports:
      coverage_report:
        coverage_format: cobertura
        path: build-cov/coverage_cobertura.xml
  coverage: '/lines.*: (\d+\.\d+)%/'
```

La directive `coverage:` de GitLab CI extrait le pourcentage depuis la sortie de `lcov --summary` via une regex, et l'affiche comme badge dans l'interface. La section 38 détaille cette intégration.

## Résumé de la structure finale

Après intégration, la structure du projet inclut les fichiers suivants relatifs à la couverture :

```
mon_projet/
├── CMakeLists.txt              # Option ENABLE_COVERAGE + flags
├── CMakePresets.json            # Preset "coverage"
├── .lcovrc                      # Configuration lcov/genhtml
├── .gitignore                   # Exclut coverage_report/ et *.info
├── cmake/
│   └── CodeCoverage.cmake       # Module réutilisable
├── src/
│   └── ...
└── tests/
    └── ...
```

La chaîne complète — de `cmake --preset coverage` à l'ouverture du rapport HTML — est automatisée, reproductible et intégrée dans la CI. Le développeur n'a besoin de retenir qu'une seule commande ; le pipeline CI n'a besoin que d'une seule cible.

---


⏭️ [Objectifs de couverture](/34-couverture-code/04-objectifs-couverture.md)
