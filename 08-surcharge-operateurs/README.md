🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 8 — Surcharge d'Opérateurs et Conversions

## Module 3 : Programmation Orientée Objet · Niveau Débutant-Intermédiaire

---

## Introduction

Les chapitres précédents ont posé les fondations de la programmation orientée objet en C++ : classes, encapsulation, héritage, polymorphisme. Ce chapitre aborde un mécanisme qui distingue C++ de la plupart des autres langages orientés objet : la **surcharge d'opérateurs**.

En C++, les opérateurs (`+`, `-`, `==`, `<<`, `()`, etc.) ne sont pas réservés aux types primitifs. Le langage permet de les redéfinir pour des types utilisateur — vos propres classes et structures — afin qu'ils se comportent de manière naturelle et intuitive. Un objet `Vec3` peut s'additionner avec `+`, un `Matrix` peut se multiplier avec `*`, et deux `Date` peuvent se comparer avec `<`.

Ce n'est pas du sucre syntaxique anodin. La surcharge d'opérateurs est ce qui permet à la bibliothèque standard d'offrir une syntaxe familière pour `std::string` (concaténation avec `+`), `std::vector` (accès avec `[]`), `std::cout` (insertion avec `<<`) et bien d'autres. C'est aussi ce qui fait que les algorithmes STL fonctionnent de manière transparente avec des types utilisateur — `std::sort` n'a besoin que d'un `operator<` (ou de l'opérateur spaceship `<=>` depuis C++20) pour trier n'importe quel type.

Mais cette puissance vient avec une responsabilité. Un opérateur surchargé dont le comportement est surprenant — un `+` qui modifie ses opérandes, un `==` non symétrique, un `operator*` qui effectue une addition — rend le code plus confus que si l'on avait utilisé des méthodes nommées. La règle fondamentale est simple : **surchargez un opérateur uniquement si sa sémantique est évidente pour le lecteur**.

Ce chapitre couvre la mécanique de la surcharge (comment ça s'écrit), les conventions idiomatiques (quand et comment surcharger chaque catégorie d'opérateur) et les deux innovations majeures du C++ moderne : l'opérateur spaceship `<=>` (C++20) et les opérateurs de conversion.

---

## Objectifs du chapitre

À l'issue de ce chapitre, vous serez en mesure de :

- Surcharger les opérateurs arithmétiques (`+`, `-`, `*`, `/`) et de comparaison (`==`, `!=`, `<`, `>`, `<=`, `>=`) pour vos types, en respectant les conventions idiomatiques (symétrie, const-correctness, retour par valeur).
- Implémenter correctement les opérateurs d'affectation (`=`, `+=`, `-=`, etc.) en lien avec la Rule of Five (section 6.5) et la sémantique de mouvement (chapitre 10).
- Définir des opérateurs de conversion implicites et explicites pour contrôler les conversions entre vos types et les types existants.
- Surcharger l'opérateur d'appel de fonction `operator()` pour créer des **foncteurs** (*callable objects*), un concept central pour l'utilisation des algorithmes STL et des lambdas.
- Utiliser l'opérateur spaceship `<=>` (C++20) pour générer automatiquement tous les opérateurs de comparaison à partir d'une seule déclaration, et comprendre les catégories de comparaison (`strong_ordering`, `weak_ordering`, `partial_ordering`).

---

## Prérequis

Ce chapitre s'appuie sur les notions des chapitres précédents :

- **Chapitre 6 — Classes et encapsulation** : Constructeurs, destructeurs, modificateurs d'accès, Rule of Five. Les opérateurs d'affectation (`operator=`) sont directement liés à la Rule of Five.
- **Chapitre 7 — Héritage et polymorphisme** : Comprendre le polymorphisme aide à saisir pourquoi certains opérateurs sont mieux implémentés comme fonctions membres et d'autres comme fonctions libres.
- **Chapitre 4 — Structures de contrôle et fonctions** : Surcharge de fonctions. La surcharge d'opérateurs est un cas particulier de la surcharge de fonctions, avec des conventions supplémentaires.

---

## Principes fondamentaux

Avant de plonger dans les sections, quelques principes traversent l'ensemble du chapitre :

### Un opérateur est une fonction

En C++, `a + b` est une autre façon d'écrire `operator+(a, b)` (fonction libre) ou `a.operator+(b)` (méthode membre). La surcharge d'opérateurs est donc un cas particulier de la surcharge de fonctions — elle utilise exactement le même mécanisme de résolution, avec les mêmes règles de correspondance de types.

### Fonction membre vs fonction libre

Certains opérateurs **doivent** être des fonctions membres (`=`, `[]`, `->`, `()`). D'autres **doivent** être des fonctions libres pour fonctionner correctement (les inserteurs/extracteurs `<<` et `>>`). La majorité peut être implémentée des deux façons, mais les conventions idiomatiques recommandent l'une ou l'autre selon le cas. Chaque section détaille la recommandation pour les opérateurs qu'elle couvre.

### Opérateurs non surchargeables

Quelques opérateurs ne peuvent pas être surchargés :

- `::` (résolution de portée)
- `.` (accès à un membre)
- `.*` (pointeur vers membre)
- `?:` (opérateur ternaire)
- `sizeof`, `typeid`, `alignof`

Tous les autres opérateurs sont surchargeables, y compris `new`/`delete` (allocation mémoire), `,` (opérateur virgule) et `co_await` (coroutines).

### Le principe de moindre surprise

La C++ Core Guideline **C.160** résume l'esprit de la surcharge d'opérateurs : *"Define operators primarily to mimic conventional usage."* Un `operator+` doit se comporter comme une addition, un `operator==` doit être une relation d'équivalence, un `operator<` doit être un ordre strict. Si la sémantique de votre opération ne correspond pas à l'intuition mathématique ou conventionnelle de l'opérateur, utilisez une méthode nommée.

---

## Vue d'ensemble des sections

### 8.1 — Surcharge des opérateurs arithmétiques et de comparaison

Cette section couvre les opérateurs binaires les plus courants : `+`, `-`, `*`, `/`, `%` pour l'arithmétique, et `==`, `!=`, `<`, `>`, `<=`, `>=` pour les comparaisons. Vous verrez le pattern canonique d'implémentation (définir `+=` comme membre, puis `+` comme fonction libre en termes de `+=`), la gestion de la symétrie pour les types mixtes, et les règles de const-correctness et de retour par valeur.

### 8.2 — Opérateurs d'affectation (`=` et `+=`, `-=`, etc.)

L'opérateur d'affectation `operator=` est intimement lié à la Rule of Five (section 6.5). Cette section détaille l'affectation par copie et par déplacement, le copy-and-swap idiom, et les opérateurs d'affectation composés (`+=`, `-=`, `*=`, etc.) qui servent de briques de base pour les opérateurs arithmétiques.

### 8.3 — Opérateurs de conversion (*conversion operators*)

Les opérateurs de conversion permettent à un type utilisateur d'être implicitement ou explicitement converti vers un autre type. Vous verrez la syntaxe `operator Type() const`, la différence critique entre conversions implicites et explicites (`explicit operator bool()`), et les dangers des conversions implicites incontrôlées.

### 8.4 — Opérateur d'appel de fonction (`operator()`)

L'opérateur `()` transforme un objet en **foncteur** (*callable object*) — un objet qui peut être appelé comme une fonction. C'est le mécanisme qui sous-tend les objets de fonction de la STL (`std::less`, `std::hash`), et comprendre les foncteurs est essentiel pour saisir comment les lambdas fonctionnent sous le capot (chapitre 11).

### 8.5 — Opérateur spaceship `<=>` (C++20)

L'opérateur de comparaison tripartite (*three-way comparison*), surnommé "spaceship" en raison de sa forme `<=>`, est l'une des additions les plus impactantes de C++20. Une seule déclaration `auto operator<=>(Other const&) const = default;` génère automatiquement les six opérateurs de comparaison. Cette section couvre les trois catégories de comparaison (`strong_ordering`, `weak_ordering`, `partial_ordering`), l'implémentation personnalisée et l'interaction avec `operator==`.

---

## Conventions utilisées dans ce chapitre

Les exemples de ce chapitre utilisent la compilation C++23 pour bénéficier de l'opérateur spaceship (C++20) et de `std::format` (C++23) :

```bash
g++ -std=c++23 -Wall -Wextra -Wpedantic -g -o programme source.cpp
```

Pour la section 8.5 spécifiquement, le header `<compare>` est nécessaire pour les types de résultat de `<=>`.

---

## Positionnement dans la formation

```
Module 3 : Programmation Orientée Objet
│
├── Chapitre 6 — Classes et Encapsulation ✅
│   └── Constructeurs, destructeurs, RAII, Rule of Five
│
├── Chapitre 7 — Héritage et Polymorphisme ✅
│   └── virtual, override, final, vtable, classes abstraites
│
└── 📍 Chapitre 8 — Surcharge d'Opérateurs et Conversions (vous êtes ici)
    └── Arithmétique, comparaison, affectation, conversion, foncteurs, <=>
```

> 💡 Les foncteurs introduits en section 8.4 seront mis en perspective avec les **lambdas** (chapitre 11) et `std::function` (section 11.4). L'opérateur spaceship (section 8.5) sera réutilisé tout au long de la formation comme méthode canonique de comparaison dans les conteneurs ordonnés (chapitres 13-14).

---

## Sommaire du chapitre

- [8.1 — Surcharge des opérateurs arithmétiques et de comparaison](/08-surcharge-operateurs/01-operateurs-arithmetiques.md)
- [8.2 — Opérateurs d'affectation (`=` et `+=`, `-=`, etc.)](/08-surcharge-operateurs/02-operateurs-affectation.md)
- [8.3 — Opérateurs de conversion (*conversion operators*)](/08-surcharge-operateurs/03-operateurs-conversion.md)
- [8.4 — Opérateur d'appel de fonction (`operator()`)](/08-surcharge-operateurs/04-operateur-appel.md)
- [8.5 — Opérateur spaceship `<=>` (C++20)](/08-surcharge-operateurs/05-operateur-spaceship.md)

⏭️ [Surcharge des opérateurs arithmétiques et de comparaison](/08-surcharge-operateurs/01-operateurs-arithmetiques.md)
