🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Partie II — C++ Moderne

Cette partie marque le passage du "C++ qui compile" au "C++ idiomatique". Vous avez les fondations — maintenant il s'agit d'écrire du code que vous n'aurez pas honte de relire dans six mois. Smart pointers, move semantics, ranges, concepts, `std::expected`, templates contraints : autant de patterns qui séparent un codebase moderne d'un codebase legacy. C++26 est traité ici comme standard ratifié, pas comme une curiosité expérimentale.

---

## Ce que vous allez maîtriser

- Vous serez capable de gérer la mémoire exclusivement via `std::unique_ptr`, `std::shared_ptr` et `std::weak_ptr`, sans jamais écrire `new`/`delete`.
- Vous serez capable d'exploiter la sémantique de mouvement (move semantics) pour éliminer les copies inutiles et implémenter des transferts de propriété explicites avec `std::move` et `std::forward`.
- Vous serez capable d'écrire des lambdas avec captures adaptées et de les combiner avec les algorithmes STL.
- Vous serez capable d'utiliser les ajouts C++17→C++26 en production : structured bindings, `std::optional`, `std::variant`, `std::expected`, `std::print`, `std::flat_map`, contrats, réflexion statique.
- Vous serez capable de choisir le conteneur STL approprié (séquentiel, associatif, flat) en fonction des contraintes de complexité algorithmique et de localité mémoire.
- Vous serez capable de composer des pipelines de traitement avec les Ranges (C++20) et l'opérateur `|`, et de paralléliser des algorithmes STL via les politiques d'exécution (`std::execution::par`).
- Vous serez capable d'écrire des templates contraints par des Concepts (C++20), y compris des concepts personnalisés, en remplacement de SFINAE.
- Vous serez capable de concevoir une stratégie de gestion d'erreurs cohérente combinant exceptions, `noexcept`, `std::expected` (C++23) et contrats (C++26).
- Vous serez capable d'instrumenter votre code avec `static_assert`, `std::stacktrace` (C++23) et des techniques de débogage défensif.

---

## Prérequis

- **Partie I — Fondations** complétée, en particulier :
  - Module 2 (chapitres 3-5) : système de types, gestion mémoire manuelle (stack/heap, `new`/`delete`), pointeurs.
  - Module 3 (chapitres 6-8) : classes, Rule of Five, constructeurs de copie et de déplacement, surcharge d'opérateurs.

Sans une compréhension solide de la gestion mémoire manuelle, les smart pointers et la move semantics resteront des abstractions opaques.

---

## Modules de cette partie

| # | Titre | Niveau | Chapitres | Lien |
|---|-------|--------|-----------|------|
| Module 4 | C++ Moderne (C++11 → C++26) | Intermédiaire | 9, 10, 11, 12 | [module-04-cpp-moderne.md](/module-04-cpp-moderne.md) |
| Module 5 | La Librairie Standard (STL) | Intermédiaire | 13, 14, 15, 16 | [module-05-stl.md](/module-05-stl.md) |
| Module 6 | Gestion des Erreurs et Robustesse | Intermédiaire | 17, 18 | [module-06-gestion-erreurs.md](/module-06-gestion-erreurs.md) |

---

## Fil conducteur

Le Module 4 commence par la rupture la plus importante avec le C++ pré-2011 : les smart pointers remplacent la gestion mémoire manuelle, et la move semantics redéfinit la manière dont les objets circulent dans un programme. S'ajoutent les lambdas, indispensables pour travailler avec la STL, puis un tour complet des nouveautés C++17 à C++26 — structured bindings, `std::expected`, ranges, concepts, contrats, réflexion statique, `std::execution`. Le Module 5 met ces outils en pratique sur la STL : choix de conteneurs (avec les nouveaux `std::flat_map`/`std::flat_set`), algorithmes classiques et parallèles, composition via ranges, et templates contraints par concepts. Le Module 6 ferme la boucle en structurant la gestion d'erreurs — exceptions, `noexcept`, `std::expected`, contrats C++26, assertions, `std::stacktrace` — pour que le code produit soit non seulement moderne mais robuste. À la sortie de cette partie, vous écrivez du C++ idiomatique 2026, prêt à affronter la programmation système (Partie III).

---


⏭️ [Module 4 : C++ Moderne (C++11 → C++26)](/module-04-cpp-moderne.md)
