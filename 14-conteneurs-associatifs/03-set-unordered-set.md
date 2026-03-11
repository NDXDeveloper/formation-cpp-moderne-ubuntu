🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 14.3 — std::set et std::unordered_set

## Module 5 — La Bibliothèque Standard (STL) | Niveau Intermédiaire

---

## Introduction

Les sections précédentes ont couvert les conteneurs associatifs qui stockent des **paires clé → valeur** : `std::map` et `std::unordered_map`. Mais il existe une famille de cas d'usage où la valeur n'a pas de raison d'être — on veut simplement savoir si un élément **appartient ou non** à une collection :

- Un ensemble de mots interdits dans un filtre de contenu.
- Les identifiants des connexions actives sur un serveur.
- Les numéros de ports déjà alloués.
- Les nœuds déjà visités dans un parcours de graphe.

C'est le rôle des **sets** : ils modélisent un **ensemble** au sens mathématique — une collection d'éléments uniques, sans valeur associée. La STL en propose deux variantes principales, symétriques aux deux familles de maps :

- `std::set` — ordonné, basé sur un Red-Black Tree, opérations en O(log n).
- `std::unordered_set` — non ordonné, basé sur une table de hachage, opérations en O(1) amorti.

Ainsi que leurs variantes **multi** (`std::multiset`, `std::unordered_multiset`) qui autorisent les doublons.

---

## std::set : l'ensemble ordonné

`std::set` partage exactement la même structure interne que `std::map` — un arbre Red-Black — mais chaque nœud ne contient qu'une **clé**, sans valeur associée. Les éléments sont maintenus en permanence dans l'ordre de tri.

```cpp
#include <set>
#include <print>

std::set<int> primes {7, 2, 11, 3, 5, 2, 3}; // Les doublons sont ignorés

for (int p : primes) {
    std::print("{} ", p);
}
// Affiche : 2 3 5 7 11  (trié, sans doublons)

std::print("\nTaille : {}\n", primes.size()); // 5, pas 7
```

### Insertion

L'insertion retourne un `std::pair<iterator, bool>` identique à `std::map::insert`. Le booléen indique si l'élément a été effectivement ajouté (il ne l'est pas s'il existait déjà).

```cpp
std::set<std::string> tags;

auto [it1, ok1] = tags.insert("urgent");    // ok1 == true  
auto [it2, ok2] = tags.insert("urgent");    // ok2 == false — déjà présent  
auto [it3, ok3] = tags.emplace("critical"); // ok3 == true — construit en place  
```

`std::set` n'a ni `operator[]` ni `at()` — ces méthodes n'ont pas de sens sans valeur associée.

### Recherche

```cpp
std::set<std::string> allowed {"admin", "editor", "viewer"};

// find — retourne un itérateur
auto it = allowed.find("editor");  
if (it != allowed.end()) {  
    std::print("Rôle trouvé : {}\n", *it);
}

// contains (C++20) — test d'appartenance direct
if (allowed.contains("admin")) {
    std::print("Accès administrateur autorisé\n");
}

// count — retourne 0 ou 1
if (allowed.count("hacker") == 0) {
    std::print("Rôle inconnu\n");
}
```

`contains` est la méthode la plus idiomatique depuis C++20 pour un simple test d'appartenance. Avant C++20, `count(...) > 0` était l'alternative la plus concise, bien que sémantiquement imprécise (elle suggère un comptage alors que le résultat est toujours 0 ou 1).

### Requêtes par plage

Comme `std::map`, `std::set` expose `lower_bound`, `upper_bound` et `equal_range` pour les requêtes sur des intervalles :

```cpp
std::set<int> scores {10, 25, 30, 42, 55, 67, 78, 83, 91, 100};

// Scores entre 30 et 80 (inclus)
auto from = scores.lower_bound(30);  // Premier >= 30  
auto to   = scores.upper_bound(80);  // Premier > 80  

std::print("Scores entre 30 et 80 : ");  
for (auto it = from; it != to; ++it) {  
    std::print("{} ", *it);
}
// Affiche : 30 42 55 67 78
```

C'est un avantage décisif sur `std::unordered_set`, qui ne propose pas cette fonctionnalité.

### Suppression

```cpp
std::set<int> s {1, 2, 3, 4, 5};

s.erase(3);             // Par valeur — retourne le nombre d'éléments supprimés (0 ou 1)  
s.erase(s.find(5));     // Par itérateur  
s.erase(s.begin(), s.find(4)); // Par plage — supprime [begin, find(4))  
```

### Immutabilité des éléments

Un point fondamental de `std::set` : **les éléments sont en lecture seule**. Les itérateurs de set sont des itérateurs constants — `*it` retourne une `const` référence. La raison est la même que pour les clés de `std::map` : modifier un élément en place pourrait violer l'invariant de tri de l'arbre.

```cpp
std::set<std::string> names {"Alice", "Bob"};  
auto it = names.find("Alice");  
// *it = "Alicia"; // ❌ Erreur de compilation : l'élément est const
```

Pour "modifier" un élément, il faut le supprimer puis réinsérer la nouvelle valeur. Depuis C++17, `extract` permet de le faire sans allocation :

```cpp
auto node = names.extract("Alice");  
if (!node.empty()) {  
    node.value() = "Alicia"; // Modification autorisée sur un nœud extrait
    names.insert(std::move(node));
}
```

### Transparent comparators

Comme pour `std::map`, les transparent comparators évitent les constructions temporaires lors des recherches :

```cpp
// std::less<> active la recherche hétérogène
std::set<std::string, std::less<>> words {"apple", "banana", "cherry"};

// Recherche avec un const char* — pas de std::string temporaire
bool found = words.contains("banana");

// Recherche avec un std::string_view
std::string_view sv = "cherry";  
auto it = words.find(sv);  
```

> ⭐ **Bonne pratique** : pour les sets de `std::string`, déclarez `std::less<>` systématiquement. C'est un gain de performance gratuit, identique à celui décrit en section 14.1 pour les maps.

---

## std::unordered_set : l'ensemble non ordonné

`std::unordered_set` est à `std::set` ce que `std::unordered_map` est à `std::map` : même interface de base, mais une table de hachage en interne au lieu d'un arbre. Les opérations sont en **O(1) amorti** et les éléments ne sont **pas ordonnés**.

```cpp
#include <unordered_set>
#include <print>

std::unordered_set<std::string> visited {"Paris", "Lyon", "Marseille"};

visited.insert("Toulouse");  
visited.insert("Paris"); // Ignoré — déjà présent  

if (visited.contains("Lyon")) {
    std::print("Lyon déjà visitée\n");
}

for (const auto& city : visited) {
    std::print("{} ", city);
}
// Ordre imprévisible
```

### Exigences sur le type d'élément

Comme `std::unordered_map`, le type stocké doit fournir :

- `std::hash<T>` (ou un hash functor passé en paramètre template),
- `operator==`.

Pour les types standard, c'est transparent. Pour les types personnalisés, les techniques de la sous-section 14.2.2 s'appliquent identiquement :

```cpp
struct Point {
    double x, y;
    bool operator==(const Point&) const = default;
};

struct PointHash {
    std::size_t operator()(const Point& p) const noexcept {
        std::size_t seed = 0;
        seed ^= std::hash<double>{}(p.x) + 0x9e3779b97f4a7c15ULL
                + (seed << 12) + (seed >> 4);
        seed ^= std::hash<double>{}(p.y) + 0x9e3779b97f4a7c15ULL
                + (seed << 12) + (seed >> 4);
        return seed;
    }
};

std::unordered_set<Point, PointHash> cloud;  
cloud.insert({1.0, 2.5});  
cloud.insert({3.7, 4.2});  
```

### Contrôle du rehashing

`std::unordered_set` offre les mêmes leviers que `std::unordered_map` pour contrôler la table de hachage : `reserve`, `rehash`, `max_load_factor`, `bucket_count`, `load_factor`. Les mêmes bonnes pratiques s'appliquent — en particulier `reserve` quand on connaît la taille à l'avance.

```cpp
std::unordered_set<int> ids;  
ids.reserve(100'000); // Évite les rehashings pendant le remplissage  

for (int i = 0; i < 100'000; ++i) {
    ids.insert(i);
}
```

---

## std::multiset et std::unordered_multiset

Les variantes **multi** autorisent plusieurs occurrences du même élément. Elles modélisent un **multiensemble** (ou *bag*) plutôt qu'un ensemble au sens strict.

### std::multiset

```cpp
#include <set>

std::multiset<int> grades {85, 92, 78, 92, 85, 85, 100};

std::print("Nombre de 85 : {}\n", grades.count(85)); // 3

// equal_range pour itérer sur toutes les occurrences
auto [begin, end] = grades.equal_range(92);  
for (auto it = begin; it != end; ++it) {  
    std::print("{} ", *it); // 92 92
}
```

`std::multiset` maintient l'ordre trié, y compris entre les doublons. Le parcours produit une séquence non décroissante. C'est utile comme structure triée qui accepte les répétitions — par exemple pour maintenir un historique d'événements classés par timestamp.

### std::unordered_multiset

```cpp
#include <unordered_set>

std::unordered_multiset<std::string> word_bag {"the", "cat", "the", "hat", "the"};

std::print("Occurrences de 'the' : {}\n", word_bag.count("the")); // 3
```

`erase` par valeur supprime **toutes** les occurrences :

```cpp
word_bag.erase("the"); // Supprime les 3 occurrences
```

Pour n'en supprimer qu'une seule :

```cpp
auto it = word_bag.find("cat");  
if (it != word_bag.end()) {  
    word_bag.erase(it); // Supprime une seule occurrence
}
```

### Quand utiliser les variantes multi

En pratique, les multisets sont rares. Avant d'en utiliser un, posez-vous la question : un `std::vector` trié (ou non, selon les besoins) avec `std::count` ne suffirait-il pas ? Les multisets sont justifiés quand les insertions et suppressions individuelles sont fréquentes et qu'un vecteur trié serait trop coûteux à maintenir.

---

## Opérations ensemblistes avec les algorithmes STL

La STL fournit dans `<algorithm>` des algorithmes qui implémentent les opérations mathématiques sur les ensembles : union, intersection, différence, différence symétrique. Ces algorithmes opèrent sur des **plages triées** — ils fonctionnent donc naturellement avec `std::set` et `std::multiset`.

```cpp
#include <algorithm>
#include <set>
#include <iterator>
#include <print>

std::set<int> A {1, 2, 3, 4, 5};  
std::set<int> B {3, 4, 5, 6, 7};  

std::set<int> result;
```

### Union (A ∪ B)

```cpp
std::set_union(
    A.begin(), A.end(),
    B.begin(), B.end(),
    std::inserter(result, result.begin())
);
// result = {1, 2, 3, 4, 5, 6, 7}
```

### Intersection (A ∩ B)

```cpp
result.clear();  
std::set_intersection(  
    A.begin(), A.end(),
    B.begin(), B.end(),
    std::inserter(result, result.begin())
);
// result = {3, 4, 5}
```

### Différence (A \ B)

```cpp
result.clear();  
std::set_difference(  
    A.begin(), A.end(),
    B.begin(), B.end(),
    std::inserter(result, result.begin())
);
// result = {1, 2}
```

### Différence symétrique (A △ B)

```cpp
result.clear();  
std::set_symmetric_difference(  
    A.begin(), A.end(),
    B.begin(), B.end(),
    std::inserter(result, result.begin())
);
// result = {1, 2, 6, 7}
```

### Inclusion (A ⊆ B)

```cpp
std::set<int> C {3, 4};  
bool is_subset = std::includes(A.begin(), A.end(), C.begin(), C.end());  
// is_subset == true : C ⊆ A
```

> ⚠️ Ces algorithmes exigent des plages **triées**. Ils ne fonctionnent pas correctement avec `std::unordered_set` — le parcours d'un unordered_set ne produit pas une séquence triée. Pour appliquer ces algorithmes à un unordered_set, il faut d'abord copier ses éléments dans un vecteur et le trier, ce qui annule l'intérêt. Si les opérations ensemblistes sont fréquentes, `std::set` est le conteneur naturel.

### Opérations ensemblistes sur des unordered_set

Pour des unordered_sets, on écrit les opérations manuellement. Elles sont souvent plus performantes que les algorithmes STL sur plages triées, car chaque test d'appartenance est en O(1) :

```cpp
// Intersection de deux unordered_sets
template <typename T, typename Hash>  
std::unordered_set<T, Hash> intersect(  
    const std::unordered_set<T, Hash>& a,
    const std::unordered_set<T, Hash>& b)
{
    std::unordered_set<T, Hash> result;
    // Itérer sur le plus petit, chercher dans le plus grand
    const auto& [smaller, larger] = (a.size() <= b.size())
        ? std::tie(a, b) : std::tie(b, a);

    for (const auto& elem : smaller) {
        if (larger.contains(elem)) {
            result.insert(elem);
        }
    }
    return result;
}
```

L'astuce d'itérer sur le plus petit ensemble et de chercher dans le plus grand donne une complexité de **O(min(|A|, |B|))** en moyenne — nettement meilleure que le O(|A| + |B|) des algorithmes sur plages triées quand les tailles sont très déséquilibrées.

---

## Choisir entre set et unordered_set

Le choix suit exactement la même logique que pour les maps :

| Critère | `std::set` | `std::unordered_set` |
|---|---|---|
| **Recherche** | O(log n) | O(1) amorti |
| **Insertion** | O(log n) | O(1) amorti |
| **Parcours ordonné** | Oui | Non |
| **Requêtes par plage** | `lower_bound`, `upper_bound` | Non |
| **Opérations ensemblistes STL** | Directement | Non (plages non triées) |
| **Exigence sur le type** | `operator<` | `std::hash` + `operator==` |
| **Localité mémoire** | Faible | Faible |

**Privilégier `std::unordered_set` quand** le besoin se résume à tester l'appartenance (`contains`) et à insérer/supprimer, sans notion d'ordre.

**Privilégier `std::set` quand** le parcours ordonné, les requêtes par plage ou les opérations ensemblistes STL sont nécessaires.

**Envisager `std::flat_set` (C++23)** quand le jeu de données est construit une fois puis lu intensivement — la section 14.4 détaille cette alternative.

---

## Set vs map : quand préférer un set

La question se pose parfois : faut-il un `std::set<MyStruct>` ou un `std::map<Key, MyStruct>` ? La réponse dépend de la structure des données :

- Si l'élément **est** sa propre identité (un entier, une chaîne, une coordonnée), un **set** est naturel.
- Si l'élément a une **clé d'identification** distincte de son contenu (un `id` qui donne accès à un `Employee`), un **map** est plus approprié.
- Si vous avez un set et que vous constatez que le comparateur ou le hash n'utilise qu'un sous-ensemble des champs de la structure, c'est souvent le signe qu'un map avec ce sous-ensemble comme clé serait plus clair.

---

## En bref

`std::set` et `std::unordered_set` modélisent des ensembles d'éléments uniques — le premier ordonné, le second basé sur le hachage. Leur API est un sous-ensemble de celle des maps correspondantes (pas de `operator[]`, pas de `at`, pas de valeur associée). Les éléments d'un set sont immuables via les itérateurs ; `extract` (C++17) permet la modification indirecte. Les algorithmes ensemblistes de la STL (`set_union`, `set_intersection`, etc.) opèrent sur des plages triées et s'intègrent naturellement avec `std::set`. Pour les unordered_sets, les opérations ensemblistes manuelles basées sur `contains` exploitent le O(1) du hachage et sont souvent plus performantes.

⏭️ [std::flat_map et std::flat_set (C++23) : Alternatives cache-friendly](/14-conteneurs-associatifs/04-flat-map-flat-set.md)
