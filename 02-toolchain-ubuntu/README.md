🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 2 — Mise en place de la Toolchain sur Ubuntu

> **Module 1 · L'Environnement de Développement sur Linux** · Niveau Débutant

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez capable de :

- installer et gérer plusieurs versions de **GCC** et **Clang** sur Ubuntu grâce à `update-alternatives` ;
- comprendre les différences concrètes entre ces deux compilateurs et choisir le bon outil selon le contexte ;
- assembler une **toolchain complète** (compilateur, debugger, build system, accélérateur de compilation) prête pour des projets professionnels ;
- configurer un **environnement de développement moderne** (VS Code, CLion ou Neovim) avec l'autocomplétion, le linting et le debugging intégrés ;
- compiler un programme C++ **étape par étape**, de la source jusqu'au binaire, et inspecter chaque artefact intermédiaire ;
- maîtriser les **options de compilation critiques** (warnings, optimisation, debug, standard) ;
- utiliser **`std::print`** (C++23), la nouvelle façon idiomatique d'afficher du texte en C++.

---

## Pourquoi ce chapitre est essentiel

Le langage C++ ne vit pas en isolation : il s'inscrit dans un écosystème d'outils qui, bien configuré, transforme radicalement la productivité du développeur. Un projet compilé avec les bons *flags*, accéléré par un cache de compilation, et édité dans un IDE correctement relié à `clangd` n'a rien à voir — en confort comme en qualité de code — avec un projet bricolé à la main.

Ubuntu constitue un terrain idéal pour cette mise en place. Les deux grandes familles de compilateurs (GCC et LLVM/Clang) y sont empaquetées, les outils d'analyse (sanitizers, Valgrind) s'installent en une commande, et la communauté de développeurs Linux fournit une documentation abondante. C'est aussi l'environnement que vous retrouverez dans vos conteneurs Docker, vos runners CI/CD et vos machines cloud — autant s'y sentir chez soi dès le départ.

Ce chapitre adopte une approche progressive : on commence par installer les briques de base, puis on les assemble, on les configure et on vérifie que tout fonctionne en compilant un premier programme que l'on inspecte en profondeur. Rien n'est laissé implicite.

---

## Vue d'ensemble des sections

### [2.1 — Installation des compilateurs : GCC (g++) et Clang (clang++)](/02-toolchain-ubuntu/01-installation-compilateurs.md)

Ubuntu donne accès aux deux compilateurs majeurs de l'écosystème C++. Cette section couvre leur installation, la gestion de versions multiples côte à côte via `update-alternatives`, une comparaison pragmatique de leurs forces respectives et un tour d'horizon de l'**état des lieux 2026** avec GCC 15 et Clang 20 — notamment leur support de C++26.

- [2.1.1 Gestion des versions avec update-alternatives](/02-toolchain-ubuntu/01.1-gestion-versions.md)
- [2.1.2 Comparaison GCC vs Clang : avantages et inconvénients](/02-toolchain-ubuntu/01.2-gcc-vs-clang.md)
- [2.1.3 État 2026 : GCC 15 et Clang 20 — nouveautés et support C++26](/02-toolchain-ubuntu/01.3-etat-compilateurs-2026.md) 🔥

### [2.2 — Les outils essentiels : build-essential, gdb, make, ninja-build, cmake](/02-toolchain-ubuntu/02-outils-essentiels.md)

Un compilateur seul ne suffit pas. Cette section présente le socle d'outils qu'un développeur C++ sur Ubuntu installe dès le premier jour : le méta-paquet `build-essential`, le debugger GDB, les build systems Make et Ninja, ainsi que CMake, le générateur de projets devenu standard de l'industrie.

### [2.3 — Accélération de compilation : ccache](/02-toolchain-ubuntu/03-ccache.md) ⭐

Sur un projet de taille moyenne, recompiler l'ensemble du code après un simple `make clean` peut prendre plusieurs minutes. `ccache` intercepte les appels au compilateur et met en cache les résultats : une recompilation identique devient quasi instantanée. Cette section détaille l'installation, l'intégration avec CMake et Make, et les bonnes pratiques de monitoring du cache.

- [2.3.1 Installation et configuration de ccache](/02-toolchain-ubuntu/03.1-installation-ccache.md)
- [2.3.2 Intégration avec CMake et Makefiles](/02-toolchain-ubuntu/03.2-integration-cmake-make.md)
- [2.3.3 Statistiques et monitoring du cache](/02-toolchain-ubuntu/03.3-statistiques-cache.md)

### [2.4 — Configuration de l'IDE : VS Code, CLion, Vim/Neovim](/02-toolchain-ubuntu/04-configuration-ide.md)

Un bon IDE ne remplace pas la compréhension du compilateur, mais il la complète puissamment. On y configure l'autocomplétion sémantique via `clangd`, le debugging pas-à-pas intégré, les DevContainers pour des environnements reproductibles, et les outils d'assistance par IA disponibles en 2026.

- [2.4.1 Extensions VS Code essentielles (C/C++, CMake Tools, clangd)](/02-toolchain-ubuntu/04.1-extensions-vscode.md)
- [2.4.2 Configuration du debugging intégré](/02-toolchain-ubuntu/04.2-configuration-debugging.md)
- [2.4.3 DevContainers : environnement reproductible](/02-toolchain-ubuntu/04.3-devcontainers.md)
- [2.4.4 IA-assisted tooling : Copilot, Clangd AI et complétion intelligente (2026)](/02-toolchain-ubuntu/04.4-ia-assisted-tooling.md) ⭐

### [2.5 — Premier programme : compilation manuelle et analyse](/02-toolchain-ubuntu/05-premier-programme.md)

Le moment de vérité : on écrit un programme C++, on le compile étape par étape (`-E`, `-S`, `-c`, puis liaison) et on inspecte chaque artefact produit. C'est en observant concrètement le préprocesseur, l'assembleur généré et la table des symboles que le cycle de compilation, présenté en théorie au chapitre 1, devient réel.

- [2.5.1 Compilation étape par étape (g++ -E, -S, -c)](/02-toolchain-ubuntu/05.1-compilation-etapes.md)
- [2.5.2 Inspection des binaires (nm, objdump, ldd)](/02-toolchain-ubuntu/05.2-inspection-binaires.md)
- [2.5.3 Dépendances dynamiques et résolution](/02-toolchain-ubuntu/05.3-dependances-dynamiques.md)

### [2.6 — Options de compilation critiques](/02-toolchain-ubuntu/06-options-compilation.md)

Choisir les bons *flags* de compilation n'est pas accessoire : c'est une décision architecturale qui affecte la détection de bugs, la performance de l'exécutable et la compatibilité du code. Cette section passe en revue les quatre familles d'options que tout développeur C++ doit connaître.

- [2.6.1 Warnings : -Wall, -Wextra, -Wpedantic, -Werror](/02-toolchain-ubuntu/06.1-warnings.md)
- [2.6.2 Optimisation : -O0, -O2, -O3, -Os](/02-toolchain-ubuntu/06.2-optimisation.md)
- [2.6.3 Debug : -g, -ggdb3](/02-toolchain-ubuntu/06.3-debug.md)
- [2.6.4 Standard : -std=c++17, -std=c++20, -std=c++23, -std=c++26](/02-toolchain-ubuntu/06.4-standard.md)

### [2.7 — Introduction à std::print (C++23) : le nouveau standard d'affichage](/02-toolchain-ubuntu/07-std-print.md) ⭐

Depuis C++23, `std::print` offre une alternative moderne, type-safe et performante à `std::cout` et `printf`. Cette section en propose une prise en main rapide — syntaxe, comparaison avec les approches historiques et état du support compilateur — pour que vous puissiez l'adopter immédiatement dans vos premiers programmes.

- [2.7.1 Syntaxe et comparaison avec std::cout et printf](/02-toolchain-ubuntu/07.1-syntaxe-comparaison.md)
- [2.7.2 Formatage type-safe et performant](/02-toolchain-ubuntu/07.2-formatage-type-safe.md)
- [2.7.3 État du support compilateur (GCC 15+, Clang 19+)](/02-toolchain-ubuntu/07.3-support-compilateur.md)

> 💡 *Cette section est une prise en main rapide de `std::print` pour les débutants. La couverture approfondie du système de formatage (spécificateurs, `std::format`, formateurs personnalisés) se trouve en [section 12.7 — std::print et std::format (C++23)](/12-nouveautes-cpp17-26/07-std-print-format.md).*

---

## Prérequis

Ce chapitre suppose que vous disposez d'une installation fonctionnelle d'**Ubuntu 24.04 LTS** ou d'une version ultérieure (25.10, 26.04), que ce soit en machine physique, en machine virtuelle ou via WSL 2 sous Windows. Les commandes utilisent `apt` et `sudo` ; les adaptations pour d'autres distributions Debian-based sont mineures.

Aucune connaissance préalable du C++ n'est requise — c'est justement l'objet de cette formation — mais vous devriez être à l'aise avec un terminal Linux (navigation dans l'arborescence, édition de fichiers, variables d'environnement). Si le chapitre 1 a été suivi, vous possédez déjà les bases conceptuelles sur le cycle de compilation et le format ELF ; nous allons maintenant les mettre en pratique.

---

## Conventions utilisées dans ce chapitre

Les commandes à exécuter dans un terminal sont présentées ainsi :

```bash
sudo apt update && sudo apt install build-essential
```

Les extraits de code C++ sont annotés avec le standard minimum requis lorsque c'est pertinent :

```cpp
// C++23 minimum
#include <print>

int main() {
    std::print("Hello, {}!\n", "Ubuntu");
}
```

Les sorties de commande sont préfixées par un commentaire `# Output :` pour les distinguer des commandes elles-mêmes.

Les fichiers de configuration (CMake, JSON, YAML) sont identifiés par leur nom en en-tête de bloc de code.

---

## Temps estimé

| Section | Durée estimée |
|---|---|
| 2.1 Installation des compilateurs | 30 – 45 min |
| 2.2 Outils essentiels | 20 – 30 min |
| 2.3 ccache | 20 – 30 min |
| 2.4 Configuration IDE | 40 – 60 min |
| 2.5 Premier programme | 30 – 45 min |
| 2.6 Options de compilation | 30 – 40 min |
| 2.7 Introduction à std::print | 15 – 20 min |
| **Total chapitre 2** | **3 h – 4 h 30** |

---

## Dépendances avec les autres chapitres

| Chapitre lié | Relation |
|---|---|
| [Chapitre 1 — Introduction au C++ et à l'écosystème Linux](/01-introduction-cpp-linux/README.md) | Prérequis — fournit les bases théoriques sur le cycle de compilation et le format ELF que ce chapitre met en pratique. |
| [Chapitre 3 — Types, variables et opérateurs](/03-types-variables-operateurs/README.md) | Suite directe — utilise la toolchain mise en place ici pour écrire les premiers programmes significatifs. |
| [Chapitre 12 — Nouveautés C++17/C++20/C++23/C++26](/12-nouveautes-cpp17-26/README.md) | Approfondissement — la section 12.7 reprend `std::print` et `std::format` en détail. |
| [Chapitre 26 — CMake](/26-cmake/README.md) | Approfondissement — CMake est introduit ici comme outil, puis traité exhaustivement au chapitre 26. |
| [Chapitre 29 — Débogage avancé](/29-debogage/README.md) | Approfondissement — GDB et les sanitizers sont introduits ici, puis approfondis au chapitre 29. |

---


⏭️ [Installation des compilateurs : GCC (g++) et LLVM (clang++)](/02-toolchain-ubuntu/01-installation-compilateurs.md)
