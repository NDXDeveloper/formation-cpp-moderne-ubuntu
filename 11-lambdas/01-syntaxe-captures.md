🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 11.1 — Syntaxe des lambdas et types de captures

## Objectif de cette section

Cette section décortique la syntaxe complète d'une expression lambda en C++ et présente en détail le mécanisme de **capture**, qui est le cœur de leur puissance. À l'issue de cette section et de ses sous-sections (11.1.1 à 11.1.4), vous saurez choisir le bon mode de capture pour chaque situation, anticiper les problèmes de durée de vie, et éviter les pièges classiques liés aux captures par référence.

---

## Anatomie complète d'une lambda

Une expression lambda en C++ suit une syntaxe précise dont chaque élément est optionnel ou obligatoire selon le contexte :

```
[captures](paramètres) mutable constexpr noexcept -> type_retour { corps }
```

Décomposons chaque partie :

```cpp
//  1        2            3       4         5           6              7
//  |        |            |       |         |           |              |
    [captures](paramètres) mutable constexpr noexcept -> type_retour { corps }
```

**1. `[captures]`** — La clause de capture. C'est l'élément distinctif des lambdas. Elle définit quelles variables du contexte englobant sont accessibles dans le corps de la lambda, et comment elles le sont (par copie ou par référence). Les crochets `[]` sont **toujours obligatoires**, même vides.

**2. `(paramètres)`** — La liste de paramètres, identique à celle d'une fonction ordinaire. Elle peut être omise entièrement si la lambda ne prend aucun argument et qu'aucun des spécificateurs suivants n'est utilisé. En pratique, on écrit souvent `()` même sans paramètre, par clarté.

**3. `mutable`** *(optionnel)* — Par défaut, l'`operator()` généré par le compilateur est `const`, ce qui signifie que les captures par valeur ne peuvent pas être modifiées. Le mot-clé `mutable` lève cette restriction.

**4. `constexpr`** *(optionnel, C++17)* — Indique que la lambda peut être évaluée à la compilation. Depuis C++17, les lambdas sont implicitement `constexpr` lorsque c'est possible, mais le spécifier explicitement rend l'intention claire et provoque une erreur de compilation si la contrainte ne peut être satisfaite.

**5. `noexcept`** *(optionnel)* — Garantit que la lambda ne lèvera pas d'exception. Utile pour les optimisations du compilateur et pour les conteneurs de la STL qui peuvent exploiter cette garantie (voir section 17.4).

**6. `-> type_retour`** *(optionnel)* — Le type de retour explicite (*trailing return type*). Il est déduit automatiquement lorsque le corps ne contient qu'un seul `return` ou lorsque tous les `return` retournent le même type. La spécification explicite est nécessaire lorsque la déduction est ambiguë.

**7. `{ corps }`** — Le corps de la lambda, identique au corps de n'importe quelle fonction.

### Exemples de syntaxe progressive

La forme la plus minimale — une lambda sans capture, sans paramètre, sans valeur de retour :

```cpp
auto greet = [] { std::print("Hello\n"); };  
greet();  // Affiche : Hello  
```

Ajout de paramètres :

```cpp
auto add = [](int a, int b) { return a + b; };  
std::print("{}\n", add(3, 4));  // Affiche : 7  
```

Le type de retour est déduit comme `int` car l'expression `a + b` avec deux `int` produit un `int`. Si la déduction est ambiguë ou si on souhaite forcer un type particulier, on utilise le trailing return type :

```cpp
auto divide = [](int a, int b) -> double {
    return static_cast<double>(a) / b;
};
std::print("{}\n", divide(7, 2));  // Affiche : 3.5
```

Avec `noexcept` et `constexpr` :

```cpp
auto square = [](int x) constexpr noexcept -> int {
    return x * x;
};

// Utilisable à la compilation
static_assert(square(5) == 25);
```

---

## Le type unique d'une lambda

Chaque expression lambda produit un objet dont le type est **unique et anonyme**, généré par le compilateur. Deux lambdas ayant exactement le même corps et les mêmes captures ont pourtant des types distincts :

```cpp
auto f = [](int x) { return x + 1; };  
auto g = [](int x) { return x + 1; };  

// f et g ont des types différents !
// static_assert(std::is_same_v<decltype(f), decltype(g)>);  // Erreur : false
```

Cette caractéristique a des conséquences pratiques directes :

- **`auto` est le moyen naturel** de stocker une lambda. On ne peut pas écrire son type explicitement puisqu'il est anonyme.
- **On ne peut pas déclarer un paramètre de fonction** avec le type d'une lambda spécifique. Pour accepter une lambda en paramètre, on utilise soit un **template**, soit `std::function` (voir section 11.4).
- **Chaque lambda a une taille propre**, qui dépend de ce qu'elle capture. Une lambda sans capture a une taille de 1 octet (le minimum pour avoir une adresse unique). Une lambda capturant un `std::string` par valeur aura au moins la taille de ce `std::string`.

```cpp
auto empty = [] {};  
auto with_int = [x = 42] {};  
auto with_str = [s = std::string("hello")] {};  

std::print("Taille lambda vide     : {} octets\n", sizeof(empty));     // 1  
std::print("Taille lambda avec int : {} octets\n", sizeof(with_int));  // 4  
std::print("Taille lambda avec str : {} octets\n", sizeof(with_str));  // 32 (typique)  
```

---

## Lambdas sans capture et conversion en pointeur de fonction

Les lambdas **sans capture** (clause `[]` vide) bénéficient d'une propriété spéciale : elles sont implicitement convertibles en **pointeur de fonction**. Cette conversion est possible parce qu'une lambda sans capture ne dépend d'aucun état — elle se comporte exactement comme une fonction libre.

```cpp
auto lambda = [](int a, int b) { return a + b; };

// Conversion implicite en pointeur de fonction
int (*fn_ptr)(int, int) = lambda;

std::print("{}\n", fn_ptr(3, 4));  // Affiche : 7
```

Cette propriété est essentielle pour l'interopérabilité avec le code C et les API qui attendent des pointeurs de fonction (callbacks C, `qsort`, API POSIX, etc.) :

```cpp
#include <cstdlib>

// qsort attend un pointeur de fonction de type int(*)(const void*, const void*)
void sort_c_style(int* arr, size_t n) {
    std::qsort(arr, n, sizeof(int), [](const void* a, const void* b) -> int {
        int ia = *static_cast<const int*>(a);
        int ib = *static_cast<const int*>(b);
        return (ia > ib) - (ia < ib);
    });
}
```

Dès qu'une lambda **capture** quoi que ce soit, cette conversion n'est plus possible — la lambda porte un état interne, ce qu'un simple pointeur de fonction ne peut pas représenter :

```cpp
int offset = 10;  
auto with_capture = [offset](int x) { return x + offset; };  

// int (*ptr)(int) = with_capture;  // ERREUR de compilation
```

---

## Vue d'ensemble des modes de capture

La clause de capture `[...]` est le mécanisme par lequel une lambda accède aux variables de sa portée englobante. C++ offre une granularité fine pour contrôler **quelles** variables sont capturées et **comment** elles le sont.

Voici un résumé des modes disponibles :

```cpp
int x = 10;  
int y = 20;  
int z = 30;  

// Aucune capture — pas d'accès aux variables locales
auto a = []         { /* x, y, z inaccessibles */ };

// Capture explicite par valeur — copie de x
auto b = [x]        { return x; };

// Capture explicite par référence — référence vers x
auto c = [&x]       { return x; };

// Capture par défaut — toutes par valeur
auto d = [=]        { return x + y + z; };

// Capture par défaut — toutes par référence
auto e = [&]        { return x + y + z; };

// Capture mixte — par défaut valeur, sauf y par référence
auto f = [=, &y]    { return x + y + z; };

// Capture mixte — par défaut référence, sauf x par valeur
auto g = [&, x]     { return x + y + z; };

// Init capture (C++14) — crée une nouvelle variable dans la lambda
auto h = [val = x * 2] { return val; };

// Init capture avec move (C++14) — transfert de propriété
auto ptr = std::make_unique<int>(42);  
auto i = [p = std::move(ptr)] { return *p; };  
```

### Tableau récapitulatif

| Syntaxe | Signification | Standard |
|---|---|---|
| `[]` | Aucune capture | C++11 |
| `[x]` | Capture `x` par valeur (copie) | C++11 |
| `[&x]` | Capture `x` par référence | C++11 |
| `[=]` | Capture toutes les variables utilisées par valeur | C++11 |
| `[&]` | Capture toutes les variables utilisées par référence | C++11 |
| `[=, &x]` | Toutes par valeur, sauf `x` par référence | C++11 |
| `[&, x]` | Toutes par référence, sauf `x` par valeur | C++11 |
| `[this]` | Capture le pointeur `this` (accès aux membres) | C++11 |
| `[*this]` | Capture une **copie** de l'objet `*this` | C++17 |
| `[x = expr]` | Init capture : `x` initialisé par `expr` | C++14 |
| `[&x = ref]` | Init capture par référence | C++14 |

### Règles de syntaxe de la clause de capture

Quelques règles à respecter dans la composition de la clause de capture :

Une capture par défaut (`=` ou `&`) doit apparaître **en premier** si elle est présente :

```cpp
auto ok  = [=, &x] { return x; };   // Correct : défaut puis exception
// auto bad = [&x, =] { ... };      // ERREUR : défaut pas en premier
```

On ne peut pas capturer une variable dans le même mode que le défaut — ce serait redondant et le compilateur le refuse :

```cpp
// auto bad = [=, x] { return x; };    // ERREUR : x déjà capturé par = 
// auto bad = [&, &x] { return x; };   // ERREUR : x déjà capturé par &
```

Les deux captures par défaut ne peuvent pas coexister :

```cpp
// auto bad = [=, &] { ... };  // ERREUR : deux défauts
```

---

## Le moment de la capture

Un point souvent source de confusion : **la capture a lieu au moment de la création de la lambda**, pas au moment de son appel. Pour les captures par valeur, c'est la valeur au moment de la définition qui est copiée :

```cpp
int value = 10;  
auto snapshot = [value] { return value; };  

value = 42;  // Modification après la création de la lambda

std::print("{}\n", snapshot());  // Affiche : 10 (pas 42 !)
```

Pour les captures par référence, la lambda accède toujours à la variable originale — elle voit donc les modifications ultérieures :

```cpp
int value = 10;  
auto live = [&value] { return value; };  

value = 42;

std::print("{}\n", live());  // Affiche : 42
```

Cette distinction est fondamentale et explique pourquoi les captures par référence sont dangereuses lorsque la lambda survit à la portée de la variable capturée.

---

## Le mot-clé `mutable`

Par défaut, l'`operator()` d'une lambda est `const`. Cela signifie que les captures par valeur sont en lecture seule dans le corps de la lambda :

```cpp
int counter = 0;
// auto inc = [counter]() { return ++counter; };  // ERREUR : counter est const
```

Le mot-clé `mutable` lève cette restriction en rendant l'`operator()` non-const :

```cpp
int counter = 0;  
auto inc = [counter]() mutable { return ++counter; };  

std::print("{}\n", inc());  // Affiche : 1  
std::print("{}\n", inc());  // Affiche : 2  
std::print("{}\n", inc());  // Affiche : 3  

// La variable originale n'est PAS modifiée — c'est la copie interne qui l'est
std::print("{}\n", counter);  // Affiche : 0
```

Il est important de comprendre que `mutable` ne modifie pas la variable originale — la lambda possède sa **propre copie** et c'est cette copie qu'elle modifie. La variable `counter` dans la portée englobante reste à `0`.

L'analogie avec les foncteurs rend ce comportement naturel :

```cpp
// Équivalent foncteur de la lambda mutable ci-dessus
class __lambda {
    int counter;  // Membre non-const
public:
    __lambda(int c) : counter(c) {}
    int operator()() { return ++counter; }  // Non-const grâce à mutable
};
```

---

## Dangling references : le piège majeur des captures

Le risque le plus sérieux avec les lambdas est la **capture par référence d'une variable dont la durée de vie est plus courte que celle de la lambda**. Lorsque la lambda est invoquée après la destruction de la variable capturée, on obtient un comportement indéfini (*undefined behavior*) — un *dangling reference* :

```cpp
// ⚠️ DANGER : ce code contient un comportement indéfini
std::function<int()> create_counter() {
    int count = 0;
    return [&count]() { return ++count; };  // count sera détruit !
}

auto counter = create_counter();
// counter();  // UNDEFINED BEHAVIOR : count n'existe plus
```

La variable `count` vit sur la stack de `create_counter`. Lorsque cette fonction retourne, `count` est détruite, mais la lambda conserve une référence vers elle. Tout appel ultérieur accède à de la mémoire invalide.

La solution est de capturer par **valeur** lorsque la lambda doit survivre à la portée de la variable :

```cpp
// ✅ Correct : capture par valeur
std::function<int()> create_counter() {
    int count = 0;
    return [count]() mutable { return ++count; };
}

auto counter = create_counter();  
std::print("{}\n", counter());  // 1  
std::print("{}\n", counter());  // 2  
```

### Règle pratique

La règle à retenir est directe :

- **La lambda est utilisée localement** (dans la même portée, passée à un algorithme STL qui s'exécute immédiatement) → la capture par référence `[&]` est sûre et efficace.
- **La lambda est stockée, retournée, ou passée à un thread** → la capture par valeur `[=]` ou par init capture avec `std::move` est le choix sûr.

```cpp
void safe_local_usage(std::vector<int>& data, int threshold) {
    // ✅ Capture par référence — exécution immédiate, threshold est vivant
    std::erase_if(data, [&threshold](int v) { return v < threshold; });
}

auto safe_deferred_usage(int initial) {
    // ✅ Capture par valeur — la lambda survit à la portée
    return [initial]() { return initial * 2; };
}
```

---

## Lambdas et `constexpr` (C++17)

Depuis C++17, les lambdas peuvent être utilisées dans des contextes `constexpr` — elles sont évaluables à la compilation si leur corps le permet :

```cpp
constexpr auto square = [](int x) { return x * x; };

// Évaluation à la compilation
static_assert(square(5) == 25);  
constexpr int result = square(7);  

// Évaluation à l'exécution — même lambda
int runtime_val = 6;  
std::print("{}\n", square(runtime_val));  // 36  
```

En C++17, les lambdas sont **implicitement** `constexpr` lorsque c'est possible. Ajouter `constexpr` explicitement sert à garantir que toute modification qui rendrait la lambda non-constexpr provoquera une erreur de compilation.

C++20 étend cette capacité en permettant les lambdas `consteval`, qui doivent **obligatoirement** être évaluées à la compilation :

```cpp
auto compile_only = [](int x) consteval { return x * x; };

constexpr int ok = compile_only(5);  // ✅ Évaluation compile-time
// int bad = compile_only(runtime_val);  // ERREUR : pas évaluable à la compilation
```

---

## Lambdas et inférence de type retour

Le compilateur déduit le type de retour d'une lambda automatiquement dans la grande majorité des cas. La déduction suit les mêmes règles que `auto` pour les fonctions :

```cpp
// Retour déduit : int
auto add = [](int a, int b) { return a + b; };

// Retour déduit : double (promotion)
auto mixed = [](int a, double b) { return a + b; };
```

La déduction échoue — ou produit un résultat inattendu — lorsque les branches de retour ont des types incompatibles :

```cpp
// ERREUR : types de retour incompatibles (int vs double)
// auto ambiguous = [](bool flag) {
//     if (flag) return 42;
//     else return 3.14;
// };

// ✅ Solution : spécifier le type de retour
auto fixed = [](bool flag) -> double {
    if (flag) return 42;
    else return 3.14;
};
```

Le trailing return type est également utile lorsqu'on veut retourner une **référence** — sans lui, `auto` déduit toujours un type valeur :

```cpp
std::string name = "Alice";

// Retourne une COPIE (auto déduit std::string)
auto by_value = [&name]() { return name; };

// Retourne une RÉFÉRENCE (trailing return type explicite)
auto by_ref = [&name]() -> const std::string& { return name; };
```

---

## Plan des sous-sections

Les quatre sous-sections qui suivent explorent chaque mode de capture en profondeur, avec des exemples pratiques, des pièges à éviter, et des recommandations :

- **11.1.1 — Capture par valeur `[=]`** : copie des variables, comportement `const` par défaut, coût de la copie, quand l'utiliser.
- **11.1.2 — Capture par référence `[&]`** : accès direct, mutation, dangling references, guidelines de sécurité.
- **11.1.3 — Capture de `this`** : accès aux membres dans les méthodes, `[this]` vs `[*this]`, durée de vie.
- **11.1.4 — Captures mixtes et init captures** : granularité fine, captures généralisées C++14, déplacement de `std::unique_ptr`, et patterns avancés.

---


⏭️ [Capture par valeur](/11-lambdas/01.1-capture-valeur.md)
