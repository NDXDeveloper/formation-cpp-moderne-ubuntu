🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12. Nouveautés C++17/C++20/C++23/C++26 ⭐

## Une décennie de transformation du langage

Entre C++17 et C++26, le langage C++ a connu sa période d'évolution la plus intense depuis sa création. En l'espace de trois cycles de standardisation, le comité ISO a introduit des fonctionnalités qui redéfinissent profondément la manière d'écrire, de structurer et de raisonner sur du code C++. Ce chapitre constitue le cœur du Module 4 : il couvre l'ensemble de ces nouveautés, des ajouts pragmatiques de C++17 jusqu'aux changements architecturaux majeurs de C++26, standard désormais ratifié.

## Pourquoi ce chapitre est essentiel

Le C++ que l'on écrit en 2026 n'a plus grand-chose à voir avec celui de 2011. Les smart pointers et la sémantique de mouvement (chapitres 9 et 10) ont posé les bases du C++ moderne, mais c'est dans les standards C++17 à C++26 que le langage a véritablement changé de paradigme :

- **C++17** a simplifié le code au quotidien. Les structured bindings, `std::optional`, `std::variant`, `if constexpr` ou encore `std::filesystem` ont éliminé une quantité considérable de code boilerplate. C'est le standard qui a rendu le C++ moderne accessible et agréable à utiliser au jour le jour.

- **C++20** a été qualifié de « nouveau C++11 » par la communauté — et à juste titre. Avec les Concepts, les Ranges, les Coroutines et les Modules, ce standard a introduit quatre piliers qui changent la façon même de concevoir une architecture C++. Les templates deviennent lisibles grâce aux contraintes explicites, les algorithmes s'enchaînent en pipelines fonctionnels, et la programmation asynchrone entre dans le langage natif.

- **C++23** a consolidé et comblé les lacunes. `std::expected` offre une gestion d'erreurs élégante sans exceptions, `std::print` et `std::format` modernisent enfin l'affichage, `std::flat_map` apporte des conteneurs cache-friendly dans la STL, et `std::mdspan` ouvre la voie au calcul multidimensionnel performant. Ce standard a transformé des bibliothèques tierces éprouvées (comme `{fmt}`) en fonctionnalités standard.

- **C++26**, ratifié récemment, marque un tournant. Les Contrats (préconditions, postconditions, assertions) apportent une robustesse formalisée. La réflexion statique permet l'introspection des types à la compilation. Le pattern matching avec `inspect` rapproche C++ des langages fonctionnels modernes. Et `std::execution` (Senders/Receivers) pose un nouveau modèle d'asynchronisme standardisé, destiné à remplacer `std::async` et `std::future`.

## Organisation du chapitre

Ce chapitre est structuré par thème fonctionnel plutôt que par strict ordre chronologique, afin de regrouper les concepts liés et de faciliter l'apprentissage progressif. Chaque section peut néanmoins être abordée indépendamment.

### Simplification du code quotidien (C++17)

Les sections 12.1 à 12.2 couvrent les structured bindings et les types utilitaires (`std::optional`, `std::variant`, `std::any`) qui simplifient radicalement la manipulation de données composites et la gestion des valeurs absentes ou polymorphes.

### Vues et accès mémoire (C++20/C++23)

La section 12.3 introduit `std::span`, une vue légère sur des données contiguës en mémoire, tandis que la section 12.10 étend ce concept au multidimensionnel avec `std::mdspan`. La section 12.9 présente `std::flat_map` et `std::flat_set`, des conteneurs ordonnés à mémoire contiguë optimisés pour le cache CPU.

### Programmation générique contrainte (C++20)

La section 12.4 traite des Concepts, le mécanisme qui rend enfin les templates lisibles et les messages d'erreur compréhensibles. C'est l'une des fonctionnalités les plus transformatrices de C++20 pour le code générique.

### Programmation fonctionnelle et pipelines (C++20)

La section 12.5 couvre les Ranges, qui introduisent la composition d'algorithmes par pipelines et l'évaluation paresseuse — un changement de paradigme dans la manière d'écrire des traitements de données.

### Programmation asynchrone (C++20/C++23/C++26)

Les sections 12.6, 12.11 et 12.14.4 forment un arc cohérent : des coroutines de base (C++20), en passant par `std::generator` qui les rend accessibles (C++23), jusqu'à `std::execution` (Senders/Receivers) qui standardise l'asynchronisme dans C++26.

### Formatage et affichage moderne (C++23)

La section 12.7 approfondit `std::print` et `std::format`, le système de formatage type-safe qui remplace avantageusement `printf` et `std::cout`. Cette section complète l'introduction rapide vue en section 2.7.

### Gestion d'erreurs et robustesse (C++23/C++26)

Les sections 12.8 et 12.12 présentent respectivement `std::expected` (gestion d'erreurs sans exceptions) et `std::stacktrace` (traces d'exécution standard), tandis que la section 12.14.1 couvre les Contrats de C++26.

### Modularisation du code (C++20)

La section 12.13 fait le point sur les Modules en 2026 : leur promesse, leur état de support réel dans GCC 15 et Clang 20, et ce qui fonctionne en pratique dans un projet de production.

### Le standard C++26 (ratifié)

La section 12.14 est dédiée aux grandes nouveautés de C++26 : Contrats, réflexion statique, pattern matching et `std::execution`. Elle inclut un état des lieux du support compilateur en mars 2026.

## Prérequis

Ce chapitre suppose que les notions suivantes sont acquises :

- **Gestion de la mémoire** (chapitre 5) — stack vs heap, allocation dynamique.
- **Programmation orientée objet** (chapitres 6-8) — classes, héritage, polymorphisme, surcharge d'opérateurs.
- **Smart pointers** (chapitre 9) — `std::unique_ptr`, `std::shared_ptr` et la philosophie du zéro `new`/`delete`.
- **Sémantique de mouvement** (chapitre 10) — l-values, r-values, `std::move`, perfect forwarding.
- **Lambdas** (chapitre 11) — syntaxe, captures, lambdas génériques.

Les templates de base (fonctions et classes templates) sont également utiles, bien que leur couverture approfondie se trouve au chapitre 16. Certaines sections avancées de ce chapitre — notamment les Concepts (12.4) et la réflexion statique (12.14.2) — y feront naturellement écho.

## Environnement recommandé

Pour tirer pleinement parti de ce chapitre, il est conseillé de disposer de :

- **GCC 15** ou **Clang 20**, qui offrent le meilleur support des fonctionnalités couvertes ici (voir section 2.1.3 pour l'installation).
- **CMake 3.31+** configuré avec le standard approprié via `target_compile_features` ou le flag `-std=c++23` / `-std=c++26` selon les sections.
- **Compiler Explorer** (godbolt.org) pour tester rapidement des snippets avec différents compilateurs et versions.

Certaines fonctionnalités de C++26, en particulier la réflexion statique et le pattern matching, peuvent ne disposer que d'un support partiel au moment de la rédaction. Chaque section concernée précise l'état du support et les alternatives disponibles.

## Fil conducteur

Tout au long de ce chapitre, un fil conducteur se dessine : le C++ évolue vers un langage qui reste performant et proche du matériel, tout en offrant un niveau d'expressivité et de sécurité qui était autrefois réservé aux langages de plus haut niveau. Chaque standard rapproche un peu plus le langage de cet objectif. Comprendre cette trajectoire, c'est comprendre où le C++ va — et pourquoi il reste, en 2026, un choix pertinent et puissant pour le system programming, le DevOps et le développement haute performance.

---


⏭️ [Structured Bindings (C++17)](/12-nouveautes-cpp17-26/01-structured-bindings.md)
