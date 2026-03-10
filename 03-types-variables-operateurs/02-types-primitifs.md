🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 3.2 — Types primitifs, tailles et représentation mémoire

> **Chapitre 3 — Types, Variables et Opérateurs** · Section 2 sur 5  
> Prérequis : [3.1 — Le typage statique fort et l'inférence de type](/03-types-variables-operateurs/01-typage-statique-inference.md)

---

## Introduction

Les types primitifs — aussi appelés types fondamentaux ou *built-in types* — sont les briques élémentaires à partir desquelles tout programme C++ est construit. Ce sont les types que le langage fournit nativement, sans qu'aucun `#include` ne soit nécessaire pour les utiliser : entiers, flottants, caractères, booléens.

Contrairement à un langage comme Python, où un entier peut croître indéfiniment et où le programmeur n'a jamais à se soucier de la taille en mémoire d'un nombre, le C++ expose directement la réalité matérielle. Un `int` occupe un nombre précis d'octets, représente une plage de valeurs finie, et est stocké en mémoire selon un agencement dicté par l'architecture du processeur. Comprendre cette réalité n'est pas un détail académique — c'est une nécessité pratique dès que l'on travaille sur du code performant, du code réseau, de la sérialisation binaire ou de la programmation système.

Cette section explore les types primitifs sous trois angles complémentaires : **ce qu'ils représentent** (sémantique), **combien de place ils occupent** (taille), et **comment ils sont disposés en mémoire** (alignement).

---

## Pourquoi la taille des types est importante

En C++, la taille d'un type détermine directement deux choses : la **plage de valeurs** qu'il peut représenter et l'**empreinte mémoire** qu'il consomme. Un `int` de 32 bits peut stocker des valeurs entre −2 147 483 648 et 2 147 483 647. Dépasser cette plage provoque un *integer overflow* — un comportement indéfini pour les types signés, un wrap-around silencieux pour les non-signés. Ni l'un ni l'autre ne produit de message d'erreur à l'exécution.

La taille a aussi un impact direct sur les performances. Les processeurs modernes manipulent les données par blocs alignés : lire un `int` de 4 octets aligné sur une frontière de 4 octets prend un seul cycle mémoire. Lire le même `int` à cheval sur deux lignes de cache peut coûter considérablement plus cher. C'est pourquoi le compilateur insère parfois du *padding* (remplissage) entre les membres d'une structure pour respecter les contraintes d'alignement — un phénomène que `sizeof` et `alignof` permettent d'observer.

---

## Le problème de la portabilité

L'une des particularités du C++ (héritée du C) est que la norme ne fixe **pas** la taille exacte de la plupart des types primitifs. Elle impose uniquement des **garanties minimales** :

- `char` fait au moins 8 bits.
- `short` fait au moins 16 bits.
- `int` fait au moins 16 bits.
- `long` fait au moins 32 bits.
- `long long` fait au moins 64 bits.

La norme garantit aussi la relation d'ordre suivante :

```
sizeof(char) ≤ sizeof(short) ≤ sizeof(int) ≤ sizeof(long) ≤ sizeof(long long)
```

En pratique, sur les plateformes modernes les plus courantes (x86_64 sous Linux avec GCC ou Clang), les tailles sont bien établies : `char` = 1 octet, `short` = 2, `int` = 4, `long` = 8, `long long` = 8. Mais cette réalité n'est pas universelle. Sur Windows 64 bits avec MSVC, `long` vaut 4 octets, pas 8. Sur certaines plateformes embarquées, `int` peut valoir 2 octets.

Cette variabilité signifie que du code écrit en supposant qu'un `int` fait toujours 32 bits peut compiler et fonctionner correctement sur votre machine, puis produire des résultats incorrects sur une autre plateforme. C'est un problème réel dans les contextes suivants :

- **Protocoles réseau** : un message binaire dont un champ est défini comme « un entier de 32 bits » ne peut pas être représenté de manière fiable par un `int`, dont la taille varie.
- **Sérialisation** : écrire un `long` dans un fichier sur Linux (8 octets) et le relire sur Windows (4 octets) corrompra les données.
- **Code multi-plateforme** : un projet compilé avec GCC sur Linux, MSVC sur Windows et Clang sur macOS doit produire le même comportement partout.

La solution à ce problème est l'utilisation des **types à largeur fixe** définis dans `<cstdint>` : `int8_t`, `int16_t`, `int32_t`, `int64_t` et leurs équivalents non signés (`uint8_t`, `uint16_t`, etc.). Ces types garantissent une taille exacte, quelle que soit la plateforme. Nous les examinerons en détail dans la sous-section 3.2.1.

---

## Les catégories de types primitifs

Le C++ organise ses types fondamentaux en plusieurs catégories.

**Types entiers** : `short`, `int`, `long`, `long long`, et leurs variantes `unsigned`. C'est la catégorie la plus riche, avec des dizaines de combinaisons possibles entre taille et signe. Le type `char` est techniquement un type entier (il stocke un nombre), mais il est généralement utilisé pour représenter des caractères. Les types à largeur fixe de `<cstdint>` appartiennent aussi à cette catégorie.

**Types à virgule flottante** : `float` (simple précision, 32 bits), `double` (double précision, 64 bits) et `long double` (précision étendue, taille variable selon la plateforme). Ils suivent le standard IEEE 754 sur la quasi-totalité des plateformes modernes, ce qui offre des garanties fortes sur leur comportement arithmétique.

**Type booléen** : `bool`, qui ne peut prendre que deux valeurs — `true` et `false`. Bien qu'un seul bit suffise théoriquement, `sizeof(bool)` vaut 1 octet sur toutes les plateformes courantes, car l'octet est la plus petite unité adressable en mémoire.

**Type `void`** : un type incomplet qui représente l'absence de valeur. Il ne peut pas être utilisé pour déclarer une variable, mais il sert à indiquer qu'une fonction ne retourne rien (`void f()`) ou à déclarer des pointeurs génériques (`void*`).

**Type `std::nullptr_t`** (C++11) : le type du littéral `nullptr`, utilisé pour représenter un pointeur nul de manière type-safe, en remplacement de l'ancien `NULL` hérité du C.

---

## `sizeof` et `alignof` : deux outils d'observation

Le C++ fournit deux opérateurs compile-time pour inspecter les caractéristiques mémoire des types.

**`sizeof`** retourne la taille en octets d'un type ou d'une expression. C'est un opérateur, pas une fonction — il est évalué à la compilation et n'a aucun coût à l'exécution :

```cpp
std::print("int    : {} octets\n", sizeof(int));      // Typiquement 4  
std::print("double : {} octets\n", sizeof(double));    // Typiquement 8  
std::print("char   : {} octet\n",  sizeof(char));      // Toujours 1 (par définition)  
```

`sizeof(char)` vaut **toujours** 1 — c'est une garantie de la norme. Tous les autres `sizeof` sont exprimés en multiples de `sizeof(char)`.

**`alignof`** (C++11) retourne l'**alignement requis** d'un type, c'est-à-dire la contrainte d'adresse que le processeur impose pour accéder efficacement à une donnée de ce type :

```cpp
std::print("alignof(int)    : {}\n", alignof(int));    // Typiquement 4  
std::print("alignof(double) : {}\n", alignof(double)); // Typiquement 8  
std::print("alignof(char)   : {}\n", alignof(char));   // Toujours 1  
```

Un `alignof(int)` de 4 signifie qu'un `int` doit être stocké à une adresse mémoire multiple de 4 pour un accès optimal. Le compilateur s'en charge automatiquement pour les variables locales et les allocations dynamiques, mais la connaissance de l'alignement est cruciale dès que l'on manipule de la mémoire brute, des structures sérialisées ou des buffers réseau.

Nous reviendrons en profondeur sur `sizeof` et `alignof` dans la sous-section 3.2.3, avec des exemples concrets d'impact sur le layout des structures.

---

## Représentation en mémoire : une vue d'ensemble

Chaque valeur stockée en mémoire n'est en fin de compte qu'une séquence de bits. Ce qui distingue un `int` d'un `float` ou d'un `char`, c'est l'**interprétation** que le processeur donne à ces bits.

**Les entiers signés** utilisent la représentation en **complément à deux** sur toutes les plateformes modernes. Depuis C++20, cette représentation est imposée par la norme — ce n'est plus un détail d'implémentation. Dans cette représentation, le bit de poids fort indique le signe : 0 pour positif, 1 pour négatif. Un `int32_t` contenant la valeur −1 est stocké comme `0xFFFFFFFF` (32 bits tous à 1).

**Les entiers non signés** utilisent une représentation binaire directe. Un `uint32_t` contenant la valeur 255 est stocké comme `0x000000FF`. L'arithmétique non signée est définie modulo 2^N (où N est le nombre de bits), ce qui signifie que le dépassement « boucle » de manière prévisible — contrairement aux entiers signés, pour lesquels le dépassement est un comportement indéfini.

**Les flottants** suivent le standard IEEE 754 : un bit de signe, un exposant biaisé et une mantisse (fraction). Un `float` (32 bits) offre environ 7 chiffres significatifs de précision ; un `double` (64 bits) en offre environ 15-16. La représentation flottante implique que certaines valeurs décimales exactes (comme 0.1) n'ont pas de représentation binaire exacte — une source classique de bugs dans les comparaisons d'égalité entre flottants.

**Les booléens** sont stockés sur un octet entier. La valeur `false` correspond à 0, la valeur `true` correspond à 1. Toute valeur entière non nulle convertie en `bool` donne `true`.

**Les caractères** (`char`) occupent un octet et stockent un code numérique (ASCII pour les 128 premiers, variable au-delà selon l'encodage). La norme C++ ne spécifie pas si `char` est signé ou non — c'est un choix d'implémentation. Pour garantir le signe, utilisez `signed char` ou `unsigned char` explicitement.

---

## Ce que nous allons couvrir

Les trois sous-sections suivantes examinent chaque catégorie en détail :

- **3.2.1 — Entiers : `int`, `long`, `short`, `int32_t`, `int64_t`.** Types entiers signés et non signés, garanties de la norme, types à largeur fixe de `<cstdint>`, promotions et conversions entre types entiers, pièges de l'arithmétique non signée.

- **3.2.2 — Flottants : `float`, `double`, `long double`.** Représentation IEEE 754, précision et erreurs d'arrondi, comparaison de flottants, littéraux et suffixes, choix entre `float` et `double`.

- **3.2.3 — `sizeof` et `alignof` : Taille et alignement.** Inspection des types et des structures, padding et layout mémoire, `alignas` pour contrôler l'alignement, impact sur les performances et la sérialisation.

---


⏭️ [Entiers : int, long, short, int32_t, int64_t](/03-types-variables-operateurs/02.1-entiers.md)
