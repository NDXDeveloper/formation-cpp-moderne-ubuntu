🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 2.2 — Les outils essentiels : build-essential, gdb, make, ninja-build, cmake

> **Chapitre 2 · Mise en place de la Toolchain sur Ubuntu** · Niveau Débutant

---

## Introduction

Un compilateur seul ne suffit pas pour travailler efficacement sur un projet C++. Dès que votre code dépasse un ou deux fichiers sources, vous avez besoin d'un **build system** pour orchestrer la compilation, d'un **debugger** pour traquer les bugs, et d'un **générateur de projets** pour rendre le tout portable et maintenable.

Cette section présente les cinq briques fondamentales qu'un développeur C++ sur Ubuntu installe dès le premier jour : le méta-paquet `build-essential`, le debugger **GDB**, les build systems **Make** et **Ninja**, et le générateur de projets **CMake**. L'objectif est de les installer, de comprendre leur rôle respectif et de vérifier que tout fonctionne — nous approfondirons leur utilisation dans les chapitres ultérieurs.

---

## Vue d'ensemble : qui fait quoi ?

Avant de plonger dans les détails de chaque outil, il est utile de comprendre comment ils s'articulent dans un workflow typique :

```
Vous (développeur)
    │
    ▼
 CMakeLists.txt          ← Vous décrivez votre projet ici
    │
    ▼
  CMake                  ← Générateur : lit CMakeLists.txt,
    │                       produit des fichiers de build
    ▼
 build.ninja             ← Fichiers de build (ou Makefiles)
 (ou Makefile)
    │
    ▼
 Ninja (ou Make)         ← Build system : orchestre les appels
    │                       au compilateur
    ▼
 g++ / clang++           ← Compilateur : transforme les .cpp
    │                       en fichiers objets, puis en binaire
    ▼
 programme               ← Votre exécutable
    │
    ▼
  GDB                    ← Debugger : exécute le programme
                            pas-à-pas pour traquer les bugs
```

En résumé : **CMake génère**, **Ninja (ou Make) orchestre**, **g++/clang++ compile**, **GDB débugue**. Chaque outil a un rôle bien défini, et c'est cette séparation des responsabilités qui rend la chaîne robuste et flexible.

---

## build-essential : le socle minimal

### Ce qu'il contient

`build-essential` est un **méta-paquet** Debian/Ubuntu : il ne contient pas de logiciel en soi, mais déclare des dépendances vers un ensemble cohérent d'outils de compilation. En l'installant, vous obtenez d'un coup :

- **gcc** et **g++** — les compilateurs C et C++ de GNU (la version par défaut de votre release Ubuntu) ;
- **libc6-dev** (ou **libc-dev**) — les en-têtes et bibliothèques de développement de la bibliothèque C standard GNU (glibc) ;
- **make** — le build system GNU Make ;
- **dpkg-dev** — les outils nécessaires pour construire des paquets Debian.

C'est le strict minimum pour compiler un programme C ou C++ sur Ubuntu.

### Installation

```bash
sudo apt update  
sudo apt install build-essential  
```

### Vérification

```bash
gcc --version | head -1  
g++ --version | head -1  
make --version | head -1  
```

Les trois commandes doivent afficher un numéro de version. Si c'est le cas, le socle est en place.

### Pourquoi c'est la première chose à installer

Quasiment tous les autres outils de développement C++ supposent que `build-essential` est présent. CMake s'attend à trouver un compilateur C/C++ dans le `PATH`. Les bibliothèques que vous installerez via `apt` (par exemple `libssl-dev`, `libfmt-dev`) nécessitent les en-têtes de développement C pour fonctionner. Installer `build-essential` en premier évite un grand nombre d'erreurs de configuration par la suite.

---

## GDB : le debugger GNU

### Rôle

GDB (GNU Debugger) est le debugger de référence sur Linux. Il permet d'exécuter un programme pas-à-pas, d'inspecter la valeur des variables à chaque instant, de placer des points d'arrêt (breakpoints), d'examiner la pile d'appels (call stack) et de diagnostiquer les crashes via les core dumps. C'est un outil en ligne de commande, mais il est également le moteur de débogage derrière les interfaces graphiques de VS Code, CLion et d'autres IDE.

### Installation

GDB n'est pas inclus dans `build-essential`. Installez-le séparément :

```bash
sudo apt install gdb
```

Vérification :

```bash
gdb --version | head -1
# Output (exemple) :
# GNU gdb (Ubuntu 15.1-0ubuntu1) 15.1
```

### Premier contact

Pour utiliser GDB, il faut que votre programme ait été compilé avec les **symboles de débogage** (flag `-g`) :

```bash
g++ -std=c++23 -g -o mon_programme mon_programme.cpp
```

Lancement de GDB :

```bash
gdb ./mon_programme
```

Quelques commandes essentielles pour un premier contact :

```
(gdb) break main          # Poser un breakpoint sur la fonction main
(gdb) run                 # Lancer l'exécution
(gdb) next                # Exécuter la ligne suivante (sans entrer dans les fonctions)
(gdb) step                # Exécuter la ligne suivante (en entrant dans les fonctions)
(gdb) print variable      # Afficher la valeur d'une variable
(gdb) backtrace           # Afficher la pile d'appels
(gdb) continue            # Reprendre l'exécution jusqu'au prochain breakpoint
(gdb) quit                # Quitter GDB
```

> 💡 En pratique, vous utiliserez GDB le plus souvent **à travers votre IDE** (VS Code, CLion, Neovim avec nvim-dap) qui fournit une interface graphique par-dessus ces mêmes commandes. Mais connaître les commandes de base est indispensable pour le débogage en SSH, dans les conteneurs Docker et dans les pipelines CI.

### L'alternative : LLDB

LLDB est le debugger du projet LLVM, équivalent de GDB. Il est installable via `sudo apt install lldb-20` (ou la version correspondante à votre installation LLVM). Son interface en ligne de commande est similaire à celle de GDB, avec quelques différences syntaxiques. Sur Linux, GDB reste le standard le plus répandu ; nous l'utiliserons tout au long de cette formation. Le chapitre 29 (Débogage avancé) couvre GDB en profondeur.

---

## Make : le build system historique

### Rôle

GNU Make est un **build system** : il lit un fichier de recettes (le `Makefile`) qui décrit les dépendances entre fichiers sources et objets, et n'exécute que les commandes nécessaires pour mettre à jour ce qui a changé. C'est le build system le plus ancien et le plus universel du monde Unix/Linux.

### Installation

Make est inclus dans `build-essential`. Si vous avez besoin de le réinstaller isolément :

```bash
sudo apt install make
```

### Fonctionnement en bref

Un `Makefile` minimal pour un projet C++ :

```makefile
CXX      = g++  
CXXFLAGS = -std=c++23 -Wall -Wextra -O2  

programme: main.o utils.o
	$(CXX) $(CXXFLAGS) -o programme main.o utils.o

main.o: main.cpp utils.h
	$(CXX) $(CXXFLAGS) -c main.cpp

utils.o: utils.cpp utils.h
	$(CXX) $(CXXFLAGS) -c utils.cpp

clean:
	rm -f *.o programme
```

Exécution :

```bash
make            # Compile ce qui a changé  
make clean      # Supprime les artefacts de compilation  
make -j$(nproc) # Compile en parallèle (autant de jobs que de cœurs CPU)  
```

### Forces et limites

Make est simple, universel et ne nécessite aucune dépendance supplémentaire. Il est présent sur quasiment toutes les machines Unix/Linux. Pour un petit projet de quelques fichiers, un Makefile écrit à la main est parfaitement raisonnable.

Ses limites apparaissent sur les projets plus gros : la syntaxe est parfois piégeuse (tabulations obligatoires, variables implicites peu lisibles), la gestion des dépendances entre en-têtes est manuelle, et les performances en builds parallèles sont en retrait par rapport aux alternatives modernes. C'est pourquoi, dans la pratique professionnelle, Make est presque toujours **généré par CMake** plutôt qu'écrit à la main.

Le chapitre 28 couvre Makefiles en détail pour les cas où vous en aurez besoin.

---

## Ninja : le build system ultra-rapide

### Rôle

Ninja est un build system conçu pour une seule chose : **être rapide**. Créé par Evan Martin chez Google en 2012 pour accélérer la compilation de Chrome, Ninja est volontairement minimaliste : il ne fournit pas de langage de haut niveau pour écrire des règles. Ses fichiers `build.ninja` sont destinés à être **générés par un outil** (typiquement CMake ou Meson), pas écrits à la main.

### Pourquoi Ninja est plus rapide que Make

La différence de performance entre Ninja et Make vient de choix architecturaux fondamentaux :

**Phase de parsing.** Quand vous lancez un build, le build system doit d'abord lire et interpréter ses fichiers de configuration pour construire le graphe de dépendances. Make utilise un langage expressif mais complexe (variables, fonctions, conditionnelles, inclusions récursives) qui rend cette phase coûteuse sur les gros projets. Ninja utilise un format binaire minimal et linéaire, sans conditionnelle ni fonction : le parsing est quasi instantané, même sur des projets de plusieurs milliers de fichiers.

**Gestion du parallélisme.** Ninja a été conçu dès le départ pour maximiser la saturation des cœurs CPU. Son scheduler de jobs est plus efficace que celui de Make pour maintenir tous les cœurs occupés, en minimisant les temps d'attente entre les tâches.

**Détection des fichiers à recompiler.** Ninja utilise un fichier journal (`.ninja_log`) pour comparer les timestamps et les commandes de compilation. Si la commande n'a pas changé et que les fichiers sources n'ont pas été modifiés, la reconstruction est sautée. Cette approche est plus robuste que le mécanisme de timestamps pur de Make.

Sur un projet comme Chromium (des dizaines de milliers de fichiers sources), la différence entre Make et Ninja peut atteindre un facteur 10× sur la phase de « no-op build » (quand rien n'a changé et qu'il faut simplement vérifier que tout est à jour). Sur des projets plus modestes, le gain est de l'ordre de 2× à 5× sur les rebuilds incrémentaux.

### Installation

```bash
sudo apt install ninja-build
```

Vérification :

```bash
ninja --version
# Output (exemple) :
# 1.12.1
```

### Utilisation avec CMake

Ninja ne s'utilise quasiment jamais seul. On demande à CMake de générer des fichiers `build.ninja` au lieu de `Makefile` en spécifiant le générateur :

```bash
cmake -B build -G Ninja  
cmake --build build  
```

C'est le workflow recommandé dans cette formation et dans la plupart des projets professionnels. Nous reviendrons en détail sur l'intégration CMake + Ninja à la section 26.5.

### Ninja ou Make ?

En 2026, **Ninja est le générateur recommandé** pour CMake. L'écosystème s'est largement standardisé autour de ce choix :

- Les CMake Presets officiels de nombreux projets open source spécifient Ninja comme générateur par défaut.
- Les CI/CD professionnels utilisent Ninja pour réduire les temps de build.
- Les IDE comme VS Code (via CMake Tools) et CLion détectent automatiquement Ninja s'il est installé et le privilégient.

Make reste pertinent pour les petits projets sans CMake, les scripts d'automatisation (un Makefile de « raccourcis » qui encapsule des commandes CMake, Docker, etc.) et la maintenance de code legacy.

---

## CMake : le générateur de projets

### Rôle

CMake n'est ni un compilateur ni un build system : c'est un **générateur de systèmes de build**. Vous décrivez votre projet dans un fichier `CMakeLists.txt` — les exécutables, les bibliothèques, les dépendances, les options de compilation — et CMake produit les fichiers natifs du build system de votre choix (Ninja, Make, ou d'autres). C'est une couche d'abstraction qui rend vos projets portables entre systèmes d'exploitation, compilateurs et build systems.

CMake est devenu le **standard de facto** de l'industrie C++ : la grande majorité des projets open source C++ (Qt, LLVM, OpenCV, Boost, gRPC, etc.) l'utilisent, et il est supporté nativement par tous les IDE majeurs.

### Installation

Ubuntu fournit CMake via ses dépôts, mais la version empaquetée peut être ancienne. Pour obtenir une version récente :

```bash
# Version des dépôts Ubuntu (peut être ancienne)
sudo apt install cmake

# Vérification
cmake --version
```

Si la version est antérieure à 3.25, envisagez d'installer une version plus récente via le PPA Kitware ou via pip :

```bash
# Méthode 1 : PPA Kitware (recommandé pour Ubuntu)
sudo apt install gpg wget  
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \  
    | gpg --dearmor - \
    | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null

echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] \  
https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" \  
    | sudo tee /etc/apt/sources.list.d/kitware.list

sudo apt update  
sudo apt install cmake  

# Méthode 2 : via pip (alternative rapide)
pip install --user cmake
```

Vérification :

```bash
cmake --version
# Output (exemple) :
# cmake version 3.31.4
```

> 💡 **Quelle version viser ?** En mars 2026, CMake 3.31+ est la version courante. Pour cette formation, toute version ≥ 3.25 convient. Certaines fonctionnalités modernes (CMake Presets avancés, améliorations du support des modules C++20) nécessitent des versions plus récentes — nous le signalerons le cas échéant.

### Workflow de base

Un projet CMake minimal se compose d'un fichier `CMakeLists.txt` à la racine du projet :

```cmake
cmake_minimum_required(VERSION 3.25)  
project(MonProjet LANGUAGES CXX)  

set(CMAKE_CXX_STANDARD 23)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  

add_executable(mon_programme main.cpp)
```

Le workflow de compilation se décompose en deux étapes distinctes :

```bash
# Étape 1 : Configuration (génération des fichiers de build)
cmake -B build -G Ninja

# Étape 2 : Compilation
cmake --build build
```

L'étape de configuration lit le `CMakeLists.txt`, détecte le compilateur, vérifie les dépendances et produit les fichiers de build dans le répertoire `build/`. L'étape de compilation appelle Ninja (ou Make) qui orchestre les appels au compilateur.

Cette séparation en deux étapes est fondamentale : la configuration n'est exécutée qu'une fois (ou quand le `CMakeLists.txt` change), tandis que la compilation est exécutée à chaque modification du code source.

### Pourquoi un répertoire de build séparé ?

L'option `-B build` indique à CMake de placer tous les fichiers générés dans un répertoire `build/` séparé des sources. C'est ce qu'on appelle un **out-of-source build** (build hors source). Cette pratique est essentielle pour plusieurs raisons :

- Les fichiers générés (objets, exécutables, caches) ne polluent pas l'arborescence des sources.
- Un simple `rm -rf build/` suffit pour repartir d'un état propre.
- Vous pouvez maintenir plusieurs répertoires de build simultanément (un pour GCC, un pour Clang, un en Debug, un en Release).
- Le `.gitignore` n'a besoin que d'une seule ligne : `build/`.

```bash
# Exemple : deux builds en parallèle
cmake -B build-gcc   -G Ninja -DCMAKE_CXX_COMPILER=g++-15  
cmake -B build-clang -G Ninja -DCMAKE_CXX_COMPILER=clang++-20  

cmake --build build-gcc  
cmake --build build-clang  
```

### Exécuter le programme compilé

Après la compilation, l'exécutable se trouve dans le répertoire de build :

```bash
./build/mon_programme
```

### Les commandes CMake à connaître

| Commande | Rôle |
|---|---|
| `cmake -B build -G Ninja` | Configurer le projet (générer les fichiers Ninja) |
| `cmake --build build` | Compiler le projet |
| `cmake --build build --target clean` | Nettoyer les artefacts de compilation |
| `cmake --build build -j $(nproc)` | Compiler en parallèle (Ninja le fait automatiquement) |
| `cmake --build build --verbose` | Compiler en affichant les commandes exécutées |
| `cmake --install build --prefix /usr/local` | Installer les fichiers compilés |

Le chapitre 26 couvre CMake en profondeur : targets, bibliothèques, gestion des dépendances, presets, toolchains de cross-compilation.

---

## Installation complète en une commande

Pour installer l'ensemble de la toolchain de base en une seule commande :

```bash
sudo apt update && sudo apt install -y \
    build-essential \
    gdb \
    ninja-build \
    cmake
```

Si vous avez suivi la section 2.1 pour installer des versions spécifiques de GCC et Clang, ajoutez-les à cette commande. Le résultat est une toolchain complète et prête à l'emploi.

---

## Vérification de l'installation

Un script de vérification rapide pour confirmer que tout est en place :

```bash
echo "=== Toolchain Ubuntu — Vérification ==="  
echo ""  

for tool in gcc g++ make ninja gdb cmake; do
    printf "%-10s : " "$tool"
    if command -v $tool &>/dev/null; then
        $tool --version 2>&1 | head -1
    else
        echo "NON INSTALLÉ ❌"
    fi
done

echo ""  
echo "=== Test de build CMake + Ninja ==="  

# Créer un projet temporaire
TMPDIR=$(mktemp -d)  
cat > "$TMPDIR/CMakeLists.txt" << 'EOF'  
cmake_minimum_required(VERSION 3.20)  
project(ToolchainTest LANGUAGES CXX)  
set(CMAKE_CXX_STANDARD 23)  
add_executable(test_build main.cpp)  
EOF  

cat > "$TMPDIR/main.cpp" << 'EOF'
#include <iostream>
int main() {
    std::cout << "Toolchain OK — CMake + Ninja + g++\n";
}
EOF

# Configurer et compiler
if cmake -S "$TMPDIR" -B "$TMPDIR/build" -G Ninja &>/dev/null && \
   cmake --build "$TMPDIR/build" &>/dev/null; then
    "$TMPDIR/build/test_build"
    echo "Build CMake + Ninja : OK ✅"
else
    echo "Build CMake + Ninja : ÉCHEC ❌"
fi

rm -rf "$TMPDIR"
```

Si tout est correctement installé, le script doit afficher les versions de chaque outil et terminer par `Toolchain OK`.

---

## Résumé : rôle de chaque outil

| Outil | Type | Rôle | Installé par |
|---|---|---|---|
| **gcc / g++** | Compilateur | Transforme les `.cpp` en binaire | `build-essential` |
| **make** | Build system | Orchestre la compilation selon un Makefile | `build-essential` |
| **gdb** | Debugger | Exécution pas-à-pas, breakpoints, inspection | `apt install gdb` |
| **ninja** | Build system | Alternative rapide à Make, conçu pour être généré | `apt install ninja-build` |
| **cmake** | Générateur | Décrit le projet, génère des fichiers Ninja/Make | `apt install cmake` |

---

## Ce qu'il faut retenir

- **`build-essential`** est le premier paquet à installer : il fournit gcc, g++, make et les en-têtes de développement C. C'est le socle sur lequel tout le reste repose.
- **GDB** est le debugger standard de Linux. Compilez avec `-g` pour inclure les symboles de débogage, puis lancez `gdb ./programme` pour une session interactive. Les IDE utilisent GDB en arrière-plan.
- **Make** est le build system historique : universel mais lent sur les gros projets. En pratique, il est presque toujours généré par CMake.
- **Ninja** est l'alternative rapide à Make : conçu pour être généré (par CMake ou Meson), il excelle en builds incrémentaux et parallèles. C'est le générateur recommandé en 2026.
- **CMake** est le générateur de projets standard de l'industrie C++. Il décrit votre projet dans un `CMakeLists.txt` et produit les fichiers Ninja ou Make. Le workflow `cmake -B build -G Ninja && cmake --build build` est celui que vous utiliserez quotidiennement.

---


⏭️ [Accélération de compilation : ccache (Compiler Cache)](/02-toolchain-ubuntu/03-ccache.md)
