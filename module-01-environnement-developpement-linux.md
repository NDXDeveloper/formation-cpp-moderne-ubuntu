🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 1 — L'Environnement de Développement sur Linux

> 🎯 Niveau : Débutant

Ce module installe l'ensemble de la chaîne de compilation C++ sur Ubuntu et vous fait comprendre ce qui se passe entre votre code source et le binaire ELF qui s'exécute. Pas de code métier ici — uniquement l'outillage, le pipeline de compilation, et la configuration de l'environnement de travail. C'est le socle technique sans lequel rien de ce qui suit dans la formation ne peut être vérifié, débogué ou optimisé.

---

## Objectifs pédagogiques

1. **Comprendre** le cycle de compilation complet d'un programme C++ : préprocesseur, compilation, assemblage, édition de liens.
2. **Installer** et configurer GCC 15 et Clang 20 sur Ubuntu, avec gestion des versions via `update-alternatives`.
3. **Analyser** la structure d'un exécutable ELF avec `readelf`, `objdump`, `nm` et `ldd`.
4. **Configurer** un environnement de développement productif (VS Code avec clangd, DevContainers, debugging intégré).
5. **Utiliser** ccache pour accélérer les compilations répétées et vérifier son efficacité via les statistiques de cache.
6. **Maîtriser** les options de compilation critiques : warnings (`-Wall -Wextra -Werror`), optimisation (`-O2`, `-O3`), debug (`-g`), standard (`-std=c++23`, `-std=c++26`).
7. **Utiliser** `std::print` (C++23) comme remplacement moderne de `std::cout` et `printf`.

---

## Prérequis

Aucun prérequis interne à la formation — c'est le premier module.

Prérequis externes :
- Une machine Ubuntu (22.04 LTS ou plus récent), physique, VM ou WSL2.
- Familiarité de base avec le terminal Linux (navigation, édition de fichiers).

---

## Chapitres

### Chapitre 1 — Introduction au C++ et à l'écosystème Linux

Contexte historique et technique du C++ sur Linux. Ce chapitre couvre l'évolution du langage de C++98 à C++26, les raisons concrètes de choisir C++ pour le system programming et le DevOps, et le fonctionnement détaillé du pipeline de compilation.

- Histoire du C++ : les grandes étapes de C++98 à C++26 (ratifié) et ce que chaque standard a changé.
- Positionnement du C++ face aux alternatives (Rust, Go, Python) pour le system programming.
- Pipeline de compilation décomposé : préprocesseur (`#include`, `#define`, macros), compilation (génération du code objet), édition de liens (résolution des symboles).
- Format ELF : structure (headers, sections, segments), inspection avec `readelf` et `objdump`.

### Chapitre 2 — Mise en place de la Toolchain sur Ubuntu

Installation et configuration de tous les outils nécessaires pour compiler, déboguer et développer en C++ sur Ubuntu. C'est le chapitre le plus opérationnel du module — chaque section produit un résultat vérifiable.

- Installation de GCC (`g++`) et LLVM (`clang++`), gestion multi-versions avec `update-alternatives`, état GCC 15 / Clang 20 et support C++26.
- Outils essentiels : `build-essential`, `gdb`, `make`, `ninja-build`, `cmake`.
- ccache : installation, intégration CMake/Makefiles, monitoring des statistiques de cache.
- Configuration IDE : extensions VS Code (C/C++, CMake Tools, clangd), debugging intégré, DevContainers, IA-assisted tooling (Copilot, Clangd AI).
- Premier programme compilé manuellement étape par étape (`g++ -E`, `-S`, `-c`), inspection des binaires (`nm`, `objdump`, `ldd`), résolution des dépendances dynamiques.
- Options de compilation : warnings (`-Wall`, `-Wextra`, `-Wpedantic`, `-Werror`), optimisation (`-O0` à `-O3`, `-Os`), debug (`-g`, `-ggdb3`), sélection du standard (`-std=c++17` à `-std=c++26`).
- Introduction à `std::print` (C++23) : syntaxe, comparaison avec `std::cout`/`printf`, formatage type-safe, état du support compilateur.

> 💡 La section 2.7 (`std::print`) est une prise en main rapide. La couverture approfondie de `std::format` et `std::print` se trouve en section 12.7.

---

## Points de vigilance

- **Confusion version GCC / version standard.** Avoir GCC 15 installé ne signifie pas que votre code compile en C++26. Il faut passer explicitement `-std=c++26` — la version du compilateur et le standard activé sont deux choses distinctes. Vérifiez avec `g++ -dM -E -x c++ /dev/null | grep cplusplus`.
- **PATH avec plusieurs versions de Clang.** Si vous installez Clang 18 et Clang 20, `clang++` dans votre PATH peut pointer vers l'ancienne version. `update-alternatives` est là pour ça — ne vous fiez pas à `which clang++` sans vérifier `clang++ --version`.
- **ccache qui ne cache pas.** Une erreur fréquente est d'installer ccache sans le déclarer comme `CMAKE_C_COMPILER_LAUNCHER` / `CMAKE_CXX_COMPILER_LAUNCHER` dans CMake, ou sans le placer en préfixe dans le PATH. Vérifiez avec `ccache -s` que le hit rate augmente réellement après une recompilation.
- **DevContainers qui masquent la toolchain locale.** Les DevContainers fournissent un environnement reproductible, mais si vous ne comprenez pas la toolchain qu'ils embarquent, vous ne saurez pas diagnostiquer un problème de compilation hors conteneur. Maîtrisez d'abord l'installation locale avant de passer aux DevContainers.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Installer et maintenir une toolchain C++ complète sur Ubuntu avec gestion multi-versions.
- Compiler un programme en contrôlant chaque étape du pipeline et inspecter le binaire résultant.
- Configurer ccache et vérifier qu'il accélère effectivement vos builds.
- Travailler dans un IDE configuré pour C++ (VS Code + clangd) avec debugging intégré.
- Choisir les bonnes options de compilation selon le contexte (développement, debug, release).
- Écrire un premier programme utilisant `std::print` (C++23) au lieu de `std::cout`.

---


⏭️ [Introduction au C++ et à l'écosystème Linux](/01-introduction-cpp-linux/README.md)
