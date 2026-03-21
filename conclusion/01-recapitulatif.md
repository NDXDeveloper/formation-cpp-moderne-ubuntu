🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Récapitulatif des compétences acquises

## Vue synthétique de votre parcours — Du débutant à l'expert

---

Ce récapitulatif organise l'ensemble des compétences couvertes par la formation en domaines cohérents. Il remplit deux fonctions : valider ce que vous maîtrisez, et identifier les zones où un retour ciblé serait bénéfique.

Pour chaque domaine, les modules sources sont indiqués entre parenthèses. Le niveau de maîtrise attendu à l'issue de la formation est précisé : **Fondamental** (vous le comprenez et savez l'utiliser), **Opérationnel** (vous pouvez l'appliquer en contexte projet), ou **Expert** (vous pouvez concevoir des solutions avancées et former d'autres développeurs).

---

## 1. Environnement de développement et toolchain Linux

**Niveau attendu : Opérationnel**  
*(Modules 1–2)*

Vous savez installer, configurer et maintenir une toolchain C++ complète sur Ubuntu. Cela inclut la gestion de plusieurs versions de compilateurs (GCC 15, Clang 20) via `update-alternatives`, le choix éclairé entre GCC et Clang selon le contexte, et la connaissance des nouveautés apportées par ces versions en termes de support C++26. Vous maîtrisez les outils fondamentaux — `make`, `ninja-build`, `cmake`, `gdb` — et savez accélérer vos compilations avec `ccache`.

Votre IDE est configuré de manière professionnelle : extensions VS Code (C/C++, CMake Tools, clangd), debugging intégré, DevContainers pour la reproductibilité, et éventuellement tooling assisté par IA (Copilot, Clangd AI). Vous comprenez le cycle complet de compilation (préprocesseur → compilateur → assembleur → éditeur de liens) non pas comme une abstraction, mais comme une suite d'étapes que vous savez exécuter individuellement (`g++ -E`, `-S`, `-c`) et inspecter (`nm`, `objdump`, `ldd`, `readelf`).

Vous connaissez la structure du format ELF (headers, sections, segments), vous savez lire les dépendances dynamiques d'un binaire, et vous maîtrisez les options de compilation critiques : niveaux de warnings (`-Wall`, `-Wextra`, `-Wpedantic`, `-Werror`), optimisation (`-O0` à `-O3`, `-Os`), debug (`-g`, `-ggdb3`) et sélection du standard (`-std=c++17` à `-std=c++26`).

---

## 2. Langage C++ — Fondamentaux

**Niveau attendu : Fondamental → Opérationnel**  
*(Modules 2–3)*

Vous maîtrisez le système de types de C++ : types primitifs et leur représentation mémoire (`sizeof`, `alignof`), types entiers à largeur fixe (`int32_t`, `int64_t`), flottants, et le mécanisme d'inférence de type avec `auto` et `decltype`. Vous savez quand utiliser `auto` et quand un type explicite apporte plus de clarté.

Les quatre opérateurs de cast (`static_cast`, `reinterpret_cast`, `const_cast`, `dynamic_cast`) n'ont plus de secret pour vous : vous connaissez le cas d'usage de chacun et les risques associés. Vous raisonnez naturellement en termes de portée (scope) et de durée de vie (lifetime), et vous distinguez clairement `const`, `constexpr` et `consteval`.

Côté structures de contrôle, vous utilisez les formes modernes du langage : `if constexpr`, `switch` avec initialisation (C++17), range-based `for`. Vous maîtrisez le passage de paramètres (par valeur, par référence, par référence constante, par pointeur), la surcharge de fonctions, les valeurs par défaut et les fonctions inline. Vous avez pris en main `std::print` (C++23) comme alternative moderne à `std::cout` et `printf`.

---

## 3. Gestion de la mémoire

**Niveau attendu : Opérationnel**  
*(Modules 2, 4)*

C'est le cœur de C++ et vous le savez. Vous comprenez la distinction stack/heap non pas de façon théorique, mais en termes concrets : diagramme mémoire d'un processus, caractéristiques de chaque zone, implications sur la performance et la durée de vie des objets. Vous connaissez `new`/`delete` et `new[]`/`delete[]` — et vous savez pourquoi vous ne devez plus les utiliser directement dans du code moderne.

L'arithmétique des pointeurs vous est familière. Vous identifiez les dangers classiques (memory leaks, dangling pointers, double free) et savez les détecter avec Valgrind et AddressSanitizer (`-fsanitize=address`).

Surtout, vous avez adopté la gestion automatique de la mémoire via les smart pointers : `std::unique_ptr` pour la possession exclusive (avec transfert via `std::move` et custom deleters), `std::shared_ptr` pour la possession partagée (comptage de références), et `std::weak_ptr` pour briser les cycles. Vous utilisez systématiquement `std::make_unique` et `std::make_shared`, et la règle est claire : **pas de `new`/`delete` dans du code moderne**.

---

## 4. Programmation orientée objet

**Niveau attendu : Opérationnel**  
*(Module 3)*

Vous savez concevoir des classes bien encapsulées : constructeurs (par défaut, paramétré, copie, déplacement), listes d'initialisation, destructeurs, modificateurs d'accès. Le principe RAII (Resource Acquisition Is Initialization) est devenu un réflexe — vous l'appliquez naturellement pour gérer fichiers, connexions, verrous et toute ressource nécessitant un nettoyage déterministe.

La **Règle des 5** est maîtrisée : vous savez quand et comment implémenter le destructeur, le constructeur de copie, l'opérateur d'affectation par copie, le constructeur de déplacement et l'opérateur d'affectation par déplacement — et surtout, vous savez quand laisser le compilateur les générer.

L'héritage (simple, multiple, virtuel) et le polymorphisme dynamique (`virtual`, `override`, `final`, vtable) sont acquis. Vous connaissez les classes abstraites et les interfaces pures, et vous êtes conscient du coût en performance du polymorphisme dynamique — ce qui vous permet de choisir en connaissance de cause entre dispatch dynamique et alternatives statiques (CRTP, templates).

La surcharge d'opérateurs complète votre maîtrise OOP, y compris l'opérateur spaceship `<=>` (C++20) pour les comparaisons automatiques.

---

## 5. C++ moderne (C++11 → C++26)

**Niveau attendu : Opérationnel → Expert**  
*(Module 4)*

C'est ici que votre profil se distingue. Vous ne vous contentez pas de connaître les smart pointers : vous maîtrisez la **sémantique de mouvement** en profondeur — lvalues vs rvalues, `std::move`, move constructors, perfect forwarding avec `std::forward`, RVO et copy elision. Vous savez pourquoi un `std::move` sur un `const` objet ne déplace rien, et vous écrivez du code qui exploite naturellement les transferts de propriété.

Les **lambdas** sont un outil quotidien : captures par valeur, par référence, de `this`, init captures, lambdas génériques (C++14), lambdas templatées. Vous les combinez avec les algorithmes STL et `std::function`.

Votre connaissance des nouveautés du langage couvre plusieurs standards :

- **C++17** : structured bindings, `std::optional`, `std::variant`, `std::any`, `if constexpr`, switch avec initialisation.  
- **C++20** : Concepts et clauses `requires`, Ranges et pipelines avec `|`, `std::span`, `std::jthread`, coroutines, modules (avec leurs limites actuelles).  
- **C++23** : `std::print` et `std::format`, `std::expected` pour la gestion d'erreurs sans exceptions, `std::flat_map` et `std::flat_set`, `std::mdspan`, `std::generator`, `std::stacktrace`.  
- **C++26** (en ratification) : contrats (préconditions, postconditions, assertions), réflexion statique, pattern matching (`inspect`/`is`), `std::execution` (Senders/Receivers) pour l'asynchronisme standardisé.

Vous connaissez l'état du support compilateur pour chacune de ces fonctionnalités en GCC 15 et Clang 20, et vous savez faire la distinction entre ce qui est production-ready et ce qui reste expérimental.

---

## 6. Librairie standard (STL)

**Niveau attendu : Opérationnel**  
*(Module 5)*

Vous connaissez les conteneurs séquentiels (`std::vector`, `std::array`, `std::list`, `std::forward_list`, `std::deque`, `std::span`) et associatifs (`std::map`, `std::unordered_map`, `std::set`, `std::unordered_set`, `std::flat_map`, `std::flat_set`). Pour chacun, vous comprenez le fonctionnement interne, la complexité algorithmique des opérations, et les cas d'invalidation des itérateurs. Vous choisissez le bon conteneur en fonction du profil d'accès (lecture séquentielle, recherche, insertion, cache-friendliness).

Les algorithmes de la STL font partie de votre vocabulaire courant : recherche (`find`, `binary_search`), tri (`sort`, `stable_sort`), transformation (`transform`, `accumulate`), manipulation (`copy`, `move`, `remove_if`). Vous les combinez avec les Ranges C++20 pour écrire des pipelines lisibles (views, lazy evaluation, opérateur `|`). Vous connaissez les politiques d'exécution parallèle (`std::execution::seq`, `par`, `par_unseq`, `unseq`) et leurs implications.

Les **templates** sont maîtrisés au-delà de l'utilisation basique : templates de fonctions et de classes, spécialisation partielle et totale, SFINAE, variadic templates, fold expressions (C++17), et surtout Concepts (C++20) pour contraindre les templates avec des messages d'erreur lisibles. Vous savez créer vos propres concepts et utiliser ceux de la STL.

---

## 7. Gestion des erreurs et robustesse

**Niveau attendu : Opérationnel**  
*(Module 6)*

Vous disposez de plusieurs stratégies de gestion d'erreurs et savez choisir la bonne selon le contexte. Les exceptions (`try`/`catch`/`throw`) sont maîtrisées : hiérarchie standard, exceptions personnalisées, spécification `noexcept` et son impact sur les performances. Vous connaissez les alternatives modernes : `std::expected` (C++23) pour les erreurs attendues, codes d'erreur pour les chemins critiques en performance, et les contrats C++26 (préconditions, postconditions) comme filet de sécurité supplémentaire.

Le débogage défensif fait partie de votre pratique : `assert` et `static_assert`, compilation conditionnelle, logging structuré, et `std::stacktrace` (C++23) pour des traces d'exécution exploitables en production.

---

## 8. Programmation système Linux

**Niveau attendu : Opérationnel → Expert**  
*(Module 7)*

Vous savez programmer au niveau système sur Linux. Le système de fichiers est manipulé via `std::filesystem` (C++17) pour les opérations courantes et via les appels POSIX (`open`, `read`, `write`, `close`) quand le contrôle fin est nécessaire. Vous gérez les permissions, les droits et les erreurs système.

Les signaux POSIX (`SIGINT`, `SIGTERM`, `SIGSEGV`) sont compris : installation de handlers avec `sigaction`, problématiques d'interaction entre signaux et threads.

La **programmation concurrente** est une compétence centrale : création et gestion de threads (`std::thread`, `std::jthread`), synchronisation (mutex, `lock_guard`, `unique_lock`, `scoped_lock`), variables de condition, atomiques et memory ordering, programmation asynchrone (`std::async`, `std::future`), thread-safety et détection de data races. Vous connaissez les algorithmes parallèles et leur application en contexte multi-thread.

En **networking**, vous maîtrisez les sockets TCP/UDP (API POSIX), le multiplexage I/O (`select`, `poll`, `epoll`), les librairies réseau modernes (Standalone Asio, Boost.Asio), les clients HTTP (cpr, cpp-httplib) et la communication RPC haute performance avec gRPC et Protocol Buffers (définition de services `.proto`, génération de code, streaming bidirectionnel).

La communication inter-processus est couverte : `fork`/`exec`, pipes, shared memory (`mmap`), message queues POSIX.

---

## 9. Parsing et formats de données

**Niveau attendu : Opérationnel**  
*(Module 8)*

Vous savez lire, écrire et valider les formats de données courants en C++ :

- **JSON** avec nlohmann/json (parsing, sérialisation d'objets C++, gestion d'erreurs).  
- **YAML** avec yaml-cpp (fichiers de configuration).  
- **TOML** avec toml++ (alternative moderne).  
- **XML** avec pugixml (systèmes legacy).  
- **Protocol Buffers** (sérialisation binaire performante, définition de messages `.proto`, génération de code).  
- **FlatBuffers** (zéro-copy serialization).  
- **MessagePack** (JSON binaire compact).

Vous connaissez les compromis entre ces formats (lisibilité, performance, taille, schéma) et appliquez les bonnes pratiques de validation de schémas.

---

## 10. Build systems et gestion de projet

**Niveau attendu : Opérationnel → Expert**  
*(Module 9)*

**CMake** est votre build system principal. Vous écrivez des `CMakeLists.txt` modernes basés sur les targets (`add_executable`, `add_library`, `target_link_libraries`, `target_include_directories`) avec une maîtrise claire de la visibilité `PUBLIC`/`PRIVATE`/`INTERFACE`. Vous gérez les dépendances externes via `find_package`, `FetchContent` et `add_subdirectory`, et vous standardisez vos configurations avec CMake Presets. Vous générez pour Ninja par défaut (`cmake -G Ninja`) et connaissez les nouveautés de CMake 3.31+.

La **gestion des dépendances** est maîtrisée : Conan 2.0 (conanfile.py, profils, intégration CMake) et vcpkg comme alternative. Vous comprenez le linkage statique (`.a`) vs dynamique (`.so`) et savez distribuer des librairies sur Linux.

Vous connaissez la syntaxe des Makefiles, le fonctionnement de Ninja et ses avantages, Meson comme alternative montante, et les compromis entre ces build systems.

---

## 11. Débogage, profiling et qualité de code

**Niveau attendu : Opérationnel**  
*(Module 10)*

Votre arsenal de débogage est complet : GDB en ligne de commande (breakpoints conditionnels, inspection, watchpoints), débogage via IDE, analyse de core dumps pour le post-mortem. Les quatre sanitizers sont intégrés à votre workflow : AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer et MemorySanitizer.

En analyse mémoire, Valgrind (memcheck, lecture de rapports) et Massif (heap profiling) vous permettent de traquer fuites et consommation excessive.

Le **profiling** CPU avec `perf` (record, report, compteurs matériels), `gprof`, flamegraphs et Hotspot vous donne une vision précise des points chauds de vos applications.

L'**analyse statique** est systématique : clang-tidy avec une configuration `.clang-tidy` adaptée, cppcheck pour la détection d'erreurs, clang-format pour le formatage automatique. Ces outils sont intégrés dans votre workflow quotidien.

---

## 12. Tests et qualité logicielle

**Niveau attendu : Opérationnel**  
*(Module 11)*

Vous écrivez des **tests unitaires** avec Google Test : tests simples (`TEST`), tests avec fixtures (`TEST_F`), tests paramétrés (`TEST_P`), assertions et matchers. Le mocking avec Google Mock vous permet d'isoler les composants sous test. Vous connaissez et pratiquez le Test-Driven Development (TDD) en C++.

La **couverture de code** est mesurée avec gcov/lcov, avec génération de rapports HTML et intégration dans CMake. Vous avez des objectifs de couverture définis et suivis.

Le **benchmarking** avec Google Benchmark vous permet de produire des mesures de performance fiables et de les interpréter correctement.

---

## 13. DevOps et Cloud Native

**Niveau attendu : Opérationnel**  
*(Modules 12–13)*

Vous savez créer des **outils CLI** professionnels : parsing d'arguments avec CLI11 (options, flags, sous-commandes, validation, aide automatique), formatage avancé avec fmt, gestion des couleurs et du TTY. Vous connaissez l'architecture d'un outil CLI de qualité production (à la `kubectl` ou `git`).

La **dockerisation** d'applications C++ est maîtrisée : choix de l'image de base (Ubuntu vs Alpine), multi-stage builds (stage de compilation + stage d'exécution minimal), gestion des librairies partagées dans les conteneurs, images distroless pour la sécurité et la réduction de surface d'attaque.

Vos **pipelines CI/CD** sont opérationnels sur GitLab CI et GitHub Actions : structure des fichiers de configuration, stages (build → test → package), accélération avec ccache et sccache (cache distribué), matrix builds multi-compilateur/multi-version, cross-compilation (ARM, RISC-V), automatisation des tests et de l'analyse statique, gestion des artifacts et des releases.

Le **packaging** couvre les formats DEB, RPM et AppImage pour la distribution universelle.

L'**observabilité** est intégrée dès la conception : logging structuré avec spdlog (niveaux, sinks, formatage), logs JSON pour l'agrégation, métriques Prometheus, tracing distribué avec OpenTelemetry C++, health checks et readiness probes.

---

## 14. Optimisation de performance

**Niveau attendu : Expert**  
*(Module 14)*

Vous comprenez le fonctionnement du cache CPU (L1, L2, L3), les cache lines, le false sharing, et vous appliquez les principes du data-oriented design pour maximiser la localité des données. La branch prediction et son impact sur les conditions sont intégrés à votre raisonnement.

SIMD et vectorisation (SSE, AVX) vous sont accessibles, que ce soit via les intrinsics ou l'auto-vectorisation du compilateur. Vous appliquez PGO (Profile-Guided Optimization) et LTO (Link-Time Optimization) pour les builds de production.

Les conteneurs `std::flat_map`/`std::flat_set` font partie de votre arsenal pour les scénarios cache-sensitive.

---

## 15. Programmation bas niveau et lock-free

**Niveau attendu : Expert**  
*(Module 14)*

Inline assembly, manipulation de bits et bitfields, memory ordering et barrières mémoire, structures lock-free et compare-and-swap (CAS) — vous disposez des outils pour écrire du code haute performance dans les scénarios où les abstractions de haut niveau ne suffisent pas. Vous savez aussi que ces techniques sont rarement nécessaires, et vous ne les employez que lorsque le profiling le justifie.

---

## 16. Interopérabilité

**Niveau attendu : Opérationnel**  
*(Module 15)*

Votre code C++ ne vit pas en isolation. Vous savez exposer une interface C (`extern "C"`) pour l'interopérabilité ABI, appeler du C++ depuis Python via pybind11 (et connaissez nanobind comme alternative plus rapide), dialoguer avec Rust via le bridge cxx (et autocxx pour les bindings automatiques), et compiler pour le web avec Emscripten (WebAssembly).

Vous avez une vision stratégique de l'interopérabilité C++/Rust : quand et comment introduire Rust dans un projet C++ existant, sur la base des retours d'expérience de l'industrie 2025-2026.

---

## 17. Patterns, architecture et sécurité

**Niveau attendu : Expert**  
*(Module 16)*

Les **design patterns** sont implémentés en C++ idiomatique : Singleton thread-safe, Factory/Abstract Factory, Builder fluent, Observer, Strategy, Command. Vous maîtrisez les patterns avancés spécifiques à C++ : CRTP, type erasure, dependency injection.

La **sécurité** est une préoccupation transversale : vous identifiez les vulnérabilités classiques (buffer overflow, integer overflow, use-after-free), vous compilez avec les protections appropriées (`-fstack-protector`, `-D_FORTIFY_SOURCE`, ASLR/PIE), et vous pratiquez le fuzzing (AFL++, LibFuzzer) pour découvrir les failles avant la production.

Vous connaissez le contexte réglementaire 2026 (NSA, CISA, Cyber Resilience Act), l'état des Safety Profiles, les stratégies de hardening avec les sanitizers en production, et vous avez une position informée sur la question de savoir si un C++ safe-by-default est atteignable — et quand l'interopérabilité avec Rust est la meilleure réponse.

---

## 18. Organisation professionnelle et collaboration

**Niveau attendu : Opérationnel**  
*(Module 17)*

Vous structurez vos projets selon les conventions professionnelles (`src/`, `include/`, `tests/`, `docs/`), avec une séparation `.h`/`.cpp` propre, des namespaces bien définis, et une documentation Doxygen maintenue. Vous connaissez et appliquez un standard de codage (Google C++ Style Guide, LLVM Style, ou C++ Core Guidelines).

Votre workflow Git intègre des **pre-commit hooks** (clang-format, clang-tidy, tests rapides) configurés via le framework pre-commit. Vous pratiquez des code reviews efficaces, gérez la dette technique de manière proactive, et appliquez le semantic versioning avec des changelogs structurés.

---

## 19. Veille technologique et communauté

**Niveau attendu : Fondamental**  
*(Module 17)*

Vous disposez d'une bibliothèque de référence (Effective C++, C++ Concurrency in Action, A Tour of C++, Embracing Modern C++ Safely) et vous suivez les conférences majeures (CppCon, Meeting C++, C++ Now, ACCU). Vous comprenez le processus de standardisation ISO, vous savez suivre les proposals sur open-std.org et GitHub, et vous avez un aperçu des directions explorées pour C++29.

Les communautés (Stack Overflow, Reddit r/cpp, Discord C++) et les outils comme Compiler Explorer (godbolt.org) font partie de votre pratique quotidienne.

---

## Grille d'auto-évaluation

Pour chaque domaine listé ci-dessus, évaluez-vous honnêtement sur une échelle à trois niveaux :

- **À consolider** — Vous comprenez les concepts mais manquez de pratique. Retournez aux modules correspondants et réalisez les exercices et mini-projets.  
- **Acquis** — Vous pouvez appliquer ces compétences en contexte projet avec confiance. Passez à la mise en pratique sur des projets personnels ou open source.  
- **Maîtrisé** — Vous pouvez enseigner ces sujets et concevoir des solutions avancées. Concentrez-vous sur les domaines où vous êtes encore en phase de consolidation.

L'objectif n'est pas d'être expert partout. C'est de savoir où vous en êtes, et de choisir consciemment vos prochains axes de progression. La section **[Trajectoires professionnelles](/conclusion/02-trajectoires.md)** vous aidera à prioriser en fonction du parcours qui vous attire.

---


⏭️ [Trajectoires professionnelles](/conclusion/02-trajectoires.md)
