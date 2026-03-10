🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 3.1 — Le typage statique fort et l'inférence de type

> **Chapitre 3 — Types, Variables et Opérateurs** · Section 1 sur 5  
> Prérequis : avoir compilé et exécuté un premier programme C++ (section 2.5)

---

## Introduction

La toute première chose que le compilateur C++ fait lorsqu'il rencontre une déclaration de variable, c'est d'en déterminer le **type**. Ce type est fixé à la compilation et ne changera jamais au cours de l'exécution du programme. C'est ce qu'on appelle le **typage statique**. Par ailleurs, C++ est qualifié de langage à typage **fort** : les conversions entre types incompatibles ne sont pas silencieuses — elles provoquent soit une erreur de compilation, soit un avertissement explicite que le programmeur est tenu de traiter.

Cette rigueur contraste avec l'approche de langages à typage dynamique comme Python, où une même variable peut successivement contenir un entier, une chaîne de caractères et une liste sans que l'interpréteur ne proteste. En C++, une variable déclarée comme `int` reste un `int` pour toute sa durée de vie :

```cpp
int count = 42;  
count = "hello"; // ❌ Erreur de compilation : impossible d'affecter  
                 //    un const char* à un int
```

Ce comportement n'est pas une contrainte arbitraire. Il constitue un **contrat** entre le programmeur et le compilateur : « je te dis exactement ce que cette donnée représente, et en échange tu vérifies que je ne fais rien d'incohérent avec ». Ce contrat est la pierre angulaire de la fiabilité et de la performance du C++.

---

## Typage statique : ce que le compilateur vérifie pour vous

Lorsque le compilateur connaît le type de chaque expression, il peut effectuer un ensemble de vérifications qui seraient impossibles dans un langage à typage dynamique.

**Compatibilité des opérations.** Additionner deux `int` est valide. Additionner un `int` et un `std::string` ne l'est pas. Le compilateur le sait et rejette l'opération avant même que le programme ne s'exécute :

```cpp
int a = 10;  
std::string s = "hello";  
auto result = a + s; // ❌ Erreur de compilation : pas d'opérateur + entre int et string  
```

**Vérification des appels de fonctions.** Le compilateur s'assure que chaque argument passé à une fonction correspond au type attendu par le paramètre. Si une fonction attend un `double` et que vous lui passez un `std::vector<int>`, l'erreur est détectée immédiatement :

```cpp
double square_root(double x);

std::vector<int> v = {1, 2, 3};  
auto r = square_root(v); // ❌ Erreur : aucune conversion de vector<int> vers double  
```

**Taille et alignement mémoire.** Connaître le type, c'est connaître la taille exacte de l'objet en mémoire. Le compilateur peut ainsi calculer les offsets dans les structures, aligner les données sur les frontières requises par le processeur et optimiser les accès mémoire.

**Résolution de la surcharge.** C++ autorise plusieurs fonctions portant le même nom mais acceptant des types de paramètres différents (surcharge, couverte au chapitre 4). Le compilateur choisit la bonne version en analysant les types des arguments — un mécanisme qui repose entièrement sur le typage statique.

---

## Typage fort : les conversions ne sont pas gratuites

En C++, le qualificatif « fort » signifie que le langage ne convertit pas silencieusement n'importe quel type vers n'importe quel autre. Il existe bien des **conversions implicites** — par exemple, un `int` peut être promu en `double` dans une expression arithmétique — mais elles suivent des règles précises et le compilateur émet des avertissements lorsqu'une conversion risque de perdre de l'information.

Considérons cet exemple :

```cpp
int large = 100'000;  
short small = large; // ⚠️ Warning : narrowing conversion de int vers short  
```

La valeur `100'000` dépasse la capacité d'un `short` sur la plupart des plateformes (typiquement 16 bits, max 32 767). Le compilateur le détecte et prévient le développeur. Avec l'option `-Werror`, cet avertissement devient une erreur bloquante — une pratique recommandée en production.

Le C++ moderne renforce cette philosophie avec les **conversions restrictives** (*narrowing conversions*) interdites dans les initialisations par accolades :

```cpp
int x = 3.14;   // ⚠️ Compile avec un warning (initialisation classique)  
int y{3.14};    // ❌ Erreur de compilation (initialisation par accolades)  
```

L'initialisation par accolades (`{}`) refuse toute perte de données. C'est pourquoi elle est recommandée par les C++ Core Guidelines comme forme d'initialisation par défaut.

---

## Le problème de la verbosité

Si le typage statique est un atout, il peut aussi devenir un fardeau lorsque le type est évident d'après le contexte. Avant C++11, le programmeur devait répéter explicitement le type dans des situations où le compilateur disposait déjà de toute l'information nécessaire :

```cpp
// Style pré-C++11 : le type est écrit deux fois
std::map<std::string, std::vector<int>>::const_iterator it = my_map.begin();
```

Cette ligne est correcte, mais elle est longue, difficile à lire et pénible à maintenir. Si le type de `my_map` change, il faut mettre à jour la déclaration de `it` manuellement — une source d'erreurs.

Le problème ne se limite pas aux itérateurs. Toute expression dont le type résulte d'un calcul de template peut produire des noms de types extrêmement longs, parfois sur plusieurs lignes. La verbosité nuit alors à la lisibilité sans apporter d'information supplémentaire au lecteur humain.

---

## L'inférence de type : laisser le compilateur travailler

C++11 a introduit une solution élégante à ce problème : **l'inférence de type**. L'idée est simple — si le compilateur peut déduire le type d'une variable à partir de son expression d'initialisation, il n'est pas nécessaire que le programmeur l'écrive explicitement.

Deux mécanismes complémentaires ont été ajoutés au langage :

**`auto`** demande au compilateur de déduire le type d'une variable à partir de son initialiseur. L'exemple précédent devient :

```cpp
// C++11 et après : le compilateur déduit le type
auto it = my_map.begin();
```

Le type de `it` est exactement le même qu'avant — `std::map<std::string, std::vector<int>>::const_iterator` — mais le programmeur n'a plus besoin de l'écrire. Le code est plus court, plus lisible, et il s'adapte automatiquement si le type du conteneur change.

**`decltype`** permet d'extraire le type d'une expression sans l'évaluer. C'est un outil de métaprogrammation qui trouve son utilité dans les templates et les déclarations de types dérivés :

```cpp
int x = 42;  
decltype(x) y = 10; // y est de type int, comme x  
```

Ces deux mécanismes ne suppriment pas le typage statique — ils le rendent **implicite** là où il était **explicite**. Le type existe toujours, il est toujours vérifié à la compilation, il est toujours fixe. Seule l'écriture change.

---

## Un équilibre à trouver

L'inférence de type est un outil puissant, mais comme tout outil, elle peut être mal utilisée. Si `auto` rend le code plus concis lorsque le type est évident, il peut aussi le rendre opaque lorsque le type n'est pas déductible à la lecture :

```cpp
auto result = compute(data); // Quel est le type de result ?
```

Sans connaître la signature de `compute`, le lecteur ne sait pas si `result` est un entier, un flottant, un vecteur ou un pointeur. Dans ce cas, un type explicite améliore la compréhension :

```cpp
std::vector<double> result = compute(data); // Intention claire
```

La communauté C++ a convergé vers un ensemble d'heuristiques pragmatiques pour guider le choix entre `auto` et un type explicite. Herb Sutter les a popularisées sous le nom de *"Almost Always Auto"* (AAA), tandis que d'autres auteurs, comme les rédacteurs des Google C++ Style Guidelines, adoptent une posture plus conservatrice. Nous examinerons ces recommandations en détail dans la sous-section 3.1.3.

---

## Ce que nous allons couvrir

Les trois sous-sections suivantes explorent les mécanismes d'inférence en profondeur :

- **3.1.1 — `auto` : Déduction automatique du type.** Règles de déduction, interaction avec les qualificateurs (`const`, `&`, `&&`), pièges courants et bonnes pratiques.

- **3.1.2 — `decltype` : Extraction du type d'une expression.** Différences avec `auto`, `decltype(auto)` (C++14), et cas d'usage concrets dans les templates et les types de retour.

- **3.1.3 — Quand utiliser `auto` vs type explicite.** Heuristiques de décision, recommandations des style guides, et exemples de code comparés.

---


⏭️ [auto : Déduction automatique du type](/03-types-variables-operateurs/01.1-auto.md)
