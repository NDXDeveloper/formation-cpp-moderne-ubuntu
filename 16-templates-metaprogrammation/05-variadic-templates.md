🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 16.5 — Variadic templates (C++11)

## Chapitre 16 : Templates et Métaprogrammation · Module 5 : La STL

---

## Introduction

Jusqu'ici, tous les templates que nous avons écrits acceptaient un nombre fixe de paramètres : un type `T`, deux types `T` et `U`, un type et une valeur `N`. Mais certains problèmes nécessitent un nombre **arbitraire** de paramètres — inconnu au moment de l'écriture du template, déterminé uniquement au moment de l'instanciation.

Comment écrire une fonction qui accepte 1, 3 ou 15 arguments de types quelconques ? Comment concevoir un `std::tuple` capable de stocker n'importe quelle combinaison de types ? Comment implémenter un `std::make_unique` qui transmet ses arguments au constructeur, quel que soit leur nombre ?

La réponse est le **variadic template**, introduit en C++11. Un variadic template accepte zéro ou plusieurs paramètres template, regroupés dans un **parameter pack**. Ce mécanisme est le fondement de nombreux composants de la STL moderne et un pilier de la métaprogrammation.

---

## Syntaxe fondamentale : le parameter pack

Un parameter pack se déclare avec l'opérateur ellipsis (`...`). Il existe deux catégories de packs :

- Un **template parameter pack** : zéro ou plusieurs paramètres template.
- Un **function parameter pack** : zéro ou plusieurs paramètres de fonction.

```cpp
//           template parameter pack
//                    ↓
template <typename... Types>  
void exemple(Types... args) {  
    //        ↑
    //  function parameter pack
}
```

`Types` est un pack de types. `args` est un pack de valeurs correspondantes. Les deux sont liés : si `Types` contient `{int, double, std::string}`, alors `args` contient trois valeurs de ces types respectifs.

```cpp
exemple(42, 3.14, std::string("hello"));
// Types = {int, double, std::string}
// args  = {42, 3.14, "hello"}

exemple();
// Types = {} (vide)
// args  = {} (vide)

exemple(true);
// Types = {bool}
// args  = {true}
```

---

## `sizeof...` : connaître la taille du pack

L'opérateur `sizeof...` retourne le nombre d'éléments dans un parameter pack, sous forme de `constexpr std::size_t` :

```cpp
template <typename... Types>  
void info(Types... args) {  
    std::print("Nombre de types : {}\n", sizeof...(Types));
    std::print("Nombre d'arguments : {}\n", sizeof...(args));
    // Les deux valeurs sont toujours identiques
}
```

```cpp
info(1, 2.0, "trois");  // 3, 3  
info();                   // 0, 0  
info('a');                // 1, 1  
```

`sizeof...` est évalué à la compilation. Il ne parcourt pas les éléments du pack ; il en retourne simplement le nombre.

---

## Pack expansion : déployer le pack

Un parameter pack ne peut pas être utilisé directement comme une variable ordinaire. Pour « ouvrir » le pack et accéder à ses éléments, on utilise le **pack expansion** — l'opérateur `...` placé **après** un pattern contenant le pack :

```cpp
pattern...
```

Le compilateur remplace cette expression par une liste séparée par des virgules, où le pattern est appliqué à chaque élément du pack.

### Expansion simple

```cpp
template <typename... Types>  
void appeler_print(Types... args) {  
    // Expansion : chaque élément est passé individuellement
    // Si args = {42, 3.14, "hi"}, l'expansion produit :
    // std::print("{} ", 42), std::print("{} ", 3.14), std::print("{} ", "hi")
    (std::print("{} ", args), ...);  // Fold expression — section 16.7
}
```

### Expansion dans un appel de fonction

L'expansion peut se produire dans la liste d'arguments d'un appel de fonction :

```cpp
template <typename... Args>  
auto creer_tuple(Args... args) {  
    return std::tuple<Args...>(args...);
    //                ~~~~      ~~~~
    //      expansion des types   expansion des valeurs
}
```

```cpp
auto t = creer_tuple(1, 2.0, "trois"s);
// Produit : std::tuple<int, double, std::string>(1, 2.0, "trois"s)
```

### Expansion avec transformation

Le pattern peut contenir une transformation appliquée à chaque élément :

```cpp
template <typename... Args>  
auto creer_tuple_pointeurs(Args&... args) {  
    return std::make_tuple(&args...);
    //                      ~~~~~
    //   Pattern : &elem → appliqué à chaque élément du pack
}
```

```cpp
int a = 1;  
double b = 2.0;  
std::string c = "trois";  

auto t = creer_tuple_pointeurs(a, b, c);
// Produit : std::make_tuple(&a, &b, &c)
// t est de type std::tuple<int*, double*, std::string*>
```

Un autre exemple avec une conversion de type :

```cpp
template <typename... Args>  
auto en_doubles(Args... args) {  
    return std::make_tuple(static_cast<double>(args)...);
}
```

```cpp
auto t = en_doubles(1, 2.0f, 3L, true);
// Produit : std::make_tuple(1.0, 2.0, 3.0, 1.0)
// t est de type std::tuple<double, double, double, double>
```

### Contextes d'expansion

Le pack expansion peut apparaître dans de nombreux contextes :

```cpp
template <typename... Bases>  
class MultiDerive : public Bases... {    // Expansion dans la liste d'héritage  
public:  
    MultiDerive(const Bases&... args)
        : Bases(args)... {}              // Expansion dans la liste d'initialisation
};
```

```cpp
template <typename... Args>  
void f(Args... args) {  
    auto t = std::tuple{args...};        // Expansion dans un initializer
    [[maybe_unused]] auto a = {args...}; // Expansion dans un braced-init-list
}
```

---

## Récursion de templates : l'approche classique

Avant C++17 et les fold expressions, la technique standard pour traiter les éléments d'un parameter pack un par un était la **récursion de templates**. Le principe : séparer le premier élément (*head*) du reste du pack (*tail*), traiter le head, puis appeler récursivement le template avec le tail.

### Patron de récursion

```cpp
// Cas de base : aucun argument restant
void afficher() {
    std::print("\n");
}

// Cas récursif : traiter le premier argument, puis les suivants
template <typename T, typename... Rest>  
void afficher(T premier, Rest... reste) {  
    std::print("{} ", premier);
    afficher(reste...);  // Appel récursif avec le pack réduit d'un élément
}
```

```cpp
afficher(1, 2.5, "hello", true);
// Étape 1 : T=int,    premier=1,     reste={2.5, "hello", true}  → print "1 "
// Étape 2 : T=double, premier=2.5,   reste={"hello", true}       → print "2.5 "
// Étape 3 : T=char*,  premier="hello", reste={true}               → print "hello "
// Étape 4 : T=bool,   premier=true,  reste={}                     → print "true "
// Étape 5 : pack vide → appelle afficher() (cas de base)          → print "\n"
```

À chaque appel, le compilateur instancie une nouvelle version du template avec un pack réduit d'un élément. Le cas de base (fonction non-template sans arguments) met fin à la récursion.

### Accumuler un résultat

La récursion permet de calculer une valeur à partir de tous les éléments du pack :

```cpp
// Cas de base
template <typename T>  
T somme(T valeur) {  
    return valeur;
}

// Cas récursif
template <typename T, typename... Rest>  
auto somme(T premier, Rest... reste) {  
    return premier + somme(reste...);
}
```

```cpp
auto s = somme(1, 2, 3, 4, 5);      // 15  
auto d = somme(1.0, 2.5, 3.5);       // 7.0  
auto m = somme(std::string("a"), std::string("b"), std::string("c"));  // "abc"  
```

### Limites de la récursion

La récursion de templates génère autant d'instanciations que d'éléments dans le pack. Pour un pack de 100 éléments, le compilateur produit 100 fonctions distinctes. Cela peut :

- augmenter les temps de compilation ;
- atteindre la limite de profondeur de templates du compilateur (typiquement 256 ou 1024) ;
- produire des messages d'erreur profondément imbriqués.

Les **fold expressions** (section 16.7) éliminent ces inconvénients pour les cas simples.

---

## `if constexpr` : récursion simplifiée (C++17)

Depuis C++17, `if constexpr` permet d'intégrer le cas de base directement dans le template récursif, sans surcharge séparée :

```cpp
template <typename T, typename... Rest>  
auto somme(T premier, Rest... reste) {  
    if constexpr (sizeof...(reste) == 0) {
        return premier;                    // Cas de base
    } else {
        return premier + somme(reste...);  // Cas récursif
    }
}
```

La branche non sélectionnée par `if constexpr` n'est pas compilée. Quand `reste` est vide, l'appel récursif `somme(reste...)` n'est même pas vérifié. Cela élimine le besoin d'une surcharge dédiée au cas de base.

---

## Perfect forwarding avec les parameter packs

L'un des usages les plus importants des variadic templates est le **perfect forwarding** — transmettre des arguments à une autre fonction en préservant exactement leurs catégories de valeur (lvalue, rvalue, const, etc.).

### Le problème

Quand un argument est passé à une fonction intermédiaire, sa catégorie de valeur peut être perdue :

```cpp
template <typename... Args>  
void wrapper(Args... args) {  
    fonction_cible(args...);  // Chaque argument est copié puis passé comme lvalue
}
```

Même si l'appelant passe un temporaire (rvalue), le paramètre `args` est un lvalue à l'intérieur de `wrapper`. La sémantique de mouvement (chapitre 10) n'est pas exploitée.

### La solution : `Args&&...` + `std::forward`

```cpp
template <typename... Args>  
void wrapper(Args&&... args) {  
    fonction_cible(std::forward<Args>(args)...);
}
```

Décortiquons cette signature :

- `Args&&...` déclare chaque paramètre comme une **forwarding reference** (ou *universal reference*). Grâce aux règles de déduction des références (*reference collapsing*), un lvalue est déduit comme `T&` et un rvalue comme `T`.
- `std::forward<Args>(args)...` applique le pack expansion au pattern `std::forward<Args>(args)`. Chaque argument est transmis avec sa catégorie de valeur originale.

```cpp
void cible(int& a, const std::string& b, std::vector<int>&& c) {
    // ...
}

int x = 42;  
std::string s = "hello";  

wrapper(x, s, std::vector{1, 2, 3});
// Expansion produit :
// cible(std::forward<int&>(x),              → transmis comme lvalue (int&)
//       std::forward<const std::string&>(s), → transmis comme lvalue (const string&)
//       std::forward<std::vector<int>>(tmp)) → transmis comme rvalue (vector&&)
```

### `std::make_unique` et `std::make_shared`

Le perfect forwarding avec variadic templates est exactement le mécanisme utilisé par `std::make_unique` et `std::make_shared` :

```cpp
// Implémentation simplifiée de std::make_unique
template <typename T, typename... Args>  
std::unique_ptr<T> make_unique(Args&&... args) {  
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
```

```cpp
auto p = std::make_unique<std::string>(5, 'x');
// Appelle : new std::string(std::forward<int>(5), std::forward<char>('x'))
// Résultat : unique_ptr vers "xxxxx"
```

Quel que soit le nombre et le type des arguments du constructeur de `T`, `make_unique` les transmet fidèlement grâce au variadic pack et au perfect forwarding. C'est la raison pour laquelle cette approche a rendu `new` direct obsolète dans le code moderne (section 9.4).

---

## Application : conteneur hétérogène avec `std::tuple`

`std::tuple` est l'un des variadic templates les plus emblématiques de la STL. Il stocke un nombre arbitraire de valeurs de types différents :

```cpp
std::tuple<int, double, std::string> t{42, 3.14, "hello"};

auto entier  = std::get<0>(t);   // 42  
auto flottant = std::get<1>(t);  // 3.14  
auto chaine  = std::get<2>(t);   // "hello"  
```

### Construire un tuple simplifié

Pour comprendre la mécanique interne, voici une implémentation pédagogique minimale :

```cpp
// Cas de base : tuple vide
template <typename... Types>  
class SimpleTuple {};  

// Cas récursif : stocke le premier type, hérite du reste
template <typename Head, typename... Tail>  
class SimpleTuple<Head, Tail...> : private SimpleTuple<Tail...> {  
public:  
    SimpleTuple(const Head& head, const Tail&... tail)
        : SimpleTuple<Tail...>(tail...), valeur_{head} {}

    Head& head() { return valeur_; }
    const Head& head() const { return valeur_; }

    SimpleTuple<Tail...>& tail() {
        return static_cast<SimpleTuple<Tail...>&>(*this);
    }

private:
    Head valeur_;
};
```

```cpp
SimpleTuple<int, double, char> t{42, 3.14, 'A'};

std::print("{}\n", t.head());                 // 42 (int)  
std::print("{}\n", t.tail().head());          // 3.14 (double)  
std::print("{}\n", t.tail().tail().head());   // 'A' (char)  
```

Le pattern est classique : la spécialisation partielle `<Head, Tail...>` sépare le premier type du reste. La classe hérite récursivement de `SimpleTuple<Tail...>`, qui sépare à son tour *son* premier type, et ainsi de suite jusqu'au tuple vide.

> **Note** — La vraie implémentation de `std::tuple` dans les compilateurs modernes utilise des techniques plus sophistiquées (indexation directe, EBO — *Empty Base Optimization*), mais le principe récursif reste le même.

---

## Application : émetteur d'événements type-safe

Les variadic templates permettent de concevoir des systèmes type-safe là où d'autres langages recourent à des mécanismes dynamiques. Voici un émetteur d'événements qui vérifie à la compilation que les types des arguments correspondent à la signature enregistrée :

```cpp
#include <functional>
#include <vector>

template <typename... Args>  
class Emetteur {  
public:  
    using Callback = std::function<void(Args...)>;

    void on(Callback cb) {
        callbacks_.push_back(std::move(cb));
    }

    void emit(Args... args) const {
        for (const auto& cb : callbacks_) {
            cb(args...);
        }
    }

private:
    std::vector<Callback> callbacks_;
};
```

```cpp
// Émetteur pour un événement "position changée" (x, y, timestamp)
Emetteur<double, double, long> position_changed;

position_changed.on([](double x, double y, long ts) {
    std::print("Position ({}, {}) at {}\n", x, y, ts);
});

position_changed.on([](double x, double y, long) {
    std::print("Distance from origin: {:.2f}\n", std::sqrt(x*x + y*y));
});

position_changed.emit(3.0, 4.0, 1709123456L);
// Position (3, 4) at 1709123456
// Distance from origin: 5.00
```

Tenter d'enregistrer un callback avec la mauvaise signature produit une erreur **à la compilation**, pas à l'exécution.

---

## Expansion dans différents contextes : récapitulatif

Le pack expansion est versatile et s'utilise dans de nombreux contextes syntaxiques. Voici un récapitulatif des formes les plus courantes :

```cpp
template <typename... Ts>  
class Exemple : public Ts... {                // (1) Liste d'héritage  
public:  
    Exemple(const Ts&... args) : Ts(args)... {} // (2) Liste d'initialisation

    using Ts::operator()...;                   // (3) Using declarations (C++17)
};

template <typename... Args>  
void f(Args&&... args) {  
    g(std::forward<Args>(args)...);            // (4) Arguments de fonction

    auto t = std::make_tuple(args...);         // (5) Arguments de constructeur

    auto t2 = std::make_tuple((args * 2)...);  // (6) Avec transformation

    [[maybe_unused]]
    bool results[] = {traiter(args)...};       // (7) Array initializer

    auto lambda = [&args...] {                 // (8) Capture lambda (C++11)
        g(args...);
    };
}
```

Chaque contexte suit la même règle : `pattern...` est remplacé par `pattern_1, pattern_2, ..., pattern_N`, une application du pattern à chaque élément du pack.

---

## Variadic templates et héritage : l'overload pattern

Un pattern élégant combine variadic templates, héritage et lambdas pour créer un objet qui surcharge `operator()` avec plusieurs signatures. C'est le célèbre **overload pattern**, très utile avec `std::visit` et `std::variant` (section 12.2) :

```cpp
// Classe qui hérite de tous les types passés et expose leurs operator()
template <typename... Ts>  
struct Overload : Ts... {  
    using Ts::operator()...;  // Rend tous les operator() visibles (C++17)
};

// Deduction guide (C++17)
template <typename... Ts>  
Overload(Ts...) -> Overload<Ts...>;  
```

```cpp
std::variant<int, double, std::string> v = "hello"s;

std::visit(Overload{
    [](int val)               { std::print("int: {}\n", val); },
    [](double val)            { std::print("double: {}\n", val); },
    [](const std::string& val){ std::print("string: {}\n", val); }
}, v);
// Affiche : string: hello
```

`Overload` hérite de trois lambdas (via `Ts...`), expose leurs trois `operator()` (via `using Ts::operator()...`), et la résolution de surcharge sélectionne le bon lors de l'appel. Tout est résolu à la compilation.

---

## Packs de paramètres non-type

Les variadic templates ne sont pas limités aux types. On peut aussi créer des packs de **valeurs constantes** :

```cpp
template <int... Ns>  
struct IntSequence {  
    static constexpr std::size_t size = sizeof...(Ns);
};
```

La STL fournit `std::integer_sequence` et son alias `std::index_sequence` pour générer des séquences d'indices à la compilation :

```cpp
template <typename Tuple, std::size_t... Is>  
void afficher_tuple_impl(const Tuple& t, std::index_sequence<Is...>) {  
    ((std::print("{}: {} ", Is, std::get<Is>(t))), ...);
    std::print("\n");
}

template <typename... Types>  
void afficher_tuple(const std::tuple<Types...>& t) {  
    afficher_tuple_impl(t, std::index_sequence_for<Types...>{});
}
```

```cpp
auto t = std::make_tuple(42, 3.14, "hello"s);  
afficher_tuple(t);  
// 0: 42 1: 3.14 2: hello
```

`std::index_sequence_for<Types...>` génère la séquence `0, 1, 2, ..., N-1` où `N = sizeof...(Types)`. Le pack `Is...` capture ces indices et permet d'accéder à chaque élément du tuple via `std::get<Is>(t)`.

Ce pattern « indices trick » est l'idiome standard pour itérer sur un tuple à la compilation. Il est utilisé massivement dans les implémentations de la STL.

---

## Variadic templates vs `std::initializer_list`

Les deux mécanismes acceptent un nombre variable d'arguments, mais ils sont fondamentalement différents :

| Critère | Variadic templates | `std::initializer_list<T>` |
|---|---|---|
| **Types des éléments** | Chaque élément peut avoir un type distinct | Tous les éléments ont le même type `T` |
| **Résolution** | Compile-time | Runtime |
| **Nombre d'éléments** | Connu à la compilation (`sizeof...`) | Connu à l'exécution (`.size()`) |
| **Overhead** | Aucun (inline, monomorphisation) | Copie dans un tableau interne |
| **Usage typique** | Forwarding, tuples, dispatch | Initialisation uniforme (`{1, 2, 3}`) |

```cpp
// Variadic : chaque argument peut être d'un type différent
template <typename... Args>  
void variadic_fn(Args... args);  

variadic_fn(1, "hello", 3.14);  // OK — int, const char*, double

// initializer_list : tous les éléments doivent être du même type
void init_list_fn(std::initializer_list<int> vals);

init_list_fn({1, 2, 3});         // OK
// init_list_fn({1, "hello"});   // ERREUR : types incompatibles
```

En règle générale, utilisez `std::initializer_list` pour des listes homogènes (valeurs d'initialisation, séquences de données) et les variadic templates pour des arguments hétérogènes ou quand le perfect forwarding est nécessaire.

---

## Impact sur la compilation

Les variadic templates génèrent une instanciation pour chaque combinaison de types utilisée. Une fonction appelée avec `(int, double, std::string)` et `(int, float, std::string)` produit deux instanciations distinctes. Pour la récursion de templates, chaque niveau de récursion est aussi une instanciation séparée.

Conséquences pratiques :

- **Temps de compilation** : des packs très longs (> 50 éléments) avec récursion profonde peuvent ralentir significativement la compilation. Les fold expressions (section 16.7) atténuent ce problème en éliminant la récursion.
- **Taille du binaire** : chaque instanciation produit du code machine. Le compilateur et le linker éliminent les doublons (*COMDAT folding*), mais des combinaisons de types très nombreuses augmentent la taille de l'exécutable.
- **Profondeur de templates** : les compilateurs imposent une limite de profondeur d'instanciation récursive (modifiable avec `-ftemplate-depth=N` sur GCC/Clang).

---

## Bonnes pratiques

**Utilisez les fold expressions (C++17) plutôt que la récursion.** Pour les opérations simples (somme, affichage, application d'une fonction à chaque élément), les fold expressions sont plus concises, plus rapides à compiler et ne souffrent pas des limites de profondeur. La section 16.7 les couvre en détail.

**Utilisez le perfect forwarding systématiquement.** Quand un variadic template transmet ses arguments à une autre fonction, la signature `Args&&... args` combinée à `std::forward<Args>(args)...` est le pattern correct. Passer par valeur (`Args... args`) introduit des copies inutiles.

**Préférez `if constexpr` au cas de base séparé.** Depuis C++17, `if constexpr (sizeof...(rest) == 0)` est plus lisible qu'une surcharge dédiée au pack vide et garde toute la logique dans un seul template.

**Nommez les packs de manière significative.** `Args` est convenable pour un pack d'arguments génériques. Pour des cas plus spécifiques, `Columns`, `Handlers`, `Mixins` communiquent mieux l'intention.

**Limitez la profondeur de récursion.** Si vous traitez des packs potentiellement très longs, envisagez des techniques non-récursives (fold expressions, index sequences) pour rester dans les limites du compilateur et maintenir des temps de compilation raisonnables.

**Documentez les contraintes sur les éléments du pack.** Un variadic template qui exige que chaque type soit copiable, formatable ou convertible en `std::string` devrait le documenter explicitement — ou mieux, l'exprimer avec des Concepts (section 16.6) :

```cpp
template <std::formattable<char>... Args>   // C++23 : chaque type doit être formatable  
void print_all(Args&&... args) {  
    (std::print("{} ", args), ...);
}
```

---

## En résumé

Les variadic templates permettent d'écrire des templates acceptant un nombre arbitraire de paramètres, de types éventuellement différents. Le **parameter pack** (`typename... Types`, `Types... args`) regroupe ces paramètres, et le **pack expansion** (`pattern...`) les déploie dans n'importe quel contexte syntaxique.

La récursion de templates (head/tail) est l'approche classique pour traiter les éléments un par un. Le **perfect forwarding** (`Args&&... args` + `std::forward<Args>(args)...`) est le pattern essentiel pour transmettre des arguments sans perte de sémantique. Les **index sequences** permettent d'itérer sur les éléments d'un tuple à la compilation.

Ce mécanisme sous-tend `std::tuple`, `std::make_unique`, `std::make_shared`, `std::variant`, `std::visit` et de nombreux autres composants de la STL. La section suivante (16.6) explore les **Concepts C++20**, qui apportent un moyen déclaratif et lisible de contraindre les types dans ces templates — y compris les packs. La section 16.7 complète le tableau avec les **fold expressions**, qui simplifient radicalement les opérations sur les packs.

⏭️ [Concepts (C++20) pour contraindre les templates](/16-templates-metaprogrammation/06-concepts.md)
