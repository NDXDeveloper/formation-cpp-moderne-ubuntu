🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 15.5 — Itérateurs : input, output, forward, bidirectional, random_access

## Chapitre 15 — Algorithmes de la STL

---

## Introduction

Les itérateurs sont la **colle** entre les conteneurs et les algorithmes. Chaque algorithme de la STL est défini non pas en termes de conteneurs, mais en termes de **catégories d'itérateurs**. `std::sort` ne sait pas qu'il trie un `std::vector` — il sait seulement qu'il manipule des itérateurs random-access. `std::find` ne sait pas qu'il parcourt une `std::list` — il sait seulement qu'il avance un itérateur input.

Cette abstraction est au cœur de la généricité de la STL. Comprendre les catégories d'itérateurs, c'est comprendre pourquoi certains algorithmes fonctionnent avec certains conteneurs et pas d'autres, et c'est savoir écrire du code générique qui s'adapte automatiquement aux capacités du conteneur sous-jacent.

```cpp
#include <iterator>
#include <vector>
#include <list>
#include <forward_list>
```

---

## Le concept d'itérateur

Un itérateur est un objet qui **pointe** vers un élément d'une séquence et qui sait comment **avancer** vers l'élément suivant. À son niveau le plus basique, un itérateur ressemble à un pointeur — et ce n'est pas un hasard. Les pointeurs C bruts sont les itérateurs les plus primitifs, et les itérateurs de la STL sont une généralisation de ce concept.

Tout itérateur supporte au minimum :

- **Déréférencement** : `*it` — accéder à l'élément pointé.
- **Incrémentation** : `++it` — avancer à l'élément suivant.
- **Comparaison** : `it1 == it2` et `it1 != it2` — savoir si deux itérateurs pointent au même endroit.

Ce qui distingue les catégories d'itérateurs, c'est l'ensemble des opérations **supplémentaires** qu'ils supportent au-delà de ce minimum.

---

## Les cinq catégories classiques

La hiérarchie classique (pré-C++20) définit cinq catégories d'itérateurs, chacune ajoutant des capacités par rapport à la précédente :

```
                    Capacités croissantes
                    ─────────────────────▶

  ┌──────────┐   ┌──────────┐   ┌───────────────┐   ┌──────────────┐
  │  Input   │   │ Forward  │   │ Bidirectional │   │ Random       │
  │ Iterator │──▶│ Iterator │──▶│   Iterator    │──▶│ Access       │
  └──────────┘   └──────────┘   └───────────────┘   │ Iterator     │
  ┌──────────┐                                       └──────────────┘
  │  Output  │
  │ Iterator │
  └──────────┘
```

Chaque catégorie supérieure est un **sur-ensemble** de la précédente : un itérateur bidirectional peut faire tout ce qu'un forward iterator fait, plus reculer. Un itérateur random-access peut faire tout ce qu'un bidirectional fait, plus sauter à une position arbitraire.

Un algorithme qui demande un forward iterator accepte aussi un bidirectional ou un random-access, car ceux-ci offrent au moins les mêmes capacités. L'inverse n'est pas vrai : passer un forward iterator à un algorithme qui exige un random-access provoque une erreur de compilation.

---

## Input Iterator — Lecture en une passe

L'itérateur input est le plus restrictif. Il ne permet qu'un **seul parcours en avant, en lecture** :

- `*it` — lire l'élément courant (lecture seule).
- `++it` — avancer à l'élément suivant.
- `it1 == it2`, `it1 != it2` — comparer.

Une fois incrémenté, l'itérateur précédent n'est **plus valide**. On ne peut pas revenir en arrière, et on ne peut pas parcourir la séquence une seconde fois. C'est le modèle d'un flux de données à usage unique — comme lire depuis un fichier ou un réseau.

L'exemple canonique est `std::istream_iterator`, qui lit depuis un flux d'entrée :

```cpp
#include <iostream>
#include <sstream>
#include <iterator>
#include <vector>

std::istringstream input("10 20 30 40 50");

// Lire tous les entiers du flux dans un vector
std::vector<int> v{
    std::istream_iterator<int>(input),   // début : premier élément lu
    std::istream_iterator<int>()         // fin : end-of-stream
};
// v == {10, 20, 30, 40, 50}
```

Les algorithmes compatibles avec les input iterators sont ceux qui ne parcourent la séquence **qu'une seule fois** et **de gauche à droite** : `std::find`, `std::count`, `std::accumulate`, `std::for_each`, `std::copy`…

```cpp
// Compter les éléments directement depuis un flux, sans les stocker
std::istringstream input("10 20 30 40 50");

int n = std::distance(
    std::istream_iterator<int>(input),
    std::istream_iterator<int>()
);
// n == 5
```

---

## Output Iterator — Écriture en une passe

L'itérateur output est le symétrique de l'input iterator : il ne permet qu'un **seul parcours en avant, en écriture** :

- `*it = value` — écrire une valeur à la position courante.
- `++it` — avancer à la position suivante.

On ne peut pas lire la valeur, pas revenir en arrière, pas faire deux passes. C'est le modèle d'un flux de sortie ou d'un conteneur qu'on remplit progressivement.

Les exemples canoniques :

```cpp
#include <iostream>
#include <iterator>
#include <vector>

std::vector<int> v = {1, 2, 3, 4, 5};

// Écrire vers la sortie standard
std::copy(v.begin(), v.end(),
    std::ostream_iterator<int>(std::cout, " "));
// Affiche : 1 2 3 4 5

std::cout << "\n";
```

`std::back_inserter`, `std::front_inserter` et `std::inserter` sont des output iterators qui insèrent dans un conteneur :

```cpp
std::vector<int> src = {1, 2, 3};  
std::vector<int> dst;  

// back_inserter : appelle push_back à chaque écriture
std::copy(src.begin(), src.end(), std::back_inserter(dst));
// dst == {1, 2, 3}

std::list<int> lst;

// front_inserter : appelle push_front à chaque écriture
std::copy(src.begin(), src.end(), std::front_inserter(lst));
// lst == {3, 2, 1} — l'ordre est inversé car chaque push_front insère au début
```

`std::inserter` insère à une position donnée — utile pour les conteneurs associatifs :

```cpp
std::set<int> s;

std::copy(src.begin(), src.end(), std::inserter(s, s.end()));
// s == {1, 2, 3}
```

Les output iterators apparaissent comme **destination** dans les algorithmes : `std::copy`, `std::transform`, `std::merge`, `std::set_union`…

---

## Forward Iterator — Parcours multiple en avant

Le forward iterator combine les capacités de l'input et de l'output iterator, et ajoute une propriété cruciale : le **multi-pass**. On peut parcourir la séquence plusieurs fois, et copier un itérateur pour revenir à une position sauvegardée :

- `*it` — lire **et** écrire l'élément courant.
- `++it` — avancer.
- `it1 == it2`, `it1 != it2` — comparer.
- **Multi-pass** : sauvegarder un itérateur, avancer, et revenir à la position sauvegardée.

```cpp
std::forward_list<int> fl = {10, 20, 30, 40, 50};

auto it = fl.begin();  
auto saved = it;       // sauvegarde — valide pour un forward iterator  

++it;
++it;
// *it == 30

// saved est toujours valide et pointe toujours vers 10
// *saved == 10
```

Le conteneur typique est `std::forward_list` (liste simplement chaînée), qui ne peut avancer que dans un sens mais permet de reparcourir.

Les algorithmes nécessitant un forward iterator incluent `std::adjacent_find`, `std::search`, `std::unique`, `std::replace`, `std::remove`… Ce sont des algorithmes qui doivent parfois consulter l'élément courant et le suivant simultanément, ou faire des comparaisons entre positions — ce qui nécessite de sauvegarder un itérateur.

---

## Bidirectional Iterator — Avancer et reculer

Le bidirectional iterator ajoute la capacité de **reculer** :

- Toutes les opérations du forward iterator.
- `--it` — reculer à l'élément précédent.

```cpp
std::list<int> lst = {10, 20, 30, 40, 50};

auto it = lst.end();
--it;   // *it == 50
--it;   // *it == 40
++it;   // *it == 50 (retour en avant)
```

Les conteneurs fournissant des bidirectional iterators sont `std::list`, `std::set`, `std::map`, `std::multiset`, `std::multimap` (et toutes les variantes ordonnées). Leur structure interne (listes doublement chaînées, arbres binaires) permet de naviguer dans les deux sens, mais pas de sauter à une position arbitraire.

Les algorithmes qui exigent spécifiquement un bidirectional iterator sont ceux qui ont besoin de parcourir la séquence **à rebours** : `std::reverse`, `std::stable_partition`, `std::inplace_merge`, `std::copy_backward`, `std::move_backward`, `std::prev`…

```cpp
std::list<int> lst = {1, 2, 3, 4, 5};

// std::reverse nécessite un bidirectional iterator
std::reverse(lst.begin(), lst.end());
// lst == {5, 4, 3, 2, 1}

// std::sort ne fonctionnerait PAS ici — il exige random-access
// std::sort(lst.begin(), lst.end());  // ❌ Erreur de compilation
```

---

## Random Access Iterator — Accès en O(1) à n'importe quelle position

Le random access iterator est le plus puissant. Il ajoute l'**arithmétique de pointeurs** complète :

- Toutes les opérations du bidirectional iterator.
- `it + n`, `it - n` — sauter de n positions en avant ou en arrière.
- `it += n`, `it -= n` — version in-place.
- `it1 - it2` — calculer la distance entre deux itérateurs en O(1).
- `it[n]` — accès indexé (équivalent de `*(it + n)`).
- `it1 < it2`, `it1 > it2`, `it1 <= it2`, `it1 >= it2` — comparaison ordinale.

```cpp
std::vector<int> v = {10, 20, 30, 40, 50};

auto it = v.begin();  
it += 3;          // *it == 40  
auto it2 = it - 2; // *it2 == 20  
int d = it - it2;   // d == 2  

bool before = (it2 < it);  // true  
int val = v.begin()[3];     // val == 40  
```

Les conteneurs fournissant des random access iterators sont `std::vector`, `std::array`, `std::deque`, `std::string` et les tableaux C bruts. Leur stockage contigu (ou quasi-contigu pour `deque`) permet l'accès en O(1) à n'importe quel élément.

Les algorithmes exigeant des random access iterators sont ceux qui **sautent** dans la séquence plutôt que d'avancer séquentiellement : `std::sort`, `std::nth_element`, `std::partial_sort`, `std::binary_search` (et `lower_bound`, `upper_bound` — qui fonctionnent en O(log n) avec random-access, mais dégradent en O(n) avec un forward iterator).

---

## Tableau récapitulatif

| Catégorie | Opérations clés | Conteneurs typiques | Algorithmes nécessitant cette catégorie |
|---|---|---|---|
| Input | `*it` (lecture), `++it`, `==` | `istream_iterator` | `find`, `count`, `accumulate`, `copy` |
| Output | `*it = val`, `++it` | `ostream_iterator`, `back_inserter` | Destination de `copy`, `transform` |
| Forward | Input + Output + multi-pass | `forward_list`, `unordered_set/map` | `adjacent_find`, `search`, `unique`, `remove` |
| Bidirectional | Forward + `--it` | `list`, `set`, `map` | `reverse`, `copy_backward`, `stable_partition` |
| Random Access | Bidirectional + `it+n`, `it-n`, `it[n]`, `<` | `vector`, `array`, `deque`, `string` | `sort`, `nth_element`, `binary_search` |

---

## Impact sur la complexité algorithmique

La catégorie d'itérateur affecte directement les performances de certains algorithmes. L'exemple le plus frappant est `std::advance` et `std::distance` :

```cpp
#include <iterator>

// Sur un vector (random-access) : O(1)
std::vector<int> v(1000000);  
auto it = v.begin();  
std::advance(it, 500000);   // Un seul saut : it += 500000  

// Sur une list (bidirectional) : O(n)
std::list<int> lst(1000000);  
auto it2 = lst.begin();  
std::advance(it2, 500000);  // 500 000 incrémentations : ++it, ++it, ++it...  
```

`std::advance` et `std::distance` s'adaptent automatiquement à la catégorie d'itérateur grâce à une technique appelée **tag dispatching** (ou en C++20, via des concepts). Avec un random-access iterator, `std::advance(it, n)` fait `it += n` en O(1). Avec un forward ou bidirectional iterator, il fait `++it` n fois en O(n).

Le même phénomène touche `std::lower_bound` : avec un random-access iterator, il effectue une vraie dichotomie en O(log n). Avec un forward iterator, il peut toujours fonctionner, mais chaque « saut » au milieu coûte O(n), dégradant la complexité globale à O(n).

C'est pourquoi le choix du conteneur importe : utiliser `std::lower_bound` sur une `std::list` compile, mais la recherche dichotomique perd tout son avantage.

---

## Itérateurs inversés : rbegin / rend

Tous les conteneurs fournissant des bidirectional iterators offrent aussi des **itérateurs inversés** via `rbegin()` et `rend()`. Un itérateur inversé parcourt la séquence **de la fin vers le début**, mais avec l'interface habituelle `++it` pour avancer (dans le sens inversé) :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// Parcours inversé
for (auto rit = v.rbegin(); rit != v.rend(); ++rit) {
    std::print("{} ", *rit);
}
// Affiche : 5 4 3 2 1
```

Les itérateurs inversés sont directement utilisables avec les algorithmes :

```cpp
std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

// Trouver le dernier élément supérieur à 5
auto rit = std::find_if(v.rbegin(), v.rend(), [](int x) { return x > 5; });  
if (rit != v.rend()) {  
    std::print("Dernier > 5 : {}\n", *rit);
    // Dernier > 5 : 6

    // Convertir en itérateur normal pour obtenir l'index
    auto it = rit.base() - 1;
    std::print("Index : {}\n", std::distance(v.begin(), it));
    // Index : 7
}
```

La méthode `.base()` convertit un reverse iterator en iterator normal. Attention : `.base()` pointe **un élément après** l'élément désigné par le reverse iterator (à cause de la façon dont l'inversion est implémentée). D'où le `- 1` dans l'exemple ci-dessus.

---

## Itérateurs constants : cbegin / cend

Depuis C++11, tous les conteneurs fournissent `cbegin()` et `cend()` qui renvoient des **itérateurs constants** — interdisant la modification des éléments :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

for (auto it = v.cbegin(); it != v.cend(); ++it) {
    std::print("{} ", *it);
    // *it = 42;  // ❌ Erreur de compilation : itérateur constant
}
```

En pratique, la range-based `for` loop avec `const auto&` est plus lisible :

```cpp
for (const auto& x : v) {
    std::print("{} ", x);
}
```

Les itérateurs constants sont surtout utiles quand on passe des itérateurs à des fonctions et qu'on veut garantir au niveau du type que les éléments ne seront pas modifiés. La combinaison `crbegin()` / `crend()` existe aussi pour le parcours inversé constant.

---

## Itérateurs d'insertion : back_inserter, front_inserter, inserter

Vus brièvement dans la section sur les output iterators, ces adaptateurs méritent un approfondissement car ils sont omniprésents avec les algorithmes :

### std::back_inserter

Appelle `push_back` sur le conteneur à chaque écriture. Fonctionne avec tout conteneur offrant `push_back` : `vector`, `deque`, `list`, `string`…

```cpp
std::vector<int> src = {1, 2, 3, 4, 5};  
std::vector<int> dst = {10, 20};  

std::copy(src.begin(), src.end(), std::back_inserter(dst));
// dst == {10, 20, 1, 2, 3, 4, 5}
```

### std::front_inserter

Appelle `push_front` à chaque écriture. Fonctionne avec `deque`, `list`, `forward_list`… mais **pas** `vector` (qui n'a pas de `push_front`).

```cpp
std::vector<int> src = {1, 2, 3};  
std::deque<int> dst = {10, 20};  

std::copy(src.begin(), src.end(), std::front_inserter(dst));
// dst == {3, 2, 1, 10, 20} — ordre inversé car chaque insertion va au début
```

### std::inserter

Appelle `insert` à une position donnée. Fonctionne avec tous les conteneurs qui supportent `insert`, y compris les conteneurs associatifs :

```cpp
std::vector<int> src = {1, 2, 3};  
std::vector<int> dst = {10, 20, 30, 40};  

// Insérer à la position 2 (avant le 30)
std::copy(src.begin(), src.end(), std::inserter(dst, dst.begin() + 2));
// dst == {10, 20, 1, 2, 3, 30, 40}
```

Pour les conteneurs associatifs, la position est un **hint** que le conteneur peut ignorer (l'emplacement réel est déterminé par l'ordre ou le hash) :

```cpp
std::set<int> s = {10, 30, 50};  
std::vector<int> src = {20, 40, 60};  

std::copy(src.begin(), src.end(), std::inserter(s, s.end()));
// s == {10, 20, 30, 40, 50, 60}
```

---

## std::next et std::prev — Navigation sûre

C++11 a introduit `std::next` et `std::prev` pour avancer ou reculer un itérateur de manière sûre, sans modifier l'itérateur original :

```cpp
std::list<int> lst = {10, 20, 30, 40, 50};

auto it = lst.begin();  
auto third = std::next(it, 2);   // avance de 2, it inchangé  
// *third == 30, *it == 10

auto last = lst.end();  
auto before_last = std::prev(last);  // recule de 1  
// *before_last == 50
```

Avantage par rapport à l'arithmétique directe : `std::next` et `std::prev` fonctionnent avec **n'importe quelle catégorie** d'itérateur (forward et au-delà pour `next`, bidirectional et au-delà pour `prev`). L'arithmétique `it + n` n'est disponible que pour les random-access iterators.

```cpp
// Fonctionne avec une list (bidirectional)
std::list<int> lst = {10, 20, 30, 40, 50};  
auto it = std::next(lst.begin(), 3);  // ✅ O(n) mais correct  
// *it == 40

// L'arithmétique directe ne compile pas
// auto it2 = lst.begin() + 3;  // ❌ Erreur : list n'a pas d'itérateur random-access
```

---

## C++20 : Itérateurs et Concepts

C++20 a reformulé la hiérarchie des itérateurs en termes de **concepts** (dans `<iterator>` et `<ranges>`), rendant les contraintes explicites et les messages d'erreur plus clairs :

- `std::input_iterator`
- `std::output_iterator`
- `std::forward_iterator`
- `std::bidirectional_iterator`
- `std::random_access_iterator`
- `std::contiguous_iterator` ← **nouveau en C++20**

Le concept `std::contiguous_iterator` est un sur-ensemble de random-access : il garantit que les éléments sont **contigus en mémoire** (comme un tableau C). `std::vector`, `std::array`, `std::string` et `std::span` fournissent des contiguous iterators. `std::deque` fournit des random-access iterators mais **pas** des contiguous iterators (ses éléments sont répartis en blocs).

Cette distinction permet à certains algorithmes et fonctionnalités d'optimiser davantage quand la contiguïté est garantie — par exemple, `std::span` exige des contiguous iterators, et `std::ranges::data()` ne fonctionne qu'avec des ranges contigus.

### Vérifier la catégorie à la compilation

Avec les concepts C++20, on peut contraindre les templates :

```cpp
#include <iterator>
#include <concepts>

// Cette fonction n'accepte que des itérateurs random-access
template<std::random_access_iterator Iter>  
void my_algorithm(Iter first, Iter last) {  
    auto mid = first + (last - first) / 2;  // sûr : random-access garanti
    // ...
}

std::vector<int> v = {1, 2, 3};  
my_algorithm(v.begin(), v.end());    // ✅ Compile  

std::list<int> lst = {1, 2, 3};
// my_algorithm(lst.begin(), lst.end());  // ❌ Erreur claire :
// "constraint not satisfied: std::random_access_iterator<std::list<int>::iterator>"
```

Avant C++20, la même contrainte se faisait via des **iterator tags** et SFINAE, avec des messages d'erreur souvent incompréhensibles. Les concepts sont une amélioration majeure de l'ergonomie.

---

## Hiérarchie complète (C++20)

```
  ┌────────────────────┐
  │   input_iterator   │  Lecture, une passe ou plus
  └────────┬───────────┘
           │
  ┌────────▼───────────┐
  │  forward_iterator  │  + multi-pass, + écriture
  └────────┬───────────┘
           │
  ┌────────▼───────────────┐
  │ bidirectional_iterator │  + recul (--it)
  └────────┬───────────────┘
           │
  ┌────────▼────────────────┐
  │ random_access_iterator  │  + arithmétique (it+n, it[n], <)
  └────────┬────────────────┘
           │
  ┌────────▼──────────────┐
  │ contiguous_iterator   │  + mémoire contiguë garantie
  └───────────────────────┘

  ┌────────────────────┐
  │  output_iterator   │  Écriture seule (branche séparée)
  └────────────────────┘
```

---

## Quel conteneur fournit quel itérateur ?

| Conteneur | Catégorie d'itérateur | Contiguë en mémoire |
|---|---|---|
| Tableau C brut, `int[]` | Contiguous | Oui |
| `std::array` | Contiguous | Oui |
| `std::vector` | Contiguous | Oui |
| `std::string` | Contiguous | Oui |
| `std::span` | Contiguous | Oui |
| `std::deque` | Random Access | Non (blocs) |
| `std::list` | Bidirectional | Non |
| `std::set`, `std::map` | Bidirectional | Non |
| `std::multiset`, `std::multimap` | Bidirectional | Non |
| `std::forward_list` | Forward | Non |
| `std::unordered_set`, `std::unordered_map` | Forward | Non |

Ce tableau explique directement pourquoi `std::sort` fonctionne avec `vector` et `deque` mais pas avec `list` ou `set` : il exige au minimum un random-access iterator. Et pourquoi `std::reverse` fonctionne avec `list` mais pas avec `forward_list` : il exige au minimum un bidirectional iterator.

---

## Écrire du code générique adapté à la catégorie

Quand on écrit des fonctions génériques, la catégorie d'itérateur détermine les opérations disponibles. C++20 rend cette contrainte explicite et élégante :

```cpp
#include <iterator>
#include <concepts>

// Version optimisée pour random-access : O(1)
template<std::random_access_iterator Iter>  
auto middle_element(Iter first, Iter last) {  
    return first + (last - first) / 2;
}

// Version générique pour forward et au-delà : O(n)
template<std::forward_iterator Iter>  
auto middle_element(Iter first, Iter last) {  
    auto slow = first;
    auto fast = first;
    while (fast != last) {
        ++fast;
        if (fast != last) {
            ++fast;
            ++slow;
        }
    }
    return slow;  // technique du "tortue et lièvre"
}
```

La résolution de surcharge choisit automatiquement la version la plus spécifique : random-access pour `vector`, forward pour `forward_list`, etc. Avant C++20, ce dispatch se faisait manuellement avec les iterator tags — fonctionnel mais verbeux.

---

## Synthèse

Les itérateurs sont l'abstraction qui donne à la STL sa puissance combinatoire : tout conteneur peut être utilisé avec tout algorithme, à condition que la catégorie d'itérateur soit suffisante. Les cinq catégories classiques (input, output, forward, bidirectional, random-access) — auxquelles C++20 ajoute contiguous — forment une hiérarchie de capacités croissantes. Connaître cette hiérarchie permet de comprendre instantanément pourquoi `std::sort` refuse une `std::list` et pourquoi `std::lower_bound` sur une `std::list` est O(n) au lieu de O(log n). Les concepts C++20 transforment ces contraintes implicites en erreurs de compilation claires et exploitables, rendant le code générique à la fois plus sûr et plus lisible.

⏭️ [Ranges (C++20) : Simplification des algorithmes](/15-algorithmes-stl/06-ranges.md)
