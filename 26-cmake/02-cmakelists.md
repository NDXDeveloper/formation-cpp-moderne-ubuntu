🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 26.2 Écrire un CMakeLists.txt : targets, libraries, executables

> **Objectif** : Maîtriser le modèle mental de CMake moderne — les **cibles** (*targets*) et leurs **propriétés** — pour écrire des fichiers `CMakeLists.txt` clairs, modulaires et maintenables. Cette section pose les concepts fondamentaux ; les sous-sections 26.2.1 à 26.2.4 les détailleront un par un.

---

## Le modèle mental : tout est une cible

Si vous ne deviez retenir qu'une seule idée de ce chapitre, ce serait celle-ci : **en CMake moderne, tout tourne autour des cibles**. Une cible (*target*) est l'unité fondamentale du modèle de construction. C'est l'objet auquel vous attachez toutes les informations nécessaires à la compilation : les fichiers sources, les répertoires d'includes, les flags de compilation, les définitions de préprocesseur, les dépendances, et le standard C++ requis.

CMake reconnaît trois grandes familles de cibles :

| Famille | Commande de création | Ce que ça produit |
|---------|---------------------|-------------------|
| **Exécutable** | `add_executable()` | Un binaire exécutable |
| **Bibliothèque** | `add_library()` | Une `.a` (statique), `.so` (partagée), ou rien (INTERFACE) |
| **Cible importée** | Créée par `find_package()` | Référence vers une bibliothèque externe déjà compilée |

Chaque cible est un **nœud** dans le graphe de dépendances du projet. Quand vous écrivez `target_link_libraries(A PRIVATE B)`, vous créez un **arc** de A vers B. CMake parcourt ce graphe pour déterminer l'ordre de compilation, propager les propriétés, et calculer les lignes de commande exactes passées au compilateur et au linker.

---

## Propriétés et propagation

Une cible n'est pas qu'un nom associé à des fichiers sources. C'est un **conteneur de propriétés** (*properties*). Chaque propriété décrit un aspect de la compilation ou de l'utilisation de cette cible. Les propriétés les plus courantes sont :

| Propriété | Commandée par | Exemple |
|-----------|---------------|---------|
| Répertoires d'includes | `target_include_directories()` | `-I/path/to/include` |
| Définitions de préprocesseur | `target_compile_definitions()` | `-DUSE_SSL=1` |
| Options de compilation | `target_compile_options()` | `-Wall -Wextra` |
| Fonctionnalités C++ requises | `target_compile_features()` | `cxx_std_23` |
| Bibliothèques liées | `target_link_libraries()` | `-lssl -lcrypto` |
| Options de linkage | `target_link_options()` | `-static-libstdc++` |

Le point crucial est que ces propriétés **se propagent** le long du graphe de dépendances. C'est là que le système de visibilité entre en jeu.

---

## Le système de visibilité : PUBLIC, PRIVATE, INTERFACE

Chaque fois que vous attachez une propriété à une cible via les commandes `target_*`, vous devez spécifier un **niveau de visibilité**. Ce choix détermine qui voit cette propriété :

### PRIVATE

La propriété s'applique **uniquement** à la cible elle-même. Les cibles qui dépendent d'elle n'en héritent pas.

```cmake
target_compile_definitions(my_lib PRIVATE INTERNAL_DEBUG=1)
```

Ici, seul le code de `my_lib` verra la macro `INTERNAL_DEBUG`. Un exécutable qui lie contre `my_lib` n'en saura rien — et c'est exactement ce qu'on veut pour un détail d'implémentation interne.

### PUBLIC

La propriété s'applique **à la cible elle-même ET à toutes les cibles qui en dépendent**. Elle se propage vers l'aval du graphe.

```cmake
target_include_directories(my_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
```

Le répertoire `include/` est nécessaire à la fois pour compiler `my_lib` (ses `.cpp` incluent ses propres headers) et pour compiler tout code qui utilise `my_lib` (il doit pouvoir inclure `<my_project/core.h>`). `PUBLIC` est le bon choix.

### INTERFACE

La propriété s'applique **uniquement aux cibles qui dépendent de celle-ci**, mais pas à la cible elle-même. C'est le cas typique des bibliothèques *header-only* : elles n'ont pas de sources à compiler, mais les consommateurs ont besoin de connaître le chemin des headers.

```cmake
target_include_directories(header_only_lib INTERFACE ${PROJECT_SOURCE_DIR}/include)
```

### Résumé visuel

Considérons une chaîne de dépendances simple : `app` → `net_lib` → `crypto_lib`.

```
app  ──depends on──▶  net_lib  ──depends on──▶  crypto_lib
```

Si `crypto_lib` déclare une propriété :

| Visibilité | `crypto_lib` la voit ? | `net_lib` la voit ? | `app` la voit ? |
|:----------:|:----------------------:|:-------------------:|:---------------:|
| `PRIVATE` | ✅ | ❌ | ❌ |
| `PUBLIC` | ✅ | ✅ | ✅ |
| `INTERFACE` | ❌ | ✅ | ✅ |

La propagation des propriétés `PUBLIC` est **transitive** : si `crypto_lib` déclare un include directory en `PUBLIC`, et que `net_lib` lie contre `crypto_lib` en `PUBLIC`, alors `app` hérite automatiquement de cet include directory — sans avoir à le déclarer explicitement. Cette transitivité est ce qui rend CMake moderne si puissant : vous déclarez les propriétés une seule fois, au bon endroit, et elles se propagent correctement dans tout le graphe.

---

## La règle d'or pour choisir la visibilité

En pratique, le choix se résume à une question simple sur chaque propriété :

> **Est-ce que cette propriété apparaît dans les headers publics de ma cible ?**

- **Oui** → `PUBLIC`. Le consommateur a besoin de cette information pour compiler le code qui inclut vos headers. Exemples typiques : le répertoire `include/`, une définition de préprocesseur utilisée dans un header public, une dépendance dont les types apparaissent dans votre API.

- **Non** → `PRIVATE`. C'est un détail d'implémentation invisible pour le consommateur. Exemples typiques : un répertoire `internal/`, un flag de warning, une dépendance utilisée uniquement dans les `.cpp`.

- **La cible n'a pas de sources propres** (header-only, alias) → `INTERFACE`. Tout est destiné aux consommateurs.

Respecter cette logique produit des graphes de dépendances propres, où chaque cible expose le minimum nécessaire et rien de plus. C'est le principe de l'encapsulation, appliqué au niveau du build system.

---

## Anatomie d'un CMakeLists.txt de sous-répertoire

Reprenons la structure de la section 26.1 et examinons à quoi ressemble un `CMakeLists.txt` complet pour le sous-répertoire `src/`, qui déclare la bibliothèque principale du projet :

```cmake
# src/CMakeLists.txt

# ── Déclaration de la cible bibliothèque ──────────────────────
add_library(my_project_core
    core.cpp
    network.cpp
)

# ── Alias avec namespace (bonne pratique) ─────────────────────
add_library(my_project::core ALIAS my_project_core)

# ── Include directories ───────────────────────────────────────
target_include_directories(my_project_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
)

# ── Dépendances ───────────────────────────────────────────────
target_link_libraries(my_project_core
    PUBLIC
        my_project::utils       # types de utils/ visibles dans nos headers
    PRIVATE
        OpenSSL::SSL            # utilisé seulement dans les .cpp
)

# ── Warnings (via module personnalisé) ────────────────────────
include(CompilerWarnings)  
set_project_warnings(my_project_core)  
```

Ce fichier illustre tous les concepts que nous venons d'introduire. Décortiquons les éléments moins évidents.

### Les cibles alias avec namespace

```cmake
add_library(my_project::core ALIAS my_project_core)
```

Une cible **alias** crée un second nom pour une cible existante, sans créer de nouvelle cible de build. Le nom utilise le séparateur `::` qui imite les namespaces C++. Cette pratique, recommandée par les mainteneurs de CMake, offre deux avantages.

Le premier est la **cohérence avec `find_package`**. Quand vous consommez une bibliothèque externe via `find_package()`, les cibles importées utilisent la notation à namespace : `OpenSSL::SSL`, `GTest::gtest_main`, `Boost::filesystem`. En exposant vos propres cibles avec la même convention, le code CMake des consommateurs est identique qu'ils intègrent votre bibliothèque comme sous-projet ou comme paquet installé :

```cmake
# Identique dans les deux cas :
target_link_libraries(my_app PRIVATE my_project::core)
```

Le second avantage est la **détection d'erreurs**. Si vous tapez `my_project::croe` (typo), CMake signale immédiatement une erreur car il sait qu'un nom contenant `::` doit correspondre à une cible existante. Sans le namespace, un nom mal tapé serait silencieusement interprété comme un flag de linker, provoquant une erreur cryptique bien plus tard.

### Generator expressions pour les include directories

```cmake
target_include_directories(my_project_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
)
```

Nous avons introduit les generator expressions `BUILD_INTERFACE` / `INSTALL_INTERFACE` en section 26.1. Ici, leur rôle est clair : quand le projet est compilé depuis ses sources (`BUILD_INTERFACE`), les headers publics se trouvent dans `${PROJECT_SOURCE_DIR}/include`. Quand la bibliothèque est installée et consommée par un autre projet (`INSTALL_INTERFACE`), les headers se trouvent dans `include/` relativement au préfixe d'installation.

Le répertoire `internal/` est en `PRIVATE` : seul le code de `my_project_core` peut inclure les headers d'implémentation. Les consommateurs n'y ont pas accès.

### Visibilité des dépendances

```cmake
target_link_libraries(my_project_core
    PUBLIC
        my_project::utils
    PRIVATE
        OpenSSL::SSL
)
```

`my_project::utils` est en `PUBLIC` parce que les types définis dans la bibliothèque utils apparaissent dans les headers publics de `my_project_core`. Si un consommateur inclut `<my_project/core.h>` et que ce header fait référence à un type de `utils/`, le consommateur a besoin de connaître les include directories de utils — la propagation `PUBLIC` s'en charge automatiquement.

`OpenSSL::SSL` est en `PRIVATE` parce qu'OpenSSL n'apparaît que dans les fichiers `.cpp` de `my_project_core`. Les headers publics ne mentionnent pas OpenSSL, donc le consommateur n'a pas besoin de savoir qu'OpenSSL est utilisé en interne.

---

## Les variables CMake vs les propriétés de cibles

Un piège fréquent chez les débutants est de confondre les **variables CMake** et les **propriétés de cibles**. Les variables sont des valeurs globales ou locales à un scope CMake, manipulées avec `set()` :

```cmake
set(CMAKE_CXX_STANDARD 23)           # Variable globale  
set(MY_SOURCES core.cpp network.cpp)  # Variable locale  
```

Les propriétés sont des attributs attachés à une cible spécifique, manipulées avec les commandes `target_*` :

```cmake
target_compile_features(my_project_core PUBLIC cxx_std_23)  # Propriété de cible
```

Les deux mécanismes coexistent, mais en CMake moderne, **les propriétés de cibles sont toujours préférées** aux variables globales pour tout ce qui concerne la compilation. La raison est simple : une variable comme `CMAKE_CXX_STANDARD` s'applique aveuglément à toutes les cibles du scope, tandis qu'une propriété de cible est ciblée et se propage de manière contrôlée.

Un cas concret illustre la différence. Supposons que votre projet dépend d'une bibliothèque tierce qui ne compile qu'en C++17, alors que votre code utilise C++23 :

```cmake
# ❌ Problème avec les variables globales
set(CMAKE_CXX_STANDARD 23)         # Appliqué à TOUT le projet  
add_subdirectory(third_party/old_lib)  # old_lib va recevoir -std=c++23 et peut casser  
```

```cmake
# ✅ Solution avec les propriétés de cibles
target_compile_features(my_project_core PUBLIC cxx_std_23)
# old_lib garde son propre standard, non impacté
```

Dans la pratique, les variables globales comme `CMAKE_CXX_STANDARD` restent acceptables dans le `CMakeLists.txt` racine pour définir le standard **par défaut** du projet. Mais dès qu'un besoin de différenciation apparaît, les propriétés de cibles prennent le relais.

---

## Flux de lecture CMake et portée des variables

CMake lit les fichiers `CMakeLists.txt` de manière séquentielle, de haut en bas. Quand il rencontre `add_subdirectory(src)`, il ouvre `src/CMakeLists.txt`, l'exécute entièrement, puis revient au fichier parent et continue. Ce mécanisme crée une **hiérarchie de scopes** :

```
CMakeLists.txt (racine)          ← scope parent
  ├── libs/utils/CMakeLists.txt  ← scope enfant (hérite des variables du parent)
  ├── src/CMakeLists.txt         ← scope enfant
  ├── apps/CMakeLists.txt        ← scope enfant
  └── tests/CMakeLists.txt       ← scope enfant
```

Les règles de portée sont les suivantes :

- un scope enfant **hérite** de toutes les variables du parent au moment de l'appel `add_subdirectory()` ;
- une modification de variable dans un scope enfant (`set(VAR value)`) **n'affecte pas** le parent — la modification est locale au scope enfant ;
- pour modifier une variable du parent depuis un enfant, il faut utiliser `set(VAR value PARENT_SCOPE)`, mais c'est à éviter autant que possible car cela crée des effets de bord difficiles à suivre.

Les **cibles**, en revanche, sont **globales** : une cible créée dans `libs/utils/CMakeLists.txt` est visible et utilisable depuis `src/CMakeLists.txt`, `tests/CMakeLists.txt`, ou n'importe quel autre `CMakeLists.txt` du projet. C'est pour cela que l'ordre des `add_subdirectory()` dans le fichier racine a son importance : si `src/CMakeLists.txt` référence `my_project_utils`, le `add_subdirectory(libs/utils)` doit apparaître avant `add_subdirectory(src)`.

---

## Bonnes pratiques transversales

Avant de plonger dans le détail de chaque commande (sections 26.2.1 à 26.2.4), voici un condensé des pratiques à intérioriser dès maintenant :

**Nommez vos cibles de manière explicite et unique.** Préfixez avec le nom du projet : `my_project_core`, pas `core`. Créez des alias namespacés : `my_project::core`. Cela élimine les conflits et améliore les messages d'erreur.

**Déclarez les sources explicitement.** Listez chaque fichier source dans `add_library()` ou `add_executable()`. N'utilisez pas `file(GLOB ...)` pour collecter automatiquement les sources — CMake ne peut pas détecter l'ajout ou la suppression d'un fichier source avec un glob, ce qui provoque des builds incohérents sans reconfiguration manuelle.

```cmake
# ❌ Fragile : CMake ne détecte pas les nouveaux fichiers
file(GLOB SOURCES "*.cpp")  
add_library(my_lib ${SOURCES})  

# ✅ Fiable : chaque source est explicitement listée
add_library(my_lib
    core.cpp
    network.cpp
    parser.cpp
)
```

**Utilisez les commandes `target_*`, jamais les commandes globales.** `target_include_directories()` au lieu de `include_directories()`. `target_compile_definitions()` au lieu de `add_definitions()`. `target_compile_options()` au lieu de `add_compile_options()`. Les commandes globales sont l'héritage d'une époque révolue.

**Choisissez la visibilité minimale suffisante.** En cas de doute entre `PUBLIC` et `PRIVATE`, commencez par `PRIVATE`. Vous obtiendrez une erreur de compilation explicite si un consommateur a effectivement besoin de la propriété — à ce moment-là, vous pourrez la remonter en `PUBLIC` en connaissance de cause. L'inverse (tout mettre en `PUBLIC` par facilité) ne produit pas d'erreurs, mais pollue les dépendances transitives et ralentit les builds.

---

## Plan des sous-sections

Les quatre sous-sections suivantes détaillent les commandes CMake au cœur de l'écriture d'un `CMakeLists.txt` :

| Sous-section | Thème |
|-------------|-------|
| **26.2.1** | `add_executable` et `add_library` — créer des cibles de différents types |
| **26.2.2** | `target_link_libraries` — déclarer les dépendances entre cibles |
| **26.2.3** | `target_include_directories` — gérer les chemins d'inclusion |
| **26.2.4** | `PUBLIC`, `PRIVATE`, `INTERFACE` — maîtriser la propagation en profondeur |

Chaque sous-section s'appuie sur le projet exemple de la section 26.1 et enrichit progressivement les `CMakeLists.txt` que nous avons esquissés.

---

> **À suivre** : La sous-section 26.2.1 détaille les commandes `add_executable()` et `add_library()` — les deux briques fondamentales pour créer des cibles dans CMake, avec leurs variantes (STATIC, SHARED, OBJECT, INTERFACE) et les bonnes pratiques associées.

⏭️ [add_executable et add_library](/26-cmake/02.1-add-executable-library.md)
