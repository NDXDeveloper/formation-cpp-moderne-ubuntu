🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 26.3 Gestion des dépendances et sous-répertoires

> **Objectif** : Comprendre les trois mécanismes de CMake pour intégrer des bibliothèques externes dans un projet — `find_package()`, `FetchContent` et `add_subdirectory()` — savoir quand utiliser chacun, et maîtriser les principes communs qui les sous-tendent.

---

## Le problème fondamental

Aucun projet C++ sérieux ne vit en autarcie. Parsing JSON, cryptographie, compression, logging, tests unitaires, networking — vous dépendez de bibliothèques tierces. Et c'est précisément là que le C++ a historiquement souffert par rapport à des écosystèmes comme Python (pip), Rust (cargo) ou JavaScript (npm). Il n'existe pas de gestionnaire de paquets universel intégré au langage.

CMake ne résout pas ce problème à lui seul — ce n'est pas un gestionnaire de paquets. Mais il fournit les **mécanismes d'intégration** qui permettent à votre projet de trouver, télécharger et consommer des dépendances, quel que soit leur mode de distribution. Ces mécanismes fonctionnent seuls ou en complément de vrais gestionnaires de paquets comme Conan et vcpkg (couverts au chapitre 27).

---

## Les trois approches

CMake offre trois mécanismes pour intégrer une dépendance dans votre projet. Chacun répond à un scénario différent :

### `find_package()` — trouver une bibliothèque déjà installée

La dépendance est **pré-installée** sur le système (via `apt`, Conan, vcpkg, ou une compilation manuelle). CMake la localise grâce à un fichier de configuration ou un module de recherche, et expose ses cibles importées.

```cmake
find_package(OpenSSL REQUIRED)  
target_link_libraries(my_lib PRIVATE OpenSSL::SSL)  
```

C'est l'approche la plus classique et celle qui offre les meilleurs temps de configuration : rien à télécharger, rien à compiler.

### `FetchContent` — télécharger et intégrer à la configuration

La dépendance est **téléchargée** automatiquement par CMake (typiquement depuis un dépôt Git ou une archive) et intégrée dans le build comme si elle faisait partie de votre projet. L'utilisateur n'a rien à installer au préalable.

```cmake
include(FetchContent)  
FetchContent_Declare(googletest  
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
)
FetchContent_MakeAvailable(googletest)  
target_link_libraries(my_test PRIVATE GTest::gtest_main)  
```

C'est l'approche la plus autonome : le build est reproductible sans dépendance externe pré-installée.

### `add_subdirectory()` — inclure un répertoire de sources

La dépendance est **déjà présente** dans votre arborescence — soit parce que vous l'avez copiée dans un répertoire `third_party/`, soit parce que c'est un sous-module Git, soit parce que c'est une bibliothèque interne de votre projet.

```cmake
add_subdirectory(third_party/fmt)  
target_link_libraries(my_lib PRIVATE fmt::fmt)  
```

C'est l'approche la plus simple quand les sources sont déjà là.

---

## Comment choisir ?

Le choix entre les trois mécanismes dépend de la nature de la dépendance et du contexte de votre projet. Voici un arbre de décision pratique :

```
La dépendance est-elle une bibliothèque interne de votre projet ?
│
├── OUI (votre propre code, organisé en sous-bibliothèques)
│   └── → add_subdirectory()
│       C'est le cas de libs/utils dans notre projet exemple (section 26.1).
│
└── NON (bibliothèque tierce)
    │
    La dépendance est-elle couramment disponible sur les systèmes cibles ?
    (installable via apt, présente dans Conan/vcpkg, ou fournie par l'OS)
    │
    ├── OUI
    │   └── → find_package()
    │       Exemples : OpenSSL, ZLIB, Threads, Boost, Protobuf.
    │       C'est l'approche la plus légère — pas de téléchargement au build.
    │
    └── NON ou vous voulez un build 100% autonome
        │
        Voulez-vous versionner les sources dans votre dépôt ?
        │
        ├── OUI (sous-module Git, copie dans third_party/)
        │   └── → add_subdirectory()
        │       Contrôle total, pas de téléchargement, mais surcharge le dépôt.
        │
        └── NON
            └── → FetchContent
                Téléchargement automatique à la configuration.
                Reproductible grâce au tag Git ou au hash d'archive.
```

En pratique, un projet professionnel mélange souvent les trois approches. Un `CMakeLists.txt` racine typique pourrait ressembler à :

```cmake
# Dépendances système — find_package
find_package(OpenSSL REQUIRED)  
find_package(Threads REQUIRED)  
find_package(ZLIB QUIET)  

# Dépendances téléchargées — FetchContent
include(FetchContent)  
FetchContent_Declare(spdlog  
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.3
)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
)
FetchContent_MakeAvailable(spdlog googletest)

# Sous-répertoires internes — add_subdirectory
add_subdirectory(libs/utils)  
add_subdirectory(src)  
add_subdirectory(apps)  
```

---

## Le point commun : les cibles importées

Quel que soit le mécanisme utilisé, le résultat est le même du point de vue du consommateur : une **cible CMake** utilisable dans `target_link_libraries()`. C'est le contrat fondamental de CMake moderne — l'uniformité de l'interface de consommation.

| Mécanisme | Ce qui se passe en coulisse | Ce que vous utilisez |
|-----------|---------------------------|---------------------|
| `find_package(OpenSSL)` | Localise OpenSSL sur le système, crée des cibles importées | `OpenSSL::SSL`, `OpenSSL::Crypto` |
| `FetchContent(spdlog)` | Télécharge spdlog, exécute son `CMakeLists.txt` | `spdlog::spdlog` |
| `add_subdirectory(libs/utils)` | Exécute le `CMakeLists.txt` du sous-répertoire | `my_project::utils` |

Pour le code en aval, la consommation est identique :

```cmake
target_link_libraries(my_project_core
    PRIVATE
        OpenSSL::SSL          # via find_package
        spdlog::spdlog        # via FetchContent
        my_project::utils     # via add_subdirectory
)
```

Cette uniformité est ce qui rend le système de dépendances de CMake aussi flexible. Vous pouvez migrer une dépendance de `FetchContent` vers `find_package` (parce que vous avez commencé à l'installer via Conan, par exemple) sans toucher une seule ligne dans les `CMakeLists.txt` des consommateurs — seule la déclaration de la dépendance change.

---

## Stratégie avec les gestionnaires de paquets

Les trois mécanismes natifs de CMake peuvent être combinés avec des gestionnaires de paquets C++ qui ajoutent une couche de résolution et de versioning :

**Conan 2.0** (couvert en section 27.2) installe les dépendances et génère des fichiers de configuration CMake. Vos `find_package()` fonctionnent ensuite normalement — Conan a simplement placé les bonnes bibliothèques et les bons fichiers CMake aux bons endroits.

**vcpkg** (couvert en section 27.3) fonctionne de manière similaire : il installe les dépendances dans un répertoire dédié et configure CMake pour les trouver via `find_package()`.

Dans les deux cas, le flux est :

```
Conan/vcpkg installe les dépendances
        │
        ▼
find_package() les trouve (grâce aux fichiers de config générés)
        │
        ▼
target_link_libraries() les consomme (cibles importées)
```

L'intérêt est que votre `CMakeLists.txt` reste propre et indépendant du gestionnaire de paquets. Si vous passez de Conan à vcpkg, ou si un utilisateur préfère installer les dépendances manuellement, les mêmes `find_package()` fonctionnent sans modification.

---

## Bonnes pratiques transversales

Avant de plonger dans le détail de chaque mécanisme, quelques principes s'appliquent quelle que soit l'approche choisie.

**Consommez toujours via des cibles namespacées.** Les noms avec `::` (comme `OpenSSL::SSL`, `GTest::gtest_main`, `fmt::fmt`) garantissent une erreur immédiate en cas de typo et fonctionnent de manière identique qu'ils proviennent de `find_package`, `FetchContent` ou `add_subdirectory`.

**Séparez la déclaration de la consommation.** La déclaration d'une dépendance (`find_package`, `FetchContent_Declare`) appartient au `CMakeLists.txt` racine ou à un module dédié. La consommation (`target_link_libraries`) appartient au `CMakeLists.txt` de la cible qui en a besoin. Cette séparation facilite les changements de stratégie d'intégration.

**Versionnez vos dépendances.** Que ce soit un tag Git dans `FetchContent_Declare`, une contrainte de version dans `find_package`, ou un commit précis de sous-module Git — ne laissez jamais une dépendance pointer vers une branche mobile (`main`, `master`). La reproductibilité du build en dépend.

**Rendez les dépendances optionnelles quand c'est pertinent.** Certaines bibliothèques sont des enrichissements (compression ZLIB, support TLS) plutôt que des prérequis. Utilisez `find_package(ZLIB QUIET)` et conditionnez la fonctionnalité, plutôt que de faire échouer toute la configuration si ZLIB n'est pas installé.

**Documentez la stratégie d'intégration.** Un paragraphe dans le README ou un commentaire dans le `CMakeLists.txt` racine indiquant quelles dépendances sont attendues sur le système et lesquelles sont téléchargées automatiquement épargne un temps considérable aux nouveaux contributeurs.

---

## Plan des sous-sections

Les trois sous-sections suivantes détaillent chaque mécanisme en profondeur :

| Sous-section | Mécanisme | Ce que vous apprendrez |
|-------------|-----------|----------------------|
| **26.3.1** | `find_package()` | Modes Config et Module, fichiers `*Config.cmake` et `Find*.cmake`, composants, versions, variables résultantes |
| **26.3.2** | `FetchContent` | Déclaration, téléchargement, options de la dépendance, `FetchContent_MakeAvailable`, pièges courants |
| **26.3.3** | `add_subdirectory()` | Inclusion de répertoires internes et externes, sous-modules Git, isolation de scope |

---

> **À suivre** : La sous-section 26.3.1 détaille `find_package()` — le mécanisme le plus ancien et le plus utilisé pour localiser des bibliothèques pré-installées sur le système.

⏭️ [find_package](/26-cmake/03.1-find-package.md)
