🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 15.4 — Manipulation : std::copy, std::move, std::remove_if

## Chapitre 15 — Algorithmes de la STL

---

## Introduction

Les algorithmes de manipulation réorganisent, copient, déplacent ou éliminent des éléments au sein d'une séquence ou entre deux séquences. Contrairement aux algorithmes de transformation (section 15.3) qui produisent de nouvelles valeurs, les algorithmes de manipulation travaillent sur les éléments **tels qu'ils sont** — ils les repositionnent sans les modifier fondamentalement.

Cette famille regroupe des opérations omniprésentes dans le code courant : copier une partie d'un conteneur vers un autre, supprimer les éléments indésirables, éliminer les doublons, inverser ou faire pivoter une séquence. Tous se trouvent dans `<algorithm>`.

```cpp
#include <algorithm>
#include <vector>
#include <string>
#include <iterator>   // std::back_inserter, std::ostream_iterator
```

---

## Copie : std::copy et ses variantes

### std::copy — Copie de base

`std::copy` copie les éléments de l'intervalle source `[first, last)` vers une destination. Il renvoie un itérateur pointant juste après le dernier élément écrit :

```cpp
std::vector<int> src = {1, 2, 3, 4, 5};  
std::vector<int> dst(src.size());  

std::copy(src.begin(), src.end(), dst.begin());
// dst == {1, 2, 3, 4, 5}
```

La destination doit avoir une capacité suffisante. Écrire au-delà de la fin est un comportement indéfini silencieux. Deux approches sûres :

```cpp
// Approche 1 : pré-allouer
std::vector<int> dst(src.size());  
std::copy(src.begin(), src.end(), dst.begin());  

// Approche 2 : back_inserter (insertion dynamique)
std::vector<int> dst;  
std::copy(src.begin(), src.end(), std::back_inserter(dst));  
```

### Copier un sous-intervalle

La puissance de la paire d'itérateurs : on copie exactement ce qu'on veut.

```cpp
std::vector<int> src = {10, 20, 30, 40, 50, 60, 70};  
std::vector<int> dst;  

// Copier les éléments d'index 2 à 4 inclus
std::copy(src.begin() + 2, src.begin() + 5, std::back_inserter(dst));
// dst == {30, 40, 50}
```

### std::copy_if — Copie filtrée

`std::copy_if` ne copie que les éléments satisfaisant un prédicat. C'est l'équivalent d'un filtre :

```cpp
std::vector<int> src = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};  
std::vector<int> evens;  

std::copy_if(src.begin(), src.end(), std::back_inserter(evens),
    [](int x) { return x % 2 == 0; }
);
// evens == {2, 4, 6, 8, 10}
```

Un cas d'usage DevOps — extraire les entrées de log d'un niveau donné :

```cpp
struct LogEntry {
    std::string message;
    int level;  // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR
};

std::vector<LogEntry> all_logs = /* ... */;  
std::vector<LogEntry> errors;  

std::copy_if(all_logs.begin(), all_logs.end(), std::back_inserter(errors),
    [](const LogEntry& e) { return e.level >= 3; }
);
```

### std::copy_n — Copier N éléments

```cpp
std::vector<int> src = {10, 20, 30, 40, 50};  
std::vector<int> dst;  

std::copy_n(src.begin(), 3, std::back_inserter(dst));
// dst == {10, 20, 30}
```

### std::copy_backward — Copie en sens inverse

`std::copy_backward` copie les éléments en partant de la fin vers le début. Son usage principal est le décalage d'éléments **vers la droite** à l'intérieur d'un même conteneur, où `std::copy` classique corromprait les données à cause du chevauchement :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 0, 0};

// Décaler les 5 premiers éléments d'une position vers la droite
// std::copy serait incorrect ici (chevauchement, copie vers la droite)
std::copy_backward(v.begin(), v.begin() + 5, v.begin() + 6);
// v == {1, 1, 2, 3, 4, 5, 0}

v[0] = 99;  // insérer la nouvelle valeur au début
// v == {99, 1, 2, 3, 4, 5, 0}
```

La règle pour le chevauchement :

- **Décalage vers la droite** (destination après la source) → `std::copy_backward`.
- **Décalage vers la gauche** (destination avant la source) → `std::copy`.
- **Aucun chevauchement** → l'un ou l'autre, sans différence.

### Versions Ranges (C++20)

```cpp
std::vector<int> src = {1, 2, 3, 4, 5};  
std::vector<int> dst(src.size());  

std::ranges::copy(src, dst.begin());

std::vector<int> evens;  
std::ranges::copy_if(src, std::back_inserter(evens),  
    [](int x) { return x % 2 == 0; }
);
```

---

## Déplacement : std::move (algorithme)

Il ne faut pas confondre `std::move` le **cast** (déclaré dans `<utility>`, vu au chapitre 10) avec `std::move` l'**algorithme** (déclaré dans `<algorithm>`). L'algorithme `std::move` déplace les éléments d'une séquence source vers une destination, en utilisant la sémantique de mouvement :

```cpp
std::vector<std::string> src = {"alpha", "bravo", "charlie", "delta"};  
std::vector<std::string> dst(src.size());  

std::move(src.begin(), src.end(), dst.begin());
// dst == {"alpha", "bravo", "charlie", "delta"}
// src contient des chaînes dans un état "moved-from" (valide mais indéterminé)
// Typiquement des chaînes vides, mais ce n'est pas garanti par le standard
```

L'intérêt est la **performance** : déplacer des `std::string` ou des `std::vector` est en O(1) par élément (transfert de pointeurs internes), tandis que les copier est en O(n) par élément. Sur de grandes collections d'objets lourds, la différence est majeure.

### std::move_backward

Même logique que `copy_backward`, pour les décalages vers la droite avec chevauchement :

```cpp
std::vector<std::string> v = {"A", "B", "C", "D", "E", "", ""};

std::move_backward(v.begin(), v.begin() + 5, v.begin() + 6);
// Décale les 5 premiers éléments d'une position vers la droite par mouvement
```

### Quand utiliser std::move (algorithme) vs std::copy

La règle est directe : si vous n'avez plus besoin des éléments source après l'opération, utilisez `std::move`. Si vous en avez encore besoin, utilisez `std::copy`. Le cas typique du `std::move` algorithme est le transfert d'éléments entre deux conteneurs quand la source sera détruite ou réinitialisée juste après.

---

## Suppression : std::remove, std::remove_if et le Remove-Erase Idiom

### Le problème fondamental

Les algorithmes de la STL opèrent sur des **itérateurs**, pas sur des conteneurs. Ils ne peuvent donc pas modifier la taille d'un conteneur — ils n'ont pas accès au conteneur lui-même. C'est une conséquence directe de la séparation conteneur/algorithme, et c'est le piège le plus célèbre de la STL.

`std::remove` et `std::remove_if` ne **suppriment rien**. Ils réorganisent les éléments en déplaçant ceux à conserver au début de la séquence, et renvoient un itérateur vers la **nouvelle fin logique**. Les éléments entre cette nouvelle fin et la fin réelle du conteneur sont dans un état valide mais indéterminé.

### std::remove — Retirer par valeur

```cpp
std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};

auto new_end = std::remove(v.begin(), v.end(), 2);
// v == {1, 3, 5, 7, ?, ?, ?}  — les ? sont des valeurs indéterminées
//                   ▲
//               new_end

// La taille du vector n'a PAS changé
std::print("Taille : {}\n", v.size());
// Taille : 7 — toujours 7 !
```

Pour réellement supprimer les éléments, il faut appeler `.erase()` sur le conteneur avec l'itérateur renvoyé :

```cpp
std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};

// Le Remove-Erase Idiom classique
v.erase(std::remove(v.begin(), v.end(), 2), v.end());
// v == {1, 3, 5, 7}
// v.size() == 4
```

C'est une ligne idiomatique que tout développeur C++ doit reconnaître. Elle se lit : « efface tous les éléments à partir de la nouvelle fin logique (renvoyée par `remove`) jusqu'à la fin réelle du conteneur ».

### std::remove_if — Retirer selon un prédicat

Même principe, avec un prédicat :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Supprimer les multiples de 3
v.erase(
    std::remove_if(v.begin(), v.end(), [](int x) { return x % 3 == 0; }),
    v.end()
);
// v == {1, 2, 4, 5, 7, 8, 10}
```

Sur un objet complexe :

```cpp
struct Process {
    std::string name;
    int pid;
    bool zombie;
};

std::vector<Process> procs = {
    {"nginx", 1001, false},
    {"defunct", 1042, true},
    {"postgres", 1100, false},
    {"old_worker", 1200, true},
    {"redis", 1300, false}
};

// Supprimer les processus zombies
procs.erase(
    std::remove_if(procs.begin(), procs.end(),
        [](const Process& p) { return p.zombie; }),
    procs.end()
);
// procs == {nginx, postgres, redis}
```

### C++20 : std::erase et std::erase_if — Enfin la simplicité

C++20 a ajouté des fonctions libres `std::erase` et `std::erase_if` pour les conteneurs standard. Elles encapsulent le remove-erase idiom en un seul appel et renvoient le nombre d'éléments supprimés :

```cpp
std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};

// C++20 — une seule ligne, pas de piège
auto removed = std::erase(v, 2);
// v == {1, 3, 5, 7}
// removed == 3

std::vector<int> v2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};  
auto removed2 = std::erase_if(v2, [](int x) { return x % 3 == 0; });  
// v2 == {1, 2, 4, 5, 7, 8, 10}
// removed2 == 3
```

Ces fonctions existent pour `vector`, `deque`, `string`, `list`, `forward_list`, `set`, `map`, `unordered_set`, `unordered_map` et leurs variantes `multi`. Pour du code C++20, elles remplacent totalement le remove-erase idiom.

---

## Déduplication : std::unique

`std::unique` supprime les **doublons consécutifs** d'une séquence. Comme `std::remove`, il ne modifie pas la taille du conteneur — il renvoie un itérateur vers la nouvelle fin logique :

```cpp
std::vector<int> v = {1, 1, 2, 2, 2, 3, 3, 4, 5, 5};

auto new_end = std::unique(v.begin(), v.end());  
v.erase(new_end, v.end());  
// v == {1, 2, 3, 4, 5}
```

Le mot-clé est **consécutifs**. Si les doublons ne sont pas adjacents, `std::unique` ne les voit pas :

```cpp
std::vector<int> v = {1, 3, 2, 1, 3, 2};

auto new_end = std::unique(v.begin(), v.end());  
v.erase(new_end, v.end());  
// v == {1, 3, 2, 1, 3, 2} — aucun doublon consécutif, rien ne change
```

Pour supprimer tous les doublons (consécutifs ou non), il faut d'abord **trier** la séquence :

```cpp
std::vector<int> v = {3, 1, 2, 1, 3, 2, 4};

std::sort(v.begin(), v.end());
// v == {1, 1, 2, 2, 3, 3, 4}

v.erase(std::unique(v.begin(), v.end()), v.end());
// v == {1, 2, 3, 4}
```

Ce pattern sort + unique + erase est idiomatique pour la déduplication complète. L'ensemble de l'opération est en O(n log n) dominé par le tri.

### std::unique avec prédicat

On peut personnaliser la notion de « doublon » avec un prédicat binaire :

```cpp
struct LogEntry {
    std::string message;
    int severity;
};

std::vector<LogEntry> logs = {
    {"disk full", 3},
    {"disk full", 3},
    {"disk full", 3},
    {"service ok", 1},
    {"timeout", 2},
    {"timeout", 2}
};

// Dédupliquer les messages consécutifs identiques
logs.erase(
    std::unique(logs.begin(), logs.end(),
        [](const LogEntry& a, const LogEntry& b) {
            return a.message == b.message;
        }),
    logs.end()
);
// logs == {"disk full"(3), "service ok"(1), "timeout"(2)}
```

---

## Inversion : std::reverse

`std::reverse` inverse l'ordre des éléments in-place :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

std::reverse(v.begin(), v.end());
// v == {5, 4, 3, 2, 1}
```

Pour obtenir une copie inversée sans modifier l'original, `std::reverse_copy` écrit dans une destination :

```cpp
std::vector<int> src = {1, 2, 3, 4, 5};  
std::vector<int> dst(src.size());  

std::reverse_copy(src.begin(), src.end(), dst.begin());
// dst == {5, 4, 3, 2, 1}
// src inchangé
```

---

## Rotation : std::rotate

`std::rotate` fait pivoter les éléments de sorte que l'élément pointé par l'itérateur `middle` devienne le premier :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

std::rotate(v.begin(), v.begin() + 2, v.end());
// v == {3, 4, 5, 1, 2}
//       ^^^^^^^^^  ^^^^
//       anciens [2..4]  anciens [0..1]
```

La signature est `rotate(first, middle, last)` : les éléments de `[middle, last)` sont placés avant ceux de `[first, middle)`.

Cas d'usage — implémenter un buffer circulaire, ou déplacer un élément à une position arbitraire :

```cpp
// Déplacer l'élément à l'index 4 vers l'index 1
std::vector<std::string> menu = {"Home", "About", "Blog", "Contact", "Shop"};

// Rotation de la sous-séquence [1, 5) pour amener l'index 4 en position 1
std::rotate(menu.begin() + 1, menu.begin() + 4, menu.end());
// menu == {"Home", "Shop", "About", "Blog", "Contact"}
```

`std::rotate` est en O(n) et opère in-place. `std::rotate_copy` écrit dans une destination sans modifier la source.

---

## Mélange aléatoire : std::shuffle

`std::shuffle` (C++11) réordonne aléatoirement les éléments en utilisant un générateur de nombres aléatoires :

```cpp
#include <random>

std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

std::mt19937 rng(42);  // seed fixe pour reproductibilité  
std::shuffle(v.begin(), v.end(), rng);  
// v est dans un ordre aléatoire, reproductible avec la même seed
```

L'ancien `std::random_shuffle` (déprécié en C++14, supprimé en C++17) utilisait `rand()` en interne, ce qui posait des problèmes de qualité aléatoire et de thread-safety. Utilisez toujours `std::shuffle` avec un moteur `<random>`.

---

## Partitionnement : std::partition et std::stable_partition

`std::partition` réorganise les éléments de sorte que tous ceux satisfaisant le prédicat se retrouvent **avant** ceux qui ne le satisfont pas. Il renvoie un itérateur vers le début de la seconde partition :

```cpp
std::vector<int> v = {8, 3, 5, 1, 9, 2, 7, 4, 6};

auto partition_point = std::partition(v.begin(), v.end(),
    [](int x) { return x % 2 == 0; }
);
// Éléments pairs au début, impairs à la fin
// v pourrait être {8, 4, 6, 2, 9, 1, 7, 5, 3} (l'ordre interne n'est pas garanti)
//                  ^^^^^^^^^^^  ^^^^^^^^^^^^^^^
//                   pairs        impairs
//                              ▲ partition_point

std::print("Nombre de pairs : {}\n",
           std::distance(v.begin(), partition_point));
// Nombre de pairs : 4
```

### std::stable_partition — Avec préservation de l'ordre

`std::stable_partition` fait la même chose mais **préserve l'ordre relatif** au sein de chaque partition :

```cpp
std::vector<int> v = {8, 3, 5, 1, 9, 2, 7, 4, 6};

auto pp = std::stable_partition(v.begin(), v.end(),
    [](int x) { return x % 2 == 0; }
);
// v == {8, 2, 4, 6, 3, 5, 1, 9, 7}
//       ^^^^^^^^^^  ^^^^^^^^^^^^^^^^
//       pairs (ordre préservé)  impairs (ordre préservé)
```

Un cas d'usage fréquent — trier une file de tâches en plaçant les prioritaires devant sans perturber l'ordre d'arrivée :

```cpp
struct Task {
    std::string name;
    bool urgent;
    int arrival_order;
};

std::vector<Task> queue = {
    {"backup", false, 1},
    {"deploy", true, 2},
    {"cleanup", false, 3},
    {"hotfix", true, 4},
    {"report", false, 5}
};

std::stable_partition(queue.begin(), queue.end(),
    [](const Task& t) { return t.urgent; }
);
// queue == {deploy(2), hotfix(4), backup(1), cleanup(3), report(5)}
//           ^^^^^^^^^^^^^^^^^^    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//           urgentes (ordre 2→4)  non-urgentes (ordre 1→3→5)
```

### std::partition_copy — Séparer dans deux conteneurs

`std::partition_copy` copie les éléments dans deux destinations distinctes selon le prédicat, en conservant la source intacte :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};  
std::vector<int> evens, odds;  

std::partition_copy(v.begin(), v.end(),
    std::back_inserter(evens),
    std::back_inserter(odds),
    [](int x) { return x % 2 == 0; }
);
// evens == {2, 4, 6, 8, 10}
// odds  == {1, 3, 5, 7, 9}
// v inchangé
```

### std::is_partitioned et std::partition_point

`std::is_partitioned` vérifie si une séquence est déjà partitionnée selon un prédicat :

```cpp
std::vector<int> v = {2, 4, 6, 1, 3, 5};

bool partitioned = std::is_partitioned(v.begin(), v.end(),
    [](int x) { return x % 2 == 0; }
);
// true — tous les pairs sont avant tous les impairs
```

`std::partition_point` renvoie l'itérateur vers le point de partition d'une séquence déjà partitionnée (en O(log n) avec des itérateurs random-access) :

```cpp
auto pp = std::partition_point(v.begin(), v.end(),
    [](int x) { return x % 2 == 0; }
);
// *pp == 1 (premier élément ne satisfaisant pas le prédicat)
```

---

## Remplissage : std::fill et std::fill_n

`std::fill` assigne une valeur identique à tous les éléments d'un intervalle :

```cpp
std::vector<int> v(10);

std::fill(v.begin(), v.end(), 42);
// v == {42, 42, 42, 42, 42, 42, 42, 42, 42, 42}

// Remplir partiellement
std::fill(v.begin(), v.begin() + 5, 0);
// v == {0, 0, 0, 0, 0, 42, 42, 42, 42, 42}
```

`std::fill_n` remplit N éléments à partir d'un itérateur de début :

```cpp
std::vector<int> v(10, 0);

std::fill_n(v.begin() + 3, 4, 99);
// v == {0, 0, 0, 99, 99, 99, 99, 0, 0, 0}
```

---

## Échange : std::swap_ranges et std::iter_swap

`std::swap_ranges` échange les éléments de deux séquences de même longueur :

```cpp
std::vector<int> a = {1, 2, 3, 4, 5};  
std::vector<int> b = {10, 20, 30, 40, 50};  

std::swap_ranges(a.begin(), a.end(), b.begin());
// a == {10, 20, 30, 40, 50}
// b == {1, 2, 3, 4, 5}
```

`std::iter_swap` échange les valeurs pointées par deux itérateurs individuels :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

std::iter_swap(v.begin(), v.begin() + 4);
// v == {5, 2, 3, 4, 1}
```

---

## Remplacement : std::replace et std::replace_if

`std::replace` remplace toutes les occurrences d'une valeur par une autre, in-place :

```cpp
std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};

std::replace(v.begin(), v.end(), 2, 99);
// v == {1, 99, 3, 99, 5, 99, 7}
```

`std::replace_if` remplace selon un prédicat :

```cpp
std::vector<int> v = {1, -2, 3, -4, 5, -6, 7};

std::replace_if(v.begin(), v.end(),
    [](int x) { return x < 0; }, 0
);
// v == {1, 0, 3, 0, 5, 0, 7}
```

Les variantes `std::replace_copy` et `std::replace_copy_if` écrivent dans une destination sans modifier la source.

---

## Opérations ensemblistes sur séquences triées

La STL fournit des algorithmes pour traiter deux séquences **triées** comme des ensembles. Ces algorithmes nécessitent que les deux séquences soient triées selon le même critère :

```cpp
std::vector<int> a = {1, 2, 3, 4, 5};  
std::vector<int> b = {3, 4, 5, 6, 7};  
std::vector<int> result;  

// Union : tous les éléments (sans doublons si les sources n'en ont pas)
std::set_union(a.begin(), a.end(), b.begin(), b.end(),
    std::back_inserter(result));
// result == {1, 2, 3, 4, 5, 6, 7}

result.clear();

// Intersection : éléments communs
std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
    std::back_inserter(result));
// result == {3, 4, 5}

result.clear();

// Différence : éléments de a absents de b
std::set_difference(a.begin(), a.end(), b.begin(), b.end(),
    std::back_inserter(result));
// result == {1, 2}

result.clear();

// Différence symétrique : éléments dans a ou b, mais pas les deux
std::set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(),
    std::back_inserter(result));
// result == {1, 2, 6, 7}
```

`std::merge` fusionne deux séquences triées en une seule séquence triée :

```cpp
std::vector<int> a = {1, 3, 5, 7};  
std::vector<int> b = {2, 4, 6, 8};  
std::vector<int> merged;  

std::merge(a.begin(), a.end(), b.begin(), b.end(),
    std::back_inserter(merged));
// merged == {1, 2, 3, 4, 5, 6, 7, 8}
```

`std::inplace_merge` fusionne deux moitiés triées **à l'intérieur d'un même conteneur** :

```cpp
std::vector<int> v = {1, 3, 5, 7, 2, 4, 6, 8};
//                     ^^^^^^^^^  ^^^^^^^^^
//                     trié       trié

std::inplace_merge(v.begin(), v.begin() + 4, v.end());
// v == {1, 2, 3, 4, 5, 6, 7, 8}
```

`std::includes` vérifie si une séquence triée contient tous les éléments d'une autre :

```cpp
std::vector<int> all = {1, 2, 3, 4, 5, 6, 7, 8};  
std::vector<int> sub = {2, 4, 6};  

bool contains = std::includes(all.begin(), all.end(), sub.begin(), sub.end());
// contains == true
```

Toutes ces opérations sont en O(n + m) — un parcours linéaire des deux séquences suffit grâce au prérequis de tri.

---

## Résumé : choisir le bon algorithme de manipulation

| Besoin | Algorithme | Modifie la source | Complexité |
|---|---|---|---|
| Copier | `copy`, `copy_if`, `copy_n` | Non | O(n) |
| Copie inversée | `copy_backward` | Non | O(n) |
| Déplacer | `move`, `move_backward` | Oui (moved-from) | O(n) |
| Supprimer par valeur | `remove` + `erase` / `std::erase` (C++20) | Oui | O(n) |
| Supprimer par prédicat | `remove_if` + `erase` / `std::erase_if` (C++20) | Oui | O(n) |
| Dédupliquer | `unique` + `erase` (séquence triée) | Oui | O(n) |
| Inverser | `reverse` / `reverse_copy` | Oui / Non | O(n) |
| Pivoter | `rotate` / `rotate_copy` | Oui / Non | O(n) |
| Mélanger | `shuffle` | Oui | O(n) |
| Partitionner | `partition` / `stable_partition` | Oui | O(n) |
| Séparer en deux | `partition_copy` | Non | O(n) |
| Remplir | `fill`, `fill_n` | Oui | O(n) |
| Remplacer | `replace`, `replace_if` | Oui | O(n) |
| Échanger | `swap_ranges` | Oui (les deux) | O(n) |
| Union/intersection | `set_union`, `set_intersection`… | Non | O(n + m) |
| Fusionner | `merge`, `inplace_merge` | Non / Oui | O(n + m) |

---

## Synthèse

Les algorithmes de manipulation forment la boîte à outils quotidienne du développeur C++. Le pattern le plus important à retenir est le **remove-erase idiom** — ou mieux, son remplacement par `std::erase` / `std::erase_if` en C++20. La distinction entre `std::copy` et `std::move` (algorithme) reflète la même logique que la distinction entre copie et mouvement au niveau du langage : si la source n'est plus nécessaire, déplacez pour gagner en performance. Les opérations ensemblistes (`set_union`, `set_intersection`…) sont un outil puissant souvent négligé, mais elles exigent des données triées. Enfin, `std::partition` et `std::stable_partition` offrent un compromis entre un tri complet (coûteux) et aucun tri du tout quand on a juste besoin de séparer les éléments en deux groupes.

⏭️ [Itérateurs : input, output, forward, bidirectional, random_access](/15-algorithmes-stl/05-iterateurs.md)
