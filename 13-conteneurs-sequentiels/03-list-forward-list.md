🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 13.3 — std::list et std::forward_list : Listes chaînées

## Chapitre 13 : Conteneurs Séquentiels

---

## Introduction

`std::list` et `std::forward_list` sont les deux conteneurs séquentiels de la STL basés sur des **listes chaînées**. `std::list` implémente une liste **doublement chaînée** (chaque nœud pointe vers le précédent et le suivant), tandis que `std::forward_list` (C++11) implémente une liste **simplement chaînée** (chaque nœud pointe uniquement vers le suivant).

Ces conteneurs occupent une niche bien précise : ils offrent une insertion et une suppression en **O(1) garanti** à n'importe quelle position (à condition de disposer d'un itérateur), et surtout, ils garantissent la **stabilité des itérateurs** — aucune insertion ni suppression n'invalide les itérateurs existants (sauf ceux pointant vers l'élément supprimé). C'est leur avantage décisif sur `std::vector`.

En contrepartie, leur **localité mémoire catastrophique** les rend significativement plus lents que `std::vector` pour le parcours séquentiel et la plupart des opérations réelles. Ils ne doivent être utilisés que dans des situations précises, identifiées par profiling.

---

## Anatomie d'une liste chaînée

### `std::list` : liste doublement chaînée

Chaque élément est stocké dans un **nœud** alloué indépendamment sur le heap. Un nœud contient la donnée, un pointeur vers le nœud précédent et un pointeur vers le nœud suivant :

```
std::list<int> lst{10, 20, 30};

  head                                              tail
   │                                                 │
   ▼                                                 ▼
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│ prev: nullptr│◀────│ prev         │◀────│ prev         │
│ data: 10     │     │ data: 20     │     │ data: 30     │
│ next  ───────│────▶│ next  ───────│────▶│ next: nullptr│
└──────────────┘     └──────────────┘     └──────────────┘
    0x7f1a...            0x7f3c...            0x7f8e...
       ▲                    ▲                    ▲
       │                    │                    │
  allocations indépendantes — adresses non contiguës
```

Chaque nœud est alloué séparément par `new`, ce qui signifie que les nœuds sont dispersés en mémoire. Le passage d'un nœud au suivant implique un saut à une adresse potentiellement éloignée, provoquant des *cache misses*.

### `std::forward_list` : liste simplement chaînée

`std::forward_list` supprime le pointeur `prev`, ce qui réduit la mémoire par nœud mais empêche le parcours inverse :

```
std::forward_list<int> flst{10, 20, 30};

  head
   │
   ▼
┌──────────┐     ┌──────────┐     ┌──────────┐
│ data: 10 │     │ data: 20 │     │ data: 30 │
│ next ────│────▶│ next ────│────▶│ next: nil│
└──────────┘     └──────────┘     └──────────┘
```

L'économie d'un pointeur par nœud (8 octets sur x86_64) peut sembler modeste, mais elle s'accumule sur des millions d'éléments. `std::forward_list` est conçu pour être aussi léger qu'une liste chaînée C écrite à la main — c'est son objectif affiché dans le standard.

---

## Déclaration et construction

### `std::list`

```cpp
#include <list>
#include <print>

int main() {
    // Construction vide
    std::list<int> l1;

    // Avec taille et valeur
    std::list<int> l2(5, 42);          // {42, 42, 42, 42, 42}

    // Par liste d'initialisation
    std::list<int> l3{10, 20, 30, 40};

    // Par copie
    std::list<int> l4 = l3;

    // Par plage d'itérateurs
    std::list<int> l5(l3.begin(), l3.end());

    // CTAD (C++17)
    std::list l6{1.0, 2.0, 3.0};      // std::list<double>

    std::println("l3 size : {}", l3.size());  // 4
}
```

### `std::forward_list`

```cpp
#include <forward_list>
#include <print>

int main() {
    std::forward_list<int> fl1;
    std::forward_list<int> fl2(5, 42);
    std::forward_list<int> fl3{10, 20, 30, 40};
    std::forward_list fl4{1.0, 2.0, 3.0};   // CTAD

    // ⚠️ forward_list n'a PAS de méthode size()
    // Il faut utiliser std::distance si nécessaire
    auto n = std::distance(fl3.begin(), fl3.end());
    std::println("fl3 éléments : {}", n);  // 4
}
```

L'absence de `size()` sur `std::forward_list` est un choix de design délibéré : maintenir un compteur interne ajouterait un surcoût mémoire (un `size_t` supplémentaire par liste) et un surcoût à chaque insertion/suppression. Le standard a choisi la performance minimale plutôt que la commodité.

---

## Opérations d'insertion

### Insertion dans `std::list`

`std::list` offre une insertion en O(1) à n'importe quelle position, à condition de disposer d'un itérateur. Contrairement à `std::vector`, aucun élément n'est déplacé — seuls les pointeurs des nœuds voisins sont mis à jour :

```cpp
#include <list>
#include <print>

int main() {
    std::list<int> lst{10, 30, 40};

    // Insertion en tête — O(1)
    lst.push_front(5);
    // {5, 10, 30, 40}

    // Insertion en fin — O(1)
    lst.push_back(50);
    // {5, 10, 30, 40, 50}

    // Insertion au milieu — O(1) une fois l'itérateur obtenu
    auto it = std::next(lst.begin(), 2);  // pointe sur 30
    lst.insert(it, 20);
    // {5, 10, 20, 30, 40, 50}

    // emplace : construction en place
    lst.emplace(it, 25);  // insère avant 30
    // {5, 10, 20, 25, 30, 40, 50}

    // emplace_front / emplace_back
    lst.emplace_front(1);
    lst.emplace_back(100);

    for (auto val : lst) std::print("{} ", val);
    // Sortie : 1 5 10 20 25 30 40 50 100
}
```

Un point essentiel : l'itérateur `it` qui pointait sur 30 **reste valide** après toutes ces insertions. C'est la grande force de `std::list`.

### Insertion dans `std::forward_list`

`std::forward_list` ne possède pas `push_back`, `insert`, ni `emplace`. À la place, elle offre des variantes **"after"** qui insèrent *après* un itérateur donné. La raison est que dans une liste simplement chaînée, insérer *avant* un nœud nécessiterait de connaître le nœud précédent — ce qui impliquerait un parcours O(n) depuis le début.

```cpp
#include <forward_list>
#include <print>

int main() {
    std::forward_list<int> fl{10, 30, 40};

    // Insertion en tête — la seule opération "push"
    fl.push_front(5);
    // {5, 10, 30, 40}

    // Insertion après un itérateur — O(1)
    auto it = fl.begin();              // pointe sur 5
    std::advance(it, 1);              // pointe sur 10
    fl.insert_after(it, 20);
    // {5, 10, 20, 30, 40}

    // emplace_after : construction en place
    fl.emplace_after(it, 15);
    // {5, 10, 15, 20, 30, 40}

    // before_begin() : itérateur spécial avant le premier élément
    // Permet d'insérer au tout début sans push_front
    fl.insert_after(fl.before_begin(), 1);
    // {1, 5, 10, 15, 20, 30, 40}

    for (auto val : fl) std::print("{} ", val);
    // Sortie : 1 5 10 15 20 30 40
}
```

`before_begin()` est un itérateur spécial propre à `std::forward_list`. Il pointe *avant* le premier élément et ne peut pas être déréférencé (`*fl.before_begin()` est un comportement indéfini). Son seul usage est de servir de point d'ancrage pour `insert_after` et `erase_after` lorsqu'on veut opérer sur le premier élément.

---

## Opérations de suppression

### `std::list`

```cpp
#include <list>
#include <print>

int main() {
    std::list<int> lst{10, 20, 30, 40, 50, 60};

    // Suppression en tête et en fin — O(1)
    lst.pop_front();   // {20, 30, 40, 50, 60}
    lst.pop_back();    // {20, 30, 40, 50}

    // Suppression par itérateur — O(1)
    auto it = std::next(lst.begin(), 1);  // pointe sur 30
    auto suivant = lst.erase(it);
    // {20, 40, 50} — suivant pointe sur 40

    std::println("Après erase : *suivant = {}", *suivant);  // 40

    // Suppression par valeur — supprime TOUTES les occurrences
    lst.push_back(40);
    // {20, 40, 50, 40}
    lst.remove(40);
    // {20, 50}

    // Suppression par prédicat
    lst.push_back(15);
    lst.push_back(25);
    // {20, 50, 15, 25}
    lst.remove_if([](int n) { return n > 20; });
    // {20, 15}

    for (auto val : lst) std::print("{} ", val);
    // Sortie : 20 15
}
```

### `std::forward_list`

Comme pour l'insertion, la suppression utilise des variantes **"after"** :

```cpp
#include <forward_list>
#include <print>

int main() {
    std::forward_list<int> fl{10, 20, 30, 40, 50};

    // Suppression du premier élément
    fl.pop_front();
    // {20, 30, 40, 50}

    // Suppression après un itérateur
    auto it = fl.begin();   // pointe sur 20
    fl.erase_after(it);     // supprime 30
    // {20, 40, 50}

    // Suppression d'une plage (after)
    // fl.erase_after(first, last) supprime ]first, last[
    fl.erase_after(fl.begin(), fl.end());
    // {20}

    // remove et remove_if fonctionnent comme pour std::list
    fl = {5, 10, 15, 20, 25};
    fl.remove_if([](int n) { return n % 10 != 0; });
    // {10, 20}

    for (auto val : fl) std::print("{} ", val);
    // Sortie : 10 20
}
```

> 💡 Depuis C++20, `std::erase` et `std::erase_if` fonctionnent aussi avec `std::list` et `std::forward_list`, offrant une syntaxe unifiée identique à celle de `std::vector`.

---

## Opérations spécifiques aux listes

Les listes chaînées possèdent des opérations qui exploitent leur structure en nœuds pour atteindre des performances impossibles avec un `std::vector`. Ces opérations manipulent les pointeurs des nœuds sans copier ni déplacer aucune donnée.

### `splice` : transfert de nœuds entre listes

`splice` transfère des nœuds d'une liste à une autre en O(1) — seuls les pointeurs sont réarrangés, aucun élément n'est copié ni alloué :

```cpp
#include <list>
#include <print>

auto afficher = [](const std::string& nom, const std::list<int>& l) {
    std::print("{}: ", nom);
    for (auto val : l) std::print("{} ", val);
    std::println("");
};

int main() {
    std::list<int> a{1, 2, 3, 4, 5};
    std::list<int> b{100, 200, 300};

    // Transférer TOUS les éléments de b dans a (avant la position donnée)
    auto pos = std::next(a.begin(), 2);  // pointe sur 3
    a.splice(pos, b);
    // a = {1, 2, 100, 200, 300, 3, 4, 5}
    // b = {} (vide — les nœuds ont été transférés, pas copiés)

    afficher("a", a);  // a: 1 2 100 200 300 3 4 5
    afficher("b", b);  // b:

    // Transférer un seul élément
    std::list<int> c{999};
    a.splice(a.begin(), c, c.begin());
    afficher("a", a);  // a: 999 1 2 100 200 300 3 4 5

    // Transférer une plage
    std::list<int> d{7, 8, 9};
    a.splice(a.end(), d, d.begin(), d.end());
    afficher("a", a);  // a: 999 1 2 100 200 300 3 4 5 7 8 9
}
```

`splice` est l'opération qui justifie à elle seule l'existence de `std::list` dans certains cas d'usage. Transférer un million d'éléments entre deux listes est une opération O(1) constante, alors que le même transfert avec `std::vector` nécessiterait de copier ou déplacer chaque élément.

`std::forward_list` offre `splice_after` avec la même sémantique, adaptée à l'API "after".

### `sort` : tri intégré

Les listes ont leur propre méthode `sort()` car `std::sort` de la STL requiert des itérateurs à accès aléatoire, que les listes ne fournissent pas :

```cpp
#include <list>
#include <print>

int main() {
    std::list<int> lst{50, 20, 40, 10, 30};

    // Tri par défaut (croissant)
    lst.sort();
    for (auto val : lst) std::print("{} ", val);
    std::println("");  // 10 20 30 40 50

    // Tri avec comparateur personnalisé (décroissant)
    lst.sort(std::greater<int>{});
    for (auto val : lst) std::print("{} ", val);
    std::println("");  // 50 40 30 20 10
}
```

Le tri de `std::list` est un merge sort en O(n log n), mais en pratique significativement plus lent que le tri d'un `std::vector` à cause de la mauvaise localité de cache.

### `merge` : fusion de listes triées

`merge` fusionne deux listes **déjà triées** en une seule liste triée, en O(n). Comme `splice`, c'est une opération par manipulation de pointeurs :

```cpp
#include <list>
#include <print>

int main() {
    std::list<int> a{1, 3, 5, 7};
    std::list<int> b{2, 4, 6, 8};

    a.merge(b);
    // a = {1, 2, 3, 4, 5, 6, 7, 8}
    // b = {} (vide)

    for (auto val : a) std::print("{} ", val);
    // Sortie : 1 2 3 4 5 6 7 8
}
```

> ⚠️ Les deux listes **doivent** être triées avant l'appel à `merge`. Si elles ne le sont pas, le résultat est indéfini selon le standard (en pratique, vous obtiendrez une liste dans un ordre imprévisible).

### `unique` : suppression des doublons consécutifs

```cpp
#include <list>
#include <print>

int main() {
    std::list<int> lst{1, 1, 2, 3, 3, 3, 4, 4, 5};

    lst.unique();
    // {1, 2, 3, 4, 5} — seuls les doublons CONSÉCUTIFS sont supprimés

    for (auto val : lst) std::print("{} ", val);
    // Sortie : 1 2 3 4 5
}
```

Pour supprimer tous les doublons (pas seulement les consécutifs), triez la liste d'abord avec `sort()`, puis appelez `unique()`.

### `reverse` : inversion en O(n)

```cpp
#include <list>
#include <print>

int main() {
    std::list<int> lst{1, 2, 3, 4, 5};

    lst.reverse();

    for (auto val : lst) std::print("{} ", val);
    // Sortie : 5 4 3 2 1
}
```

---

## Invalidation des itérateurs

C'est l'avantage majeur des listes chaînées par rapport à `std::vector` :

| Opération | `std::list` / `std::forward_list` |  
|---|---|  
| Insertion (n'importe où) | **Aucune invalidation** |  
| Suppression | **Seul** l'itérateur vers l'élément supprimé |  
| `splice` / `splice_after` | **Aucune invalidation** (les itérateurs suivent les nœuds) |  
| `sort` | **Aucune invalidation** (les nœuds sont réarrangés, pas recréés) |  
| `merge` | **Aucune invalidation** |  
| `clear` | Tous les itérateurs |

Cette stabilité est fondamentale dans les scénarios où des structures externes (index secondaires, graphes, callbacks) maintiennent des itérateurs ou des pointeurs vers les éléments du conteneur.

```cpp
#include <list>
#include <print>

int main() {
    std::list<int> lst{10, 20, 30, 40, 50};

    auto it30 = std::next(lst.begin(), 2);  // pointe sur 30
    std::println("Avant : *it30 = {}", *it30);

    // Insertions massives
    lst.push_front(1);
    lst.push_back(99);
    lst.insert(it30, 25);  // insère 25 AVANT 30

    // it30 pointe toujours sur 30
    std::println("Après : *it30 = {}", *it30);  // 30

    // Suppression d'éléments voisins
    lst.pop_front();
    lst.pop_back();

    // it30 pointe toujours sur 30
    std::println("Encore : *it30 = {}", *it30);  // 30
}
```

---

## Le coût caché : localité de cache

La section d'introduction du chapitre a présenté le problème de localité de cache. Il est crucial de le quantifier pour comprendre pourquoi `std::list` est rarement le bon choix en pratique.

Considérons un parcours séquentiel de n éléments entiers. Avec `std::vector`, les éléments sont contigus : chaque ligne de cache (64 octets) contient 16 `int` consécutifs. Le processeur précharge les lignes suivantes (*prefetching*), et le parcours se fait presque à la vitesse de la bande passante mémoire.

Avec `std::list`, chaque nœud contient la donnée (4 octets pour un `int`) plus deux pointeurs (16 octets sur x86_64), soit au minimum 24 octets par nœud — auxquels s'ajoute le surcoût d'alignement et les métadonnées de l'allocateur. Chaque nœud est alloué indépendamment et peut se trouver n'importe où en mémoire. Le passage d'un nœud au suivant est un saut imprévisible pour le prefetcher du processeur.

```
Surcoût mémoire par élément (type int, x86_64) :

std::vector<int>       :  4 octets/élément   (données uniquement)  
std::list<int>         : ~40 octets/élément   (données + 2 pointeurs + overhead allocateur)  
std::forward_list<int> : ~32 octets/élément   (données + 1 pointeur + overhead allocateur)  
```

Pour stocker 1 million d'entiers, un `std::vector` utilise environ 4 Mo, tandis qu'une `std::list` utilise environ 40 Mo — 10 fois plus. Et ces 40 Mo sont dispersés en mémoire, rendant le prefetching du CPU inefficace.

En pratique, même pour une insertion au milieu (opération théoriquement O(1) pour une liste et O(n) pour un vecteur), `std::vector` est souvent plus rapide que `std::list` jusqu'à plusieurs milliers d'éléments, car le coût du décalage d'éléments contigus en mémoire est compensé par l'efficacité du cache.

---

## Quand utiliser une liste chaînée ?

Les listes chaînées sont le bon choix dans un nombre limité de situations spécifiques.

**Stabilité des itérateurs et des adresses requise.** Si des structures externes maintiennent des pointeurs ou itérateurs vers les éléments, et que des insertions/suppressions fréquentes doivent préserver ces références, `std::list` est le seul conteneur séquentiel qui le garantit.

**Transferts fréquents entre conteneurs avec `splice`.** Si votre algorithme déplace fréquemment des blocs d'éléments entre plusieurs conteneurs (scheduler, file de priorité évolutive), `splice` en O(1) est imbattable.

**Insertions/suppressions massives au milieu, confirmées par profiling.** Si le profiling démontre que les insertions au milieu sont le goulot d'étranglement *et* que le jeu de données est suffisamment grand pour que le O(n) du décalage dans un vecteur soit réellement un problème, alors une liste peut être bénéfique. Mais mesurez d'abord.

**`std::forward_list` pour l'empreinte mémoire minimale.** Si vous implémentez une table de hachage avec chaînage séparé, ou toute structure où des milliers de petites listes coexistent, `std::forward_list` offre le surcoût minimal.

Dans tous les autres cas, commencez par `std::vector`.

---

## Récapitulatif comparatif

| Caractéristique | `std::list` | `std::forward_list` |  
|---|---|---|  
| Type de chaînage | Double | Simple |  
| Parcours | Bidirectionnel | Avant uniquement |  
| `size()` | O(1) | **Indisponible** |  
| `push_back` | O(1) | **Indisponible** |  
| `push_front` | O(1) | O(1) |  
| `insert` / `insert_after` | O(1) avec itérateur | O(1) avec itérateur |  
| `pop_back` | O(1) | **Indisponible** |  
| `pop_front` | O(1) | O(1) |  
| `splice` / `splice_after` | O(1) | O(1) |  
| `sort` | O(n log n) | O(n log n) |  
| `merge` | O(n) | O(n) |  
| `reverse` | O(n) | O(n) |  
| Surcoût/élément (x86_64) | ~2 pointeurs + alloc | ~1 pointeur + alloc |  
| Itérateurs | Bidirectionnels | Forward |  
| Stabilité itérateurs | Totale | Totale |  
| Accès par index | Non (O(n)) | Non (O(n)) |  
| `before_begin()` | Non | Oui |

---

## Bonnes pratiques

**Ne choisissez pas une liste par réflexe algorithmique.** Le fait qu'une opération soit O(1) pour une liste et O(n) pour un vecteur ne signifie pas que la liste sera plus rapide en pratique. La constante cachée dans le O(1) d'une liste (cache miss, allocation) est souvent bien supérieure au O(n) d'un vecteur pour des tailles réalistes.

**Mesurez avant de migrer vers une liste.** Si vous suspectez qu'un `std::vector` est trop lent pour vos insertions/suppressions, profilez les deux alternatives sur vos données réelles. Le résultat est souvent contre-intuitif.

**Préférez `std::forward_list` à `std::list` quand vous n'avez pas besoin du parcours inverse.** L'économie d'un pointeur par nœud est significative sur de grandes collections, et l'API "after" rappelle au développeur la nature fondamentale du conteneur.

**Exploitez `splice` quand c'est pertinent.** C'est l'opération qui distingue véritablement les listes des autres conteneurs. Si votre algorithme ne bénéficie pas de `splice` ni de la stabilité des itérateurs, vous n'avez probablement pas besoin d'une liste.

**Utilisez `remove` et `remove_if` plutôt que l'idiome erase-remove.** Les méthodes membres de `std::list` et `std::forward_list` sont plus efficaces que la combinaison `std::remove_if` + `erase`, car elles manipulent directement les pointeurs des nœuds sans déplacer les éléments.

⏭️ [std::deque : File double entrée](/13-conteneurs-sequentiels/04-deque.md)
