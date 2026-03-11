🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 15.1 — Recherche : std::find, std::binary_search

## Chapitre 15 — Algorithmes de la STL

---

## Introduction

Chercher un élément dans une collection est l'opération la plus fondamentale de la programmation. La STL propose une famille d'algorithmes de recherche couvrant tous les cas de figure : recherche linéaire quand les données ne sont pas triées, recherche dichotomique quand elles le sont, et toute une palette de variantes pour chercher selon un prédicat, compter des occurrences, ou vérifier des propriétés globales sur une séquence.

Le choix du bon algorithme de recherche dépend essentiellement de deux questions : les données sont-elles **triées** ? Et cherche-t-on un **élément précis** ou un élément satisfaisant une **condition** ?

Tous les algorithmes présentés ici se trouvent dans l'en-tête `<algorithm>`, à l'exception de quelques-uns issus de `<numeric>`.

```cpp
#include <algorithm>
#include <vector>
#include <string>
#include <print>
```

---

## Recherche linéaire : std::find et std::find_if

### std::find — Trouver une valeur exacte

`std::find` parcourt la séquence du début à la fin et renvoie un itérateur vers le **premier** élément égal à la valeur cherchée. Si aucun élément ne correspond, il renvoie l'itérateur de fin.

```cpp
std::vector<int> v = {10, 25, 42, 7, 33, 42, 18};

auto it = std::find(v.begin(), v.end(), 42);

if (it != v.end()) {
    std::print("Trouvé : {} à l'index {}\n", *it, std::distance(v.begin(), it));
    // Trouvé : 42 à l'index 2
} else {
    std::print("Non trouvé\n");
}
```

Quelques points importants :

- La comparaison utilise `operator==`. Pour des types personnalisés, cet opérateur doit être défini (ou généré via `= default` avec le spaceship operator en C++20).
- L'algorithme s'arrête dès le premier match. Si la valeur apparaît plusieurs fois, seule la première occurrence est renvoyée.
- La complexité est **O(n)** dans le pire cas — chaque élément est examiné au plus une fois.

### std::find_if — Trouver selon un prédicat

Quand on ne cherche pas une valeur précise mais un élément satisfaisant une condition, `std::find_if` prend un prédicat (callable renvoyant `bool`) :

```cpp
std::vector<int> v = {10, 25, 42, 7, 33, 18};

// Trouver le premier nombre impair
auto it = std::find_if(v.begin(), v.end(), [](int x) {
    return x % 2 != 0;
});

if (it != v.end()) {
    std::print("Premier impair : {}\n", *it);
    // Premier impair : 25
}
```

C'est l'algorithme de recherche le plus polyvalent. Combiné aux lambdas, il couvre une infinité de cas : chercher une chaîne contenant un mot, un objet dont un champ dépasse un seuil, un élément satisfaisant une condition complexe multi-critères.

```cpp
struct Employee {
    std::string name;
    int department;
    double salary;
};

std::vector<Employee> team = {
    {"Alice", 42, 75000.0},
    {"Bob", 17, 62000.0},
    {"Carol", 42, 88000.0},
    {"Dave", 23, 71000.0}
};

// Trouver le premier employé du département 42 gagnant plus de 80k
auto it = std::find_if(team.begin(), team.end(), [](const Employee& e) {
    return e.department == 42 && e.salary > 80000.0;
});

if (it != team.end()) {
    std::print("Trouvé : {}\n", it->name);
    // Trouvé : Carol
}
```

### std::find_if_not — Trouver le premier qui ne satisfait PAS un prédicat

Introduit en C++11, `std::find_if_not` est le complément logique. Il renvoie un itérateur vers le premier élément pour lequel le prédicat renvoie `false` :

```cpp
std::vector<int> v = {2, 4, 6, 7, 8, 10};

// Trouver le premier non-pair
auto it = std::find_if_not(v.begin(), v.end(), [](int x) {
    return x % 2 == 0;
});

if (it != v.end()) {
    std::print("Premier non-pair : {}\n", *it);
    // Premier non-pair : 7
}
```

On pourrait obtenir le même résultat avec `std::find_if` et un prédicat inversé, mais `find_if_not` exprime l'intention plus clairement et évite la double négation.

---

## Versions Ranges (C++20)

Tous ces algorithmes existent dans le namespace `std::ranges`, avec deux avantages : on peut passer directement le conteneur (pas besoin de `.begin()` / `.end()`), et les **projections** permettent de chercher sur un champ sans écrire de lambda complète.

```cpp
#include <ranges>

std::vector<int> v = {10, 25, 42, 7, 33, 18};

// Passage direct du conteneur
auto it = std::ranges::find(v, 42);

// find_if avec ranges
auto it2 = std::ranges::find_if(v, [](int x) { return x > 30; });
```

### Projections : une puissance souvent sous-estimée

Les projections sont l'un des apports les plus élégants des algorithmes Ranges. Elles permettent de « projeter » chaque élément avant la comparaison, sans modifier les données :

```cpp
struct Employee {
    std::string name;
    int department;
    double salary;
};

std::vector<Employee> team = {
    {"Alice", 42, 75000.0},
    {"Bob", 17, 62000.0},
    {"Carol", 42, 88000.0}
};

// Chercher l'employée "Carol" — projection sur le champ name
auto it = std::ranges::find(team, "Carol", &Employee::name);

if (it != team.end()) {
    std::print("Département de Carol : {}\n", it->department);
    // Département de Carol : 42
}

// Chercher le premier du département 17
auto it2 = std::ranges::find(team, 17, &Employee::department);
```

Sans projection, il aurait fallu écrire un `find_if` avec une lambda. La projection rend le code plus concis et l'intention plus explicite. Elle fonctionne avec n'importe quel callable — un pointeur vers membre, une lambda, une fonction libre.

---

## Compter les occurrences : std::count et std::count_if

Quand on ne cherche pas un élément mais qu'on veut savoir **combien** d'éléments correspondent, `std::count` et `std::count_if` parcourent toute la séquence et renvoient un entier :

```cpp
std::vector<int> v = {1, 2, 3, 2, 5, 2, 7, 2};

// Compter les occurrences de 2
auto n = std::count(v.begin(), v.end(), 2);  
std::print("Nombre de 2 : {}\n", n);  
// Nombre de 2 : 4

// Compter les éléments supérieurs à 3
auto m = std::count_if(v.begin(), v.end(), [](int x) { return x > 3; });  
std::print("Éléments > 3 : {}\n", m);  
// Éléments > 3 : 2
```

La complexité est toujours **O(n)** — il n'y a pas de raccourci, toute la séquence est parcourue. Pour les conteneurs associatifs (`std::map`, `std::set`, `std::unordered_map`…), préférez la méthode membre `.count()` qui exploite la structure interne du conteneur et peut être plus efficace.

Versions Ranges :

```cpp
auto n = std::ranges::count(v, 2);  
auto m = std::ranges::count_if(v, [](int x) { return x > 3; });  
```

---

## Vérifier des propriétés : all_of, any_of, none_of

Ces trois algorithmes (C++11) répondent à des questions booléennes sur l'ensemble de la séquence. Ils sont particulièrement expressifs et remplacent avantageusement des boucles `for` avec des flags :

```cpp
std::vector<int> v = {2, 4, 6, 8, 10};

// Tous pairs ?
bool all_even = std::all_of(v.begin(), v.end(), [](int x) {
    return x % 2 == 0;
});
// true

// Au moins un supérieur à 9 ?
bool has_big = std::any_of(v.begin(), v.end(), [](int x) {
    return x > 9;
});
// true

// Aucun négatif ?
bool no_neg = std::none_of(v.begin(), v.end(), [](int x) {
    return x < 0;
});
// true
```

Comportement sur séquence vide — un détail qui piège régulièrement :

- `std::all_of` sur une séquence vide renvoie **`true`** (vrai par vacuité).
- `std::any_of` sur une séquence vide renvoie **`false`**.
- `std::none_of` sur une séquence vide renvoie **`true`**.

C'est logiquement cohérent (la logique formelle définit « pour tout x dans ∅, P(x) » comme vrai), mais cela peut surprendre si on ne s'y attend pas. Un commentaire dans le code ne fait pas de mal quand la séquence peut être vide.

```cpp
std::vector<int> empty_vec;

// Attention : true par vacuité
bool result = std::all_of(empty_vec.begin(), empty_vec.end(), [](int x) {
    return x > 1000;
});
// result == true — "tous les éléments de {} sont > 1000" est vrai par vacuité
```

Versions Ranges :

```cpp
bool all_even = std::ranges::all_of(v, [](int x) { return x % 2 == 0; });  
bool has_big  = std::ranges::any_of(v, [](int x) { return x > 9; });  
bool no_neg   = std::ranges::none_of(v, [](int x) { return x < 0; });  
```

---

## Recherche dichotomique : std::binary_search et famille

Quand les données sont **triées**, la recherche linéaire est un gaspillage. La recherche dichotomique (binary search) divise l'espace de recherche par deux à chaque étape, passant de O(n) à **O(log n)** — une différence dramatique sur de grands volumes (chercher dans 1 million d'éléments passe de ~1 000 000 comparaisons à ~20).

La STL offre quatre algorithmes de recherche dichotomique. Tous exigent que la séquence soit triée selon le même critère utilisé pour la recherche.

### std::binary_search — Existence pure

`std::binary_search` répond à une question simple : l'élément **existe-t-il** dans la séquence triée ? Il renvoie un `bool`.

```cpp
std::vector<int> v = {1, 3, 5, 7, 9, 11, 13, 15};
// La séquence DOIT être triée

bool found = std::binary_search(v.begin(), v.end(), 7);
// found == true

bool not_found = std::binary_search(v.begin(), v.end(), 8);
// not_found == false
```

La limitation majeure de `std::binary_search` est qu'il ne dit **pas où** se trouve l'élément. Il renvoie uniquement `true` ou `false`. Si vous avez besoin de la position, utilisez `std::lower_bound` ou `std::upper_bound`.

### std::lower_bound — Première position possible

`std::lower_bound` renvoie un itérateur vers le **premier** élément qui n'est **pas inférieur** à la valeur cherchée (c'est-à-dire ≥). C'est la borne inférieure de l'intervalle où la valeur pourrait être insérée en maintenant l'ordre trié :

```cpp
std::vector<int> v = {1, 3, 5, 5, 5, 7, 9};

auto it = std::lower_bound(v.begin(), v.end(), 5);
// *it == 5, index 2 (premier 5)

auto it2 = std::lower_bound(v.begin(), v.end(), 6);
// *it2 == 7, index 5 (premier élément >= 6)
```

C'est l'algorithme dichotomique le plus utilisé en pratique, car il combine recherche d'existence et localisation :

```cpp
std::vector<int> v = {1, 3, 5, 7, 9, 11};

auto it = std::lower_bound(v.begin(), v.end(), 7);  
if (it != v.end() && *it == 7) {  
    std::print("Trouvé à l'index {}\n", std::distance(v.begin(), it));
} else {
    std::print("Non trouvé\n");
}
```

Ce pattern — `lower_bound` suivi d'une vérification d'égalité — est idiomatique en C++ et préférable à `binary_search` quand on a besoin de la position.

### std::upper_bound — Première position après

`std::upper_bound` renvoie un itérateur vers le **premier** élément **strictement supérieur** à la valeur cherchée :

```cpp
std::vector<int> v = {1, 3, 5, 5, 5, 7, 9};

auto it = std::upper_bound(v.begin(), v.end(), 5);
// *it == 7, index 5 (premier élément > 5)
```

La combinaison `lower_bound` / `upper_bound` délimite l'intervalle de tous les éléments égaux à la valeur cherchée :

```cpp
std::vector<int> v = {1, 3, 5, 5, 5, 7, 9};

auto lo = std::lower_bound(v.begin(), v.end(), 5);  // index 2  
auto hi = std::upper_bound(v.begin(), v.end(), 5);  // index 5  

std::print("Nombre de 5 : {}\n", std::distance(lo, hi));
// Nombre de 5 : 3

// Itérer sur toutes les occurrences
for (auto it = lo; it != hi; ++it) {
    std::print("{} ", *it);
}
// 5 5 5
```

### std::equal_range — Les deux bornes d'un coup

`std::equal_range` renvoie directement la paire `{lower_bound, upper_bound}` en un seul appel. Sa complexité reste O(2 log n), identique à deux appels séparés, mais le code est plus concis et exprime mieux l'intention :

```cpp
std::vector<int> v = {1, 3, 5, 5, 5, 7, 9};

auto [lo, hi] = std::equal_range(v.begin(), v.end(), 5);

std::print("Occurrences de 5 : {}\n", std::distance(lo, hi));
// Occurrences de 5 : 3

if (lo == hi) {
    std::print("Valeur absente\n");
}
```

Le structured binding `auto [lo, hi]` (C++17) rend la syntaxe particulièrement propre.

### Résumé visuel des quatre algorithmes dichotomiques

Pour une séquence `{1, 3, 5, 5, 5, 7, 9}` avec la valeur recherchée `5` :

```
 Index :    0     1     2     3     4     5     6
Valeurs :  [ 1 ] [ 3 ] [ 5 ] [ 5 ] [ 5 ] [ 7 ] [ 9 ]
                        ▲                  ▲
                        │                  │
                  lower_bound(5)     upper_bound(5)
                        │                  │
                        └── equal_range(5) ┘
                            (3 éléments)

binary_search(5) → true
```

---

## Recherche dichotomique avec comparateur personnalisé

Tous les algorithmes dichotomiques acceptent un comparateur en dernier paramètre. Le comparateur doit être cohérent avec l'ordre de tri de la séquence :

```cpp
// Tri décroissant
std::vector<int> v = {15, 13, 11, 9, 7, 5, 3, 1};

// Le comparateur doit refléter l'ordre de tri : std::greater
bool found = std::binary_search(v.begin(), v.end(), 9, std::greater<int>{});
// found == true

auto it = std::lower_bound(v.begin(), v.end(), 9, std::greater<int>{});
// *it == 9
```

Pour des objets complexes, le comparateur projette sur le champ de tri :

```cpp
struct Event {
    std::string name;
    int timestamp;
};

std::vector<Event> events = {
    {"boot", 100},
    {"init", 250},
    {"ready", 500},
    {"request", 750},
    {"shutdown", 1000}
};
// Trié par timestamp (croissant)

// Chercher l'événement à timestamp 500
auto it = std::lower_bound(events.begin(), events.end(), 500,
    [](const Event& e, int ts) { return e.timestamp < ts; }
);

if (it != events.end() && it->timestamp == 500) {
    std::print("Événement : {}\n", it->name);
    // Événement : ready
}
```

Attention à la signature du comparateur avec `lower_bound` : le premier paramètre est l'élément de la séquence, le second est la valeur cherchée. Pour `upper_bound`, c'est l'inverse (la valeur cherchée est le premier paramètre, l'élément de la séquence le second). Cette asymétrie est une source fréquente de bugs — `std::ranges::lower_bound` avec une projection résout ce problème plus élégamment :

```cpp
auto it = std::ranges::lower_bound(events, 500, {}, &Event::timestamp);
```

Ici, `{}` indique le comparateur par défaut (`std::less`) et `&Event::timestamp` est la projection. Plus lisible, plus sûr.

---

## Recherche de sous-séquences : std::search

`std::search` cherche la première occurrence d'une sous-séquence à l'intérieur d'une séquence plus grande. C'est l'équivalent d'un « find substring » mais généralisé à n'importe quel type :

```cpp
std::vector<int> haystack = {1, 2, 3, 4, 5, 6, 7, 8, 9};  
std::vector<int> needle   = {4, 5, 6};  

auto it = std::search(haystack.begin(), haystack.end(),
                      needle.begin(), needle.end());

if (it != haystack.end()) {
    std::print("Sous-séquence trouvée à l'index {}\n",
               std::distance(haystack.begin(), it));
    // Sous-séquence trouvée à l'index 3
}
```

La complexité est O(n × m) dans le pire cas, où n est la taille de la séquence principale et m celle de la sous-séquence. Pour la recherche de motifs dans de grandes données, C++17 a introduit des **searchers** spécialisés qui implémentent des algorithmes plus efficaces :

```cpp
#include <functional>  // pour les searchers

std::string text = "The quick brown fox jumps over the lazy dog";  
std::string pattern = "brown fox";  

// Boyer-Moore : recherche de motifs efficace O(n/m) en meilleur cas
auto it = std::search(text.begin(), text.end(),
    std::boyer_moore_searcher(pattern.begin(), pattern.end()));

if (it != text.end()) {
    std::print("Trouvé à la position {}\n",
               std::distance(text.begin(), it));
    // Trouvé à la position 10
}
```

Les trois searchers disponibles (C++17) sont `default_searcher`, `boyer_moore_searcher` et `boyer_moore_horspool_searcher`. Les deux derniers prétraitent le motif pour accélérer la recherche dans les longs textes.

---

## std::adjacent_find — Trouver des doublons consécutifs

`std::adjacent_find` cherche la première paire d'éléments adjacents identiques (ou satisfaisant un prédicat binaire). Il renvoie un itérateur vers le premier des deux :

```cpp
std::vector<int> v = {1, 2, 3, 3, 4, 5, 5, 6};

auto it = std::adjacent_find(v.begin(), v.end());  
if (it != v.end()) {  
    std::print("Premier doublon consécutif : {} à l'index {}\n",
               *it, std::distance(v.begin(), it));
    // Premier doublon consécutif : 3 à l'index 2
}
```

Avec un prédicat personnalisé, on peut chercher d'autres relations entre éléments adjacents :

```cpp
std::vector<int> v = {1, 2, 4, 3, 5, 8, 7};

// Trouver la première paire où l'élément suivant est inférieur (rupture d'ordre)
auto it = std::adjacent_find(v.begin(), v.end(), std::greater<int>{});

if (it != v.end()) {
    std::print("Rupture d'ordre : {} > {} à l'index {}\n",
               *it, *(it + 1), std::distance(v.begin(), it));
    // Rupture d'ordre : 4 > 3 à l'index 2
}
```

---

## Min et Max : std::min_element, std::max_element, std::minmax_element

Bien qu'il ne s'agisse pas strictement de « recherche » au sens classique, trouver l'élément minimum ou maximum d'une séquence est un cas de recherche fréquent :

```cpp
std::vector<int> v = {42, 17, 93, 5, 68, 31};

auto min_it = std::min_element(v.begin(), v.end());  
auto max_it = std::max_element(v.begin(), v.end());  

std::print("Min : {} (index {})\n", *min_it, std::distance(v.begin(), min_it));  
std::print("Max : {} (index {})\n", *max_it, std::distance(v.begin(), max_it));  
// Min : 5 (index 3)
// Max : 93 (index 2)
```

`std::minmax_element` (C++11) renvoie les deux en un seul parcours, ce qui est plus efficace que deux appels séparés (environ 1.5n comparaisons au lieu de 2n) :

```cpp
auto [min_it, max_it] = std::minmax_element(v.begin(), v.end());  
std::print("Min : {}, Max : {}\n", *min_it, *max_it);  
// Min : 5, Max : 93
```

Avec un comparateur personnalisé :

```cpp
struct Employee {
    std::string name;
    double salary;
};

std::vector<Employee> team = {
    {"Alice", 75000.0}, {"Bob", 62000.0}, {"Carol", 88000.0}
};

auto [lowest, highest] = std::minmax_element(team.begin(), team.end(),
    [](const Employee& a, const Employee& b) {
        return a.salary < b.salary;
    }
);

std::print("Salaire le plus bas : {} ({})\n", lowest->name, lowest->salary);  
std::print("Salaire le plus haut : {} ({})\n", highest->name, highest->salary);  
// Salaire le plus bas : Bob (62000)
// Salaire le plus haut : Carol (88000)
```

Versions Ranges avec projection :

```cpp
auto [lowest, highest] = std::ranges::minmax_element(team, {}, &Employee::salary);
```

Encore une fois, la projection élimine le besoin d'écrire un comparateur lambda complet.

---

## Choisir le bon algorithme de recherche

Le choix de l'algorithme dépend de la situation. Voici un guide de décision :

```
            Les données sont-elles triées ?
                    /            \
                  Non             Oui
                  /                 \
       Valeur exacte ?        Besoin de la position ?
        /         \              /              \
      Oui        Non           Non              Oui
       |          |             |                |
  std::find   std::find_if  binary_search    lower_bound
                                             (+ vérification ==)
```

| Situation | Algorithme | Complexité |
|---|---|---|
| Valeur exacte, données non triées | `std::find` | O(n) |
| Condition, données non triées | `std::find_if` | O(n) |
| Compter les occurrences | `std::count` / `std::count_if` | O(n) |
| Vérifier une propriété globale | `all_of` / `any_of` / `none_of` | O(n) |
| Existence, données triées | `std::binary_search` | O(log n) |
| Position, données triées | `std::lower_bound` | O(log n) |
| Intervalle d'égaux, données triées | `std::equal_range` | O(log n) |
| Sous-séquence | `std::search` | O(n × m) |
| Sous-séquence longue (texte) | `std::search` + Boyer-Moore | O(n/m) meilleur cas |
| Doublons consécutifs | `std::adjacent_find` | O(n) |
| Min / Max | `std::minmax_element` | O(n) |

Un piège courant est d'utiliser `std::find` sur un `std::vector` trié de grande taille. Dès que la séquence est triée et que le volume dépasse quelques dizaines d'éléments, `std::lower_bound` est systématiquement préférable. À 100 000 éléments, on passe de ~100 000 comparaisons à ~17. À 1 000 000, de ~1 000 000 à ~20. La différence est souvent celle entre une application réactive et une application qui rame.

Autre piège : appeler `std::binary_search` ou `std::lower_bound` sur des données **non triées** compile sans erreur et sans avertissement, mais produit des résultats incorrects. Le standard ne vérifie pas le prérequis de tri — c'est la responsabilité du développeur.

---

## Rappel : préférer les méthodes membres pour les conteneurs associatifs

Pour `std::map`, `std::set`, `std::unordered_map` et `std::unordered_set`, les méthodes membres `.find()`, `.count()` et `.contains()` (C++20) exploitent la structure interne du conteneur et sont toujours plus efficaces que les algorithmes génériques :

```cpp
std::unordered_map<std::string, int> ages = {
    {"Alice", 30}, {"Bob", 25}, {"Carol", 35}
};

// Préférer la méthode membre
if (ages.contains("Alice")) {       // C++20 — O(1) en moyenne
    std::print("Alice trouvée\n");
}

auto it = ages.find("Bob");         // O(1) en moyenne  
if (it != ages.end()) {  
    std::print("Bob a {} ans\n", it->second);
}

// Éviter : std::find sur un unordered_map est O(n) et non idiomatique
// auto it = std::find(ages.begin(), ages.end(), ???);  // ne compile même pas facilement
```

Pour `std::set` et `std::map` (arbres binaires ordonnés), les méthodes `.lower_bound()` et `.upper_bound()` sont en O(log n) et plus efficaces que les algorithmes génériques du même nom, car elles exploitent la structure arborescente au lieu de nécessiter des itérateurs random-access.

---

## Synthèse

Les algorithmes de recherche de la STL couvrent un spectre complet de besoins, de la recherche linéaire brute (`find`) à la recherche dichotomique sophistiquée (`lower_bound`, `equal_range`). Les versions Ranges (C++20) apportent lisibilité et sécurité grâce au passage direct de conteneurs et aux projections. Le point clé reste de toujours vérifier si les données sont triées avant de choisir un algorithme : cette seule question détermine si la recherche sera en O(n) ou en O(log n), et sur de grands volumes, la différence est celle entre millisecondes et microsecondes.

⏭️ [Tri : std::sort, std::stable_sort](/15-algorithmes-stl/02-tri.md)
