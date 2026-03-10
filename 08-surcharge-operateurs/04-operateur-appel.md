🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 8.4 — Opérateur d'appel de fonction (`operator()`)

## Chapitre 8 : Surcharge d'Opérateurs et Conversions · Module 3 : Programmation Orientée Objet

---

## Introduction

Parmi tous les opérateurs surchargeables en C++, `operator()` occupe une place singulière. Il ne correspond à aucune opération mathématique ni à aucune manipulation de données — il transforme un **objet** en quelque chose qui peut être **appelé comme une fonction**. Un tel objet porte le nom de **foncteur** (*function object* ou *functor*).

Pourquoi voudrait-on qu'un objet se comporte comme une fonction ? Parce qu'un objet, contrairement à une fonction libre, peut **porter un état**. Un foncteur peut capturer des paramètres de configuration lors de sa construction, les conserver en mémoire, et les utiliser à chaque appel. C'est cette capacité — combiner comportement appelable et état interne — qui rend les foncteurs indispensables dans la programmation C++ moderne.

Les foncteurs sont le mécanisme fondamental sur lequel reposent les algorithmes de la STL (chapitre 15), et comprendre leur fonctionnement est la clé pour saisir ce que les **lambdas** (chapitre 11) font sous le capot — car une lambda n'est rien d'autre qu'un foncteur généré automatiquement par le compilateur.

---

## Syntaxe de base

`operator()` se déclare comme une fonction membre dont le nom est `operator()`. Il peut accepter un nombre quelconque de paramètres et retourner n'importe quel type :

```cpp
class Salueur {
    std::string formule_;

public:
    explicit Salueur(std::string formule)
        : formule_{std::move(formule)} {}

    // operator() — rend l'objet "appelable"
    void operator()(std::string const& nom) const {
        std::println("{}, {} !", formule_, nom);
    }
};
```

L'utilisation est indiscernable d'un appel de fonction classique :

```cpp
Salueur bonjour{"Bonjour"};  
Salueur salut{"Salut"};  

bonjour("Alice");    // Bonjour, Alice !  
salut("Bob");        // Salut, Bob !  
```

`bonjour("Alice")` est syntaxiquement identique à l'appel d'une fonction `bonjour` avec l'argument `"Alice"`. Mais `bonjour` n'est pas une fonction — c'est un objet de type `Salueur`, et l'appel est résolu vers `bonjour.operator()("Alice")`.

### Contraintes syntaxiques

- `operator()` **doit** être une fonction membre (pas une fonction libre). Depuis C++23, il peut être déclaré `static` (utile pour les foncteurs sans état et les lambdas sans capture).
- Il peut être surchargé avec des signatures différentes (plusieurs `operator()` dans la même classe).
- Il peut être `const`, `noexcept`, ou même template (voir plus bas).
- Il n'y a **pas de limite** sur le nombre ou le type des paramètres.

---

## Foncteurs vs fonctions libres : l'avantage de l'état

Une fonction libre ne peut pas capturer de contexte (sauf via des variables globales, ce qui est déconseillé). Un foncteur encapsule son état comme n'importe quel objet :

```cpp
// Fonction libre — pas d'état configurable
bool est_superieur_a_10(int x) {
    return x > 10;   // seuil codé en dur
}

// Foncteur — seuil configurable
class SuperieurA {
    int seuil_;
public:
    explicit SuperieurA(int seuil) : seuil_{seuil} {}

    bool operator()(int x) const {
        return x > seuil_;
    }
};
```

Le foncteur `SuperieurA` peut être configuré à la construction pour n'importe quel seuil :

```cpp
SuperieurA sup5{5};  
SuperieurA sup100{100};  

std::println("{}", sup5(7));     // true  
std::println("{}", sup5(3));     // false  
std::println("{}", sup100(42));  // false  
```

C'est exactement cette capacité de **paramétrage** qui rend les foncteurs essentiels pour les algorithmes STL :

```cpp
#include <algorithm>
#include <vector>

std::vector<int> nombres = {3, 7, 12, 5, 18, 1, 9};

// Avec la fonction libre — seuil figé
auto it1 = std::find_if(nombres.begin(), nombres.end(), est_superieur_a_10);

// Avec le foncteur — seuil configurable
auto it2 = std::find_if(nombres.begin(), nombres.end(), SuperieurA{15});
```

Le foncteur `SuperieurA{15}` est un **objet temporaire** passé à `std::find_if`. L'algorithme appelle `operator()` sur cet objet pour chaque élément du vecteur.

---

## Foncteurs dans la bibliothèque standard

La STL fournit un ensemble de foncteurs prédéfinis dans le header `<functional>`. Ce sont des templates de classes avec un `operator()` :

```cpp
#include <functional>
#include <vector>
#include <algorithm>

std::vector<int> v = {5, 2, 8, 1, 9};

// std::greater<int> est un foncteur : operator()(int a, int b) { return a > b; }
std::sort(v.begin(), v.end(), std::greater<int>{});
// v = {9, 8, 5, 2, 1}

// std::plus<int> : operator()(int a, int b) { return a + b; }
int somme = std::accumulate(v.begin(), v.end(), 0, std::plus<int>{});
```

Depuis C++14, ces foncteurs supportent la **déduction de type** avec le paramètre `<>` vide (ou omis) — ce sont les versions "transparentes" :

```cpp
std::sort(v.begin(), v.end(), std::greater<>{});   // C++14 : type déduit  
std::sort(v.begin(), v.end(), std::greater{});     // C++17 : CTAD  
```

Les foncteurs standard les plus utilisés :

| Foncteur | Opération | Usage typique |
|---|---|---|
| `std::less<>` | `a < b` | Tri croissant (défaut de `std::sort`) |
| `std::greater<>` | `a > b` | Tri décroissant |
| `std::equal_to<>` | `a == b` | Recherche d'égalité |
| `std::plus<>` | `a + b` | Accumulation / réduction |
| `std::multiplies<>` | `a * b` | Produit / réduction |
| `std::negate<>` | `-a` | Négation unaire |
| `std::hash<T>` | Hash de `a` | Tables de hachage (`unordered_map`) |

---

## Foncteurs avec état mutable

Parfois, le foncteur a besoin de modifier son état entre les appels. Dans ce cas, `operator()` ne peut pas être `const` :

```cpp
class Compteur {
    int compte_ = 0;

public:
    int operator()(int x) {   // non-const : modifie l'état
        ++compte_;
        return x * compte_;
    }

    int total() const { return compte_; }
};
```

```cpp
Compteur c;  
std::println("{}", c(10));   // 10  (compte_ = 1)  
std::println("{}", c(10));   // 20  (compte_ = 2)  
std::println("{}", c(10));   // 30  (compte_ = 3)  
std::println("Appels : {}", c.total());  // 3  
```

> ⚠️ **Attention avec les algorithmes STL.** Le standard ne garantit pas le nombre de copies qu'un algorithme effectue de votre foncteur. Si votre foncteur est copié, chaque copie a son propre état. Pour un foncteur à état mutable, utilisez `std::ref` pour passer une référence :
>
> ```cpp
> Compteur compteur;
> std::for_each(v.begin(), v.end(), std::ref(compteur));
> // compteur.total() reflète bien tous les appels
> ```
>
> Sans `std::ref`, `std::for_each` pourrait travailler sur une copie de `compteur`, et l'original ne serait pas modifié.

---

## Surcharges multiples de `operator()`

Un foncteur peut proposer plusieurs versions de `operator()` avec des signatures différentes — c'est un cas d'utilisation parfaitement valide de la surcharge de fonctions :

```cpp
class Formateur {  
public:  
    std::string operator()(int valeur) const {
        return std::format("{}", valeur);
    }

    std::string operator()(double valeur) const {
        return std::format("{:.2f}", valeur);
    }

    std::string operator()(std::string const& valeur) const {
        return std::format("\"{}\"", valeur);
    }
};
```

```cpp
Formateur fmt;  
std::println("{}", fmt(42));         // 42  
std::println("{}", fmt(3.14159));    // 3.14  
std::println("{}", fmt("hello"));   // "hello"  
```

Ce pattern est utilisé intensivement avec `std::visit` et `std::variant` (section 12.2). Un foncteur avec plusieurs `operator()` surchargés est souvent appelé un **visiteur** :

```cpp
#include <variant>

struct Afficheur {
    void operator()(int i) const    { std::println("int: {}", i); }
    void operator()(double d) const { std::println("double: {:.2f}", d); }
    void operator()(std::string const& s) const { std::println("string: {}", s); }
};

std::variant<int, double, std::string> v = 3.14;  
std::visit(Afficheur{}, v);   // double: 3.14  
```

---

## `operator()` template : le foncteur générique

Depuis C++14 (avec les lambdas génériques) et de manière explicite avec les templates, un foncteur peut accepter **n'importe quel type** :

```cpp
struct Afficheur {
    template <typename T>
    void operator()(T const& valeur) const {
        std::println("{}", valeur);
    }
};
```

C'est exactement ce que le compilateur génère quand vous écrivez une lambda avec `auto` :

```cpp
auto afficheur = [](auto const& valeur) {
    std::println("{}", valeur);
};
```

La lambda ci-dessus est transformée par le compilateur en une classe anonyme avec un `operator()` template — un foncteur générique. Cette équivalence est explorée en détail au chapitre 11.

---

## Foncteurs et lambdas : le lien

Les lambdas (chapitre 11) ne sont que du **sucre syntaxique** pour les foncteurs. Le compilateur transforme chaque expression lambda en une classe anonyme avec un `operator()`. Comprendre les foncteurs, c'est comprendre ce qui se passe sous le capot d'une lambda.

Voici l'équivalence exacte :

```cpp
// Lambda avec capture
int seuil = 10;  
auto filtre = [seuil](int x) { return x > seuil; };  
```

Le compilateur génère quelque chose de sémantiquement équivalent à :

```cpp
// Foncteur généré par le compilateur (simplifié)
class __lambda_anonyme {
    int seuil_;   // capture par valeur
public:
    __lambda_anonyme(int seuil) : seuil_{seuil} {}

    bool operator()(int x) const {    // const par défaut
        return x > seuil_;
    }
};

auto filtre = __lambda_anonyme{seuil};
```

Les captures de la lambda deviennent des **membres** du foncteur. Le corps de la lambda devient le **corps de `operator()`**. La capture par valeur correspond à un membre copié, la capture par référence à un membre de type référence.

Cette équivalence a des implications pratiques. Quand une lambda capture par valeur et que vous voulez modifier la capture, vous devez déclarer la lambda `mutable` — ce qui correspond à retirer le `const` de l'`operator()` du foncteur sous-jacent :

```cpp
auto compteur = [n = 0]() mutable { return ++n; };
//                                   ^^^^^^^ rend operator() non-const
```

---

## Cas d'usage avancé : les foncteurs comme politiques

Les foncteurs sont au cœur du pattern de **politique** (*policy-based design*), où le comportement d'une classe ou d'un algorithme est paramétré par un type foncteur :

```cpp
template <typename Politique>  
class Cache {  
    std::unordered_map<std::string, std::string> data_;
    Politique politique_;

public:
    explicit Cache(Politique pol = {}) : politique_{std::move(pol)} {}

    void inserer(std::string const& cle, std::string const& valeur) {
        if (politique_(cle, valeur)) {   // la politique décide si on met en cache
            data_[cle] = valeur;
        }
    }

    // ...
};

// Politique : tout mettre en cache
struct ToutCacher {
    bool operator()(std::string const&, std::string const&) const {
        return true;
    }
};

// Politique : ne cacher que les petites valeurs
struct PetitesValeurs {
    std::size_t max_taille_;
    explicit PetitesValeurs(std::size_t max) : max_taille_{max} {}

    bool operator()(std::string const&, std::string const& valeur) const {
        return valeur.size() <= max_taille_;
    }
};
```

```cpp
Cache<ToutCacher> cache_complet;  
Cache<PetitesValeurs> cache_leger{PetitesValeurs{1024}};  
```

Le type du foncteur est un paramètre template — le compilateur génère du code spécialisé pour chaque politique, et l'appel à `operator()` est résolu à la compilation et inliné. C'est du **polymorphisme statique** sans aucun surcoût à l'exécution (contrairement au polymorphisme dynamique de la section 7.5).

Ce pattern est omniprésent dans la STL : `std::map` accepte un `Compare` (par défaut `std::less<Key>`), `std::unordered_map` accepte un `Hash` et un `KeyEqual`, `std::sort` accepte un comparateur quelconque. Tous ces paramètres sont des foncteurs.

---

## `std::function` : effacement de type pour les foncteurs

Les foncteurs posent un problème pratique : chaque foncteur a un **type différent**. Un `SuperieurA` n'est pas du même type qu'un `PetitesValeurs`, ni qu'une lambda. Comment stocker des foncteurs hétérogènes dans une même variable ou collection ?

C'est le rôle de `std::function` (section 11.4), qui effectue un **effacement de type** (*type erasure*) :

```cpp
#include <functional>

std::function<bool(int)> predicat;

// Peut contenir un foncteur :
predicat = SuperieurA{10};  
std::println("{}", predicat(15));   // true  

// Ou une lambda :
predicat = [](int x) { return x % 2 == 0; };  
std::println("{}", predicat(15));   // false  

// Ou une fonction libre :
bool est_positif(int x) { return x > 0; }  
predicat = est_positif;  
std::println("{}", predicat(15));   // true  
```

`std::function<bool(int)>` peut contenir **n'importe quel callable** qui accepte un `int` et retourne un `bool` — foncteur, lambda, fonction libre, pointeur de méthode. C'est le polymorphisme appliqué aux callables.

Le prix est un surcoût à l'exécution (allocation dynamique possible, indirection similaire à un appel virtuel). Pour les chemins critiques, les templates (comme dans le pattern de politique) sont préférables. `std::function` est l'outil adapté quand le type concret du callable ne peut pas être connu à la compilation — callbacks, conteneurs de handlers, injection de dépendances.

---

## Bonnes pratiques

**Marquez `operator()` comme `const` quand le foncteur ne modifie pas son état.** C'est le cas par défaut pour les prédicats, les comparateurs et la plupart des foncteurs. Les lambdas sans `mutable` génèrent un `operator() const` — suivez la même convention.

**Préférez les lambdas aux foncteurs pour le code local et ponctuel.** Si le callable n'est utilisé qu'une fois, une lambda est plus concise et plus lisible qu'une classe foncteur. Réservez les classes foncteurs pour le code réutilisable, les politiques de template, ou les cas où vous avez besoin d'un état complexe avec des méthodes auxiliaires.

**Utilisez `std::ref` pour passer un foncteur à état mutable aux algorithmes STL.** Sans cela, l'algorithme peut travailler sur une copie et votre foncteur original ne reflètera pas les modifications.

**Nommez vos foncteurs de manière descriptive.** `SuperieurA`, `EstPair`, `FormateurJSON` sont de bons noms. Un foncteur est une classe — il mérite un nom qui décrit son rôle.

**Préférez les foncteurs/lambdas aux pointeurs de fonctions.** Les pointeurs de fonctions ne peuvent pas être inlinés par le compilateur (ils sont opaques à l'optimisation), contrairement aux foncteurs dont le type est connu au moment de la compilation. C'est une des raisons pour lesquelles `std::sort` avec un foncteur ou une lambda est souvent plus rapide que `qsort` de la bibliothèque C.

---

## Résumé

| Concept | Description |
|---|---|
| **Foncteur** | Objet avec `operator()` — peut être appelé comme une fonction |
| **État interne** | Un foncteur peut capturer des données à la construction, contrairement à une fonction libre |
| **`operator()` const** | Convention par défaut — le foncteur ne modifie pas son état |
| **`operator()` mutable** | Le foncteur modifie son état entre les appels |
| **Surcharges multiples** | Plusieurs `operator()` avec des signatures différentes — pattern visiteur |
| **`operator()` template** | Foncteur générique — accepte tout type |
| **Lambda** | Sucre syntaxique pour un foncteur anonyme généré par le compilateur |
| **Foncteurs STL** | `std::less`, `std::greater`, `std::hash`, etc. — dans `<functional>` |
| **`std::function`** | Type erasure pour stocker n'importe quel callable avec une signature donnée |
| **Policy-based design** | Foncteurs comme paramètres template — polymorphisme statique sans surcoût |

---


⏭️ [Opérateur spaceship <=> (C++20)](/08-surcharge-operateurs/05-operateur-spaceship.md)
