🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.1 Structured Bindings (C++17)

## Décomposer pour mieux nommer

Avant C++17, extraire les éléments d'une paire, d'un tuple ou d'une structure en variables distinctes était un exercice verbeux et peu lisible. Les structured bindings (ou « liaisons structurées ») résolvent ce problème en permettant de déclarer et d'initialiser plusieurs variables en une seule instruction, directement à partir d'un objet composite. C'est l'une des fonctionnalités de C++17 qui a le plus immédiatement amélioré la lisibilité du code au quotidien.

## Le problème : un code inutilement verbeux

Considérons un cas classique — itérer sur une `std::map` avant C++17 :

```cpp
#include <map>
#include <string>
#include <iostream>

std::map<std::string, int> scores = {{"Alice", 95}, {"Bob", 87}, {"Clara", 92}};

// Avant C++17 : on manipule des std::pair nommés de façon générique
for (const auto& pair : scores) {
    std::cout << pair.first << " : " << pair.second << "\n";
}
```

Les noms `pair.first` et `pair.second` ne portent aucune sémantique. On ne sait pas, à la lecture, que `first` est un nom d'étudiant et `second` une note. On pourrait créer des variables locales supplémentaires, mais cela alourdit le code :

```cpp
for (const auto& pair : scores) {
    const auto& name = pair.first;
    const auto& score = pair.second;
    std::cout << name << " : " << score << "\n";
}
```

C'est lisible, mais mécanique et répétitif. Avec `std::tuple`, la situation est encore pire — il faut recourir à `std::get<>` :

```cpp
#include <tuple>

std::tuple<std::string, int, double> get_student_info() {
    return {"Alice", 95, 17.5};
}

// Avant C++17 : extraction pénible
auto info = get_student_info();  
std::string name = std::get<0>(info);  
int score = std::get<1>(info);  
double average = std::get<2>(info);  
```

## La solution : structured bindings

C++17 introduit une syntaxe qui décompose directement un objet composite en variables nommées :

```cpp
auto [name, score] = std::make_pair("Alice"s, 95);
```

La syntaxe générale est :

```cpp
auto [var1, var2, ...] = expression;
```

Le compilateur « déconstruit » l'objet retourné par `expression` et lie chaque élément à une variable nommée. Le nombre de variables entre crochets doit correspondre exactement au nombre d'éléments de l'objet décomposé.

## Types d'objets décomposables

Les structured bindings fonctionnent avec trois catégories d'objets.

### 1. Les paires et tuples

C'est le cas d'usage le plus courant, notamment avec `std::map` :

```cpp
#include <map>
#include <string>
#include <print>

std::map<std::string, int> scores = {{"Alice", 95}, {"Bob", 87}};

for (const auto& [name, score] : scores) {
    std::print("{} a obtenu {}/100\n", name, score);
}
```

Le gain en lisibilité est immédiat : `name` et `score` remplacent les anonymes `first` et `second`. Le code exprime directement son intention.

Avec `std::tuple`, même bénéfice :

```cpp
#include <tuple>
#include <string>
#include <print>

std::tuple<std::string, int, double> get_student_info() {
    return {"Alice", 95, 17.5};
}

// C++17 : extraction directe en une ligne
auto [name, score, average] = get_student_info();  
std::print("{} — score: {}, moyenne: {}\n", name, score, average);  
```

C'est une transformation radicale par rapport à l'usage de `std::get<0>`, `std::get<1>`, etc.

### 2. Les structures et classes à membres publics

Toute structure (ou classe) dont tous les membres non-statiques sont publics peut être décomposée :

```cpp
struct Point {
    double x;
    double y;
    double z;
};

Point origin{0.0, 0.0, 0.0};  
auto [x, y, z] = origin;  
// x == 0.0, y == 0.0, z == 0.0
```

L'ordre des variables dans le binding suit l'ordre de déclaration des membres dans la structure. Ici, `x` correspond au premier membre déclaré, `y` au deuxième, `z` au troisième. Les noms choisis dans le binding sont libres — ils n'ont pas besoin de correspondre aux noms des membres :

```cpp
auto [latitude, longitude, altitude] = origin;  // Valide : les noms sont indépendants
```

Cette décomposition fonctionne aussi avec les classes héritées, à condition que tous les membres soient dans une seule classe de la hiérarchie (pas de membres répartis entre la classe de base et la classe dérivée).

### 3. Les tableaux C-style

Les tableaux natifs de taille fixe sont également décomposables :

```cpp
int rgb[3] = {255, 128, 0};  
auto [r, g, b] = rgb;  
// r == 255, g == 128, b == 0
```

Ce cas est moins fréquent en C++ moderne (on préfère `std::array`), mais il peut être utile lors de l'interfaçage avec du code C ou des API bas niveau.

## Qualificateurs : const, référence et référence universelle

Les structured bindings respectent les qualificateurs appliqués au `auto`, ce qui offre un contrôle fin sur la sémantique de copie et de modification.

### Copie (par défaut)

```cpp
auto [name, score] = get_student_info();
```

Chaque variable reçoit une copie de l'élément correspondant. Modifier `name` ou `score` ne modifie pas l'objet d'origine. C'est le comportement le plus sûr, mais il implique un coût de copie pour les types lourds comme `std::string`.

### Référence constante

```cpp
const auto& [name, score] = get_student_info();
```

Les variables sont des références constantes vers les éléments de l'objet d'origine. Aucune copie n'est effectuée, et toute tentative de modification provoque une erreur de compilation. C'est le choix idiomatique pour itérer en lecture seule sur une `std::map` :

```cpp
for (const auto& [key, value] : my_map) {
    // Lecture seule : key et value ne sont pas modifiables
}
```

### Référence mutable

```cpp
auto& [name, score] = student;
```

Les variables sont des références mutables. Modifier `name` ou `score` modifie directement l'objet `student`. C'est utile quand on veut transformer les éléments d'une structure en place :

```cpp
struct Config {
    std::string host;
    int port;
};

Config cfg{"localhost", 8080};  
auto& [host, port] = cfg;  
port = 9090;  
// cfg.port == 9090
```

### Référence universelle (forwarding)

On peut également utiliser `auto&&` (disponible depuis C++17, comme les autres formes) :

```cpp
auto&& [name, score] = get_student_info();
```

Ce mécanisme suit les règles du *reference collapsing* : si l'expression est un l-value, les bindings sont des références l-value ; si c'est un r-value, le compilateur prolonge la durée de vie du temporaire. C'est la forme la plus générique, utilisée notamment dans du code template.

## Cas d'usage courants

### Résultats de fonctions à retours multiples

Les structured bindings encouragent un style de programmation où les fonctions retournent des structures ou des tuples plutôt que de multiplier les paramètres de sortie :

```cpp
#include <string>
#include <tuple>

// Retour riche via un tuple
std::tuple<bool, std::string, int> parse_config(const std::string& path) {
    // ... parsing ...
    return {true, "localhost", 8080};
}

// Extraction lisible côté appelant
auto [success, host, port] = parse_config("/etc/app/config.yaml");  
if (success) {  
    std::print("Connexion à {}:{}\n", host, port);
}
```

Ce pattern est bien plus clair que les anciennes approches par paramètres de sortie (`bool parse_config(const std::string& path, std::string& host, int& port)`), qui mélangent entrées et sorties dans la signature.

> 💡 **Préférence de style** — Pour les fonctions qui retournent plusieurs valeurs sémantiquement liées, une `struct` nommée est souvent plus lisible qu'un `std::tuple` anonyme. Le tuple est adapté aux cas ponctuels ; la structure nommée aux cas récurrents ou aux API publiques.

### Résultats d'insertion dans les conteneurs associatifs

Les méthodes `insert`, `emplace` et `try_emplace` de `std::map` et `std::unordered_map` retournent une `std::pair<iterator, bool>`. Avant C++17, exploiter ce retour était fastidieux :

```cpp
// Avant C++17
auto result = my_map.insert({"key", 42});  
if (result.second) {  
    std::cout << "Inséré : " << result.first->second << "\n";
}
```

Avec les structured bindings :

```cpp
auto [it, inserted] = my_map.insert({"key", 42});  
if (inserted) {  
    std::print("Inséré : {}\n", it->second);
}
```

Les noms `it` et `inserted` expriment directement le rôle de chaque composant du résultat. Ce pattern s'applique aussi à `try_emplace` :

```cpp
auto [it, inserted] = my_map.try_emplace("key", 42);  
if (!inserted) {  
    std::print("La clé existait déjà, valeur actuelle : {}\n", it->second);
}
```

### Décomposition dans les instructions if et switch (C++17)

C++17 permet de déclarer des variables dans la condition d'un `if` (« if with initializer »). Combiné aux structured bindings, cela donne un idiome très compact :

```cpp
if (auto [it, inserted] = cache.try_emplace(key, compute_value(key)); !inserted) {
    std::print("Cache hit pour '{}'\n", key);
}
```

La portée des variables `it` et `inserted` est limitée au bloc `if`/`else`, ce qui évite toute pollution de la portée englobante. C'est un pattern courant dans le code réseau et les systèmes de cache.

### Itération avec index via std::views::enumerate (C++23)

En combinant les structured bindings avec `std::views::enumerate` (C++23), on obtient l'équivalent d'un `enumerate` à la Python :

```cpp
#include <vector>
#include <string>
#include <ranges>
#include <print>

std::vector<std::string> names = {"Alice", "Bob", "Clara"};

for (auto [index, name] : std::views::enumerate(names)) {
    std::print("[{}] {}\n", index, name);
}
// [0] Alice
// [1] Bob
// [2] Clara
```

Ce pattern élimine le besoin de maintenir un compteur d'index séparé.

## Limites et pièges à connaître

### Nombre de variables fixe

Le nombre de variables dans le binding doit correspondre exactement au nombre d'éléments de l'objet décomposé. Il n'est pas possible d'ignorer un élément — contrairement au `_` de Python ou de Rust :

```cpp
auto [x, y, z] = get_point_3d();   // OK : 3 éléments, 3 variables  
auto [x, y] = get_point_3d();      // Erreur : 3 éléments, 2 variables  
```

Si seuls certains éléments sont nécessaires, il faut quand même les nommer tous. Une convention répandue est d'utiliser un nom explicitement « ignoré » :

```cpp
auto [x, y, unused_z] = get_point_3d();
```

Certains projets adoptent la convention `_` ou `_unused`, mais attention : en C++26, `_` devient un identifiant « placeholder » officiel qui peut être déclaré plusieurs fois sans erreur dans un même scope (cf. section 12.14). En C++17/C++20/C++23, déclarer `_` deux fois dans le même scope est une erreur.

### Pas de décomposition de classes avec membres privés

Les structured bindings ne fonctionnent pas nativement sur les classes dont les membres sont privés ou protégés :

```cpp
class Person {
    std::string name_;
    int age_;
public:
    Person(std::string n, int a) : name_(std::move(n)), age_(a) {}
    // ...
};

auto [name, age] = Person("Alice", 30);  // Erreur de compilation
```

Pour rendre une classe décomposable, il faut fournir une spécialisation de `std::tuple_size`, `std::tuple_element` et une fonction `get<>`. C'est un mécanisme avancé, rarement nécessaire en pratique :

```cpp
#include <tuple>
#include <string>

class Person {
    std::string name_;
    int age_;
public:
    Person(std::string n, int a) : name_(std::move(n)), age_(a) {}

    template <std::size_t I>
    auto get() const {
        if constexpr (I == 0) return name_;
        else if constexpr (I == 1) return age_;
    }
};

// Spécialisations requises dans namespace std
namespace std {
    template <> struct tuple_size<Person> : integral_constant<size_t, 2> {};
    template <> struct tuple_element<0, Person> { using type = std::string; };
    template <> struct tuple_element<1, Person> { using type = int; };
}

// Maintenant valide :
Person alice("Alice", 30);  
auto [name, age] = alice;  
```

Ce mécanisme est le même que celui utilisé en interne par `std::pair` et `std::tuple`. Il est surtout utile pour les auteurs de bibliothèques qui souhaitent rendre leurs types décomposables.

### Attention aux durées de vie avec les références

Lorsqu'on utilise `const auto&` ou `auto&` sur un temporaire, les règles de prolongation de durée de vie (*lifetime extension*) s'appliquent normalement. Cependant, certains cas intermédiaires peuvent surprendre :

```cpp
const auto& [name, score] = get_student_info();
// OK : le temporaire tuple est maintenu en vie pendant toute la portée

// Mais attention à ceci :
const auto& [name, score] = std::make_pair(get_name(), get_score());
// Le pair temporaire est maintenu en vie, MAIS si get_name() retourne
// une référence vers un temporaire interne, le dangling est possible.
```

La règle est simple : le structured binding prolonge la vie de l'objet composite retourné, mais pas celle des éventuels temporaires internes à cet objet. En cas de doute, préférer une copie (`auto`) plutôt qu'une référence.

## Sous le capot : ce que génère le compilateur

Pour comprendre précisément le mécanisme, voyons ce que le compilateur produit. L'instruction :

```cpp
auto [x, y] = get_point();
```

est conceptuellement équivalente à :

```cpp
auto __hidden = get_point();       // Variable cachée contenant l'objet complet  
auto& x = std::get<0>(__hidden);   // Référence vers le premier élément  
auto& y = std::get<1>(__hidden);   // Référence vers le second élément  
```

Les noms `x` et `y` ne sont pas des variables indépendantes : ce sont des alias vers les éléments de l'objet caché. C'est pourquoi modifier `x` quand le binding est par référence modifie effectivement l'objet sous-jacent. Cette distinction est subtile mais importante pour comprendre le comportement en présence de qualificateurs `const` et `&`.

## Bonnes pratiques

**Utiliser les structured bindings par défaut dans les range-based for loops sur des conteneurs associatifs.** L'expression `for (const auto& [key, value] : my_map)` est devenue l'idiome standard en C++17 et au-delà. Il n'y a pas de raison de continuer à écrire `pair.first` et `pair.second`.

**Nommer les variables de manière sémantique.** Le principal avantage des structured bindings est de donner des noms porteurs de sens. Éviter les noms génériques comme `a`, `b` ou `first`, `second` — c'est précisément ce qu'on cherche à éliminer.

**Préférer `const auto&` en lecture seule.** Pour l'itération ou l'extraction de données sans modification, `const auto&` évite les copies inutiles et exprime l'intention de lecture seule.

**Privilégier une struct nommée au-delà de 3 éléments.** Un `auto [a, b, c, d, e] = ...` devient difficile à lire et fragile face aux changements d'ordre des membres. Une structure avec des champs nommés est préférable pour les retours complexes.

**Utiliser l'idiome if-with-initializer pour les résultats d'insertion.** La combinaison `if (auto [it, inserted] = map.try_emplace(...); !inserted)` est à la fois compacte, lisible et limite la portée des variables.

---


⏭️ [std::optional, std::variant, std::any (C++17)](/12-nouveautes-cpp17-26/02-optional-variant-any.md)
