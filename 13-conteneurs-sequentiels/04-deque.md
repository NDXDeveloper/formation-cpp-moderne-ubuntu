🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 13.4 — std::deque : File double entrée

## Chapitre 13 : Conteneurs Séquentiels

---

## Introduction

`std::deque` (*double-ended queue*, prononcé "deck") est un conteneur séquentiel qui offre un compromis unique : comme `std::vector`, il fournit un accès par index en O(1) ; comme `std::list`, il permet l'insertion et la suppression en O(1) aux **deux extrémités**. C'est le seul conteneur séquentiel de la STL à combiner ces deux propriétés.

Ce compromis a un prix : une structure interne plus complexe que celle de `std::vector`, un surcoût mémoire non négligeable, et une localité de cache inférieure (mais très supérieure à `std::list`). `std::deque` est rarement le conteneur par défaut, mais il est le bon choix dans des scénarios précis — en particulier les files (FIFO), les buffers circulaires, et toute situation où des insertions en tête sont fréquentes.

---

## Structure interne : un tableau de blocs

Contrairement à `std::vector` qui stocke ses éléments dans un seul bloc contigu, `std::deque` utilise un **tableau de pointeurs vers des blocs de taille fixe** (*chunks*). Chaque bloc contient un nombre fixe d'éléments (la taille dépend de l'implémentation et du type `T`).

```
std::deque<int> dq{10, 20, 30, 40, 50, 60, 70, 80, 90};

Table de pointeurs (map) :
┌─────────┬─────────┬─────────┐
│  ptr 0  │  ptr 1  │  ptr 2  │
└────┬────┴────┬────┴────┬────┘
     │         │         │
     ▼         ▼         ▼
  Bloc 0     Bloc 1    Bloc 2
┌────┬────┐ ┌────┬────┐ ┌────┬────┐
│ 10 │ 20 │ │ 30 │ 40 │ │ 50 │ 60 │  ...
└────┴────┘ └────┴────┘ └────┴────┘

(tailles de blocs simplifiées pour l'illustration)
```

Cette architecture a plusieurs conséquences :

**Accès par index en O(1).** Pour accéder à l'élément `i`, le deque calcule dans quel bloc il se trouve (`i / taille_bloc`) et à quel offset dans ce bloc (`i % taille_bloc`). C'est une division et un modulo — du temps constant, mais légèrement plus coûteux que le simple `base + i * sizeof(T)` d'un `std::vector`.

**Insertion en tête en O(1) amorti.** Quand on insère en tête, le deque remplit le premier bloc à rebours. Quand ce bloc est plein, il alloue un nouveau bloc et ajoute un pointeur au début de la table. Pas besoin de déplacer les éléments existants.

**Pas de réallocation massive.** Un `std::vector` qui grandit doit copier *tous* ses éléments vers un nouveau bloc. Un `std::deque` qui grandit alloue simplement un nouveau bloc de taille fixe et met à jour la table de pointeurs. Les éléments existants ne bougent pas.

**Localité de cache intermédiaire.** À l'intérieur d'un bloc, les éléments sont contigus (bonne localité). Mais le passage d'un bloc au suivant implique un saut mémoire via la table de pointeurs (moins bon que `std::vector`, mais bien meilleur que `std::list`).

---

## Déclaration et construction

`std::deque` est défini dans l'en-tête `<deque>` et offre les mêmes constructeurs que `std::vector` :

```cpp
#include <deque>
#include <print>

int main() {
    // Construction vide
    std::deque<int> d1;

    // Avec taille et valeur
    std::deque<int> d2(5, 42);            // {42, 42, 42, 42, 42}

    // Par liste d'initialisation
    std::deque<int> d3{10, 20, 30, 40};

    // Par plage d'itérateurs
    std::deque<int> d4(d3.begin(), d3.end());

    // Par copie et par move
    std::deque<int> d5 = d3;
    std::deque<int> d6 = std::move(d5);

    // CTAD (C++17)
    std::deque d7{1.0, 2.0, 3.0};        // std::deque<double>

    std::println("d3 size={}, d6 size={}", d3.size(), d6.size());
    // Sortie : d3 size=4, d6 size=4
}
```

---

## Opérations aux deux extrémités

C'est la raison d'être de `std::deque` — les opérations en tête et en fin sont toutes en O(1) amorti :

```cpp
#include <deque>
#include <print>

int main() {
    std::deque<int> dq;

    // Insertion en fin — comme std::vector
    dq.push_back(30);
    dq.push_back(40);
    dq.emplace_back(50);
    // {30, 40, 50}

    // Insertion en tête — impossible en O(1) avec std::vector
    dq.push_front(20);
    dq.push_front(10);
    dq.emplace_front(5);
    // {5, 10, 20, 30, 40, 50}

    // Suppression aux deux extrémités
    dq.pop_front();    // {10, 20, 30, 40, 50}
    dq.pop_back();     // {10, 20, 30, 40}

    std::println("front={}, back={}", dq.front(), dq.back());
    // Sortie : front=10, back=40

    for (auto val : dq) std::print("{} ", val);
    // Sortie : 10 20 30 40
}
```

À titre de comparaison, `push_front` sur un `std::vector` de n éléments est O(n) car tous les éléments doivent être décalés d'une position. Sur un `std::deque`, c'est O(1) amorti.

---

## Accès par index

`std::deque` fournit `operator[]` et `at()` avec les mêmes sémantiques que `std::vector` :

```cpp
#include <deque>
#include <print>
#include <stdexcept>

int main() {
    std::deque<int> dq{10, 20, 30, 40, 50};

    // Accès par index — O(1), pas de vérification
    std::println("dq[2] = {}", dq[2]);       // 30

    // Accès avec vérification de bornes
    std::println("dq.at(2) = {}", dq.at(2)); // 30

    try {
        dq.at(100);
    } catch (const std::out_of_range& e) {
        std::println("Exception : {}", e.what());
    }

    // Modification par index
    dq[0] = 99;
    std::println("dq[0] = {}", dq[0]);  // 99
}
```

Bien que l'accès soit O(1), il implique une indirection supplémentaire par rapport à `std::vector` (consultation de la table de pointeurs, puis accès dans le bloc). En pratique, cette différence est mesurable mais rarement significative sauf dans les boucles les plus chaudes.

---

## Insertion et suppression au milieu

Comme `std::vector`, `std::deque` supporte `insert`, `emplace` et `erase` à des positions arbitraires. La complexité est O(n), mais avec une optimisation que `std::vector` ne peut pas offrir :

```cpp
#include <deque>
#include <print>

int main() {
    std::deque<int> dq{10, 20, 30, 40, 50};

    // Insertion au milieu
    auto it = dq.begin() + 2;
    dq.insert(it, 25);
    // {10, 20, 25, 30, 40, 50}

    // Suppression au milieu
    dq.erase(dq.begin() + 3);
    // {10, 20, 25, 40, 50}

    for (auto val : dq) std::print("{} ", val);
    // Sortie : 10 20 25 40 50
}
```

L'optimisation spécifique de `std::deque` : lors d'une insertion ou suppression au milieu, le deque choisit de décaler les éléments du **côté le plus court** (vers le début ou vers la fin). Si vous insérez près du début, seuls les éléments avant le point d'insertion sont déplacés. Si vous insérez près de la fin, seuls ceux après sont déplacés. Dans le pire cas (insertion au centre exact), c'est O(n/2).

---

## Pas de `data()` ni de garantie de contiguïté

Contrairement à `std::vector` et `std::array`, `std::deque` ne fournit **pas** de méthode `data()` et ne garantit **pas** la contiguïté des éléments en mémoire :

```cpp
#include <deque>
#include <vector>

int main() {
    std::vector<int> v{1, 2, 3};
    int* pv = v.data();       // ✅ OK — bloc contigu

    std::deque<int> dq{1, 2, 3};
    // int* pd = dq.data();   // ❌ ERREUR — data() n'existe pas
}
```

Cela signifie que `std::deque` est **incompatible** avec les API C qui attendent un pointeur vers un buffer contigu (`write()`, `memcpy()`, OpenGL, etc.). Si vous avez besoin de passer vos données à une telle API, vous devez soit copier les éléments dans un `std::vector`, soit utiliser un `std::vector` dès le départ.

---

## Pas de `reserve()` ni de contrôle de capacité

`std::deque` ne propose pas de `reserve()`, `capacity()` ni `shrink_to_fit()`. Sa stratégie d'allocation par blocs rend ces concepts inapplicables : il n'y a pas de "capacité" unique à contrôler, mais un ensemble de blocs alloués indépendamment.

Cela signifie aussi qu'il n'est pas possible de pré-allouer la mémoire pour éviter les allocations futures, comme on le ferait avec `std::vector::reserve()`.

---

## Invalidation des itérateurs

Les règles d'invalidation de `std::deque` sont plus complexes que celles de `std::vector` et constituent souvent une source de confusion. Elles distinguent les itérateurs des pointeurs/références :

### Insertion

| Position de l'insertion | Itérateurs | Pointeurs et références |  
|---|---|---|  
| En tête (`push_front`) | **Tous invalidés** | **Valides** |  
| En fin (`push_back`) | **Tous invalidés** | **Valides** |  
| Au milieu | **Tous invalidés** | **Tous invalidés** |

Le point surprenant : même un simple `push_back` invalide tous les itérateurs, alors que les pointeurs et références vers les éléments existants restent valides. Cela vient du fait que la table de pointeurs peut être réallouée (invalidant les itérateurs qui en dépendent), tandis que les blocs de données ne bougent pas (préservant les pointeurs et références).

### Suppression

| Position de la suppression | Itérateurs | Pointeurs et références |
|---|---|---|
| En tête (`pop_front`) | Seul l'élément supprimé | Seul l'élément supprimé |
| En fin (`pop_back`) | L'élément supprimé et `end()` | Seul l'élément supprimé |
| Au milieu | **Tous invalidés** | **Tous invalidés** |

```cpp
#include <deque>
#include <print>

int main() {
    std::deque<int> dq{10, 20, 30, 40, 50};

    int& ref = dq[2];       // référence vers 30
    int* ptr = &dq[2];      // pointeur vers 30
    auto it = dq.begin() + 2;  // itérateur vers 30

    dq.push_back(60);

    // ref et ptr sont VALIDES (les blocs n'ont pas bougé)
    std::println("ref={}, *ptr={}", ref, *ptr);  // 30, 30

    // it est INVALIDE — comportement indéfini
    // std::println("*it={}", *it);  // ❌ UB
}
```

> 💡 **En pratique** : si vous utilisez un `std::deque`, préférez stocker des index ou des pointeurs/références plutôt que des itérateurs si vous devez conserver des accès après des insertions aux extrémités.

---

## `std::deque` comme base de `std::stack` et `std::queue`

Les adaptateurs de conteneurs `std::stack` et `std::queue` de la STL utilisent `std::deque` comme conteneur sous-jacent par défaut :

```cpp
#include <stack>
#include <queue>
#include <print>

int main() {
    // std::stack utilise std::deque par défaut
    std::stack<int> pile;
    pile.push(10);
    pile.push(20);
    pile.push(30);
    std::println("top = {}", pile.top());  // 30

    // std::queue utilise std::deque par défaut
    std::queue<int> file;
    file.push(10);
    file.push(20);
    file.push(30);
    std::println("front = {}", file.front());  // 10
}
```

Ce choix est logique : `std::queue` nécessite `push_back` et `pop_front` en O(1), ce que `std::deque` fournit. `std::vector` ne peut pas servir de base à `std::queue` car son `pop_front` est O(n).

On peut substituer le conteneur sous-jacent si nécessaire :

```cpp
// Stack basée sur un vector (pas de push_front nécessaire)
std::stack<int, std::vector<int>> pile_vec;

// Queue basée sur une list
std::queue<int, std::list<int>> file_list;
```

---

## `std::deque` vs `std::vector` : quand choisir le deque ?

| Critère | `std::vector` | `std::deque` |  
|---|---|---|  
| Insertion en fin | O(1) amorti | O(1) amorti |  
| Insertion en tête | **O(n)** | **O(1) amorti** |  
| Accès par index | O(1) — rapide | O(1) — légèrement plus lent |  
| Contiguïté mémoire | Oui | Non |  
| `data()` pour interop C | Oui | Non |  
| `reserve()` | Oui | Non |  
| Réallocation | Copie tout le bloc | Alloue un nouveau petit bloc |  
| Localité de cache | Excellente | Bonne (intra-bloc) |  
| Mémoire libérée après suppression | Non (sauf `shrink_to_fit`) | Oui (blocs vides libérés) |

Les situations où `std::deque` est préférable à `std::vector` :

**Insertions/suppressions fréquentes en tête.** C'est le cas d'usage principal. Si votre structure fonctionne comme une file FIFO (ajout en fin, retrait en tête), `std::deque` est le choix naturel.

**Très grands jeux de données avec réallocations problématiques.** Un `std::vector` de 4 Go qui doit réallouer nécessite 8 Go supplémentaires temporairement (ancien + nouveau bloc). Un `std::deque` n'a jamais ce pic mémoire : il alloue de petits blocs indépendants.

**Libération progressive de la mémoire.** Quand on supprime des éléments aux extrémités d'un `std::deque`, les blocs vides sont libérés. Un `std::vector` conserve sa capacité après suppression.

Dans tous les autres cas, `std::vector` reste le meilleur choix grâce à sa simplicité, sa contiguïté mémoire et sa compatibilité avec les API C.

---

## Bonnes pratiques

**Utilisez `std::deque` quand vous avez besoin d'une file (FIFO).** C'est le cas d'usage canonique : `push_back` + `pop_front`. Si votre code n'utilise que `push_back` et jamais `push_front` ou `pop_front`, un `std::vector` est probablement un meilleur choix.

**Préférez `std::queue` et `std::stack` à un `std::deque` nu.** Si votre usage est strictement celui d'une file ou d'une pile, les adaptateurs expriment mieux l'intention et restreignent l'API aux seules opérations pertinentes.

**N'utilisez pas `std::deque` pour l'interopérabilité C.** L'absence de `data()` et de contiguïté rend le deque incompatible avec les API C. Si vous devez passer vos données à une fonction C, utilisez un `std::vector`.

**Soyez prudent avec les itérateurs.** Les règles d'invalidation des itérateurs de `std::deque` sont les plus complexes de tous les conteneurs séquentiels. En cas de doute, recalculez vos itérateurs après toute modification du conteneur.

**Considérez `std::deque` pour les très gros conteneurs.** Si votre `std::vector` atteint plusieurs gigaoctets et que les réallocations deviennent un problème (pic mémoire, latence), `std::deque` élimine ce problème en allouant par petits blocs.

⏭️ [std::span : Vue sur données contiguës (zéro allocation)](/13-conteneurs-sequentiels/05-span.md)
