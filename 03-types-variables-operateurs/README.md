🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 3 : Types, Variables et Opérateurs

> **Module 2 — Les Fondamentaux du Langage** · Niveau Débutant  
> Prérequis : [Chapitre 2 — Mise en place de la Toolchain sur Ubuntu](/02-toolchain-ubuntu/README.md)

---

## Objectifs du chapitre

Ce chapitre pose les fondations sur lesquelles repose tout programme C++. Avant d'écrire la moindre structure de contrôle, la moindre classe ou le moindre template, il est indispensable de comprendre comment le langage représente les données en mémoire, comment il les nomme, comment il les transforme et quelles garanties il offre au moment de la compilation.

À l'issue de ce chapitre, vous serez capable de :

- Comprendre la philosophie du **typage statique fort** de C++ et les mécanismes d'**inférence de type** (`auto`, `decltype`) qui rendent le code à la fois sûr et concis.
- Maîtriser les **types primitifs** du langage (entiers, flottants, booléens), leurs tailles, leur représentation mémoire et les garanties offertes par les types à largeur fixe (`int32_t`, `int64_t`, …).
- Effectuer des **conversions de type** de manière explicite et contrôlée grâce aux quatre opérateurs de cast C++ (`static_cast`, `reinterpret_cast`, `const_cast`, `dynamic_cast`), en sachant quand et pourquoi éviter les casts hérités du C.
- Distinguer la **portée** (*scope*) d'une variable de sa **durée de vie** (*lifetime*), deux notions distinctes souvent confondues par les débutants.
- Exploiter les qualificateurs `const`, `constexpr` et `consteval` pour exprimer l'immutabilité et déplacer un maximum de calculs à la compilation.

---

## Pourquoi ce chapitre est fondamental

C++ est un langage à typage statique fort. Cela signifie que chaque variable, chaque expression possède un type connu à la compilation et que le compilateur refuse (ou signale) les opérations incohérentes entre types incompatibles. C'est une différence majeure avec des langages comme Python ou JavaScript, où le type d'une variable peut changer au cours de l'exécution.

Cette rigueur a un coût — le programmeur doit réfléchir aux types qu'il manipule — mais elle offre trois avantages décisifs :

1. **Détection précoce des erreurs.** Une erreur de type découverte à la compilation ne se transformera jamais en bug silencieux en production. Le compilateur est votre premier filet de sécurité ; autant le laisser travailler.

2. **Performance prédictible.** En connaissant la taille et la représentation de chaque donnée, le compilateur peut générer du code machine optimal : alignement mémoire correct, utilisation des registres adaptés, vectorisation SIMD lorsque c'est possible. Rien de tout cela n'est envisageable si le type n'est résolu qu'à l'exécution.

3. **Lisibilité et documentation implicite.** Un prototype de fonction comme `double compute_average(std::span<const int> values)` documente immédiatement ce qu'il attend et ce qu'il renvoie, sans avoir besoin d'un commentaire supplémentaire.

Le C++ moderne (à partir de C++11) a considérablement allégé la verbosité liée au typage grâce à l'inférence de type avec `auto`. L'idée n'est pas d'abandonner le typage statique, mais de laisser le compilateur déduire ce qu'il sait déjà déduire, tout en gardant le contrôle là où c'est nécessaire. Nous verrons dans ce chapitre comment trouver le bon équilibre.

---

## Vue d'ensemble des sections

Le chapitre est organisé en cinq sections progressives.

**Section 3.1 — Le typage statique fort et l'inférence de type** introduit `auto` et `decltype`, deux mécanismes qui permettent de laisser le compilateur déduire le type d'une variable ou d'une expression. Nous verrons les règles de déduction, les pièges courants (déduction de références, qualificateurs perdus) et les heuristiques pour choisir entre un type explicite et `auto`.

**Section 3.2 — Types primitifs, tailles et représentation mémoire** passe en revue les types fondamentaux du langage : entiers signés et non-signés, flottants, booléens. On y examine comment `sizeof` et `alignof` révèlent la disposition des données en mémoire, et pourquoi les types à largeur fixe de `<cstdint>` sont indispensables dès que l'on travaille sur du code portable ou des protocoles binaires.

**Section 3.3 — Conversion de types : cast implicite vs explicite** détaille les quatre opérateurs de cast du C++ et les scénarios dans lesquels chacun est approprié. Cette section met l'accent sur la sécurité : un `static_cast` exprime une intention vérifiable par le compilateur, là où un cast C-style masque les risques.

**Section 3.4 — Portée des variables et durée de vie** clarifie la différence entre le *scope* (la zone du code où un nom est visible) et le *lifetime* (la période pendant laquelle l'objet existe réellement en mémoire). Cette distinction est essentielle pour comprendre les mécanismes de construction et destruction automatiques qui sous-tendent le principe RAII, abordé plus tard au chapitre 6.

**Section 3.5 — `const`, `constexpr` et `consteval`** explore les trois niveaux d'immutabilité offerts par le langage. `const` protège une variable à l'exécution, `constexpr` permet d'évaluer une expression dès la compilation, et `consteval` (C++20) garantit qu'une fonction ne sera *jamais* exécutée au runtime. Maîtriser ces qualificateurs est un marqueur de code C++ moderne et performant.

---

## Conventions utilisées dans ce chapitre

Tous les exemples de code sont compilables avec GCC 15 ou Clang 20 en mode C++23 (`-std=c++23`). Lorsqu'une fonctionnalité nécessite un standard antérieur ou postérieur, cela sera indiqué explicitement.

Les sorties console utilisent `std::print` (C++23) comme introduit dans la section 2.7. Si votre compilateur ne le supporte pas encore, remplacez simplement `std::print("valeur : {}\n", x)` par `std::cout << "valeur : " << x << "\n"`.

---

## Plan du chapitre

- **3.1** Le typage statique fort et l'inférence de type
  - 3.1.1 `auto` : Déduction automatique du type
  - 3.1.2 `decltype` : Extraction du type d'une expression
  - 3.1.3 Quand utiliser `auto` vs type explicite
- **3.2** Types primitifs, tailles et représentation mémoire
  - 3.2.1 Entiers : `int`, `long`, `short`, `int32_t`, `int64_t`
  - 3.2.2 Flottants : `float`, `double`, `long double`
  - 3.2.3 `sizeof` et `alignof` : Taille et alignement
- **3.3** Conversion de types : Cast implicite vs explicite
  - 3.3.1 `static_cast` : Conversions sûres
  - 3.3.2 `reinterpret_cast` : Réinterprétation mémoire
  - 3.3.3 `const_cast` : Manipulation de `const`
  - 3.3.4 `dynamic_cast` : Cast polymorphique
- **3.4** Portée des variables (Scope) et durée de vie (Lifetime)
- **3.5** `const`, `constexpr` et `consteval` (C++20)
  - 3.5.1 `const` : Immutabilité à l'exécution
  - 3.5.2 `constexpr` : Calcul à la compilation
  - 3.5.3 `consteval` : Fonctions obligatoirement compile-time

---


⏭️ [Le typage statique fort et l'inférence de type](/03-types-variables-operateurs/01-typage-statique-inference.md)
