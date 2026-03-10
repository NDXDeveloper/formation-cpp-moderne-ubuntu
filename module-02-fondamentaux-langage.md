🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Module 2 — Les Fondamentaux du Langage

> 🎯 Niveau : Débutant

Ce module couvre le cœur du langage C++ : système de types, structures de contrôle, fonctions, et surtout gestion de la mémoire. Le chapitre 5 (gestion mémoire) est volontairement le plus dense — comprendre ce qui se passe sur la stack et le heap, savoir manipuler des pointeurs, et détecter les erreurs mémoire avec Valgrind et AddressSanitizer, c'est la compétence fondamentale qui conditionne tout le reste de la formation.

---

## Objectifs pédagogiques

1. **Comprendre** le système de types C++ : typage statique fort, inférence avec `auto` et `decltype`, types primitifs à taille fixe (`int32_t`, `int64_t`), représentation mémoire (`sizeof`, `alignof`).
2. **Maîtriser** les quatre casts C++ (`static_cast`, `reinterpret_cast`, `const_cast`, `dynamic_cast`) et savoir quand chaque cast est approprié.
3. **Utiliser** `const`, `constexpr` et `consteval` (C++20) pour exprimer l'immutabilité et déplacer du calcul à la compilation.
4. **Implémenter** les différents modes de passage de paramètres (valeur, référence, référence constante, pointeur) et choisir le bon selon le contexte.
5. **Comprendre** le modèle mémoire d'un processus Linux : stack, heap, segments text/data/bss, et les caractéristiques de chaque zone.
6. **Utiliser** l'allocation dynamique (`new`/`delete`, `new[]`/`delete[]`) et l'arithmétique des pointeurs.
7. **Détecter** les erreurs mémoire (leaks, dangling pointers, double free) avec Valgrind et AddressSanitizer (`-fsanitize=address`).

---

## Prérequis

- **Module 1** complété : toolchain installée (GCC ou Clang fonctionnel), capacité à compiler un programme et inspecter le binaire résultant.

---

## Chapitres

### Chapitre 3 — Types, Variables et Opérateurs

Le système de types est la première ligne de défense du compilateur. Ce chapitre couvre la déduction de type avec `auto` et `decltype`, les types primitifs et leur représentation mémoire, les conversions (casts), la portée et durée de vie des variables, et les qualificateurs d'immutabilité.

- Inférence de type : `auto` pour la déduction automatique, `decltype` pour extraire le type d'une expression, règles sur quand préférer `auto` vs un type explicite.
- Types primitifs : entiers (`int`, `long`, `int32_t`, `int64_t`), flottants (`float`, `double`, `long double`), taille avec `sizeof`, alignement avec `alignof`.
- Casts C++ : `static_cast` (conversions sûres), `reinterpret_cast` (réinterprétation mémoire), `const_cast` (manipulation de `const`), `dynamic_cast` (cast polymorphique avec vérification runtime).
- Portée (scope) et durée de vie (lifetime) des variables — deux concepts distincts à ne pas confondre.
- `const` (immutabilité runtime), `constexpr` (calcul compile-time), `consteval` (C++20 — fonctions obligatoirement évaluées à la compilation).

### Chapitre 4 — Structures de Contrôle et Fonctions

Structures de contrôle classiques et modernes, puis les fonctions avec un focus sur les modes de passage de paramètres — le choix entre valeur, référence et pointeur est une décision que vous prendrez des centaines de fois par projet.

- Conditionnelles : `if`/`else`, `if constexpr` (résolution à la compilation), `switch` avec initialisation (C++17).
- Boucles : range-based `for` comme idiome par défaut sur les conteneurs.
- Déclaration vs définition de fonctions, séparation header/source.
- Passage de paramètres : par valeur (copie), par référence (`&`), par référence constante (`const &`), par pointeur (`*`) — avec les implications en termes de performance et de sémantique.
- Surcharge de fonctions (function overloading) : résolution par le compilateur selon la signature.
- Valeurs par défaut et fonctions `inline`.

### Chapitre 5 — Gestion de la Mémoire

Le chapitre central de ce module. La gestion mémoire est ce qui distingue C++ des langages avec garbage collector — et c'est aussi la source principale de bugs critiques. Ce chapitre part du modèle mémoire d'un processus, passe par l'allocation dynamique et l'arithmétique des pointeurs, et se termine par les outils de détection d'erreurs.

- Stack vs Heap : diagramme mémoire d'un processus Linux, caractéristiques de chaque zone (taille, vitesse, durée de vie, gestion).
- Allocation dynamique : `new`/`delete` pour les objets individuels, `new[]`/`delete[]` pour les tableaux — les deux paires ne sont pas interchangeables.
- Arithmétique des pointeurs : accès bas niveau, déplacement en mémoire, relation pointeur/tableau.
- Dangers classiques : memory leaks (mémoire allouée jamais libérée), dangling pointers (accès à de la mémoire libérée), double free (libération multiple du même bloc).
- Outils de détection : Valgrind (`memcheck`) pour détecter les fuites et accès invalides, AddressSanitizer (`-fsanitize=address`) pour une détection intégrée au compilateur avec un overhead moindre.

---

## Points de vigilance

- **Signed/unsigned mismatch silencieux.** Comparer un `int` avec un `size_t` (retourné par `std::vector::size()`) ne produit souvent qu'un warning, pas une erreur. Avec `-Wsign-compare` (inclus dans `-Wall`), le compilateur vous le signale. Sans ce flag, un `int` négatif comparé à un `size_t` donne un résultat faux sans aucun avertissement. Utilisez `-Wall -Wextra -Werror` dès le début.
- **`auto` qui déduit une copie au lieu d'une référence.** `auto x = my_vector[0];` déduit une copie, pas une référence. Si vous voulez éviter la copie, écrivez `auto& x = my_vector[0];` ou `const auto& x = my_vector[0];`. C'est un piège fréquent avec les conteneurs et les fonctions qui retournent des références.
- **Confusion `new[]` / `delete` (sans crochets).** Allouer avec `new[]` et libérer avec `delete` (au lieu de `delete[]`) est un undefined behavior. Le programme peut sembler fonctionner dans certains cas et crasher dans d'autres. Valgrind détecte cette erreur — AddressSanitizer aussi si le comportement cause un dépassement.
- **Dangling pointer après retour d'une référence locale.** Retourner une référence ou un pointeur vers une variable locale produit un dangling pointer — la variable est détruite à la sortie de la fonction, et le pointeur/référence pointe vers de la mémoire invalide. Le compilateur émet un warning (`-Wreturn-local-addr` avec GCC), mais seulement dans les cas simples. AddressSanitizer le détecte à l'exécution.

---

## Compétences acquises

À l'issue de ce module, vous savez :
- Utiliser le système de types C++ avec précision : choisir entre `auto` et type explicite, appliquer le bon cast, exploiter `constexpr`/`consteval`.
- Écrire des fonctions avec le mode de passage de paramètres adapté (valeur, référence, `const &`, pointeur).
- Dessiner le diagramme mémoire d'un programme et identifier ce qui vit sur la stack vs le heap.
- Allouer et libérer de la mémoire dynamique sans erreur (`new`/`delete`, `new[]`/`delete[]`).
- Diagnostiquer les erreurs mémoire avec Valgrind et AddressSanitizer.
- Compiler systématiquement avec `-Wall -Wextra -Werror` pour ne laisser passer aucun warning silencieux.

---


⏭️ [Types, Variables et Opérateurs](/03-types-variables-operateurs/README.md)
