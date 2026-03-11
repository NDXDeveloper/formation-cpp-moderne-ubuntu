🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 16 — Templates et Métaprogrammation

## Module 5 : La Bibliothèque Standard (STL) · Niveau Intermédiaire

---

## Présentation du chapitre

Les templates constituent l'un des mécanismes les plus puissants du langage C++. Ils permettent d'écrire du code **générique**, c'est-à-dire du code qui fonctionne avec n'importe quel type, sans sacrifier la sûreté du typage statique ni les performances à l'exécution. Toute la STL — conteneurs, algorithmes, itérateurs — repose sur ce mécanisme. Comprendre les templates, c'est comprendre la philosophie même du C++ moderne.

La métaprogrammation va un cran plus loin : elle exploite le système de templates pour effectuer des **calculs à la compilation**. Le compilateur devient alors un interpréteur, capable d'évaluer des expressions, de sélectionner des chemins de code et de générer des spécialisations optimisées avant même que le programme ne s'exécute. Ce paradigme, longtemps réservé aux experts, est devenu beaucoup plus accessible grâce aux évolutions du langage, en particulier avec les **Concepts** introduits en C++20.

---

## Pourquoi ce chapitre est essentiel

### Le code générique est partout

Dès que vous écrivez `std::vector<int>`, vous instanciez un template de classe. Dès que vous appelez `std::sort(v.begin(), v.end())`, vous utilisez un template de fonction. Les templates ne sont pas une fonctionnalité annexe : ils forment la **colonne vertébrale** de la bibliothèque standard.

Sans une compréhension solide de leur fonctionnement, les messages d'erreur du compilateur restent incompréhensibles, les choix d'architecture deviennent opaques et les optimisations de performance restent hors de portée.

### Zéro coût à l'exécution

Contrairement au polymorphisme dynamique basé sur les fonctions virtuelles et les vtables (cf. chapitre 7), les templates relèvent du **polymorphisme statique**. L'ensemble de la résolution se fait à la compilation. Le compilateur génère du code machine spécialisé pour chaque combinaison de types utilisée, ce qui élimine l'indirection et autorise l'inlining agressif. Le résultat : des performances équivalentes à du code écrit manuellement pour chaque type.

### De SFINAE aux Concepts : un bond en lisibilité

Historiquement, contraindre un template nécessitait de recourir à des techniques comme **SFINAE** (*Substitution Failure Is Not An Error*), souvent cryptiques et difficiles à maintenir. C++20 a introduit les **Concepts**, qui permettent d'exprimer ces contraintes de manière déclarative et lisible. Ce chapitre couvre les deux approches : SFINAE pour comprendre le code existant, et Concepts pour écrire du code moderne.

---

## Contenu du chapitre

Ce chapitre est organisé en sept sections progressives :

**16.1 — Templates de fonctions** : la syntaxe de base pour écrire des fonctions génériques, la déduction automatique des paramètres de type et l'instanciation explicite.

**16.2 — Templates de classes** : la création de structures de données génériques, les paramètres de type et non-type, ainsi que les valeurs par défaut de paramètres template.

**16.3 — Spécialisation partielle et totale** : comment fournir des implémentations alternatives pour des types ou des familles de types spécifiques, et les règles de sélection du compilateur.

**16.4 — SFINAE** : le mécanisme fondamental qui permet au compilateur d'écarter silencieusement une surcharge invalide plutôt que de produire une erreur. Cette section couvre `std::enable_if`, les expressions SFINAE et les patterns classiques rencontrés dans le code pré-C++20.

**16.5 — Variadic templates (C++11)** : les templates acceptant un nombre arbitraire de paramètres, la récursion de templates et le *parameter pack expansion*, essentiels pour comprendre des outils comme `std::tuple` ou `std::make_unique`.

**16.6 — Concepts (C++20) pour contraindre les templates** ⭐ : la syntaxe `requires`, les concepts standard de la STL (`std::integral`, `std::ranges::range`, etc.) et la création de concepts personnalisés. C'est la section pivot du chapitre, celle qui transforme la façon d'écrire du code générique.

**16.7 — Fold expressions (C++17)** : une syntaxe concise pour appliquer un opérateur sur l'ensemble d'un parameter pack, simplifiant considérablement les variadic templates.

---

## Prérequis

Avant d'aborder ce chapitre, vous devriez être à l'aise avec :

- les **types, variables et inférence de type** (`auto`, `decltype`) — chapitre 3 ;
- la **surcharge de fonctions** — section 4.4 ;
- les **classes, constructeurs et encapsulation** — chapitre 6 ;
- le **polymorphisme dynamique** (fonctions virtuelles, vtable) — chapitre 7 ;
- les **smart pointers** et la sémantique de mouvement — chapitres 9 et 10.

Une familiarité avec les conteneurs de la STL (chapitres 13–14) est également recommandée, car les exemples s'appuieront régulièrement sur `std::vector`, `std::map` et les algorithmes standard.

---

## Objectifs d'apprentissage

À l'issue de ce chapitre, vous serez capable de :

1. écrire des **fonctions et classes templates** avec une syntaxe correcte et idiomatique ;
2. comprendre le processus d'**instanciation** et de **déduction de type** du compilateur ;
3. utiliser la **spécialisation** (partielle et totale) pour adapter le comportement générique à des cas particuliers ;
4. lire et comprendre du code utilisant **SFINAE** et `std::enable_if` ;
5. écrire des **variadic templates** et exploiter le *pack expansion* ;
6. définir et appliquer des **Concepts C++20** pour produire des contraintes claires et des messages d'erreur exploitables ;
7. simplifier les opérations sur les parameter packs grâce aux **fold expressions**.

---

## Polymorphisme statique vs dynamique : situer les templates

Il est utile de replacer les templates dans le paysage plus large des mécanismes de polymorphisme offerts par C++ :

| Critère | Polymorphisme dynamique (chapitre 7) | Polymorphisme statique (ce chapitre) |
|---|---|---|
| **Mécanisme** | Fonctions virtuelles, vtable | Templates, Concepts |
| **Résolution** | À l'exécution (*runtime*) | À la compilation (*compile-time*) |
| **Coût** | Indirection (pointeur vtable) | Aucun coût à l'exécution |
| **Flexibilité** | Types déterminés à l'exécution | Types connus à la compilation |
| **Messages d'erreur** | Clairs (erreurs de type classiques) | Historiquement verbeux, améliorés par les Concepts |
| **Cas d'usage typique** | Plugin systems, interfaces stables | Conteneurs, algorithmes, code haute performance |

En pratique, les deux approches sont complémentaires. Un projet réel combine souvent des interfaces virtuelles pour les points d'extension architecturaux et des templates pour le code critique en performance.

---

## Conventions utilisées dans ce chapitre

Tout au long des sections qui suivent, les exemples de code adoptent les conventions suivantes :

- standard **C++20** par défaut, avec indication explicite lorsqu'une fonctionnalité requiert C++17 ou C++23 ;
- compilation avec `g++ -std=c++20 -Wall -Wextra -Wpedantic` ou `clang++ -std=c++20 -Wall -Wextra -Wpedantic` ;
- les noms de paramètres template utilisent la convention `T`, `U` pour les types et `N`, `Size` pour les paramètres non-type ;
- les messages d'erreur reproduits proviennent de GCC 15 ou Clang 20, les deux compilateurs de référence de cette formation.

---

## Liens avec les autres chapitres

Les templates ne vivent pas en isolation. Ce chapitre fait le lien avec plusieurs autres parties de la formation :

- **Chapitre 7 (Héritage et Polymorphisme)** : le polymorphisme dynamique, dont les templates offrent l'alternative statique.
- **Section 12.4 (Concepts C++20)** : un premier aperçu des Concepts est donné dans le survol des nouveautés C++20. Ce chapitre en fournit la couverture complète et approfondie.
- **Section 12.14.2 (Réflexion statique C++26)** : la réflexion statique, ratifiée en C++26, étend les capacités de métaprogrammation bien au-delà de ce que les templates seuls permettent.
- **Chapitres 13–15 (STL)** : tous les conteneurs et algorithmes de la bibliothèque standard sont des templates ; la compréhension de ce chapitre éclaire leur fonctionnement interne.
- **Section 15.7 (Algorithmes parallèles)** : les politiques d'exécution exploitent les templates pour sélectionner le mode de parallélisation à la compilation.

---

> **Note** 💡 — Les templates peuvent paraître intimidants au premier abord. Le chapitre est conçu pour une progression douce : chaque section s'appuie sur la précédente, des templates de fonctions les plus simples jusqu'aux Concepts et fold expressions. Prenez le temps d'expérimenter chaque exemple dans Compiler Explorer (godbolt.org) pour observer le code généré par le compilateur et les messages d'erreur — c'est la meilleure façon de développer une intuition solide.

⏭️ [Templates de fonctions](/16-templates-metaprogrammation/01-templates-fonctions.md)
