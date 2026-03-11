🔝 Retour au [Sommaire](/SOMMAIRE.md)

# Chapitre 15 : Algorithmes de la STL

## Module 5 — La Bibliothèque Standard (STL) | Niveau Intermédiaire

---

## Vue d'ensemble

Les conteneurs de la STL (vus aux chapitres 13 et 14) stockent les données. Les **algorithmes** de la STL les *manipulent*. La philosophie fondatrice de la Standard Template Library repose sur une séparation stricte entre ces deux responsabilités : les conteneurs possèdent les données, les algorithmes opèrent dessus, et les **itérateurs** font le lien entre les deux.

Cette architecture découplée est l'une des décisions de conception les plus influentes du C++. Plutôt que d'attacher chaque opération à chaque conteneur (ce qui produirait une explosion combinatoire de méthodes), la STL fournit des algorithmes génériques qui fonctionnent avec *n'importe quel* conteneur exposant les bons itérateurs. Un seul `std::sort` trie un `std::vector<int>`, un `std::vector<std::string>`, un `std::array<double, 100>` ou même un tableau C brut — sans duplication de code.

---

## L'en-tête `<algorithm>` et ses compagnons

La grande majorité des algorithmes se trouvent dans l'en-tête `<algorithm>`. Quelques-uns vivent dans des en-têtes compagnons :

- **`<algorithm>`** — recherche, tri, partitionnement, transformation, copie, suppression, permutations, comparaisons, etc.
- **`<numeric>`** — opérations numériques : `std::accumulate`, `std::reduce`, `std::iota`, `std::inner_product`, `std::partial_sum`, `std::transform_reduce`, etc.
- **`<ranges>`** (C++20) — versions modernisées des algorithmes, acceptant directement des *ranges* plutôt que des paires d'itérateurs, ainsi que les *views* pour composer des pipelines fonctionnels.
- **`<execution>`** (C++17) — politiques d'exécution parallèle pour accélérer les algorithmes sur plusieurs cœurs.

```cpp
#include <algorithm>   // std::sort, std::find, std::transform...
#include <numeric>     // std::accumulate, std::reduce, std::iota...
#include <ranges>      // std::ranges::sort, std::views::filter...
#include <execution>   // std::execution::par, std::execution::seq...
```

---

## Le triumvirat : Conteneur — Itérateur — Algorithme

Le modèle mental central de la STL peut se résumer ainsi :

```
┌─────────────┐      ┌─────────────┐      ┌─────────────────┐
│  Conteneur  │─────▶│  Itérateur  │─────▶│   Algorithme    │
│  (données)  │      │   (accès)   │      │  (traitement)   │
└─────────────┘      └─────────────┘      └─────────────────┘
     vector             begin/end             sort, find,
     array              rbegin/rend           transform...
     list               cbegin/cend
     deque
     ...
```

Chaque conteneur fournit des itérateurs. Chaque algorithme attend une paire d'itérateurs (ou un range en C++20) délimitant la séquence à traiter. Ce découplage signifie que :

- Ajouter un **nouveau conteneur** le rend immédiatement compatible avec *tous* les algorithmes existants.
- Ajouter un **nouvel algorithme** le rend immédiatement utilisable avec *tous* les conteneurs existants.

C'est de la généricité par composition, pas par héritage.

---

## Conventions d'appel classiques vs Ranges (C++20)

Avant C++20, tous les algorithmes s'appellent avec des paires d'itérateurs :

```cpp
std::vector<int> v = {5, 2, 8, 1, 9, 3};

// Classique : paire d'itérateurs
std::sort(v.begin(), v.end());

auto it = std::find(v.begin(), v.end(), 8);
```

Depuis C++20, les algorithmes du namespace `std::ranges` acceptent directement le conteneur :

```cpp
// C++20 Ranges : passage direct du conteneur
std::ranges::sort(v);

auto it = std::ranges::find(v, 8);
```

Les deux styles coexistent. Le style classique reste omniprésent dans le code existant, et nécessaire quand on opère sur un *sous-range* (par exemple, trier uniquement les 10 premiers éléments). Le style Ranges est plus lisible et moins sujet aux erreurs de mismatch d'itérateurs.

---

## Catégories d'algorithmes

La STL propose plus de 100 algorithmes. Ils se regroupent naturellement en grandes familles :

**Recherche** — Trouver un élément ou vérifier une propriété : `find`, `find_if`, `binary_search`, `lower_bound`, `upper_bound`, `count`, `count_if`, `any_of`, `all_of`, `none_of`…

**Tri et ordonnancement** — Ordonner les éléments : `sort`, `stable_sort`, `partial_sort`, `nth_element`, `is_sorted`…

**Transformation et accumulation** — Produire de nouvelles valeurs à partir des existantes : `transform`, `accumulate`, `reduce`, `inner_product`, `partial_sum`…

**Copie et déplacement** — Transférer des éléments : `copy`, `copy_if`, `move`, `swap_ranges`…

**Suppression et réorganisation** — Retirer ou réarranger : `remove`, `remove_if`, `unique`, `reverse`, `rotate`, `shuffle`, `partition`…

**Comparaison** — Comparer des séquences : `equal`, `mismatch`, `lexicographical_compare`…

**Ensembles** — Opérations sur des séquences triées : `set_union`, `set_intersection`, `set_difference`, `merge`…

**Min/Max** — Trouver les extrema : `min_element`, `max_element`, `minmax_element`, `clamp`…

Ce chapitre couvre les plus importants de ces familles, puis explore deux évolutions majeures : les **Ranges** (C++20) qui modernisent leur utilisation, et les **algorithmes parallèles** (C++17) qui exploitent le matériel multi-cœur.

---

## Foncteurs, lambdas et prédicats

Beaucoup d'algorithmes acceptent un *callable* en paramètre — une fonction, un foncteur ou (le plus souvent en C++ moderne) une lambda. C'est ce qui rend les algorithmes véritablement expressifs :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Compter les nombres pairs (prédicat)
auto n = std::count_if(v.begin(), v.end(), [](int x) {
    return x % 2 == 0;
});
// n == 5

// Transformer : mettre au carré chaque élément
std::vector<int> squares(v.size());  
std::transform(v.begin(), v.end(), squares.begin(), [](int x) {  
    return x * x;
});
// squares == {1, 4, 9, 16, 25, 36, 49, 64, 81, 100}

// Trouver le premier élément supérieur à 7
auto it = std::find_if(v.begin(), v.end(), [](int x) {
    return x > 7;
});
// *it == 8
```

La maîtrise des lambdas (chapitre 11) est donc un prérequis pour exploiter pleinement les algorithmes.

---

## Le piège de `remove` / `erase` (Remove-Erase Idiom)

Un point de confusion classique mérite d'être mentionné dès l'introduction. Les algorithmes comme `std::remove` et `std::remove_if` ne suppriment **pas** d'éléments d'un conteneur. Ils *réorganisent* les éléments en déplaçant ceux à conserver au début, et renvoient un itérateur vers la nouvelle fin logique. C'est ensuite au conteneur de réellement supprimer la fin :

```cpp
std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};

// Classique : remove-erase idiom
v.erase(std::remove(v.begin(), v.end(), 2), v.end());
// v == {1, 3, 5, 7}
```

Depuis C++20, la fonction libre `std::erase` (et `std::erase_if`) simplifie cette opération :

```cpp
std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};

// C++20 : directement
std::erase(v, 2);
// v == {1, 3, 5, 7}

std::erase_if(v, [](int x) { return x > 5; });
// v == {1, 3, 5}
```

---

## Complexité algorithmique : un engagement contractuel

Chaque algorithme de la STL est accompagné d'une **garantie de complexité** définie par le standard. Ce n'est pas un détail d'implémentation — c'est un contrat :

| Algorithme | Complexité |
|---|---|
| `std::find` | O(n) |
| `std::binary_search` | O(log n) — données triées |
| `std::sort` | O(n log n) en moyenne |
| `std::stable_sort` | O(n log n), O(n log² n) si mémoire insuffisante |
| `std::nth_element` | O(n) en moyenne |
| `std::count` | O(n) |
| `std::accumulate` | O(n) |

Connaître ces complexités est indispensable pour faire les bons choix. Un `std::find` sur un `vector` trié est en O(n) alors qu'un `std::binary_search` ou `std::lower_bound` y est en O(log n) — une différence qui devient critique sur de grands jeux de données.

---

## Plan du chapitre

Ce chapitre parcourt les algorithmes de la STL en cinq temps, puis introduit les deux extensions majeures du C++ moderne :

- **15.1** — **Recherche** : `std::find`, `std::binary_search` et leurs variantes.
- **15.2** — **Tri** : `std::sort`, `std::stable_sort` et les alternatives pour des besoins partiels.
- **15.3** — **Transformation** : `std::transform`, `std::accumulate` et les opérations de réduction.
- **15.4** — **Manipulation** : `std::copy`, `std::move`, `std::remove_if` et la gestion des éléments.
- **15.5** — **Itérateurs** : les cinq catégories (input, output, forward, bidirectional, random_access) et leur rôle dans le choix des algorithmes.
- **15.6** ⭐ — **Ranges (C++20)** : views, lazy evaluation et pipelines avec l'opérateur `|`.
- **15.7** ⭐ — **Algorithmes parallèles** : politiques d'exécution (`seq`, `par`, `par_unseq`, `unseq`) et parallélisation automatique.

À l'issue de ce chapitre, vous saurez choisir le bon algorithme pour chaque situation, l'utiliser idiomatiquement avec des lambdas, comprendre ses garanties de performance, et tirer parti des Ranges et de la parallélisation pour écrire du code à la fois expressif et performant.

⏭️ [Recherche : std::find, std::binary_search](/15-algorithmes-stl/01-recherche.md)
