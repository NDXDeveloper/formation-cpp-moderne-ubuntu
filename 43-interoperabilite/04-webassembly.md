🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 43.4 — WebAssembly : Compiler du C++ pour le web (Emscripten)

## Module 15 : Interopérabilité · Niveau Expert

---

## Introduction

Les sections précédentes ont couvert l'interopérabilité de C++ avec d'autres langages natifs — C, Python, Rust. Cette section explore une direction fondamentalement différente : **compiler du C++ pour qu'il s'exécute dans un navigateur web**, à des performances proches du natif, sans plugin, sans installation, accessible à tout utilisateur disposant d'un navigateur moderne.

WebAssembly (Wasm) est le format binaire qui rend cela possible. Conçu comme un standard W3C, supporté par tous les navigateurs majeurs depuis 2017, Wasm est un format d'instructions portable pour une machine virtuelle à pile. Il n'est pas un langage de programmation — c'est une cible de compilation. On écrit en C++, on compile vers Wasm, et le navigateur exécute le résultat à une vitesse que JavaScript ne peut pas atteindre.

Emscripten est l'outil de référence pour cette compilation. Basé sur LLVM et Clang, il transforme du code C/C++ en modules `.wasm` accompagnés du code JavaScript de glue nécessaire pour les charger et les exécuter dans l'environnement du navigateur.

---

## Pourquoi un développeur C++ devrait s'intéresser à WebAssembly

### Le navigateur comme plateforme d'exécution universelle

Le navigateur est la plateforme la plus ubiquiste qui existe. Chaque ordinateur, tablette et smartphone en possède un. Aucune installation n'est requise. Les mises à jour sont instantanées. La distribution est aussi simple qu'une URL. Pour un développeur C++ qui a passé sa carrière à gérer des compilations croisées, des dépendances dynamiques, des paquets DEB/RPM et des conteneurs Docker, cette promesse est séduisante.

WebAssembly transforme cette promesse en réalité pour le code natif. Un moteur de calcul scientifique, un codec vidéo, une bibliothèque de traitement d'image, un émulateur, un moteur 3D — tout cela peut tourner dans le navigateur, avec des performances typiquement situées entre 70 % et 95 % du natif selon la charge de travail.

### Les cas d'usage établis

WebAssembly n'est pas un gadget expérimental. En 2026, des produits majeurs s'appuient dessus en production :

**Outils de création.** Figma utilise Wasm pour son moteur de rendu, offrant une expérience comparable à une application de bureau directement dans le navigateur. AutoCAD Web, Photoshop Web et d'autres outils de création lourds exploitent Wasm pour le calcul intensif côté client.

**Moteurs de jeu.** Unity et Unreal Engine supportent la compilation vers WebAssembly. Des jeux complets, avec rendu 3D temps réel, physique et audio, tournent dans le navigateur sans plugin — un contraste radical avec l'ère Flash/Java Applets.

**Traitement multimédia.** FFmpeg compilé en Wasm permet le transcodage, le découpage et l'application d'effets vidéo directement dans le navigateur, sans envoyer les données vers un serveur. Les bibliothèques de traitement d'image (OpenCV, libvips) et de traitement audio suivent le même chemin.

**Calcul scientifique et simulation.** Des bibliothèques de calcul numérique (BLAS, LAPACK), des simulateurs physiques et des outils de visualisation scientifique sont portés vers Wasm pour offrir un accès instantané via le web, sans installation.

**Bases de données embarquées.** SQLite compilé en Wasm tourne dans le navigateur, offrant une base de données SQL complète côté client. DuckDB suit la même voie pour l'analytique.

**Portage de logiciels historiques.** Des applications desktop C++ existantes — certaines avec des décennies de développement — sont portées vers le web via Emscripten, prolongeant leur durée de vie sans réécriture en JavaScript.

### Ce que WebAssembly n'est pas

Quelques clarifications pour le développeur système :

**Wasm n'est pas un remplacement de JavaScript.** JavaScript reste le langage natif du web, avec un accès direct au DOM, aux événements, aux API du navigateur. Wasm est un complément pour le code intensif en calcul. Les deux coopèrent : JavaScript orchestre l'interface utilisateur, Wasm exécute le calcul lourd.

**Wasm n'a pas d'accès direct au DOM.** Un module Wasm ne peut pas manipuler directement les éléments HTML. Il communique avec JavaScript, qui agit comme intermédiaire vers le DOM et les API du navigateur.

**Wasm tourne dans une sandbox.** Comme JavaScript, le code Wasm s'exécute dans un environnement isolé sans accès direct au système de fichiers, au réseau ni au matériel. L'accès aux ressources passe par les API du navigateur, exposées via JavaScript. C'est un modèle de sécurité très différent de l'exécution native — et c'est intentionnel.

**Les performances ne sont pas identiques au natif.** Wasm est rapide — bien plus rapide que JavaScript pour le calcul intensif — mais il reste dans une sandbox avec des contraintes (pas de SIMD complet partout, pas d'accès direct au GPU hors WebGL/WebGPU, overhead de la couche d'interopérabilité JS). Pour la majorité des cas d'usage, la différence est négligeable. Pour le code le plus critique en latence, elle peut être mesurable.

---

## Emscripten : vue d'ensemble

### Qu'est-ce qu'Emscripten

Emscripten est un compilateur C/C++ vers WebAssembly, construit sur LLVM et Clang. Il ne se limite pas à la compilation : il fournit un environnement d'exécution complet qui émule les API POSIX, OpenGL (via WebGL), SDL2, les threads POSIX (via Web Workers), et un système de fichiers virtuel. L'objectif est de permettre le portage d'applications C/C++ existantes avec un minimum de modifications.

Le pipeline de compilation est le suivant :

```
Code C++ (.cpp)
      │
      ▼
  Clang (frontend LLVM)
      │
      ▼
  LLVM IR (représentation intermédiaire)
      │
      ▼
  LLVM Wasm backend
      │
      ▼
  Module .wasm (code binaire WebAssembly)
      +
  Fichier .js (code de glue JavaScript)
      +
  Fichier .html (optionnel, page de test)
      +
  Fichier .data (optionnel, système de fichiers pré-chargé)
```

Le fichier `.wasm` contient le code compilé. Le fichier `.js` contient le code de glue qui charge le module Wasm, configure les imports (syscalls, mémoire, etc.), et fournit l'interface entre Wasm et les API du navigateur. Les deux fichiers sont conçus pour fonctionner ensemble — le `.wasm` seul n'est pas autonome, car il dépend des imports configurés par le `.js`.

### Le compilateur emcc

La commande `emcc` (Emscripten Compiler Frontend) remplace `g++` ou `clang++` dans la chaîne de compilation. Elle accepte les mêmes flags et produit du Wasm au lieu d'un exécutable natif :

```bash
# Compilation native (rappel)
g++ -std=c++20 -O2 -o myapp main.cpp

# Compilation Wasm (même code source, même flags)
emcc -std=c++20 -O2 -o myapp.html main.cpp
```

Le suffixe de sortie détermine ce qu'Emscripten génère :

| Suffixe | Fichiers produits | Usage |
|---|---|---|
| `.html` | `.html` + `.js` + `.wasm` | Page de test autonome |
| `.js` | `.js` + `.wasm` | Intégration dans une page existante |
| `.wasm` | `.wasm` seul | Module standalone (cas avancé) |

### Ce qu'Emscripten émule

La force d'Emscripten réside dans la couche d'émulation qu'il fournit. Un programme C++ qui utilise les API suivantes peut souvent être compilé vers Wasm avec peu ou pas de modifications :

| API native | Émulation Emscripten | Implémentation |
|---|---|---|
| `stdio.h` (`printf`, `scanf`) | Console du navigateur | Code de glue JS |
| `stdlib.h` (`malloc`, `free`) | Mémoire linéaire Wasm | dlmalloc / emmalloc |
| Système de fichiers POSIX | Système de fichiers virtuel (MEMFS, IDBFS) | JavaScript |
| `pthread` (threads POSIX) | Web Workers + SharedArrayBuffer | JS + Wasm |
| OpenGL ES 2.0/3.0 | WebGL 1.0/2.0 | Mapping direct |
| SDL2 | SDL2 porté pour le web | Emscripten port |
| Sockets réseau | WebSockets | Émulation partielle |
| `std::chrono`, `clock()` | `performance.now()` | Code de glue JS |

Cette émulation n'est pas parfaite — certaines API POSIX n'ont pas d'équivalent web (signaux, fork, accès direct aux fichiers du système hôte), et les threads POSIX nécessitent des headers spécifiques du navigateur (SharedArrayBuffer, qui requiert des headers COOP/COEP). Mais pour la majorité des bibliothèques de calcul et de traitement de données, la compatibilité est remarquable.

---

## Emscripten et l'interopérabilité JavaScript

### Le lien avec le reste du chapitre

Emscripten est, fondamentalement, un outil d'interopérabilité — comme pybind11 (section 43.2) et `cxx` (section 43.3.2). La différence est le langage cible : JavaScript au lieu de Python ou Rust. Et le mécanisme sous-jacent est, encore une fois, `extern "C"`.

Les fonctions C++ exposées à JavaScript sont déclarées avec `extern "C"` et annotées `EMSCRIPTEN_KEEPALIVE` pour empêcher le compilateur de les éliminer lors de l'optimisation :

```cpp
#include <emscripten.h>

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    int compute(int a, int b) {
        return a + b;
    }
}
```

Côté JavaScript, la fonction est appelable via l'API Module :

```javascript
const result = Module._compute(3, 4);  // 7
```

Pour une interface plus riche, Emscripten fournit **Embind** — un système de binding qui joue le même rôle que pybind11 pour Python : exposer des classes, des méthodes, des propriétés et des types complexes C++ à JavaScript de manière idiomatique.

```cpp
#include <emscripten/bind.h>
#include <string>

class Calculator {  
public:  
    Calculator(double initial) : value_(initial) {}
    void add(double x) { value_ += x; }
    double result() const { return value_; }
private:
    double value_;
};

EMSCRIPTEN_BINDINGS(calculator) {
    emscripten::class_<Calculator>("Calculator")
        .constructor<double>()
        .function("add", &Calculator::add)
        .function("result", &Calculator::result);
}
```

```javascript
const calc = new Module.Calculator(10);  
calc.add(5);  
console.log(calc.result());  // 15  
calc.delete();  // Libérer l'objet C++  
```

La similitude avec pybind11 est frappante — et intentionnelle. Le pattern est le même : code C++ métier inchangé, couche de binding déclarative séparée, utilisation idiomatique dans le langage cible.

---

## WebAssembly au-delà du navigateur : WASI

### L'évolution vers une plateforme universelle

WebAssembly a été conçu pour le navigateur, mais son modèle — code binaire portable, sandboxé, performant — s'avère utile bien au-delà. **WASI** (*WebAssembly System Interface*) est une spécification qui définit comment les modules Wasm interagissent avec les ressources système (fichiers, réseau, horloge) de manière portable et sécurisée, hors du navigateur.

En 2026, l'écosystème WASI est en pleine maturation. WASI Preview 2 (2025) a ajouté le support des opérations asynchrones, et WASI 1.0 est attendu fin 2026 ou début 2027. Des runtimes comme Wasmtime, WasmEdge et Wasmer permettent d'exécuter des modules Wasm côté serveur, en edge computing et dans des systèmes embarqués.

Pour le développeur C++, WASI ouvre une perspective intéressante : compiler un même code source vers un module Wasm portable qui peut tourner dans un navigateur (via Emscripten), sur un serveur (via Wasmtime), ou en edge (via WasmEdge), avec des adaptations minimales.

Cette section se concentre sur le cas d'usage navigateur avec Emscripten, qui est le plus mature et le plus immédiatement utile. WASI est mentionné pour le contexte, mais son exploration approfondie dépasse le cadre de cette formation.

---

## Ce que couvrent les sous-sections

**43.4.1 — Installation Emscripten.** Installation du SDK Emscripten (`emsdk`), configuration de l'environnement, vérification de la toolchain, intégration avec CMake via la toolchain file Emscripten.

**43.4.2 — Compilation et intégration JavaScript.** Le workflow complet de compilation : du `emcc` en ligne de commande à l'intégration CMake, les options de compilation critiques (`-O2`, `-sWASM=1`, `-sEXPORT_ES6`), Embind pour l'exposition de classes, les appels bidirectionnels C++ ↔ JavaScript, le système de fichiers virtuel, et le déploiement.

---

## Prérequis

- Le cycle de compilation C++ (chapitre 1, sections 1.3 et 1.4) — Emscripten suit le même pipeline preprocessing → compilation → linking.  
- Les concepts de `extern "C"` (section 43.1) — le mécanisme d'export des fonctions vers JavaScript repose sur les mêmes principes.  
- CMake (chapitre 26) — Emscripten s'intègre dans les projets CMake via une toolchain file.  
- Des notions de base en JavaScript et HTML sont utiles pour les exemples d'intégration, mais ne sont pas un prérequis formel — les exemples sont suffisamment commentés pour être suivis par un développeur C++ sans expérience web.

---

## Positionnement dans l'écosystème d'interopérabilité

| Axe | Mécanisme | Cible | Cas d'usage |
|---|---|---|---|
| C++ ↔ C (43.1) | `extern "C"`, ABI C | Code natif | Bibliothèques, plugins |
| C++ ↔ Python (43.2) | pybind11 / nanobind | Interpréteur Python | Data science, scripting |
| C++ ↔ Rust (43.3) | `cxx` / autocxx | Code natif | Sécurité mémoire, composants système |
| **C++ → Wasm (43.4)** | **Emscripten / Embind** | **Navigateur web** | **Applications web, portage, démos** |

Le point commun entre tous ces axes : le code C++ métier reste inchangé. La couche d'interopérabilité — qu'elle s'appelle pybind11, `cxx`, ou Embind — est une couche d'adaptation séparée qui expose le code existant vers un nouvel environnement d'exécution.

> 💡 *WebAssembly est la seule forme d'interopérabilité dans ce chapitre où le code C++ ne communique pas avec un autre langage natif, mais avec un environnement d'exécution sandboxé. Les contraintes sont différentes : pas d'accès direct au système de fichiers, pas de threads natifs (Web Workers à la place), pas de sockets classiques (WebSockets à la place). Le développeur C++ qui aborde Wasm doit intérioriser ces contraintes — le code compile souvent sans modification, mais le comportement à l'exécution peut différer.*

⏭️ [Installation Emscripten](/43-interoperabilite/04.1-installation-emscripten.md)
