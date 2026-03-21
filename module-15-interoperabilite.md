🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 15 — Interopérabilité

> 🎯 Niveau : Expert

Ce module traite de la coexistence de C++ avec d'autres langages. En 2026, peu de projets sont 100% C++ — il faut exposer du code C++ à Python pour le scripting et le machine learning, interopérer avec Rust pour la sécurité mémoire et la migration progressive, et compiler vers WebAssembly pour le web. Chaque frontière de langage a ses contraintes spécifiques (ABI, ownership, memory model, threading) et ses outils dédiés. Ce module ne contient qu'un seul chapitre, mais c'est l'un des plus larges de la formation — quatre écosystèmes d'interopérabilité avec leurs librairies, leurs patterns et leurs pièges.

---

## Objectifs pédagogiques

1. **Maîtriser** l'interopérabilité C++/C via `extern "C"`, la stabilité de l'ABI C, et les contraintes de name mangling.
2. **Exposer** des fonctions et des classes C++ à Python avec pybind11, gérer les conversions de types, et évaluer nanobind comme alternative plus performante.
3. **Implémenter** une interopérabilité C++↔Rust via `cxx` (bridge typé) et `autocxx` (bindings automatiques), et définir une stratégie de migration progressive.
4. **Compiler** du C++ vers WebAssembly avec Emscripten et intégrer le module résultant dans une application JavaScript.
5. **Concevoir** des interfaces d'interopérabilité qui minimisent les copies, respectent les modèles d'ownership de chaque langage, et restent maintenables à long terme.

---

## Prérequis

- **Module 3, chapitre 6** : classes, RAII, Rule of Five — les bindings pybind11 et cxx exposent des classes C++ dont les sémantiques de copie et de déplacement doivent être correctes.
- **Module 4, chapitre 9** : smart pointers — `std::unique_ptr` et `std::shared_ptr` sont les mécanismes d'ownership que pybind11 et cxx utilisent pour gérer la durée de vie des objets à la frontière des langages.
- **Module 4, chapitre 10** : move semantics — comprendre quand un objet est copié vs déplacé à la frontière C++/Rust est critique pour la performance.
- **Module 9, chapitre 26** : CMake — pybind11 et Emscripten s'intègrent via CMake ; cxx requiert une configuration de build spécifique (Cargo + CMake ou build.rs).

---

## Chapitres

### Chapitre 43 — C++ et Autres Langages

Quatre écosystèmes d'interopérabilité couverts en profondeur : C (la base de toute FFI), Python (le cas d'usage le plus courant), Rust (le sujet le plus stratégique en 2026), et WebAssembly (la cible web).

- **C++ et C** : `extern "C"` pour désactiver le name mangling et exposer une ABI C stable. L'ABI C est la lingua franca de l'interopérabilité — tout langage sait appeler du C. Contraintes : pas de classes, pas de templates, pas d'exceptions à la frontière. Pattern : wrapper C autour d'une API C++ interne.

- **pybind11** : librairie header-only pour exposer du C++ à Python. Installation (pip, Conan, ou sous-module), exposition de fonctions (`py::def`), exposition de classes (`py::class_<>`), gestion des types (conversions automatiques `std::string` ↔ `str`, `std::vector` ↔ `list`, `std::map` ↔ `dict`), gestion du GIL (`py::gil_scoped_release` pour les opérations longues), custom holders pour `std::shared_ptr`. Compilation avec le même compilateur et les mêmes flags que l'interpréteur Python cible.

- **nanobind** : alternative moderne à pybind11 par le même auteur (Wenzel Jakob). Bindings plus petits, compilation plus rapide, meilleure gestion des types NumPy. Adapté aux projets qui nécessitent des bindings plus légers ou un temps de compilation réduit.

- **C++ et Rust — `extern "C"` manuel** : liaison bas niveau via des fonctions `extern "C"` déclarées des deux côtés. Fonctionnel mais fragile — pas de vérification de types à la frontière, gestion manuelle de la mémoire.

- **cxx** : bridge typé Rust↔C++ de référence. Définition de l'interface dans un bloc `#[cxx::bridge]` en Rust, génération automatique du code C++ correspondant. Types partagés (`CxxString`, `CxxVector`, `UniquePtr<T>`, `SharedPtr<T>`), fonctions appelables dans les deux sens. Les types traversent la frontière par référence ou par pointeur intelligent — les copies sont explicites.

- **autocxx** : génération automatique de bindings C++ → Rust à partir des headers C++. Moins de contrôle que cxx mais moins de code à écrire. Adapté aux cas où l'API C++ existe déjà et ne peut pas être modifiée.

- **Stratégie de migration progressive C++ → Rust** : quand introduire Rust (nouveaux modules, composants critiques en sécurité), comment définir la frontière (interface C ou cxx bridge), granularité de la migration (fonction, module, service), retours d'expérience industrie 2025-2026 (Android, Chromium, AWS, Microsoft).

- **WebAssembly avec Emscripten** : compilation de code C++ vers `.wasm` avec `emcc`, intégration dans JavaScript via le module généré, exposition de fonctions C++ à JavaScript (`EMSCRIPTEN_BINDINGS`), limitations (pas de threads POSIX par défaut, pas de filesystem réel, pas de sockets natifs), workarounds (Emscripten FS API, Web Workers pour le threading).

---

## Points de vigilance

- **ABI instable entre versions de compilateur pour les bindings pybind11.** Un module pybind11 compilé avec GCC 14 et chargé dans un Python compilé avec GCC 15 peut crasher silencieusement — les layouts de `std::string`, `std::vector` et des vtables peuvent différer entre versions. Recompilez systématiquement le module pybind11 avec le même compilateur, la même version, et les mêmes flags (`-D_GLIBCXX_USE_CXX11_ABI=1`) que l'interpréteur Python cible. Vérifiez avec `python -c "import sysconfig; print(sysconfig.get_config_var('CC'))"`.

- **cxx bridge qui copie par défaut.** Quand un type C++ traverse le bridge cxx vers Rust, il est copié sauf si vous passez explicitement par référence (`&CxxString`) ou par pointeur intelligent (`UniquePtr<T>`). Sur des structures volumineuses ou des appels fréquents, ces copies implicites produisent des performances surprenantes. Profilez la frontière C++/Rust et utilisez des références ou des `UniquePtr` pour les types coûteux à copier. Méfiez-vous aussi des `CxxVector` qui copient l'intégralité du vecteur — préférez passer un `&[T]` (slice) quand possible.

- **Emscripten et les fonctionnalités POSIX manquantes.** Emscripten émule une partie de l'API POSIX, mais pas tout. Les threads POSIX nécessitent `SharedArrayBuffer` (restrictions COOP/COEP côté serveur), `fork`/`exec` n'existe pas, les sockets POSIX sont remplacés par des WebSockets, `mmap` est émulé en mémoire. Si votre code C++ utilise intensivement les API système Linux (Module 7), attendez-vous à des adaptations significatives pour la cible WebAssembly. Testez avec `-s ASSERTIONS=1` pour obtenir des messages d'erreur explicites sur les fonctions non supportées.

- **Migration C++ → Rust : frontière d'interface mal définie.** Introduire Rust dans un projet C++ existant sans définir clairement la frontière crée une interface qui grossit de manière incontrôlée, avec des conversions de types dans tous les sens et un coût de maintenance supérieur au bénéfice. Règle : définissez la frontière à un niveau de granularité stable (module, service, composant), minimisez la surface d'interface (peu de fonctions, types simples), et documentez le contrat d'ownership (qui alloue, qui libère). Le bridge cxx impose cette discipline par design — profitez-en.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Exposer une API C++ à d'autres langages via un wrapper `extern "C"` quand la stabilité ABI est requise.
- Créer des bindings C++ → Python avec pybind11 (ou nanobind), en gérant les conversions de types, le GIL, et les smart pointers.
- Mettre en place une interopérabilité C++ ↔ Rust via cxx ou autocxx, avec gestion correcte de l'ownership à la frontière.
- Évaluer et planifier une migration progressive d'un codebase C++ vers Rust, avec une frontière d'interface maîtrisée.
- Compiler du C++ vers WebAssembly avec Emscripten et l'intégrer dans une application web.
- Identifier les contraintes de chaque frontière de langage (ABI, ownership, copies, threading) et concevoir des interfaces qui les respectent.

---


⏭️ [C++ et Autres Langages](/43-interoperabilite/README.md)
