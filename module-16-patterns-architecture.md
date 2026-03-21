🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 16 — Patterns et Architecture

> 🎯 Niveau : Expert

Ce module combine deux sujets qui définissent la maturité architecturale d'un développeur C++ : les design patterns adaptés au langage, et la sécurité. Le chapitre 44 couvre les patterns classiques (Singleton, Factory, Observer, Strategy) et les patterns spécifiques au C++ moderne (CRTP, type erasure, dependency injection). Le chapitre 45 traite la sécurité en profondeur — vulnérabilités classiques, flags de compilation défensifs, fuzzing avec AFL++ et LibFuzzer, et surtout le sujet central de 2026 : les safety profiles, le contexte réglementaire (NSA, CISA, Cyber Resilience Act), et la question de la sécurité mémoire en C++.

---

## Objectifs pédagogiques

1. **Implémenter** les patterns de création (Singleton thread-safe, Factory, Builder fluent), les patterns comportementaux (Observer, Strategy, Command), et les patterns spécifiques au C++ (CRTP, type erasure).
2. **Appliquer** la dependency injection en C++ sans framework, en exploitant les templates et les interfaces pour découpler les composants.
3. **Identifier** et prévenir les vulnérabilités classiques en C++ : buffer overflow, integer overflow, use-after-free.
4. **Configurer** les flags de compilation défensifs (`-fstack-protector`, `-D_FORTIFY_SOURCE`, PIE/ASLR) et comprendre ce que chacun protège.
5. **Mettre en place** du fuzzing avec AFL++ et LibFuzzer pour détecter des bugs et des vulnérabilités automatiquement.
6. **Positionner** un projet C++ par rapport aux safety profiles C++26 et au contexte réglementaire 2026, et définir une stratégie de hardening adaptée.

---

## Prérequis

- **Module 3, chapitre 7** : héritage, polymorphisme dynamique, vtable — les patterns Factory, Observer, Strategy reposent sur le polymorphisme. Le CRTP est une alternative statique au polymorphisme dynamique.
- **Module 4, chapitre 12** : `std::variant`, `std::any`, concepts — nécessaires pour le type erasure et les approches modernes des patterns.
- **Module 5, chapitre 16** : templates, SFINAE, concepts — le CRTP et la dependency injection par templates s'appuient sur la métaprogrammation.
- **Module 10, chapitre 29** : sanitizers (ASan, UBSan, MSan) — les sanitizers du chapitre 45 sont utilisés en mode hardening production, une extension de leur usage en développement.
- **Module 15, chapitre 43, section 43.3** : interopérabilité C++/Rust — la section 45.6.4 traite la migration vers Rust comme stratégie de sécurité mémoire.

---

## Chapitres

### Chapitre 44 — Patterns de Conception en C++

Les design patterns adaptés aux spécificités du C++ : gestion mémoire RAII, templates, polymorphisme statique vs dynamique. Ce chapitre ne reprend pas la théorie GoF — il montre comment chaque pattern s'implémente en C++ moderne avec ses idiomes propres.

- **Singleton thread-safe** : Meyers' Singleton (variable statique locale, thread-safe depuis C++11), limites du pattern (état global implicite, couplage, difficulté à tester). Quand l'utiliser (loggers, registries) et quand l'éviter (presque toujours).
- **Factory et Abstract Factory** : création d'objets via `std::unique_ptr`, factory functions vs factory classes, registration dynamique de types.
- **Builder fluent** : construction d'objets complexes avec chaînage de méthodes, retour par référence (`Builder&`) pour le chaînage, validation à la construction.
- **Observer, Strategy, Command** : patterns comportementaux avec `std::function` comme callback, `std::variant` pour le dispatch statique (alternative au polymorphisme dynamique), Command avec undo/redo.
- **CRTP (Curiously Recurring Template Pattern)** : polymorphisme statique — la classe dérivée se passe en paramètre template de la classe de base (`class Derived : public Base<Derived>`). Avantage : pas de vtable, inlining possible. Utilisé dans les mixins, les compteurs d'instances, et les interfaces statiques.
- **Type erasure** : technique pour effacer le type concret tout en conservant un comportement polymorphique — pattern derrière `std::function`, `std::any`, `std::move_only_function`. Implémentation custom pour les cas où `std::any` est trop permissif.
- **Dependency injection** : injection de dépendances via le constructeur (constructor injection), par template (compile-time DI), ou par interface (runtime DI avec `std::unique_ptr<Interface>`). Pas de framework nécessaire en C++ — les templates et RAII suffisent.

### Chapitre 45 — Sécurité en C++

La sécurité mémoire est le sujet le plus stratégique du C++ en 2026. Ce chapitre couvre les vulnérabilités classiques, les protections à la compilation, le fuzzing, et le positionnement vis-à-vis des safety profiles et du contexte réglementaire.

- **Buffer overflows** : stack-based et heap-based, exploitation classique, prévention avec `std::span`, `std::array`, bounds checking.
- **Integer overflows et underflows** : comportement indéfini sur les signés, wrapping sur les non-signés, détection avec UBSan (`-fsanitize=undefined`), prévention avec `std::in_range` (C++20).
- **Use-after-free et temporal safety** : accès à de la mémoire libérée, détection avec ASan, prévention avec smart pointers et RAII.
- **Compilation avec protections** : `-fstack-protector` (canary sur la stack), `-D_FORTIFY_SOURCE=2` (vérification des fonctions libc), ASLR (randomisation de l'espace d'adressage), PIE (Position-Independent Executable) — chaque flag protège contre une classe d'attaque spécifique.
- **Fuzzing** : AFL++ (fuzzing par mutation, coverage-guided, configuration et instrumentation) et LibFuzzer (intégré à LLVM, fuzzing in-process, intégration dans les tests unitaires). Le fuzzing trouve des bugs que les tests unitaires ne couvrent pas — inputs malformés, cas limites, combinaisons inattendues.
- **Sécurité mémoire en 2026** : contexte réglementaire (NSA, CISA recommandent les memory-safe languages, Cyber Resilience Act européen impose des exigences de sécurité), safety profiles C++26 (état de maturité, ce qu'ils garantissent, ce qu'ils ne garantissent pas), hardening avec les sanitizers en production (overhead acceptable pour certains workloads), stratégie de migration progressive vers Rust (quand et comment), bilan : C++ safe-by-default est-il atteignable ?

---

## Points de vigilance

- **Singleton qui masque des dépendances.** Un Singleton accessible globalement via `Logger::instance()` ou `Config::get()` crée une dépendance implicite que ni le constructeur ni l'interface de la classe consommatrice ne révèlent. Conséquence : les tests unitaires ne peuvent pas injecter un mock, l'ordre d'initialisation entre Singletons est indéterminé (static initialization order fiasco), et le couplage se propage dans tout le codebase sans être visible dans les headers. Préférez l'injection de dépendances explicite — passez la dépendance par constructeur. Si un Singleton est inévitable, limitez-le aux cas sans état partagé mutable (configuration immutable, registry read-only).

- **CRTP qui explose les temps de compilation.** Chaque combinaison de classe de base CRTP et de classe dérivée produit une instanciation de template distincte. Sur une hiérarchie large (dizaines de classes dérivées) avec des méthodes CRTP complexes, les temps de compilation augmentent significativement et les messages d'erreur deviennent illisibles. Le CRTP est adapté aux cas où le polymorphisme statique apporte un gain mesurable (hot path, inlining critique) — pas comme remplacement systématique du polymorphisme dynamique.

- **Type erasure avec `std::any` qui perd le typage statique sans bénéfice clair.** `std::any` efface le type à la compilation et le vérifie au runtime avec `std::any_cast` — si le cast échoue, c'est une exception `std::bad_any_cast`. C'est un retour au typage dynamique sans les garanties du compilateur. Utilisez `std::any` uniquement quand le type ne peut pas être connu à la compilation (plugin systems, event buses hétérogènes). Dans les autres cas, `std::variant` (ensemble fermé de types, vérifié à la compilation) ou un type erasure custom avec une interface définie sont des alternatives plus sûres.

- **Safety Profiles C++26 : comprendre les limites.** Les safety profiles ne rendent pas C++ memory-safe de manière automatique. Ils définissent des sous-ensembles du langage avec des restrictions (pas de pointeurs bruts, pas d'arithmétique de pointeurs, pas de casts dangereux) et des garanties associées. Mais leur adoption requiert des modifications du code existant, le support compilateur est encore en cours de déploiement (état mars 2026), et ils ne couvrent pas tous les types de vulnérabilités (concurrence, logique, injection). Ne présentez pas les safety profiles comme une solution complète — c'est un outil parmi d'autres dans une stratégie de hardening qui inclut les sanitizers, le fuzzing, et potentiellement l'interopérabilité Rust pour les composants les plus critiques.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Implémenter les design patterns en C++ moderne (Singleton thread-safe, Factory avec `unique_ptr`, CRTP, type erasure, DI par constructeur) et identifier quand chaque pattern est pertinent.
- Choisir entre polymorphisme dynamique (vtable) et polymorphisme statique (CRTP, templates) en fonction des contraintes de performance et de maintenabilité.
- Identifier les vulnérabilités classiques (buffer overflow, integer overflow, use-after-free) et configurer les flags de compilation défensifs appropriés.
- Mettre en place du fuzzing avec AFL++ et LibFuzzer pour détecter automatiquement des bugs et des vulnérabilités.
- Positionner un projet C++ par rapport aux safety profiles C++26 et au contexte réglementaire, et définir une stratégie de sécurité mémoire réaliste.

---


⏭️ [Patterns de Conception en C++](/44-design-patterns/README.md)
