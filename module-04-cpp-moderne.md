🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 4 — C++ Moderne (C++11 → C++26)

> 🎯 Niveau : Intermédiaire

Ce module couvre les évolutions qui ont transformé C++ d'un langage à gestion manuelle en un langage expressif avec gestion automatique des ressources, sémantique de valeur, et abstractions zero-cost. Smart pointers, move semantics, lambdas, puis un tour complet des nouveautés C++17 à C++26 — structured bindings, `std::optional`, `std::expected`, ranges, concepts, coroutines, contrats, réflexion statique, pattern matching, `std::execution`. C++26 est traité ici comme standard ratifié, avec l'état réel du support GCC 15 / Clang 20.

---

## Objectifs pédagogiques

1. **Maîtriser** les smart pointers (`std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`) comme remplacement systématique de `new`/`delete`.
2. **Comprendre** la sémantique de mouvement (l-values vs r-values, `std::move`, `std::forward`) et son impact sur la performance et le design d'API.
3. **Implémenter** des lambdas avec les différents modes de capture et les combiner avec les algorithmes STL et `std::function`.
4. **Utiliser** les ajouts C++17 en code courant : structured bindings, `std::optional`, `std::variant`, `std::any`.
5. **Appliquer** les concepts (C++20) pour contraindre les templates, et les ranges pour composer des pipelines de traitement.
6. **Exploiter** les ajouts C++23 en production : `std::expected`, `std::print`/`std::format`, `std::flat_map`, `std::mdspan`, `std::generator`, `std::stacktrace`.
7. **Connaître** les grandes nouveautés C++26 ratifiées : contrats, réflexion statique, pattern matching, `std::execution` (Senders/Receivers).

---

## Prérequis

- **Module 2, chapitre 5** : gestion mémoire manuelle (stack/heap, `new`/`delete`, pointeurs, dangling pointers) — les smart pointers sont une abstraction au-dessus de ce modèle.
- **Module 3, chapitre 6** : Rule of Five (constructeurs de copie/déplacement, opérateurs d'affectation) — la move semantics étend directement ces concepts.
- **Module 3, chapitre 7** : polymorphisme dynamique et vtable — nécessaire pour comprendre le coût comparé des approches statiques (concepts, CRTP) vs dynamiques.

---

## Chapitres

### Chapitre 9 — Smart Pointers : Gestion Automatique de la Mémoire

Remplacement complet de `new`/`delete` par des smart pointers. Ce chapitre établit la règle fondamentale du C++ moderne : aucun `new`/`delete` nu dans le code applicatif.

- `std::unique_ptr` : possession exclusive, transfert de propriété via `std::move`, custom deleters pour les ressources non-mémoire (file descriptors, handles).
- `std::shared_ptr` et `std::weak_ptr` : comptage de références, coût de l'atomic increment/decrement, cycles de références et comment `std::weak_ptr` les casse.
- `std::make_unique` et `std::make_shared` : pourquoi les préférer à `new` — exception safety et allocation optimisée (single allocation pour `make_shared`).
- Règle : ne jamais utiliser `new`/`delete` dans du code moderne — les seules exceptions sont les custom allocators et l'interopérabilité C.

### Chapitre 10 — Sémantique de Mouvement (Move Semantics)

La move semantics permet de transférer les ressources d'un objet à un autre sans copie. C'est l'optimisation la plus impactante introduite par C++11 — elle change la façon dont on conçoit les API et les conteneurs.

- L-values vs R-values (`&&`) : catégories de valeur en C++, quand le compilateur peut déplacer automatiquement.
- `std::move` : ne déplace rien — c'est un cast vers une r-value reference qui autorise le déplacement. L'objet source est laissé dans un état valide mais indéterminé.
- Move constructors et move assignment operators : implémentation, interaction avec la Rule of Five.
- Perfect forwarding avec `std::forward` : préserver la catégorie de valeur dans les templates — utilisé dans `std::make_unique`, `emplace_back`, etc.
- RVO (Return Value Optimization) et Copy Elision : quand le compilateur élimine le move lui-même — mandatory copy elision depuis C++17.

### Chapitre 11 — Programmation Fonctionnelle et Lambdas

Les lambdas sont le mécanisme principal pour passer du comportement aux algorithmes STL. Ce chapitre couvre la syntaxe, les modes de capture, les lambdas génériques, et `std::function`.

- Syntaxe et captures : par valeur (`[=]`), par référence (`[&]`), capture de `this`, captures mixtes, init captures (C++14) pour le move-capture.
- Lambdas génériques (C++14) : `auto` dans les paramètres, templated lambdas (C++20) avec paramètres de template explicites.
- Utilisation avec les algorithmes STL : `std::sort`, `std::transform`, `std::find_if`, `std::remove_if` — les lambdas remplacent les foncteurs.
- `std::function` : type-erased callable wrapper — utile pour stocker des callbacks, mais avec un coût (allocation heap possible, indirection).

### Chapitre 12 — Nouveautés C++17 / C++20 / C++23 / C++26

Tour complet des ajouts au langage et à la librairie standard sur quatre itérations du standard. C'est le chapitre le plus large du module — il couvre 14 sections allant des structured bindings (C++17) aux Senders/Receivers (C++26).

- **C++17** : structured bindings, `std::optional` / `std::variant` / `std::any`.
- **C++20** : `std::span` (vue zero-copy sur données contiguës), concepts (contraintes sur templates remplaçant SFINAE), ranges (pipelines avec `|`), coroutines (co_await, co_yield, co_return), modules (état de maturité en 2026).
- **C++23** : `std::print`/`std::format` (formatage type-safe), `std::expected` (gestion d'erreurs sans exceptions), `std::flat_map`/`std::flat_set` (conteneurs cache-friendly), `std::mdspan` (vues multidimensionnelles), `std::generator` (coroutines simplifiées), `std::stacktrace` (traces d'exécution standard).
- **C++26 (ratifié)** : contrats (préconditions, postconditions, assertions), réflexion statique (introspection à la compilation), pattern matching (`inspect`, `is`), `std::execution` (Senders/Receivers — modèle d'asynchronisme standardisé remplaçant `std::async`/`std::future`), état du support GCC 15 / Clang 20.

> 📎 `std::span` est introduit ici ; la couverture détaillée est en section 13.5. `std::print` est approfondi ici ; la prise en main rapide est en section 2.7.

---

## Points de vigilance

- **`std::shared_ptr` cyclique sans `std::weak_ptr`.** Deux `shared_ptr` qui se référencent mutuellement ne seront jamais libérés — le compteur de références ne tombe jamais à zéro. C'est un memory leak que Valgrind détecte mais que le programme ne signale pas autrement. Dès qu'une relation est non-owning (parent→enfant est owning, enfant→parent est non-owning), utilisez `std::weak_ptr`.

- **`std::move` sur un objet `const`.** `std::move(const_obj)` compile sans erreur ni warning — mais produit une copie, pas un déplacement. Le `const` empêche le compilateur de sélectionner le move constructor (qui prend un `T&&` non-const). Le résultat est une dégradation silencieuse de performance. Si vous marquez un objet `const`, ne le `std::move` pas.

- **Capture par référence dans une lambda qui survit à la portée capturée.** Une lambda qui capture `[&]` et qui est stockée dans un callback, retournée par une fonction, ou passée à un thread accède à des références pendantes dès que la portée d'origine se termine. Le résultat est un undefined behavior silencieux — pas de crash garanti, juste des données corrompues. Règle : si la lambda survit à la portée, capturez par valeur ou par move (init capture : `[x = std::move(obj)]`).

- **Confondre Concepts (C++20) et SFINAE.** Les concepts remplacent SFINAE pour contraindre les templates, mais ils ne fonctionnent pas de la même manière. SFINAE est un mécanisme de substitution qui écarte les overloads invalides ; les concepts sont des prédicats nommés évalués avant la substitution. Un concept qui échoue produit un message d'erreur clair, là où SFINAE produit des erreurs de substitution cryptiques. Dans du code nouveau, préférez systématiquement les concepts — SFINAE reste nécessaire uniquement pour le code pre-C++20.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Écrire du code C++ sans aucun `new`/`delete` nu, en utilisant exclusivement les smart pointers.
- Optimiser les transferts de ressources avec la move semantics et identifier les cas où RVO rend le move inutile.
- Écrire des lambdas avec le mode de capture approprié et les combiner avec les algorithmes STL.
- Utiliser les ajouts C++17 (`std::optional`, `std::variant`, structured bindings) et C++23 (`std::expected`, `std::print`, `std::flat_map`) en code de production.
- Contraindre des templates avec des concepts (C++20) au lieu de SFINAE.
- Composer des pipelines de traitement avec les ranges (C++20).
- Positionner les nouveautés C++26 (contrats, réflexion statique, pattern matching, `std::execution`) dans votre architecture.

---


⏭️ [Smart Pointers : Gestion Automatique de la Mémoire](/09-smart-pointers/README.md)
