🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 11.2 — Lambdas génériques (C++14) et templated lambdas

## Le problème : lambdas et types figés en C++11

En C++11, les paramètres d'une lambda doivent avoir un type explicite. Chaque lambda est écrite pour un ensemble de types spécifique :

```cpp
auto add_int = [](int a, int b) { return a + b; };
auto add_double = [](double a, double b) { return a + b; };
auto add_string = [](const std::string& a, const std::string& b) { return a + b; };
```

Trois lambdas pour la même opération logique — l'addition. Si on veut écrire du code générique, il faut recourir à un foncteur templaté ou à un template de fonction classique, perdant la concision et la localité des lambdas :

```cpp
// C++11 : foncteur générique — verbeux
struct GenericAdd {
    template<typename T>
    auto operator()(const T& a, const T& b) const { return a + b; }
};
```

C++14 résout ce problème avec les **lambdas génériques**, et C++20 pousse le concept plus loin avec les **lambdas templatées**, offrant un contrôle total sur les paramètres de type.

---

## Lambdas génériques avec `auto` (C++14)

### Syntaxe de base

En C++14, on peut utiliser `auto` comme type de paramètre dans une lambda. Le compilateur génère alors un `operator()` templaté — la lambda accepte n'importe quel type :

```cpp
auto add = [](auto a, auto b) { return a + b; };

std::print("{}\n", add(3, 4));           // int + int → 7
std::print("{}\n", add(1.5, 2.7));       // double + double → 4.2
std::print("{}\n", add(std::string("Hello, "), std::string("World")));  // string → Hello, World
```

Une seule lambda remplace les trois versions typées. Chaque appel avec un jeu de types différent instancie une spécialisation distincte de l'`operator()`, exactement comme un template.

### Ce que le compilateur génère

La lambda générique `[](auto a, auto b) { return a + b; }` est transformée en une closure dont l'`operator()` est un template :

```cpp
class __lambda {
public:
    template<typename T1, typename T2>
    auto operator()(T1 a, T2 b) const {
        return a + b;
    }
};
```

Chaque paramètre `auto` introduit un paramètre de template **indépendant**. Ainsi, `auto a, auto b` permet des types différents pour `a` et `b` :

```cpp
auto multiply = [](auto a, auto b) { return a * b; };

// T1 = int, T2 = double → retour double
std::print("{}\n", multiply(3, 2.5));  // 7.5
```

Si l'on souhaite que les deux paramètres aient le **même type**, il faut utiliser les lambdas templatées de C++20 (voir plus bas).

### `auto&`, `const auto&`, `auto&&`

Les qualificateurs de référence s'appliquent normalement sur `auto`, avec la même sémantique que pour les templates :

```cpp
// Par valeur — copie du paramètre
auto by_value = [](auto x) { return x; };

// Par référence const — pas de copie, pas de modification
auto by_cref = [](const auto& x) { return x.size(); };

// Par référence universelle (forwarding reference)
auto by_fwdref = [](auto&& x) {
    return std::forward<decltype(x)>(x);
};
```

La forme `auto&&` est particulièrement importante car elle crée une **forwarding reference** (aussi appelée universal reference). Combinée avec `std::forward`, elle permet de préserver la catégorie de valeur (lvalue ou rvalue) du paramètre, activant le perfect forwarding. C'est le mécanisme couvert en section 10.4.

```cpp
auto wrapper = [](auto&& arg) {
    // Préserve lvalue/rvalue selon l'argument passé
    process(std::forward<decltype(arg)>(arg));
};

std::string s = "hello";
wrapper(s);                    // arg est std::string& (lvalue)
wrapper(std::string("temp"));  // arg est std::string&& (rvalue)
```

### Variadic `auto` — packs de paramètres

Les lambdas génériques supportent les **parameter packs** via `auto...`, permettant un nombre variable d'arguments :

```cpp
auto print_all = [](const auto&... args) {
    (std::print("{} ", args), ...);  // Fold expression (C++17)
    std::print("\n");
};

print_all(1, 2.5, "hello", 'c');  // 1 2.5 hello c
```

Le compilateur génère un `operator()` avec un pack de templates :

```cpp
class __lambda {
public:
    template<typename... Args>
    void operator()(const Args&... args) const {
        (std::print("{} ", args), ...);
        std::print("\n");
    }
};
```

Ce mécanisme est puissant pour écrire des wrappers génériques, des loggers, ou des fonctions de dispatch :

```cpp
// Logger générique qui formate automatiquement tous les arguments
auto log = [](std::string_view level, const auto&... args) {
    std::print("[{}] ", level);
    (std::print("{} ", args), ...);
    std::print("\n");
};

log("INFO", "User", 42, "connected from", "192.168.1.1");
// [INFO] User 42 connected from 192.168.1.1
```

---

## Lambdas templatées avec syntaxe explicite (C++20)

### Motivation

Les lambdas génériques avec `auto` ont une limitation : chaque `auto` est un type indépendant. On ne peut pas exprimer de contraintes entre les paramètres, ni nommer le type pour l'utiliser dans le corps de la lambda :

```cpp
// Avec auto : T1 et T2 sont des types potentiellement différents
auto add = [](auto a, auto b) { return a + b; };
add(1, 2.5);  // Fonctionne — int + double

// Comment forcer le MÊME type pour a et b ?
// Comment utiliser le type T à l'intérieur du corps ?
```

C++20 introduit une syntaxe de template explicite sur les lambdas, apportant la même expressivité que les templates de fonctions classiques.

### Syntaxe

Les paramètres de template sont déclarés entre `<>` juste après la clause de capture, avant la liste de paramètres :

```cpp
auto add = []<typename T>(T a, T b) { return a + b; };
```

La lambda exige maintenant que les deux arguments aient le **même type** :

```cpp
std::print("{}\n", add(3, 4));       // ✅ T = int
std::print("{}\n", add(1.5, 2.7));   // ✅ T = double

// add(1, 2.5);  // ❌ ERREUR : T ne peut pas être int ET double simultanément
```

### Accéder au type dans le corps

L'avantage majeur est de pouvoir **nommer** le type et l'utiliser dans le corps de la lambda. Avec `auto`, il faut recourir à `decltype`, ce qui alourdit le code :

```cpp
// Avec auto (C++14) — decltype partout
auto make_vector_auto = [](auto first, auto... rest) {
    std::vector<decltype(first)> result;
    result.push_back(first);
    (result.push_back(rest), ...);
    return result;
};

// Avec template explicite (C++20) — propre et lisible
auto make_vector = []<typename T, typename... Ts>(T first, Ts... rest) {
    std::vector<T> result;
    result.push_back(first);
    (result.push_back(rest), ...);
    return result;
};

auto v = make_vector(1, 2, 3, 4, 5);  // std::vector<int>
```

### Paramètres non-type (NTTP)

Les lambdas templatées supportent les paramètres non-type, comme les templates classiques :

```cpp
auto fixed_array = []<std::size_t N>() {
    return std::array<int, N>{};
};

auto arr = fixed_array.operator()<10>();  // std::array<int, 10>
```

La syntaxe d'appel avec `.operator()<N>()` est lourde — ce pattern est plus naturel lorsque le paramètre non-type est déduit d'un argument :

```cpp
auto repeat = []<std::size_t N>(const char (&str)[N]) {
    // N est déduit de la taille du tableau C
    for (std::size_t i = 0; i < N - 1; ++i) {  // -1 pour le '\0'
        std::print("{}", str[i]);
    }
    std::print("\n");
};

repeat("Hello");  // N = 6 (5 caractères + '\0')
```

### Combinaison `auto` et templates explicites

On peut combiner les deux styles dans la même lambda — certains paramètres avec des templates nommés, d'autres avec `auto` :

```cpp
auto convert_all = []<typename Target>(const auto& container) {
    std::vector<Target> result;
    result.reserve(container.size());
    for (const auto& elem : container) {
        result.push_back(static_cast<Target>(elem));
    }
    return result;
};

std::vector<int> ints = {1, 2, 3, 4, 5};
auto doubles = convert_all.operator()<double>(ints);
// doubles = {1.0, 2.0, 3.0, 4.0, 5.0}
```

---

## Lambdas templatées et Concepts (C++20)

La véritable puissance des lambdas templatées se manifeste en combinaison avec les **Concepts** (couverts en détail aux sections 12.4 et 16.6). Les concepts permettent de poser des contraintes sur les types acceptés, avec des messages d'erreur clairs en cas de violation.

### Avec `requires` sur le template

```cpp
auto add = []<typename T>
    requires std::integral<T>
(T a, T b) {
    return a + b;
};

std::print("{}\n", add(3, 4));    // ✅ int est integral
// add(1.5, 2.7);                 // ❌ ERREUR claire : double ne satisfait pas std::integral
```

### Avec un concept en place du `typename`

La forme la plus concise utilise un concept directement comme contrainte du paramètre de template :

```cpp
auto divide = []<std::floating_point T>(T a, T b) {
    return a / b;
};

std::print("{}\n", divide(7.0, 2.0));    // ✅ double est floating_point
// divide(7, 2);                          // ❌ int ne satisfait pas floating_point
```

### Concepts sur les lambdas `auto` (forme abrégée)

Les concepts s'appliquent également aux paramètres `auto`, sans avoir besoin de la syntaxe template explicite. C'est la forme la plus concise :

```cpp
auto process = [](std::integral auto x, std::floating_point auto y) {
    return static_cast<double>(x) + y;
};

std::print("{}\n", process(3, 2.5));  // ✅
// process(3.0, 2.5);                 // ❌ 3.0 n'est pas integral
```

Chaque `concept auto` contraint indépendamment son paramètre. C'est l'équivalent concis de :

```cpp
auto process = []<std::integral T1, std::floating_point T2>(T1 x, T2 y) {
    return static_cast<double>(x) + y;
};
```

### Concept personnalisé avec une lambda

On peut contraindre les lambdas avec des concepts personnalisés pour des exigences métier spécifiques :

```cpp
template<typename T>
concept Printable = requires(T t) {
    std::print("{}", t);  // T doit être formatable par std::print
};

auto log_item = [](Printable auto const& item) {
    std::print("[LOG] {}\n", item);
};

log_item(42);              // ✅
log_item("hello");         // ✅
log_item(std::string{});   // ✅
// log_item(std::mutex{});  // ❌ std::mutex n'est pas Printable
```

---

## Cas d'usage pratiques

### Comparateurs génériques

Les lambdas génériques sont idéales pour écrire des comparateurs réutilisables avec les algorithmes STL :

```cpp
auto descending = [](const auto& a, const auto& b) { return a > b; };

std::vector<int> ints = {3, 1, 4, 1, 5};
std::sort(ints.begin(), ints.end(), descending);
// ints = {5, 4, 3, 1, 1}

std::vector<std::string> words = {"banana", "apple", "cherry"};
std::sort(words.begin(), words.end(), descending);
// words = {"cherry", "banana", "apple"}
```

Un seul comparateur fonctionne avec n'importe quel type qui supporte `operator>`.

### Projections et transformations

Les lambdas génériques servent de **projections** — elles extraient un aspect d'un objet complexe pour le passer à un algorithme :

```cpp
struct Employee {
    std::string name;
    int age;
    double salary;
};

auto by_field = []<typename F>(F field) {
    return [field](const auto& a, const auto& b) {
        return std::invoke(field, a) < std::invoke(field, b);
    };
};

std::vector<Employee> team = {
    {"Alice", 30, 85000.0},
    {"Bob", 25, 72000.0},
    {"Carol", 35, 95000.0}
};

std::sort(team.begin(), team.end(), by_field(&Employee::age));
// Trié par âge : Bob(25), Alice(30), Carol(35)

std::sort(team.begin(), team.end(), by_field(&Employee::salary));
// Trié par salaire : Bob(72k), Alice(85k), Carol(95k)
```

`by_field` est une lambda **d'ordre supérieur** — elle retourne une autre lambda. Le template explicite n'est pas strictement nécessaire ici (on pourrait utiliser `auto`), mais il documente l'intention : le paramètre est un "extracteur de champ".

### Visiteur pour `std::variant`

Les lambdas génériques sont le moyen idiomatique de visiter un `std::variant` (couvert en section 12.2). Le pattern **overloaded** combine plusieurs lambdas en un seul visiteur :

```cpp
// Helper pour combiner des lambdas en un visiteur
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

using JsonValue = std::variant<int, double, std::string, bool, std::nullptr_t>;

JsonValue value = "hello";

std::visit(overloaded{
    [](int i)                { std::print("int: {}\n", i); },
    [](double d)             { std::print("double: {}\n", d); },
    [](const std::string& s) { std::print("string: {}\n", s); },
    [](bool b)               { std::print("bool: {}\n", b); },
    [](std::nullptr_t)       { std::print("null\n"); }
}, value);
// Affiche : string: hello
```

Et une lambda générique peut servir de **cas par défaut** qui attrape tous les types non traités explicitement :

```cpp
std::visit(overloaded{
    [](const std::string& s) { std::print("string: {}\n", s); },
    [](const auto& other)    { std::print("other type\n"); }  // Catch-all
}, value);
```

La lambda `const auto&` est instanciée pour chaque type du variant non couvert par une lambda plus spécifique. La résolution de surcharge favorise la correspondance la plus spécifique : une surcharge non-template (`const std::string&`) l'emporte sur une surcharge template (`const auto&`) à niveau de qualification égal.

> ⚠️ **Attention** : utiliser `auto&&` au lieu de `const auto&` comme catch-all peut produire des résultats surprenants. Le `auto&&` se déduit en référence non-const (`T&`) pour les lvalues, ce qui est un meilleur match que `const T&` — le catch-all vole alors les correspondances des surcharges spécifiques. Préférez `const auto&` pour les catch-all de visiteurs.

### Perfect forwarding dans un wrapper

Le combo `auto&&` + `std::forward` + parameter pack permet d'écrire des wrappers transparents qui transmettent les arguments sans aucune copie ni perte d'information :

```cpp
auto timed_call = [](auto&& func, auto&&... args) {
    auto start = std::chrono::steady_clock::now();

    // Perfect forwarding de la fonction ET de ses arguments
    auto result = std::forward<decltype(func)>(func)(
        std::forward<decltype(args)>(args)...
    );

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    std::print("Call took {}µs\n", ms);

    return result;
};

auto value = timed_call(some_expensive_function, arg1, arg2);
```

Ce pattern est à la base de nombreuses abstractions : logging, retry, circuit breakers, memoization. Chaque couche peut être ajoutée sans modifier la signature des fonctions wrappées.

---

## `auto` vs template explicite : quand choisir lequel ?

Les deux syntaxes coexistent en C++20 et ont des forces complémentaires :

| Critère | `auto` (C++14) | Template explicite (C++20) |
|---|---|---|
| Concision | Très concise | Plus verbeuse |
| Types indépendants par paramètre | Oui (chaque `auto` est distinct) | Au choix (même `T` ou types distincts) |
| Forcer le même type | Impossible directement | `[]<typename T>(T a, T b)` |
| Nommer le type dans le corps | Via `decltype` uniquement | Directement via `T` |
| Contraintes (concepts) | `std::integral auto x` | `requires` ou concept comme contrainte |
| Paramètres non-type | Non | Oui (`<std::size_t N>`) |
| Parameter packs typés | `auto...` (types hétérogènes) | `T...` (types homogènes possibles) |
| Lisibilité pour les templates simples | Excellente | Surqualifiée |
| Lisibilité pour les templates complexes | Faible (decltype chains) | Excellente |

**Recommandation pratique** : utilisez `auto` par défaut pour les lambdas simples (comparateurs, transformations, callbacks). Passez à la syntaxe template explicite lorsque vous avez besoin de nommer un type, de contraindre la relation entre paramètres, d'utiliser des concepts complexes, ou de manipuler des paramètres non-type.

---

## Lambdas génériques et déduction de type retour

La déduction du type de retour fonctionne comme pour les lambdas non-génériques — le compilateur déduit le type à partir de l'expression `return`. Mais avec les lambdas génériques, le type de retour **dépend du type des arguments**, ce qui crée une famille de types de retour :

```cpp
auto identity = [](auto x) { return x; };

// Le type de retour dépend de l'argument
auto i = identity(42);          // int
auto d = identity(3.14);        // double
auto s = identity(std::string("hi"));  // std::string
```

Pour les expressions plus complexes, le trailing return type peut être nécessaire :

```cpp
// Sans trailing return type : la déduction peut surprendre
auto divide = [](auto a, auto b) { return a / b; };
// divide(7, 2) → int (division entière)
// divide(7.0, 2) → double

// Avec trailing return type explicite : comportement garanti
auto safe_divide = [](auto a, auto b) -> double {
    return static_cast<double>(a) / b;
};
```

Avec les lambdas templatées C++20, on peut utiliser le type nommé dans le trailing return type :

```cpp
auto make_pair = []<typename T, typename U>(T a, U b) -> std::pair<T, U> {
    return {a, b};
};
```

---

## Interaction avec `decltype` et `std::declval`

Dans les lambdas génériques, `decltype` est l'outil principal pour raisonner sur les types à l'intérieur du corps. Un pattern fréquent est d'utiliser `decltype` pour construire le bon type de retour ou le bon conteneur :

```cpp
auto merge = [](auto&& container1, auto&& container2) {
    using ValueType = typename std::remove_reference_t<decltype(container1)>::value_type;
    std::vector<ValueType> result;
    result.reserve(container1.size() + container2.size());
    result.insert(result.end(), container1.begin(), container1.end());
    result.insert(result.end(), container2.begin(), container2.end());
    return result;
};

std::vector<int> a = {1, 2, 3};
std::list<int> b = {4, 5, 6};
auto merged = merge(a, b);  // std::vector<int>{1, 2, 3, 4, 5, 6}
```

La version templatée C++20 est nettement plus lisible :

```cpp
auto merge = []<typename T>(const std::vector<T>& a, const std::vector<T>& b) {
    std::vector<T> result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
};
```

Le type `T` est directement disponible — pas besoin de `decltype` ni de `remove_reference_t`.

---

## Lambdas génériques et `constexpr`

Les lambdas génériques sont implicitement `constexpr` lorsque leur corps le permet (C++17). Elles peuvent donc être utilisées dans des contextes compile-time avec différents types :

```cpp
constexpr auto max_of = [](auto a, auto b) { return a > b ? a : b; };

static_assert(max_of(3, 7) == 7);
static_assert(max_of(2.5, 1.8) == 2.5);

// Utilisable aussi avec des constantes de compilation
constexpr int result = max_of(100, 42);
```

Avec les lambdas templatées et les concepts, on peut contraindre les types acceptés tout en restant `constexpr` :

```cpp
constexpr auto safe_add = []<typename T>
    requires std::integral<T>
(T a, T b) -> T {
    // Détection de débordement à la compilation si possible
    return a + b;
};

static_assert(safe_add(10, 20) == 30);
// safe_add(1.0, 2.0);  // ❌ Rejeté par le concept
```

---

## Récapitulatif

| Fonctionnalité | Standard | Syntaxe | Usage principal |
|---|---|---|---|
| Lambda générique | C++14 | `[](auto x)` | Lambdas simples polymorphiques |
| Forwarding reference | C++14 | `[](auto&& x)` | Perfect forwarding dans les wrappers |
| Variadic générique | C++14 | `[](auto... args)` | Nombre variable d'arguments |
| Lambda templatée | C++20 | `[]<typename T>(T x)` | Contraintes entre paramètres, nommage de types |
| Concept abrégé | C++20 | `[](std::integral auto x)` | Contrainte concise par paramètre |
| Concept + requires | C++20 | `[]<typename T> requires C<T>(T x)` | Contraintes complexes |
| Paramètre non-type | C++20 | `[]<int N>()` | Valeurs compile-time comme paramètres |

---


⏭️ [Utilisation avec les algorithmes STL](/11-lambdas/03-lambdas-stl.md)
