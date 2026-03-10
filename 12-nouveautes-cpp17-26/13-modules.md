🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.13 Modules (C++20) : Concept et état en 2026

## La promesse — et la réalité — d'un nouveau modèle de compilation

Le système de headers C++ est un héritage direct du C des années 1970. Le principe est brutal : `#include` copie textuellement le contenu d'un fichier dans un autre. Quand un fichier source inclut `<vector>`, `<string>`, `<map>` et quelques headers de projet, le compilateur peut se retrouver à traiter des dizaines de milliers — parfois des centaines de milliers — de lignes de code, dont la grande majorité est recompilée identiquement dans chaque unité de traduction du projet. Ce mécanisme est la cause première de la lenteur de compilation du C++, et une source de problèmes subtils (pollution du namespace global, ordre des includes, macros qui interfèrent entre headers, ODR violations silencieuses).

Les Modules C++20 proposent une alternative fondamentale : un système où les interfaces sont compilées une seule fois dans une représentation binaire pré-compilée, puis importées par les fichiers qui en ont besoin. Plus de copie textuelle, plus de recompilation redondante, plus de pollution de macros. C'est un changement architectural du modèle de compilation du langage — le plus profond depuis la création du C++.

La promesse est considérable. La réalité, en 2026, est plus nuancée. Les modules fonctionnent, mais leur adoption reste progressive. Le support compilateur a atteint un niveau de maturité utilisable en production pour certains cas d'usage, tout en conservant des limitations significatives dans d'autres. Comprendre à la fois le concept et l'état réel de l'écosystème est essentiel pour décider quand et comment intégrer les modules dans un projet.

## Le modèle header : pourquoi il pose problème

Pour comprendre la valeur des modules, il faut d'abord mesurer le coût du système actuel.

### Inclusion textuelle et explosion combinatoire

Quand le préprocesseur rencontre `#include <vector>`, il insère littéralement le contenu du header `vector` — qui lui-même inclut d'autres headers internes. Sur une implémentation typique de libstdc++ (GCC 15), `#include <vector>` seul génère environ 30 000 lignes de code après préprocessing. Ajoutons quelques headers courants :

```cpp
#include <vector>       // ~30 000 lignes
#include <string>       // ~40 000 lignes (cumul ~50 000 — chevauchement)
#include <map>          // ~55 000 lignes cumulées
#include <algorithm>    // ~65 000 lignes cumulées
#include <iostream>     // ~75 000 lignes cumulées
```

Un fichier source de 100 lignes utiles peut se retrouver à demander au compilateur de traiter 75 000 lignes de code. Et ce traitement est répété pour chaque fichier `.cpp` du projet. Un projet de 200 fichiers source compilera potentiellement 200 × 75 000 = 15 millions de lignes, dont l'immense majorité est identique d'un fichier à l'autre.

### Macros et effets de bord

Les macros définies dans un header affectent tous les headers inclus après. L'ordre des `#include` devient significatif, et des macros apparemment inoffensives peuvent avoir des conséquences dévastatrices :

```cpp
#define max(a, b) ((a) > (b) ? (a) : (b))   // Macro C classique
#include <algorithm>  // ERREUR : std::max est détruit par la macro max
```

Ce type de conflit est la raison pour laquelle Windows exige souvent `#define NOMINMAX` avant d'inclure `<windows.h>`. Les modules éliminent ce problème : les macros définies dans un module n'affectent pas les importateurs.

### ODR violations silencieuses

La règle de définition unique (*One Definition Rule*, ODR) exige qu'une entité ait exactement la même définition dans toutes les unités de traduction. Avec les headers, cette règle est facilement violée de manière silencieuse — par exemple quand un header se comporte différemment selon les macros actives :

```cpp
// config.h — dans certains fichiers
#define USE_FAST_MATH 1
#include "math_utils.h"   // Définition A de MathHelper

// config.h — dans d'autres fichiers
// USE_FAST_MATH non défini
#include "math_utils.h"   // Définition B de MathHelper (différente !)

// → ODR violation : deux définitions différentes de MathHelper
// → Comportement indéfini silencieux
```

Les modules rendent ce type de violation impossible, car l'interface est compilée une seule fois dans un contexte unique.

## Les modules : le nouveau modèle

### Syntaxe de base

Un module se compose de deux parties : la **déclaration de module** (ce qui est exporté) et l'**importation** (ce qui est consommé).

**Fichier module (interface) — `math_utils.cppm` :**

```cpp
export module math_utils;    // Déclaration du module

import <cmath>;              // Import d'un header standard (header unit)

export namespace math {
    double square(double x) {
        return x * x;
    }

    double distance(double x1, double y1, double x2, double y2) {
        return std::sqrt(square(x2 - x1) + square(y2 - y1));
    }

    constexpr double pi = 3.14159265358979323846;
}
```

**Fichier consommateur — `main.cpp` :**

```cpp
import math_utils;           // Import du module — pas de #include
#include <print>

int main() {
    std::print("π = {}\n", math::pi);
    std::print("distance = {:.2f}\n", math::distance(0, 0, 3, 4));
}
```

Le mot-clé `export` marque les entités visibles de l'extérieur. Tout ce qui n'est pas exporté reste interne au module — invisible pour les importateurs, sans besoin de convention `detail::` ou de namespace anonyme.

### Vocabulaire des modules

**Module interface unit** — Le fichier qui déclare `export module nom;`. C'est la « façade » du module, analogue au header `.h`. Extension conventionnelle : `.cppm`, `.ixx` (MSVC), `.mpp`.

**Module implementation unit** — Un fichier qui déclare `module nom;` (sans `export`). Il peut accéder aux entités internes du module mais n'exporte rien. Analogue au fichier `.cpp` dans le modèle header/source.

```cpp
// math_utils_impl.cpp — implémentation séparée
module math_utils;           // Implémentation (pas export)

// Peut définir des fonctions exportées dans l'interface
// ou des fonctions internes au module
```

**Module partition** — Un sous-module qui permet de découper un gros module en parties logiques :

```cpp
// math_utils-geometry.cppm
export module math_utils:geometry;   // Partition 'geometry' du module math_utils

export namespace math {
    double distance(double x1, double y1, double x2, double y2);
}
```

```cpp
// math_utils.cppm — interface principale
export module math_utils;  
export import :geometry;     // Ré-exporter la partition geometry  
```

**Header unit** — Un header classique importé avec la syntaxe `import` plutôt que `#include`. Le compilateur le traite comme un module implicite :

```cpp
import <vector>;             // Header unit — traité comme un module  
import <string>;  
import "my_header.h";       // Header unit pour un header utilisateur  
```

Les header units offrent une migration incrémentale : on peut remplacer les `#include` par des `import` sans réécrire les headers eux-mêmes.

### Ce que le mot-clé export contrôle

`export` offre un contrôle fin sur la visibilité :

```cpp
export module my_lib;

// Exporté — visible par les importateurs
export class PublicApi {  
public:  
    void do_something();
};

export void public_function();

export namespace public_ns {
    int visible_value = 42;
}

// NON exporté — invisible de l'extérieur
class InternalHelper {       // Pas de export → privé au module
    // ...
};

void internal_function() {   // Pas de export → privé au module
    // ...
}

namespace detail {           // Pas de export, mais accessible en interne
    void helper() {}
}
```

C'est une amélioration majeure par rapport aux headers, où le concept de « privé au header » n'existe pas — tout ce qui est dans un header est visible par quiconque l'inclut. Avec les modules, l'encapsulation s'applique au niveau du module, pas seulement au niveau de la classe.

### Isolation des macros

Les macros ne traversent pas les frontières de module :

```cpp
// module_a.cppm
export module module_a;
#define BUFFER_SIZE 1024      // Cette macro reste locale à module_a
export int get_buffer_size() { return BUFFER_SIZE; }

// main.cpp
import module_a;
// BUFFER_SIZE n'est PAS défini ici — la macro ne fuit pas
int size = get_buffer_size();  // OK — via la fonction exportée
```

C'est une rupture fondamentale avec le modèle `#include`, où toute macro définie dans un header se propage à tous les fichiers qui l'incluent.

## Impact sur les temps de compilation

L'avantage le plus tangible des modules est l'accélération de la compilation. Le principe :

**Avec headers :** chaque unité de traduction qui inclut `<vector>` reparse et recompile les ~30 000 lignes du header vector. Pour 200 fichiers source, c'est 200 compilations redondantes.

**Avec modules :** le module `vector` est compilé une seule fois dans une représentation binaire intermédiaire (BMI — *Binary Module Interface*). Les 200 fichiers source importent cette représentation pré-compilée, ce qui est une opération dramatiquement plus rapide que le parsing textuel.

Les gains mesurés varient selon les projets, mais les chiffres couramment rapportés vont de **2× à 5× d'accélération** sur les temps de compilation pour les projets qui utilisent intensivement les headers de la bibliothèque standard. Les gains sont encore plus importants quand les headers de projet eux-mêmes sont convertis en modules.

Ces gains ne sont pas sans rappeler ceux des *precompiled headers* (PCH), mais les modules offrent une granularité plus fine (on importe uniquement ce dont on a besoin), une sémantique propre (pas de pollution de macros), et une intégration au langage (pas un hack de compilation).

## Interaction avec le build system

C'est le point de friction principal de l'adoption des modules. Contrairement aux headers, les modules introduisent des **dépendances de compilation entre fichiers source**. Si `main.cpp` importe `math_utils`, le compilateur doit d'abord compiler `math_utils.cppm` pour produire le BMI, avant de pouvoir compiler `main.cpp`. Le build system doit connaître ces dépendances pour ordonnancer les compilations correctement.

Avec les headers, chaque `.cpp` peut être compilé indépendamment (en parallèle). Avec les modules, un graphe de dépendances de compilation apparaît, qui contraint l'ordonnancement. C'est un changement fondamental pour les build systems.

**CMake** (version 3.28+) supporte les modules C++20 de manière expérimentale, avec un support de plus en plus stable dans les versions récentes. La détection automatique des dépendances inter-modules utilise le *dependency scanning* standardisé par le format P1689 :

```cmake
cmake_minimum_required(VERSION 3.28)  
project(my_project CXX)  

set(CMAKE_CXX_STANDARD 20)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  

add_executable(app)  
target_sources(app  
    PUBLIC FILE_SET CXX_MODULES FILES
        math_utils.cppm
    PRIVATE
        main.cpp
)
```

La directive `FILE_SET CXX_MODULES` indique à CMake que ces fichiers sont des interfaces de module. CMake se charge de l'ordonnancement et de la génération du BMI.

> 📎 *La section 12.13.2 détaille l'état du support dans CMake 3.31+, Ninja, et les autres build systems. La section 12.13.3 donne un bilan de ce qui fonctionne en production en 2026.*

## Structure de cette section

Les trois sous-sections suivantes approfondissent chaque aspect :

La section **12.13.1** détaille les avantages techniques des modules sur les headers — isolation, performance de compilation, encapsulation, élimination des problèmes d'ODR.

La section **12.13.2** fait le point sur le support réel dans GCC 15, Clang 20 et MSVC — ce qui compile, ce qui ne compile pas encore, et les workarounds courants.

La section **12.13.3** synthétise la maturité en 2026 avec un verdict pratique : dans quels types de projets les modules sont-ils recommandés aujourd'hui, et quand est-il préférable d'attendre ?

---


⏭️ [Avantages des modules sur les headers](/12-nouveautes-cpp17-26/13.1-avantages-modules.md)
