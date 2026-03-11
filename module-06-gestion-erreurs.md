🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 6 — Gestion des Erreurs et Robustesse

> 🎯 Niveau : Intermédiaire

Ce module structure la gestion d'erreurs dans un projet C++ moderne. Exceptions, `noexcept`, `std::expected` (C++23), contrats (C++26), assertions, `static_assert`, `std::stacktrace` — il ne s'agit pas de choisir un seul mécanisme mais de construire une stratégie cohérente qui combine les bons outils aux bons endroits. Le choix entre exceptions et `std::expected` est une décision architecturale qui doit être prise au niveau du projet, pas fonction par fonction.

---

## Objectifs pédagogiques

1. **Maîtriser** le mécanisme d'exceptions C++ : `try`/`catch`/`throw`, hiérarchie `std::exception`, exceptions personnalisées.
2. **Appliquer** `noexcept` sur les fonctions qui ne lèvent pas d'exception, en comprenant son impact sur les performances des conteneurs STL.
3. **Utiliser** `std::expected` (C++23) comme alternative aux exceptions pour les erreurs attendues, et comprendre les compromis par rapport aux exceptions et aux codes d'erreur.
4. **Connaître** les contrats C++26 (préconditions, postconditions, assertions) comme mécanisme complémentaire de validation.
5. **Implémenter** une stratégie de débogage défensif combinant `assert`, `static_assert`, compilation conditionnelle et logging.
6. **Utiliser** `std::stacktrace` (C++23) pour obtenir des traces d'exécution exploitables sans dépendance externe.

---

## Prérequis

- **Module 3, chapitre 6** : destructeurs et RAII — la gestion d'erreurs en C++ repose sur le déroulement de pile (stack unwinding) et la garantie que les destructeurs libèrent les ressources même en cas d'exception.
- **Module 4, chapitre 9** : smart pointers — les smart pointers sont le complément naturel des exceptions : ils garantissent l'absence de fuites mémoire quand une exception traverse une portée.
- **Module 4, chapitre 10** : move semantics — `noexcept` sur les move constructors affecte directement le comportement de `std::vector` lors des réallocations.
- **Module 4, chapitre 12, section 12.8** : introduction à `std::expected` — ce module approfondit son usage en contexte.

---

## Chapitres

### Chapitre 17 — Exceptions et Gestion d'Erreurs

Le mécanisme principal de gestion d'erreurs en C++, des bases jusqu'aux alternatives modernes. Ce chapitre couvre la syntaxe, les garanties d'exception, `noexcept`, `std::expected` (C++23) et l'introduction des contrats (C++26).

- Syntaxe `try`/`catch`/`throw` : mécanisme de stack unwinding, coût des exceptions (zéro en chemin normal, élevé en chemin d'erreur sur les implémentations Itanium ABI).
- Hiérarchie `std::exception` : `std::runtime_error`, `std::logic_error`, `std::out_of_range`, `std::invalid_argument` — quand dériver de quelle classe.
- Exceptions personnalisées : dérivation de `std::exception` ou `std::runtime_error`, ajout d'informations contextuelles (codes d'erreur, messages).
- `noexcept` : spécification sur les fonctions, `noexcept(expression)` conditionnel, impact sur les performances — `std::vector` utilise le move constructor seulement s'il est `noexcept`, sinon il copie lors des réallocations.
- `std::expected<T, E>` (C++23) : retourner soit la valeur soit l'erreur, sans le coût du stack unwinding. Comparaison avec les exceptions (erreurs exceptionnelles) et les codes d'erreur (C-style). Monadic operations : `and_then`, `or_else`, `transform`.
- Contrats C++26 : `pre(condition)`, `post(condition)`, `contract_assert(condition)` — introduction dans le contexte de la validation des entrées/sorties de fonctions.

> 📎 La couverture complète des contrats C++26 se trouve en section 12.14.1.

### Chapitre 18 — Assertions et Débogage Défensif

Les mécanismes de vérification qui ne remplacent pas la gestion d'erreurs mais la complètent — vérifications à la compilation, vérifications en développement, et traces pour le diagnostic en production.

- `assert` : vérification runtime désactivée quand `NDEBUG` est défini (builds Release). Permet de valider les invariants internes pendant le développement.
- `static_assert` : vérification à la compilation — validation de contraintes sur les types, tailles, alignements. Disponible sans overhead runtime.
- Compilation conditionnelle : `#ifdef DEBUG` pour activer du code de diagnostic, logging, vérifications supplémentaires uniquement en mode développement.
- Logging et traces d'exécution : mise en place de traces structurées pour le diagnostic des erreurs en production (prépare le terrain pour spdlog dans le Module 13).
- `std::stacktrace` (C++23) : capture de la pile d'appels au point d'erreur, sans dépendre de bibliothèques tierces ou d'outils externes (`addr2line`, `backtrace`).

> 📎 Voir aussi section 12.12 pour la couverture technique de `std::stacktrace`.

---

## Points de vigilance

- **Lever une exception dans un destructeur.** Si une exception est déjà en cours de propagation (stack unwinding) et qu'un destructeur lève une seconde exception, `std::terminate` est appelé — le programme s'arrête brutalement. Depuis C++11, les destructeurs sont implicitement `noexcept`. Si un destructeur doit gérer une opération qui peut échouer (fermeture de fichier, flush réseau), capturez l'exception à l'intérieur du destructeur et loggez l'erreur sans la propager.

- **Oublier `noexcept` sur les move constructors.** `std::vector::push_back` utilise le move constructor uniquement s'il est marqué `noexcept`. Sans cette garantie, le vector copie les éléments lors d'une réallocation pour maintenir la strong exception guarantee. Sur un vector de milliers d'objets lourds, l'impact est mesurable. Vérifiez avec `static_assert(std::is_nothrow_move_constructible_v<MyType>)`.

- **`std::expected` vs exceptions : incohérence dans la base de code.** Mélanger les deux mécanismes sans convention claire crée du code imprévisible. Les fonctions qui utilisent `std::expected` ne propagent pas les erreurs automatiquement — l'appelant doit vérifier le résultat explicitement. Les fonctions avec exceptions propagent automatiquement mais imposent le coût du stack unwinding. Le choix doit être fait au niveau architectural : exceptions pour les erreurs inattendues (assertion failure, out of memory), `std::expected` pour les erreurs attendues (parsing, I/O, validation). Documentez la convention et tenez-vous-y.

- **`assert()` désactivé en Release sans alternative.** `assert(ptr != nullptr)` disparaît en Release (quand `NDEBUG` est défini). Si la condition vérifiée est un invariant interne de développement, c'est correct. Si c'est une validation d'entrée utilisateur ou une précondition de sécurité, l'assertion ne suffit pas — il faut une vérification qui reste active en production (exception, `std::expected`, ou contrat C++26 avec le bon niveau de vérification). Ne confondez pas assertions de développement et validations de production.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Concevoir une stratégie de gestion d'erreurs cohérente combinant exceptions, `std::expected` et contrats selon le type d'erreur.
- Appliquer `noexcept` de manière systématique sur les move constructors et move assignment operators, et vérifier avec `static_assert`.
- Écrire des exceptions personnalisées dérivant de `std::exception` avec des informations contextuelles exploitables.
- Utiliser `std::expected` (C++23) avec les opérations monadiques (`and_then`, `or_else`, `transform`) pour chaîner les traitements.
- Instrumenter le code avec `assert`, `static_assert`, compilation conditionnelle, et `std::stacktrace` (C++23) pour le diagnostic.
- Différencier les assertions de développement (désactivées en Release) des validations de production (toujours actives).

---


⏭️ [Exceptions et Gestion d'Erreurs](/17-exceptions/README.md)
