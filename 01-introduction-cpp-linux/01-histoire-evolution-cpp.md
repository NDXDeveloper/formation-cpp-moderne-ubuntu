🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 1.1 — Histoire et évolution du C++ (C++98 → C++26)

> **Chapitre 1 — Introduction au C++ et à l'écosystème Linux**  
> **Niveau** : Débutant  
> **Durée estimée** : 30 à 40 minutes

---

## Introduction

Comprendre l'histoire du C++, ce n'est pas un simple détour culturel. C'est une nécessité pratique. Quand vous lisez du code dans un projet existant, vous tombez inévitablement sur des styles radicalement différents selon l'époque à laquelle il a été écrit. Un fichier rédigé en 2005 ne ressemble en rien à un fichier écrit en 2024. Connaître les grandes étapes du langage vous permet de situer le code que vous lisez, de comprendre pourquoi certaines pratiques ont été abandonnées, et de distinguer le C++ moderne du C++ hérité (souvent appelé *legacy*).

Cette section retrace le parcours du langage depuis ses origines jusqu'à la ratification du standard C++26, en mettant l'accent sur les tournants qui ont changé la façon dont on écrit du C++ au quotidien.

---

## Les origines : du C vers le C++ (1979–1998)

### La naissance chez AT&T Bell Labs

L'histoire commence en 1979, dans les laboratoires Bell d'AT&T, à Murray Hill (New Jersey). Un jeune chercheur danois, **Bjarne Stroustrup**, travaille sur sa thèse de doctorat consacrée à la simulation de réseaux distribués. Il utilise alors le langage Simula, un langage pionnier de la programmation orientée objet. Simula lui plaît pour ses abstractions élégantes — classes, héritage, polymorphisme — mais ses performances sont insuffisantes pour les systèmes qu'il étudie. De l'autre côté du spectre, le langage C offre une vitesse d'exécution proche de la machine, mais aucune facilité d'abstraction.

Stroustrup décide alors de combiner les deux mondes. Il crée un préprocesseur, appelé *Cfront*, qui traduit un sur-ensemble du C vers du C standard. Ce nouveau langage s'appelle initialement **"C with Classes"** (C avec des classes). Il ajoute au C les concepts fondamentaux de Simula : les classes, l'encapsulation, l'héritage, et un système de constructeurs et destructeurs.

### De "C with Classes" à C++

En 1983, le langage est renommé **C++** — un clin d'œil à l'opérateur d'incrémentation du C (`++`), suggérant que ce langage est le « C incrémenté ». La première édition du livre de référence, *The C++ Programming Language*, paraît en 1985. Le langage se dote progressivement de fonctionnalités majeures qui le distinguent définitivement du C : les fonctions virtuelles et le polymorphisme dynamique, la surcharge d'opérateurs, les références, les templates (introduits en 1990), et les exceptions.

À ce stade, il n'existe aucun standard formel. Chaque compilateur implémente sa propre variante du langage, ce qui pose d'énormes problèmes de portabilité.

### La première standardisation : C++98

Face à la prolifération des dialectes incompatibles, un effort de standardisation démarre en 1990 sous l'égide de l'ISO (Organisation internationale de normalisation) et de l'ANSI (American National Standards Institute). Huit années de travail aboutissent en **1998** à la publication du premier standard international : **ISO/IEC 14882:1998**, communément appelé **C++98**.

C++98 formalise le langage tel qu'il existe à la fin des années 1990. Il inclut notamment la **Standard Template Library (STL)**, une bibliothèque générique conçue par Alexander Stepanov, qui introduit les conteneurs (`vector`, `map`, `list`…), les itérateurs et les algorithmes. La STL transforme la façon de programmer en C++ : au lieu de réinventer des structures de données à chaque projet, les développeurs disposent désormais d'une bibliothèque standard riche et portable.

> 💡 **Note** — Quand un développeur expérimenté parle de « vieux C++ » ou de « C++ legacy », il désigne généralement du code écrit dans le style C++98 : utilisation massive de `new`/`delete` manuels, pointeurs bruts partout, absence de sémantique de mouvement, héritage profond et macros omniprésentes.

---

## La traversée du désert : C++03 et la longue attente (2003–2011)

En **2003**, un correctif mineur est publié sous le nom de **C++03**. Il ne s'agit pas d'une nouvelle version du langage mais d'une correction de défauts (*defect report*) du standard C++98. Aucune fonctionnalité nouvelle n'est ajoutée.

Pendant ce temps, le comité ISO travaille sur une révision majeure, initialement prévue pour 2009 et donc surnommée **C++0x**. Mais les ambitions sont considérables, les débats techniques intenses, et le processus de consensus entre les membres du comité (compilateurs, industriels, universitaires) est lent. L'année 2009 passe, puis 2010. La blague circule dans la communauté : « le x dans C++0x est hexadécimal ».

Cette période est paradoxale. D'un côté, le langage stagne en apparence. De l'autre, la bibliothèque **Boost** — un ensemble de bibliothèques open-source créé par des membres du comité — sert de terrain d'expérimentation pour les futures fonctionnalités du standard. Smart pointers, expressions régulières, threads, systèmes de fichiers : presque tout ce qui deviendra le C++ moderne est prototypé dans Boost entre 2003 et 2011.

---

## La révolution C++11 : la renaissance du langage

Le **12 août 2011**, le standard C++11 (ISO/IEC 14882:2011) est officiellement adopté. C'est un tournant historique. L'ampleur des changements est telle que Bjarne Stroustrup lui-même déclare que C++11 donne l'impression d'être « un nouveau langage ».

### Les apports majeurs de C++11

**Sémantique de mouvement et références rvalue (`&&`)**. C'est probablement le changement le plus profond. Avant C++11, transférer le contenu d'un objet vers un autre impliquait systématiquement une copie, même quand l'objet source allait être détruit immédiatement après. Les références rvalue et `std::move` permettent de *déplacer* les ressources au lieu de les copier, ce qui élimine des millions de copies inutiles dans le code existant.

**Smart pointers (`std::unique_ptr`, `std::shared_ptr`)**. Ils rendent obsolète l'utilisation directe de `new` et `delete` en encapsulant la gestion de la mémoire dynamique dans des objets qui libèrent automatiquement leurs ressources. C'est la concrétisation du principe **RAII** (*Resource Acquisition Is Initialization*), qui existait en théorie depuis les débuts du langage mais qui manquait d'outils pratiques pour la mémoire allouée sur le tas.

**Lambdas**. Les fonctions anonymes permettent de définir une logique locale directement à l'endroit où elle est utilisée, notamment dans les algorithmes de la STL. Avant C++11, il fallait déclarer un *functor* (un objet avec un `operator()`) dans une classe séparée, ce qui rendait le code verbeux et difficile à lire.

**`auto` et inférence de types**. Le mot-clé `auto` demande au compilateur de déduire automatiquement le type d'une variable à partir de son initialisation. Combiné à `decltype`, il simplifie considérablement l'écriture de code générique.

**Range-based for loop**. La boucle `for (auto& x : container)` remplace les itérateurs explicites pour les parcours simples, rendant le code plus lisible et moins sujet aux erreurs.

**`nullptr`**. Remplace la macro `NULL` et la constante `0` pour les pointeurs nuls, éliminant toute une catégorie d'ambiguïtés avec la surcharge de fonctions.

**`constexpr`**. Permet de déclarer des fonctions et des variables évaluables à la compilation, ouvrant la voie à la métaprogrammation compile-time lisible.

**Threads (`std::thread`, `std::mutex`, `std::atomic`)**. Pour la première fois, le standard C++ inclut un modèle mémoire multithread et des primitives de concurrence. Avant C++11, la programmation concurrente reposait entièrement sur des API spécifiques au système (pthreads sur Linux, Win32 threads sur Windows).

**Variadic templates**. Les templates à nombre variable de paramètres permettent d'écrire des fonctions et des classes génériques acceptant un nombre arbitraire d'arguments, sans recourir aux macros variadic du C.

> 🔥 **Point clé** — C++11 est la ligne de démarcation entre le « vieux C++ » et le « C++ moderne ». Si vous rejoignez un projet professionnel aujourd'hui, le minimum attendu est la maîtrise des idiomes C++11.

---

## L'accélération : un nouveau standard tous les trois ans

Le succès de C++11 convainc le comité ISO d'adopter un **cycle de publication fixe de trois ans**. Chaque nouvelle version apporte des améliorations incrémentales, en suivant une philosophie claire : ne pas accumuler les fonctionnalités dans une seule version massive (l'erreur de C++0x), mais livrer régulièrement des additions mûres et testées.

### C++14 (2014) — Le polish

C++14 est volontairement une version modeste. Son objectif principal est de corriger les aspérités de C++11 et de compléter ce qui manquait. Les ajouts les plus notables sont les **lambdas génériques** (paramètres `auto` dans les lambdas), l'extension de `constexpr` (les fonctions `constexpr` peuvent désormais contenir des boucles et des variables locales), les **binary literals** (`0b1010`), et les **digit separators** (`1'000'000` pour la lisibilité).

C++14 ne révolutionne rien, mais il rend C++11 plus agréable à utiliser au quotidien. C'est la version que beaucoup de projets adoptent comme baseline de production.

### C++17 (2017) — La montée en puissance

C++17 apporte des fonctionnalités plus substantielles qui changent les habitudes d'écriture.

**Structured bindings** (`auto [x, y, z] = ...`). Permet de déstructurer les paires, tuples et structures en variables nommées, à la manière de Python ou JavaScript.

**`std::optional`, `std::variant`, `std::any`**. Trois types vocabulaire qui améliorent l'expressivité : `optional` représente une valeur qui peut être absente, `variant` un type somme (union discriminée), et `any` un conteneur type-erased.

**`std::filesystem`**. Une API portable pour manipuler les fichiers et répertoires, directement inspirée de `boost::filesystem`. Plus besoin de recourir aux appels système POSIX ou aux API Windows pour des opérations courantes.

**`if constexpr`**. Permet la compilation conditionnelle dans les templates, éliminant le besoin de SFINAE dans de nombreux cas et rendant le code générique beaucoup plus lisible.

**Algorithmes parallèles**. Les algorithmes de la STL acceptent désormais des politiques d'exécution (`std::execution::par`, `std::execution::seq`) qui permettent de paralléliser les calculs sur plusieurs cœurs sans réécrire le code.

**Class Template Argument Deduction (CTAD)**. Le compilateur peut déduire les arguments template d'une classe à partir du constructeur : `std::vector v{1, 2, 3}` au lieu de `std::vector<int> v{1, 2, 3}`.

### C++20 (2020) — La deuxième révolution

C++20 est souvent comparé à C++11 par l'ampleur de ses changements. Il introduit quatre piliers majeurs qui transforment en profondeur la façon d'écrire du C++.

**Concepts**. Les contraintes sur les templates deviennent explicites et lisibles. Au lieu de messages d'erreur incompréhensibles quand un type ne satisfait pas les exigences d'un template, le compilateur vérifie les contraintes en amont et produit des diagnostics clairs. Les concepts sont au C++ ce que les *traits bounds* sont à Rust.

**Ranges**. La bibliothèque de ranges apporte une approche fonctionnelle aux algorithmes. Les pipelines composables avec l'opérateur `|` remplacent les chaînes d'itérateurs verbeuses et rendent le code déclaratif : on décrit *ce qu'on veut faire* plutôt que *comment le faire*.

**Coroutines**. Le langage intègre un mécanisme de coroutines *stackless* (sans pile dédiée) qui permet d'écrire du code asynchrone de manière séquentielle. Les coroutines sont la base de la programmation asynchrone moderne en C++, même si leur utilisation pratique nécessite des bibliothèques complémentaires.

**Modules**. Les modules visent à remplacer le système d'inclusion par `#include`, hérité du C, par un mécanisme de compilation séparée moderne qui élimine les problèmes de macros, réduit les temps de compilation et améliore l'isolation des dépendances. En pratique, le support par les compilateurs progresse lentement — nous y reviendrons en section 12.13.

Parmi les autres ajouts notables : `std::jthread` (threads auto-stoppables), l'opérateur *spaceship* (`<=>`) pour la comparaison automatique, `consteval` (fonctions obligatoirement évaluées à la compilation), `std::span` (vue non-owning sur des données contiguës), et `std::format` pour le formatage de chaînes type-safe.

### C++23 (2023) — La consolidation

C++23 poursuit la trajectoire de C++20 en consolidant ses fonctionnalités et en comblant des lacunes.

**`std::print` et `std::format`**. Le formatage type-safe inspiré de Python (et de la bibliothèque `{fmt}`) entre dans le standard. `std::print` combine formatage et sortie en une seule fonction, rendant `std::cout` et `printf` obsolètes pour les nouveaux projets.

**`std::expected`**. Un type qui représente soit une valeur de retour, soit une erreur, offrant une alternative élégante aux exceptions pour la gestion d'erreurs dans le code à haute performance ou dans les contextes où les exceptions sont désactivées.

**`std::flat_map` et `std::flat_set`**. Des conteneurs associatifs ordonnés qui stockent leurs données en mémoire contiguë, offrant de meilleures performances grâce à la localité de cache, au prix d'insertions plus coûteuses.

**`std::mdspan`**. Une vue multidimensionnelle sur des données contiguës, essentielle pour le calcul scientifique et le traitement de matrices.

**`std::generator`**. Un type de coroutine simplifié qui permet de créer facilement des générateurs paresseux (lazy), à la manière du `yield` de Python.

**`std::stacktrace`**. La possibilité d'obtenir une trace d'exécution standard, sans dépendre d'outils externes ou d'API spécifiques au système.

**Deducing this**. Les méthodes peuvent déduire le type de `this`, ce qui élimine la duplication de code entre les surcharges `const` et non-`const` et ouvre la porte à des patterns avancés (CRTP simplifié).

### C++26 (2026) — Le standard de cette formation 🔥

Le standard C++26 a été ratifié par l'ISO, et c'est celui sur lequel cette formation s'appuie lorsque les compilateurs le supportent. C++26 apporte des fonctionnalités très attendues par la communauté.

**Contrats (*Contracts*)**. Un mécanisme natif pour exprimer les préconditions, postconditions et assertions directement dans le code. Les contrats formalisent ce qui était jusqu'ici confié à des macros `assert` ou à des commentaires de documentation, et permettent au compilateur et aux outils d'analyse de vérifier automatiquement ces invariants.

**Réflexion statique (*Static Reflection*)**. La possibilité d'introspecter les types, les membres de classes et les énumérations à la compilation. La réflexion statique ouvre des perspectives considérables pour la sérialisation automatique, la génération de code, les ORMs compile-time et bien d'autres patterns qui nécessitaient auparavant des macros ou des générateurs de code externes.

**`std::execution` (Senders/Receivers)**. Un framework standardisé pour l'exécution asynchrone et la programmation concurrente structurée. `std::execution` remplace à terme `std::async` et `std::future` par un modèle composable basé sur des *senders* et des *receivers*, intégré avec les schedulers et les thread pools.

> 💡 **Note** — Le support de C++26 par les compilateurs est en cours de déploiement en mars 2026. GCC 15 et Clang 20 implémentent une partie significative des nouvelles fonctionnalités, mais certaines (comme la réflexion statique complète) ne sont pas encore disponibles partout. Nous préciserons l'état du support compilateur pour chaque fonctionnalité abordée dans la formation (voir section 12.14.5).

---

## Le processus de standardisation

Il est utile de comprendre *comment* le langage évolue, car cela explique pourquoi certaines fonctionnalités mettent des années à arriver et pourquoi le résultat final diffère parfois des attentes initiales.

Le C++ est standardisé par le comité **ISO/IEC JTC1/SC22/WG21**, généralement appelé simplement **WG21**. Ce comité est composé de plusieurs centaines de membres volontaires issus de l'industrie (Google, Microsoft, Apple, NVIDIA, Bloomberg, Intel…), du monde académique et des développeurs de compilateurs. Il se réunit physiquement trois fois par an (et virtuellement entre les réunions) pour examiner les propositions d'évolution du langage.

Le cycle de vie d'une fonctionnalité suit un parcours formalisé. Toute évolution commence par un **paper** (proposition écrite), déposée sur le site [open-std.org](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/). La proposition est examinée par un ou plusieurs **Study Groups** (SG) spécialisés (concurrence, bibliothèque, évolution du langage, etc.), puis progresse vers les groupes principaux **EWG** (*Evolution Working Group*, pour le cœur du langage) ou **LEWG** (*Library Evolution Working Group*, pour la bibliothèque standard). À chaque étape, la proposition peut être modifiée, renvoyée pour révision, ou rejetée. Le consensus est requis : il ne s'agit pas d'un vote majoritaire mais d'un accord suffisamment large pour que personne ne s'y oppose fortement.

Depuis C++11, le comité applique un **cycle de trois ans** avec une date de gel des fonctionnalités (*feature freeze*) environ 18 mois avant la publication. Toute fonctionnalité qui n'est pas prête au moment du gel est reportée au cycle suivant. Ce mécanisme évite l'effet C++0x (une version qui accumule les retards parce qu'on attend que « tout soit prêt »).

```
C++11 ──── C++14 ──── C++17 ──── C++20 ──── C++23 ──── C++26 ──── C++29 ...
 2011       2014       2017       2020       2023       2026       2029
  │                                                       │
  └── Révolutions majeures ───────────────────────────────┘
       (11, 20, 26 = tournants ; 14, 17, 23 = consolidations)
```

---

## Chronologie récapitulative

| Année | Événement | Impact |
|-------|-----------|--------|
| 1979 | Stroustrup crée "C with Classes" | Naissance du langage |
| 1983 | Renommage en C++ | Le nom que l'on connaît |
| 1985 | *The C++ Programming Language* (1ère éd.) | Première documentation de référence |
| 1990 | Introduction des templates | Programmation générique |
| 1998 | **C++98** — Premier standard ISO | STL, stabilité, portabilité |
| 2003 | C++03 — Correctif | Corrections mineures |
| 2011 | **C++11** — La renaissance | Move semantics, lambdas, auto, smart pointers, threads |
| 2014 | C++14 — Polish | Lambdas génériques, constexpr étendu |
| 2017 | C++17 — Montée en puissance | Structured bindings, optional, filesystem, if constexpr |
| 2020 | **C++20** — Deuxième révolution | Concepts, Ranges, Coroutines, Modules |
| 2023 | C++23 — Consolidation | std::print, expected, flat_map, mdspan, generator |
| 2026 | **C++26** — Standard de cette formation | Contracts, Reflection, Pattern matching, std::execution |

---

## Ce qu'il faut retenir

Le C++ de 2026 n'a plus grand-chose à voir avec le C++ des années 2000. Le langage a connu trois révolutions majeures (C++11, C++20, C++26), entrecoupées de versions de consolidation qui affinent et stabilisent les ajouts précédents. Cette formation vous enseigne le C++ tel qu'il se pratique aujourd'hui, en s'appuyant sur les idiomes modernes tout en vous donnant les clés pour comprendre et maintenir le code hérité que vous rencontrerez inévitablement en contexte professionnel.

La section suivante (1.2) explique *pourquoi* ce langage, malgré sa complexité et son âge, reste un choix stratégique en 2026 pour le DevOps et le system programming.

---


⏭️ [Pourquoi C++ pour le DevOps et le System Programming](/01-introduction-cpp-linux/02-pourquoi-cpp-devops.md)
