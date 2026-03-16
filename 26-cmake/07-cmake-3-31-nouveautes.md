🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 26.7 CMake 3.31+ : Nouveautés et meilleures pratiques 2026 ⭐

> **Objectif** : Faire le point sur l'état de CMake en mars 2026 — les versions disponibles, les fonctionnalités majeures introduites depuis CMake 3.31, la transition vers CMake 4.x, le support des C++20 Modules, et les recommandations actualisées pour un nouveau projet.

---

## Le paysage CMake en mars 2026

CMake a connu une évolution majeure entre fin 2024 et début 2026. Voici les versions pertinentes :

| Version | Date de sortie | Remarque |
|---------|---------------|----------|
| **3.31** | Novembre 2024 | Dernière release de la branche 3.x |
| **4.0** | Mars 2025 | Passage au versioning majeur, suppression de la compatibilité < 3.5 |
| **4.1** | Août 2025 | Améliorations incrémentales, `CMAKE_FIND_REQUIRED`, find améliorés |
| **4.2** | Novembre 2025 | Support CPS expérimental renforcé |
| **4.3** | En cours (RC) | CPS non expérimental, instrumentation de build |

Le titre de cette section mentionne « 3.31+ » car c'est la baseline que nous avons recommandée tout au long du chapitre. En pratique, si votre environnement le permet, CMake 4.0+ est aujourd'hui le choix recommandé pour les nouveaux projets.

---

## CMake 3.31 : les dernières nouveautés de la branche 3.x

CMake 3.31, sorti en novembre 2024, apporte plusieurs améliorations notables qui marquent la fin de la branche 3.x.

### Cible `codegen` pour la génération de code

Les générateurs Ninja et Makefile produisent désormais une cible `codegen` automatique. Cette cible pilote un sous-ensemble du graphe de build suffisant pour exécuter les commandes personnalisées marquées avec la nouvelle option `CODEGEN` :

```cmake
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated_api.cpp
    COMMAND ${PROTOC_EXECUTABLE} --cpp_out=${CMAKE_CURRENT_BINARY_DIR} api.proto
    DEPENDS api.proto
    CODEGEN    # ← Nouveau : cette commande fait partie du sous-graphe codegen
)
```

```bash
# Générer uniquement le code (protobuf, flatbuffers, etc.) sans compiler le projet
cmake --build build --target codegen
```

C'est précieux pour les workflows où la génération de code doit précéder d'autres étapes (analyse statique, indexation IDE, vérification de schéma) sans lancer un build complet.

### Commentaires dans les presets

Les fichiers `CMakePresets.json` supportent désormais la clé `$comment` à n'importe quel niveau (à partir de la version 10 du schéma de presets) :

```json
{
    "version": 10,
    "$comment": "Presets pour le projet my_project — Mars 2026",
    "configurePresets": [
        {
            "name": "default",
            "$comment": "Configuration par défaut : Ninja + Debug + ccache",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            }
        }
    ]
}
```

C'est un ajout simple mais bienvenu — les presets complexes avec de nombreuses configurations sont désormais auto-documentés.

### Workflow simplifié

La commande `cmake --workflow` accepte maintenant le nom du preset directement :

```bash
# Avant (CMake 3.30)
cmake --workflow --preset release

# Après (CMake 3.31+) — syntaxe simplifiée
cmake --workflow release
```

### Stratégie de linkage configurable

La variable `CMAKE_LINK_LIBRARIES_STRATEGY` et la propriété `LINK_LIBRARIES_STRATEGY` permettent de contrôler comment CMake génère les lignes de linkage. Cela offre un contrôle plus fin sur l'ordonnancement des bibliothèques, utile dans les cas complexes de dépendances circulaires entre bibliothèques statiques.

### Dépréciation de la compatibilité < 3.10

CMake 3.31 avertit désormais quand `cmake_minimum_required()` spécifie une version inférieure à 3.10. C'est le prélude à la suppression complète dans CMake 4.0.

---

## CMake 4.0 : le passage au versioning majeur

CMake 4.0, sorti le 28 mars 2025, est la première version majeure depuis la création de CMake. Malgré le numéro de version impressionnant, la transition depuis 3.31 est douce pour les projets modernes. Les changements majeurs sont :

### Suppression de la compatibilité < 3.5

La compatibilité avec les policies antérieures à CMake 3.5 est supprimée. Concrètement, `cmake_minimum_required(VERSION 3.4)` produit maintenant une erreur. Cependant, la syntaxe à plage reste valide :

```cmake
# ❌ Erreur dans CMake 4.0+
cmake_minimum_required(VERSION 3.4)

# ✅ Fonctionne — nécessite CMake 3.4 minimum, active les policies jusqu'à 3.25
cmake_minimum_required(VERSION 3.4...3.25)
```

Pour les projets qui dépendent de bibliothèques tierces anciennes n'ayant pas mis à jour leur `cmake_minimum_required`, CMake 4.0 fournit la variable `CMAKE_POLICY_VERSION_MINIMUM` comme contournement :

```bash
cmake -B build -G Ninja -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

Cela permet de construire des projets legacy sans modifier leur code.

### Support consolidé des C++20 Modules

Le support des C++20 Modules a quitté le statut expérimental dès **CMake 3.28** (annonce Kitware « *import CMake; the Experiment is Over!* »). CMake 4.0 consolide ce support avec des améliorations de fiabilité, notamment pour `import std;` et les interactions entre modules et bibliothèques partagées. Le mécanisme repose sur les `FILE_SET` de type `CXX_MODULES` :

```cmake
add_library(my_module)  
target_sources(my_module  
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/core.cppm
            src/utils.cppm
    PRIVATE
        src/core_impl.cpp
)

target_compile_features(my_module PUBLIC cxx_std_20)
```

CMake scanne automatiquement les dépendances entre modules et génère le bon ordre de compilation. Les fichiers `.cppm` (convention de nommage la plus courante pour les *module interface units*) sont traités comme des unités spéciales qui produisent à la fois un fichier objet et un *Built Module Interface* (BMI — fichier `.gcm` pour GCC, `.pcm` pour Clang).

Le support de `import std;` (C++23) est également fonctionnel avec les compilateurs récents. La propriété `CXX_MODULE_STD` active le support pour une cible :

```cmake
set_target_properties(my_module PROPERTIES CXX_MODULE_STD ON)
```

> ⚠️ **État en mars 2026** : le support des modules fonctionne avec Ninja comme générateur et les compilateurs GCC 15 et Clang 20. Le support Make est plus limité. Pour les nouveaux projets qui adoptent les modules, Ninja est impératif. Pour les projets existants, la migration vers les modules est encore optionnelle — le modèle classique header/source reste parfaitement supporté et ne sera pas déprécié. Voir la section 12.13 pour une couverture détaillée de l'état des modules C++20 en 2026.

### Commande `--project-file`

CMake 4.0 ajoute l'option `--project-file` pour spécifier un nom de fichier alternatif à `CMakeLists.txt`. Cette fonctionnalité est explicitement destinée aux transitions incrémentales et émet un warning — le nom standard reste `CMakeLists.txt`.

### Commande native `cmake_pkg_config()`

Introduite en 3.31 et consolidée en 4.0, cette commande permet de parser nativement les fichiers `.pc` (pkg-config) sans dépendre de l'outil externe `pkg-config` :

```cmake
cmake_pkg_config(EXTRACT libuv
    CFLAGS CFLAGS_VAR
    LIBS LIBS_VAR
)
```

C'est une alternative au module `FindPkgConfig` pour les cas où l'outil `pkg-config` n'est pas installé.

---

## CMake 4.1+ : améliorations incrémentales

### Journalisation des commandes find

CMake 4.1 enrichit le `cmake-configure-log` avec les résultats de `find_package()`, `find_path()`, `find_library()` et `find_program()`. Ces logs sont émis automatiquement lors de la première invocation ou quand le résultat change entre « trouvé » et « non trouvé ». Combiné avec `--debug-find`, le diagnostic des problèmes de détection est considérablement simplifié.

### `CMAKE_FIND_REQUIRED` global

La variable `CMAKE_FIND_REQUIRED` rend toutes les commandes find (`find_package`, `find_library`, etc.) `REQUIRED` par défaut. Les appels individuels peuvent se soustraire à ce comportement avec le nouveau mot-clé `OPTIONAL` :

```cmake
set(CMAKE_FIND_REQUIRED TRUE)

find_package(OpenSSL)          # Implicitement REQUIRED  
find_package(ZLIB OPTIONAL)    # Explicitement optionnel  
```

### Linker launcher

Les générateurs Makefile et Ninja supportent désormais un *linker launcher* (analogue au *compiler launcher* utilisé pour ccache) pour Fortran, CUDA et HIP, via `CMAKE_<LANG>_LINKER_LAUNCHER`.

---

## Common Package Specification (CPS) : l'avenir de la distribution de paquets

Le développement le plus significatif pour l'écosystème CMake à moyen terme est le **Common Package Specification** (CPS). Le CPS est un format déclaratif (basé sur JSON) pour décrire les métadonnées d'une bibliothèque — ses cibles, include directories, dépendances, et configurations — de manière **indépendante du build system**.

### Le problème résolu

Aujourd'hui, quand une bibliothèque s'installe et veut être consommable via `find_package()`, elle doit fournir des fichiers `*Config.cmake` écrits en langage CMake. C'est efficace pour les projets CMake, mais opaque pour tout autre build system (Meson, Bazel, etc.). Le CPS propose un format JSON standardisé que n'importe quel outil peut lire :

```json
{
    "name": "my_project",
    "version": "1.2.0",
    "compat-version": "1.0.0",
    "components": {
        "core": {
            "type": "archive",
            "includes": ["include/"],
            "location": "lib/libmy_project_core.a",
            "requires": ["utils"]
        }
    }
}
```

### État en mars 2026

CMake 4.0 a introduit le support de l'import de paquets CPS via `find_package()`. CMake 4.2 a ajouté les cibles `SYMBOLIC` (sous-type d'`INTERFACE`). CMake 4.3 (actuellement en RC) ajoute les commandes `install(PACKAGE_INFO)` et `export(PACKAGE_INFO)` pour générer des descriptions CPS, complétant ainsi le cycle import/export. Le support reste en cours de maturation — certains schémas CPS ne sont pas encore pris en charge.

Pour les projets en 2026, il n'est pas encore nécessaire d'adopter CPS — les fichiers `*Config.cmake` traditionnels (section 26.4) restent le mécanisme de distribution standard. Mais garder un œil sur CPS est recommandé : c'est l'infrastructure qui pourrait enfin unifier la distribution de bibliothèques C++ entre build systems.

---

## Quelle version minimale cibler en 2026 ?

Le choix de `cmake_minimum_required` dépend de votre contexte :

| Contexte | Version recommandée | Justification |
|----------|:------------------:|---------------|
| Nouveau projet, environnement contrôlé | `VERSION 3.25...4.2` | Accès à toutes les fonctionnalités modernes, compatibilité large |
| Nouveau projet, CI et développeurs à jour | `VERSION 4.0` | Simplifie — assume CMake 4.0+ partout |
| Bibliothèque open source, large audience | `VERSION 3.20...4.2` | Supporte les LTS Ubuntu plus anciennes |
| Projet existant, migration progressive | Garder l'existant, monter progressivement | Pas de raison de casser un projet qui fonctionne |

La syntaxe à plage `VERSION <min>...<max>` est recommandée pour les bibliothèques : elle active les policies modernes quand le CMake installé le supporte, tout en restant compatible avec les versions plus anciennes.

```cmake
# Recommandation pour un nouveau projet en 2026
cmake_minimum_required(VERSION 3.25...4.2)  
project(my_project VERSION 1.0.0 LANGUAGES CXX)  
```

---

## Checklist des meilleures pratiques CMake en 2026

Cette checklist synthétise les recommandations de l'ensemble du chapitre 26, mises à jour pour l'écosystème 2026.

### Structure et organisation

- Utiliser un *out-of-source build* : `cmake -B build`, jamais `cmake .`
- Séparer les headers publics (`include/project/`) de l'implémentation (`src/`)
- Séparer les exécutables (`apps/`) des bibliothèques (`src/`, `libs/`) pour faciliter le testing
- Extraire les modules CMake réutilisables dans `cmake/` (warnings, sanitizers)
- Maintenir un `CMakePresets.json` versionné pour standardiser les configurations

### CMakeLists.txt

- Écrire toutes les commandes en **minuscules** (`add_executable`, pas `ADD_EXECUTABLE`)
- Lister les sources **explicitement** — ne pas utiliser `file(GLOB)`
- Utiliser exclusivement les commandes `target_*` — jamais les commandes globales (`include_directories`, `add_definitions`, `link_libraries`)
- Spécifier **toujours** la visibilité : `PRIVATE`, `PUBLIC` ou `INTERFACE`
- Appliquer la **visibilité minimale** : PRIVATE par défaut, PUBLIC uniquement si la propriété apparaît dans les headers publics
- Créer des **alias namespacés** pour chaque bibliothèque : `add_library(proj::lib ALIAS proj_lib)`
- Préfixer les noms de cibles et les options avec le nom du projet

### Dépendances

- Utiliser `find_package()` pour les dépendances système courantes (OpenSSL, ZLIB, Threads)
- Utiliser `FetchContent` pour les dépendances légères et les frameworks de test (GoogleTest, spdlog, fmt)
- Toujours versionner les dépendances (tag Git fixe, `URL_HASH` pour les archives)
- Consommer via des **cibles importées** (`Package::Component`), jamais via les variables héritées
- Désactiver les tests et exemples des dépendances FetchContent

### Build et tooling

- Utiliser **Ninja** comme générateur : `cmake -G Ninja`
- Activer **ccache** : `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache`
- Activer `CMAKE_EXPORT_COMPILE_COMMANDS ON` pour générer `compile_commands.json` (clangd, analyse statique)
- Activer les warnings stricts sur vos cibles (PRIVATE) : `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`
- Utiliser les **CMake Presets** pour documenter les configurations supportées

### Standards C++

- Fixer le standard via `CMAKE_CXX_STANDARD` + `CMAKE_CXX_STANDARD_REQUIRED ON` + `CMAKE_CXX_EXTENSIONS OFF`
- Ou utiliser `target_compile_features(target PUBLIC cxx_std_23)` pour un contrôle par cible
- C++20 Modules : adoptables en 2026 pour les nouveaux projets avec Ninja + GCC 15 / Clang 20, mais optionnels

### Installation et export

- Utiliser les generator expressions `BUILD_INTERFACE` / `INSTALL_INTERFACE` pour les include directories PUBLIC
- Fournir des fichiers `*Config.cmake` et `*ConfigVersion.cmake` pour la consommation via `find_package()`
- Utiliser `SameMajorVersion` comme politique de compatibilité par défaut

---

## Rester à jour

CMake suit un cycle de release semestriel (environ deux versions majeures par an depuis le passage à la branche 4.x). Quelques ressources pour suivre les évolutions :

- **Release notes officielles** : consultez les notes de chaque version sur le site CMake pour identifier les fonctionnalités pertinentes pour votre projet
- **Modern CMake** (cliutils.gitlab.io/modern-cmake) : guide communautaire régulièrement mis à jour, avec un résumé des nouveautés par version
- **Professional CMake** (Craig Scott) : le livre de référence, mis à jour à chaque version majeure
- **Blog Kitware** (kitware.com/blog) : articles techniques des mainteneurs de CMake, couvrant les fonctionnalités en développement et les bonnes pratiques

L'écosystème CMake évolue activement — la montée en puissance du CPS, la maturation des C++20 Modules, et l'amélioration continue de l'expérience développeur (presets, diagnostics, performances) font de CMake un outil de plus en plus agréable à utiliser malgré sa réputation historique.

---

> **Fin du chapitre 26.** Vous maîtrisez désormais CMake en profondeur — de la structure d'un projet (26.1) à l'écriture de `CMakeLists.txt` (26.2), en passant par la gestion des dépendances (26.3), la génération de fichiers (26.4), Ninja (26.5), la cross-compilation (26.6), et l'état de l'art 2026 (26.7). Le chapitre 27 approfondit la gestion des dépendances avec les gestionnaires de paquets Conan et vcpkg.

⏭️ [Gestion des Dépendances](/27-gestion-dependances/README.md)
