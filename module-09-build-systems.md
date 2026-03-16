🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 9 — Build Systems et Gestion de Projet

> 🎯 Niveau : Avancé

Ce module couvre l'infrastructure de build d'un projet C++ professionnel. CMake comme générateur central (targets, dépendances transitives, presets, cross-compilation), Conan 2.0 et vcpkg pour la gestion des dépendances, linkage statique vs dynamique, puis les build systems d'exécution — Ninja, Make, Meson. Un projet C++ dont le build est mal structuré devient inmaintenable bien avant que le code lui-même ne pose problème.

---

## Objectifs pédagogiques

1. **Structurer** un projet CMake moderne basé sur les targets (`add_executable`, `add_library`, `target_link_libraries` avec `PUBLIC`/`PRIVATE`/`INTERFACE`).
2. **Gérer** les dépendances externes avec `find_package`, `FetchContent`, `add_subdirectory`, et les gestionnaires de paquets (Conan 2.0, vcpkg).
3. **Configurer** Conan 2.0 avec `conanfile.py`, profils, settings et intégration CMake.
4. **Choisir** entre linkage statique (`.a`) et dynamique (`.so`) en fonction du contexte de déploiement, et comprendre les implications (taille du binaire, résolution au runtime, ODR).
5. **Utiliser** CMake Presets pour standardiser les configurations de build (debug, release, sanitizers, cross-compilation).
6. **Comprendre** les différences entre Make, Ninja et Meson, et justifier le choix du build system selon le projet.

---

## Prérequis

- **Module 1, chapitre 2** : toolchain de base (GCC, Clang, cmake, ninja-build, make) installée et fonctionnelle — ce module suppose que vous savez compiler un programme simple.
- **Module 1, chapitre 2, section 2.3** : ccache — l'intégration ccache/CMake est approfondie ici.
- **Module 1, chapitre 2, section 2.6** : options de compilation (warnings, optimisation, debug, standard) — CMake les configure via des target properties.

---

## Chapitres

### Chapitre 26 — CMake : Le Standard de l'Industrie

CMake est le générateur de build dominant en C++. Ce chapitre couvre l'écriture de `CMakeLists.txt` modernes basés sur les targets, la gestion des dépendances, la génération pour Ninja, la cross-compilation, et les nouveautés CMake 3.31+.

- Structure d'un projet CMake moderne : répertoires, fichier racine `CMakeLists.txt`, sous-projets.
- Targets : `add_executable`, `add_library`, `target_link_libraries`, `target_include_directories` — tout passe par les targets, pas par les variables globales.
- `PUBLIC` / `PRIVATE` / `INTERFACE` : contrôle des dépendances transitives — `PUBLIC` propage aux consommateurs, `PRIVATE` reste interne, `INTERFACE` propage sans utiliser soi-même.
- Gestion des dépendances : `find_package` (librairies système installées), `FetchContent` (téléchargement et build à la configuration), `add_subdirectory` (sous-projets locaux).
- Configuration et génération de fichiers : `configure_file`, variables de build injectées dans le code.
- Génération pour Ninja : `cmake -G Ninja` — recommandé par défaut pour la vitesse de build incrémental.
- Toolchains et cross-compilation : fichiers toolchain pour ARM, RISC-V, spécification du sysroot et du compilateur cible.
- CMake 3.31+ : nouveautés et meilleures pratiques 2026.

### Chapitre 27 — Gestion des Dépendances

Le problème historique de C++ : il n'y a pas de gestionnaire de paquets universel. Ce chapitre couvre les deux solutions dominantes (Conan 2.0, vcpkg), le linkage statique vs dynamique, et les CMake Presets.

- Le problème des librairies en C++ : ABI compatibility, versions multiples, absence de standard de packaging.
- **Conan 2.0** : nouvelle API basée sur `conanfile.py` (vs `conanfile.txt`), profils (compilateur, OS, architecture, build type), settings, intégration CMake via `CMakeToolchain` et `CMakeDeps`.
- **vcpkg** : alternative Microsoft, intégration CMake via `CMAKE_TOOLCHAIN_FILE`, mode manifest (`vcpkg.json`).
- Linkage statique (`.a`) vs dynamique (`.so`) : taille du binaire, résolution des symboles au runtime (`ld.so`), impact sur le déploiement (conteneurs Docker, distribution), risques d'ODR violations en cas de mixing.
- Installation et distribution de librairies sur Linux : `CMAKE_INSTALL_PREFIX`, `install()`, `pkg-config`, `find_package` avec les fichiers `Config.cmake`.
- **CMake Presets** : fichiers `CMakePresets.json` et `CMakeUserPresets.json` pour standardiser les configurations (debug, release, sanitizers, CI) — partagés dans le repo, utilisables par tous les développeurs et la CI.

### Chapitre 28 — Makefile, Ninja et Build Automation

Les build systems qui exécutent les commandes générées par CMake (ou écrites manuellement). Make est historique, Ninja est rapide, Meson est l'alternative montante dans l'écosystème Linux.

- Makefiles : syntaxe de base (targets, dépendances, recettes), variables, règles patterns (`%.o: %.cpp`). Nécessaire à connaître pour lire les builds existants et les projets legacy.
- **Ninja** : build system conçu pour la vitesse — pas de syntaxe complexe, fichiers `build.ninja` générés (pas écrits à la main), parallélisme maximal par défaut. Pourquoi Ninja est plus rapide que Make : pas de parsing de Makefile complexe, pas de règles récursives, graphe de dépendances optimisé.
- **Meson** : build system déclaratif (syntaxe Python-like), backend Ninja, utilisé par des projets majeurs (GNOME, systemd, Mesa, GStreamer). Avantage : configuration plus lisible que CMake. Limite : écosystème plus restreint.
- Comparaison Make vs Ninja vs Meson : performances (temps de build incrémental), expressivité, écosystème, adoption industrielle.

---

## Points de vigilance

- **Variables globales CMake au lieu de target properties.** Utiliser `include_directories()`, `link_libraries()`, ou `add_definitions()` au lieu de leurs équivalents `target_*` pollue toutes les targets du projet. Les dépendances transitives deviennent incontrôlables — une target hérite d'include paths et de flags qui ne la concernent pas, ce qui provoque des collisions de symboles et des temps de compilation inutiles. Règle : ne jamais utiliser les commandes globales. Tout passe par `target_include_directories`, `target_link_libraries`, `target_compile_definitions` avec le bon scope (`PUBLIC`/`PRIVATE`/`INTERFACE`).

- **Conan profile non spécifié.** Si vous ne passez pas de profil explicite (`conan install . -pr:b=default -pr:h=my_profile`), Conan utilise le profil `default` qui est auto-détecté au premier lancement. Ce profil peut ne pas correspondre à votre compilateur actif (mauvaise version de GCC, mauvais standard C++, mauvais build type). Résultat : des librairies compilées avec des options incompatibles, et des erreurs de link cryptiques. Créez un profil explicite par projet et versionnez-le dans le repo.

- **Mixing de linkage statique et dynamique de la même librairie.** Linker une librairie en statique dans un exécutable et en dynamique dans un shared object chargé par ce même exécutable crée deux copies des symboles en mémoire. C'est une violation ODR (One Definition Rule) — les variables globales et statiques existent en double, ce qui produit des bugs silencieux (état incohérent, double free). La solution : choisir un seul mode de linkage par librairie pour l'ensemble du projet.

- **`FetchContent` qui retélécharge à chaque `cmake` configure.** Par défaut, `FetchContent_Declare` télécharge le contenu dans le build directory. Si vous supprimez le build directory (clean build) ou si le cache CMake est invalidé, le téléchargement recommence. Sur un projet avec 10+ dépendances FetchContent, c'est plusieurs minutes de configure. Solutions : utiliser `FETCHCONTENT_BASE_DIR` pour un cache partagé entre les builds, ou passer à un vrai gestionnaire de paquets (Conan, vcpkg) pour les dépendances stables.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Écrire un `CMakeLists.txt` moderne basé sur les targets avec dépendances transitives correctement scopées (`PUBLIC`/`PRIVATE`/`INTERFACE`).
- Gérer les dépendances d'un projet C++ avec Conan 2.0 (profils, `conanfile.py`, intégration CMake) ou vcpkg.
- Choisir entre linkage statique et dynamique selon le contexte de déploiement, et éviter les violations ODR.
- Standardiser les configurations de build avec CMake Presets et les partager entre développeurs et CI.
- Générer des builds avec Ninja pour des temps de compilation optimaux.
- Lire et comprendre des Makefiles existants, et évaluer Meson comme alternative à CMake.

---


⏭️ [CMake : Le Standard de l'Industrie](/26-cmake/README.md)
