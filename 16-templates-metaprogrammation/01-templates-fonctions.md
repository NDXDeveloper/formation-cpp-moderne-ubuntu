🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 16.1 — Templates de fonctions

## Chapitre 16 : Templates et Métaprogrammation · Module 5 : La STL

---

## Introduction

Les templates de fonctions sont le point d'entrée naturel dans la programmation générique en C++. L'idée est simple : plutôt que de réécrire la même logique pour chaque type (`int`, `double`, `std::string`, etc.), on écrit la fonction **une seule fois** en laissant le type en paramètre. Le compilateur se charge ensuite de générer automatiquement une version spécialisée pour chaque type effectivement utilisé dans le programme.

Ce mécanisme est au cœur de la bibliothèque standard. Quand vous appelez `std::sort`, `std::find` ou `std::max`, vous invoquez des templates de fonctions. Comprendre leur fonctionnement, c'est comprendre comment la STL opère en coulisses.

---

## Le problème : la duplication de code

Imaginons que nous ayons besoin d'une fonction qui retourne le maximum de deux valeurs. Sans templates, il faut écrire une version par type :

```cpp
int max_int(int a, int b) {
    return (a > b) ? a : b;
}

double max_double(double a, double b) {
    return (a > b) ? a : b;
}

std::string max_string(const std::string& a, const std::string& b) {
    return (a > b) ? a : b;
}
```

Le corps de chaque fonction est **strictement identique**. Seul le type change. Cette duplication pose plusieurs problèmes : elle est fastidieuse à écrire, difficile à maintenir (une correction de bug doit être propagée dans chaque version) et impossible à étendre à tous les types imaginables. C'est exactement le problème que les templates résolvent.

---

## Syntaxe de base

Un template de fonction se déclare en préfixant la signature avec le mot-clé `template` suivi d'une **liste de paramètres template** entre chevrons `< >` :

```cpp
template <typename T>  
T maximum(T a, T b) {  
    return (a > b) ? a : b;
}
```

Décortiquons cette déclaration :

- `template` — introduit une déclaration template.
- `<typename T>` — déclare un **paramètre de type** nommé `T`. Ce nom est arbitraire ; `T` est une convention, mais `Type`, `Element` ou tout autre identifiant valide fonctionne.
- `T maximum(T a, T b)` — la signature de la fonction utilise `T` comme type de retour et comme type des paramètres.

Le mot-clé `typename` peut être remplacé par `class` dans ce contexte. Les deux sont strictement équivalents pour déclarer un paramètre de type :

```cpp
template <class T>   // Équivalent à template <typename T>  
T maximum(T a, T b) {  
    return (a > b) ? a : b;
}
```

La convention moderne privilégie `typename`, qui exprime plus clairement l'intention. `class` est un héritage historique qui n'implique pas que `T` doive être une classe.

---

## Instanciation d'un template

Un template de fonction n'est pas une fonction. C'est un **modèle** (un patron, un *blueprint*) à partir duquel le compilateur génère de véritables fonctions. Ce processus de génération s'appelle l'**instanciation**.

L'instanciation se produit lorsque le template est utilisé avec un type concret :

```cpp
int main() {
    int a = maximum(3, 7);              // Instancie maximum<int>
    double b = maximum(3.14, 2.71);     // Instancie maximum<double>
    std::string c = maximum(
        std::string("alpha"),
        std::string("beta")
    );                                   // Instancie maximum<std::string>
}
```

Pour chaque combinaison de types distincte, le compilateur génère une fonction indépendante dans le code objet. C'est comme si vous aviez écrit manuellement `maximum_int`, `maximum_double` et `maximum_string` — mais sans la duplication dans le code source.

> **Point clé** — Le code d'un template n'est compilé que lorsqu'il est instancié. Si vous déclarez un template mais ne l'utilisez jamais avec un type donné, aucun code machine n'est généré pour ce type.

---

## Déduction de type (Template Argument Deduction)

Dans l'exemple précédent, nous n'avons pas précisé le type `T` lors de l'appel : le compilateur l'a **déduit** automatiquement à partir des arguments passés. Ce mécanisme s'appelle la **déduction des arguments template** (*template argument deduction*).

```cpp
maximum(3, 7);       // T est déduit comme int (les deux arguments sont int)  
maximum(3.14, 2.71); // T est déduit comme double  
```

La déduction fonctionne en comparant les types des arguments fournis avec les paramètres de la signature du template. Le compilateur cherche une substitution de `T` qui rend les types compatibles.

### Quand la déduction échoue

La déduction échoue si les arguments ne permettent pas de déterminer un type unique et cohérent pour `T` :

```cpp
maximum(3, 2.71);  // ERREUR : T = int (depuis 3) ou T = double (depuis 2.71) ?
```

Le compilateur ne choisit pas à votre place. Il ne fait pas de conversion implicite pendant la déduction. Trois solutions s'offrent à vous.

**Solution 1 : spécifier le type explicitement**

```cpp
maximum<double>(3, 2.71);  // Force T = double, 3 est converti en 3.0
```

**Solution 2 : harmoniser les types à l'appel**

```cpp
maximum(3.0, 2.71);  // Les deux arguments sont double → T = double
```

**Solution 3 : utiliser des paramètres template distincts**

Si la fonction doit naturellement accepter deux types différents, on peut utiliser deux paramètres template. Nous verrons cette approche dans la section sur les templates à paramètres multiples, plus loin dans cette section.

---

## Instanciation explicite

Même si la déduction automatique couvre la majorité des cas, il est toujours possible de spécifier explicitement le ou les types template lors de l'appel :

```cpp
auto r1 = maximum<int>(3, 7);          // Explicite mais redondant ici  
auto r2 = maximum<long>(3, 7);         // Force l'instanciation pour long  
auto r3 = maximum<double>(10, 20);     // Les int sont convertis en double  
```

L'instanciation explicite est indispensable dans certains cas :

- quand la déduction est ambiguë (comme vu ci-dessus) ;
- quand le type de retour ne peut pas être déduit des arguments ;
- quand on souhaite forcer une conversion.

---

## Paramètres template multiples

Un template peut accepter plusieurs paramètres de type. C'est utile quand les arguments de la fonction n'ont pas nécessairement le même type :

```cpp
template <typename T, typename U>  
auto addition(T a, U b) -> decltype(a + b) {  
    return a + b;
}
```

```cpp
auto r1 = addition(3, 2.5);            // T = int, U = double → retourne double  
auto r2 = addition(1.0f, 100L);        // T = float, U = long → retourne float (conversion arithmétique)  
```

Le type de retour utilise ici un **trailing return type** avec `decltype` pour que le compilateur déduise le type du résultat de l'expression `a + b`. En C++14 et au-delà, on peut simplifier en laissant `auto` seul en type de retour :

```cpp
template <typename T, typename U>  
auto addition(T a, U b) {  
    return a + b;  // Le compilateur déduit le type de retour (C++14)
}
```

---

## Paramètres template non-type

Les paramètres template ne sont pas limités aux types. On peut également passer des **valeurs constantes** connues à la compilation. Ces paramètres sont appelés **non-type template parameters** (NTTP) :

```cpp
template <typename T, int N>  
T multiplier(T valeur) {  
    return valeur * N;
}
```

```cpp
auto r1 = multiplier<int, 3>(10);       // Retourne 30  
auto r2 = multiplier<double, 5>(2.4);   // Retourne 12.0  
```

Le paramètre `N` doit être une **constante connue à la compilation**. Il est intégré directement dans le code généré, ce qui permet au compilateur d'optimiser agressivement (déroulement de boucles, précalcul, etc.).

Les types autorisés pour les NTTP sont :

- les types entiers (`int`, `long`, `std::size_t`, etc.) ;
- les types énumérés (`enum`) ;
- les pointeurs et références vers des objets à liaison externe ;
- `auto` pour laisser le compilateur déduire le type du NTTP (C++17) ;
- les types littéraux définis par l'utilisateur sous certaines conditions (C++20).

Un exemple classique de NTTP dans la STL est `std::array` :

```cpp
std::array<int, 5> tableau;  // 5 est un paramètre template non-type
```

### NTTP avec `auto` (C++17)

Depuis C++17, on peut utiliser `auto` pour un paramètre non-type, laissant le compilateur déduire son type :

```cpp
template <auto N>  
void afficher_valeur() {  
    std::print("Valeur : {}, type : {}\n", N, typeid(N).name());
}
```

```cpp
afficher_valeur<42>();     // N est int  
afficher_valeur<'A'>();    // N est char  
afficher_valeur<true>();   // N est bool  
```

Cela offre une flexibilité supplémentaire lorsque le type exact du paramètre n'est pas important, seul le fait qu'il soit une constante à la compilation compte.

---

## Templates et passage par référence

La question du passage par valeur ou par référence se pose naturellement avec les templates. Le template `maximum` présenté plus haut prend ses arguments **par valeur**, ce qui implique des copies. Pour des types légers comme `int` ou `double`, c'est optimal. Pour des types lourds comme `std::string` ou `std::vector`, c'est coûteux.

La solution idiomatique est de passer par **référence constante** :

```cpp
template <typename T>  
const T& maximum(const T& a, const T& b) {  
    return (a > b) ? a : b;
}
```

Ce template fonctionne aussi bien avec des `int` qu'avec des `std::string`, sans copie inutile. Le `const` garantit que la fonction ne modifie pas les arguments.

### Attention aux références sur des temporaires

Le passage par référence constante introduit un piège subtil lorsque la fonction retourne une référence vers un de ses arguments :

```cpp
// Dangereux si utilisé avec des temporaires !
template <typename T>  
const T& maximum(const T& a, const T& b) {  
    return (a > b) ? a : b;
}

// La référence retournée pointe vers un temporaire détruit
const std::string& ref = maximum(std::string("hello"), std::string("world"));
// ref est un dangling reference !
```

Lorsque les arguments sont des temporaires (rvalues), leur durée de vie se termine à la fin de l'expression. La référence retournée devient alors invalide. Pour des fonctions utilitaires simples, le retour par valeur est souvent plus sûr :

```cpp
template <typename T>  
T maximum(const T& a, const T& b) {  // Retour par valeur  
    return (a > b) ? a : b;
}
```

---

## Surcharge de fonctions et templates

Les templates de fonctions coexistent avec les fonctions ordinaires et participent au mécanisme de **résolution de surcharge** (*overload resolution*). Les règles sont les suivantes :

1. Si une **fonction non-template** correspond exactement aux arguments, elle est préférée.
2. Sinon, le compilateur tente d'instancier les templates candidats et sélectionne la meilleure correspondance.

```cpp
// Surcharge non-template pour const char*
const char* maximum(const char* a, const char* b) {
    return (std::strcmp(a, b) > 0) ? a : b;
}

// Template générique
template <typename T>  
T maximum(T a, T b) {  
    return (a > b) ? a : b;
}
```

```cpp
maximum(3, 7);               // Appelle maximum<int> (pas de surcharge non-template pour int)  
maximum("hello", "world");   // Appelle maximum(const char*, const char*) — correspondance exacte  
```

La surcharge non-template pour `const char*` est essentielle ici : sans elle, le template comparerait les **adresses des pointeurs** (l'opérateur `>` sur des pointeurs), pas le contenu des chaînes.

### Forcer l'appel du template

Si vous souhaitez explicitement appeler le template plutôt que la surcharge non-template, utilisez la syntaxe d'instanciation explicite avec des chevrons vides :

```cpp
maximum<>("hello", "world");  // Force l'instanciation du template
```

Les chevrons vides `<>` indiquent au compilateur de ne considérer que les surcharges template, même si une correspondance exacte non-template existe.

---

## Valeur par défaut des paramètres template

Comme pour les paramètres de fonctions, les paramètres template peuvent avoir des **valeurs par défaut** :

```cpp
template <typename T = int>  
T zero() {  
    return T{};  // Value-initialization : 0 pour int, 0.0 pour double, etc.
}
```

```cpp
auto a = zero();        // T = int par défaut → retourne 0  
auto b = zero<double>();// T = double → retourne 0.0  
```

Les valeurs par défaut sont particulièrement utiles pour les paramètres template non-type :

```cpp
template <typename T, int Precision = 2>  
void afficher(T valeur) {  
    std::print("{:.{}f}\n", static_cast<double>(valeur), Precision);
}
```

```cpp
afficher(3.14159);       // Affiche "3.14" (Precision = 2 par défaut)  
afficher<double, 4>(3.14159); // Affiche "3.1416"  
```

Comme pour les arguments par défaut des fonctions, les paramètres template avec valeur par défaut doivent être placés **après** les paramètres sans défaut.

---

## Templates et le modèle de compilation

Un aspect fondamental des templates est leur interaction avec le modèle de compilation C++. Contrairement aux fonctions ordinaires, un template ne peut pas être séparé classiquement entre un fichier d'en-tête (`.h`) et un fichier source (`.cpp`).

### Pourquoi le code doit être visible dans le header

Lorsque le compilateur rencontre un appel comme `maximum<int>(3, 7)`, il doit générer le code de `maximum` spécialisé pour `int`. Pour cela, il a besoin de la **définition complète** du template, pas seulement de sa déclaration.

Or, en compilation séparée, chaque fichier `.cpp` est compilé indépendamment (*translation unit*). Si la définition du template se trouve dans un `.cpp` différent de celui qui l'utilise, le compilateur ne la voit pas et ne peut pas effectuer l'instanciation.

```
// maximum.h — Déclaration seule
template <typename T>  
T maximum(T a, T b);   // Le compilateur ne peut pas instancier sans le corps  

// maximum.cpp — Définition
template <typename T>  
T maximum(T a, T b) {  
    return (a > b) ? a : b;
}

// main.cpp — Utilisation
#include "maximum.h"
int main() {
    maximum(3, 7);  // ERREUR de link : symbole maximum<int> non trouvé
}
```

L'erreur ne se produit pas à la compilation mais à l'**édition de liens** : le linker cherche le symbole `maximum<int>` qui n'a jamais été généré.

### La solution standard : tout dans le header

La pratique universelle est de placer la définition complète du template dans le fichier d'en-tête :

```cpp
// maximum.h — Déclaration ET définition
#pragma once

template <typename T>  
T maximum(T a, T b) {  
    return (a > b) ? a : b;
}
```

Chaque *translation unit* qui inclut `maximum.h` peut alors instancier le template pour les types dont elle a besoin. Le linker se charge ensuite d'éliminer les instanciations dupliquées (*COMDAT folding*).

### Impact sur les temps de compilation

Placer tout le code template dans les headers signifie que ce code est recompilé dans chaque *translation unit* qui l'inclut. Pour de petites fonctions utilitaires, l'impact est négligeable. Pour des templates volumineux (certains composants de Boost, par exemple), cela peut ralentir considérablement la compilation.

Plusieurs stratégies atténuent ce problème :

- **ccache** (section 2.3) : cache les résultats de compilation pour éviter de recompiler du code identique.
- **Instanciation explicite** : on déclare dans le header et on force l'instanciation pour des types spécifiques dans un `.cpp` dédié (voir ci-dessous).
- **Modules C++20** (section 12.13) : les modules évitent le reparsing des headers, réduisant significativement les temps de compilation des templates.

### Instanciation explicite dans un fichier source

Si l'ensemble des types utilisés est connu à l'avance, on peut forcer l'instanciation dans un fichier `.cpp` :

```cpp
// maximum.h — Déclaration seule
#pragma once

template <typename T>  
T maximum(T a, T b);  

// maximum.cpp — Définition + instanciations explicites
template <typename T>  
T maximum(T a, T b) {  
    return (a > b) ? a : b;
}

// Instanciations explicites pour les types nécessaires
template int maximum<int>(int, int);  
template double maximum<double>(double, double);  
template float maximum<float>(float, float);  
```

Cette approche confine la compilation du template à une seule *translation unit*, accélérant les builds. L'inconvénient est qu'il faut lister manuellement chaque instanciation nécessaire. Si un autre fichier tente d'utiliser `maximum<long>` sans l'avoir déclaré, une erreur de link se produira.

---

## Récursion de templates : un premier pas vers la métaprogrammation

Les templates peuvent s'appeler eux-mêmes avec des paramètres différents, ouvrant la porte à la **récursion à la compilation**. C'est l'un des fondements de la métaprogrammation template.

Un exemple classique est le calcul de la puissance entière :

```cpp
template <typename T, int N>  
constexpr T puissance(T base) {  
    if constexpr (N == 0) {
        return T{1};
    } else if constexpr (N < 0) {
        return T{1} / puissance<T, -N>(base);
    } else {
        return base * puissance<T, N - 1>(base);
    }
}
```

```cpp
constexpr auto r1 = puissance<double, 3>(2.0);   // 8.0, calculé à la compilation  
constexpr auto r2 = puissance<int, 0>(42);        // 1  
constexpr auto r3 = puissance<double, -2>(2.0);   // 0.25  
```

Grâce à `if constexpr` (C++17), les branches non sélectionnées ne sont même pas compilées, ce qui simplifie considérablement ce type de code par rapport aux techniques pré-C++17 qui reposaient sur la spécialisation de templates pour gérer les cas de base.

> **Note** — Le mot-clé `constexpr` combiné avec `if constexpr` rend cette approche à la fois lisible et efficace. Pour une couverture approfondie de `constexpr` et `consteval`, voir la section 3.5.

---

## Bonnes pratiques

**Préférer la déduction automatique.** Ne spécifiez pas explicitement les types template quand le compilateur peut les déduire. L'appel `maximum(3, 7)` est plus lisible et maintenable que `maximum<int>(3, 7)`.

**Choisir des noms de paramètres template significatifs.** Pour un ou deux paramètres, `T` et `U` suffisent. Au-delà, des noms expressifs améliorent la lisibilité : `Container`, `Predicate`, `OutputIterator`.

**Passer les types lourds par référence constante.** Un template destiné à fonctionner avec des types arbitraires devrait prendre ses arguments par `const T&` plutôt que par valeur, pour éviter les copies coûteuses.

**Placer les définitions dans les headers.** Sauf raison précise (instanciation explicite pour optimiser les builds), la définition complète du template doit être dans le fichier d'en-tête.

**Limiter la complexité.** Un template de fonction devrait rester simple et ciblé. Si la logique nécessite de nombreuses branches conditionnelles selon le type, c'est un signal qu'il faut envisager la spécialisation (section 16.3) ou les Concepts (section 16.6).

**Tester avec plusieurs types.** Un template compilé avec un seul type peut masquer des erreurs. Testez systématiquement avec des types variés : entiers, flottants, chaînes, types personnalisés.

---

## En résumé

Les templates de fonctions permettent d'écrire une logique unique qui s'adapte à n'importe quel type, sans surcoût à l'exécution. Le compilateur génère du code spécialisé pour chaque type utilisé via le processus d'instanciation. La déduction automatique des arguments rend l'utilisation transparente dans la majorité des cas. Les paramètres template peuvent être des types (`typename T`) ou des valeurs constantes (`int N`), et les templates participent naturellement à la résolution de surcharge aux côtés des fonctions ordinaires.

Ce mécanisme fondamental est la brique sur laquelle reposent les templates de classes (section 16.2), la spécialisation (section 16.3) et, plus largement, l'ensemble de la STL.

⏭️ [Templates de classes](/16-templates-metaprogrammation/02-templates-classes.md)
