🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 13.6 — Complexité algorithmique (Big O) et choix du conteneur

## Chapitre 13 : Conteneurs Séquentiels

---

## Introduction

La notation Big O est l'outil théorique de référence pour comparer les performances des opérations sur les conteneurs. Le standard C++ spécifie des garanties de complexité pour chaque opération de chaque conteneur, et ces garanties sont contractuelles — toute implémentation conforme doit les respecter.

Cependant, la complexité asymptotique ne raconte qu'une partie de l'histoire. Un algorithme O(1) peut être plus lent qu'un algorithme O(n) pour des tailles réalistes si sa constante cachée est suffisamment élevée. Et sur les architectures modernes, le facteur dominant n'est souvent ni le nombre d'opérations ni le nombre de comparaisons, mais le **nombre de cache misses**. Cette section rappelle les fondamentaux du Big O, compare les conteneurs séquentiels sous cet angle, puis explique pourquoi le profiling reste indispensable.

---

## Rappel : la notation Big O

La notation Big O décrit le comportement asymptotique d'une opération quand la taille des données n tend vers l'infini. Elle ignore les constantes multiplicatives et les termes d'ordre inférieur :

| Notation | Nom | Exemple concret |  
|---|---|---|  
| O(1) | Constant | Accès par index dans un `vector` |  
| O(log n) | Logarithmique | Recherche dans un `std::set` (arbre) |  
| O(n) | Linéaire | Parcours d'un conteneur, insertion au milieu d'un `vector` |  
| O(n log n) | Quasi-linéaire | Tri (`std::sort`) |  
| O(n²) | Quadratique | Tri par insertion naïf, boucles imbriquées |

Ce que Big O **dit** : comment le temps d'exécution évolue quand n double. Un O(n) double, un O(n²) quadruple, un O(1) reste constant.

Ce que Big O **ne dit pas** : le temps absolu d'une opération. Un O(1) qui prend 500 ns (cache miss + allocation heap) est plus lent qu'un O(n) qui parcourt 50 éléments contigus en 20 ns.

---

## Complexité comparée des conteneurs séquentiels

Le tableau suivant synthétise les complexités garanties par le standard pour les opérations les plus courantes :

### Accès

| Opération | `vector` | `array` | `deque` | `list` | `forward_list` |  
|---|---|---|---|---|---|  
| Accès par index `[i]` | **O(1)** | **O(1)** | **O(1)** | O(n) | O(n) |  
| `front()` | O(1) | O(1) | O(1) | O(1) | O(1) |  
| `back()` | O(1) | O(1) | O(1) | O(1) | — |

### Insertion

| Opération | `vector` | `array` | `deque` | `list` | `forward_list` |  
|---|---|---|---|---|---|  
| En fin (`push_back`) | **O(1) amorti** | — | **O(1) amorti** | O(1) | — |  
| En tête (`push_front`) | O(n) | — | **O(1) amorti** | O(1) | O(1) |  
| Au milieu (avec itérateur) | O(n) | — | O(n) | **O(1)** | **O(1)** |

### Suppression

| Opération | `vector` | `array` | `deque` | `list` | `forward_list` |  
|---|---|---|---|---|---|  
| En fin (`pop_back`) | **O(1)** | — | **O(1)** | O(1) | — |  
| En tête (`pop_front`) | O(n) | — | **O(1)** | O(1) | O(1) |  
| Au milieu (avec itérateur) | O(n) | — | O(n) | **O(1)** | **O(1)** |

### Recherche

| Opération | `vector` | `array` | `deque` | `list` | `forward_list` |  
|---|---|---|---|---|---|  
| Recherche linéaire (`std::find`) | O(n) | O(n) | O(n) | O(n) | O(n) |  
| Recherche binaire (si trié) | **O(log n)** | **O(log n)** | **O(log n)** | — | — |

La recherche binaire (`std::lower_bound`, `std::binary_search`) requiert des itérateurs à accès aléatoire pour être en O(log n). Sur `std::list` et `std::forward_list`, elle fonctionne techniquement mais devient O(n) car avancer d'une position est O(1) et la recherche doit avancer O(log n) fois de O(n/log n) positions en moyenne — le tout se simplifie en O(n).

### Autres opérations

| Opération | `vector` | `array` | `deque` | `list` | `forward_list` |  
|---|---|---|---|---|---|  
| `sort` | O(n log n) | O(n log n) | O(n log n) | O(n log n) | O(n log n) |  
| `splice` (transfert complet) | — | — | — | **O(1)** | **O(1)** |  
| `merge` (listes triées) | — | — | — | O(n) | O(n) |  
| `swap` | O(1) | **O(n)** | O(1) | O(1) | O(1) |

Le `swap` de `std::array` est O(n) car il doit échanger chaque élément individuellement (les données sont dans l'objet, pas derrière un pointeur).

---

## Le piège du Big O : la constante cachée

La notation Big O masque un facteur crucial : la **constante** devant le terme dominant. Quand on écrit O(n), le temps réel est `c × n` pour une certaine constante `c`. Et cette constante varie énormément d'un conteneur à l'autre.

Prenons l'insertion au milieu : `std::list` est O(1) et `std::vector` est O(n). En théorie, la liste gagne. En pratique, voici ce qui se passe réellement.

**`std::vector` — insertion au milieu :**
1. Décaler les éléments suivants d'une position → opération `memmove` sur un bloc contigu, optimisée par le CPU et le cache.
2. Écrire le nouvel élément.
3. Coût total : quelques nanosecondes par élément déplacé grâce à la localité de cache.

**`std::list` — insertion au milieu :**
1. Trouver la position d'insertion → O(n) pour atteindre l'itérateur si on ne l'a pas déjà.
2. Appeler `new` pour allouer un nœud → appel système potentiel, fragmentation heap.
3. Mettre à jour 4 pointeurs.
4. Coût total : l'allocation seule peut prendre 50-200 ns, soit plus que le décalage de centaines d'éléments contigus dans un vector.

La complexité O(1) de l'insertion dans la liste ne compte que l'étape 3 — elle suppose que vous **avez déjà** l'itérateur. Le coût de l'allocation n'est pas comptabilisé dans la complexité car il est considéré comme O(1) amorti par le standard, mais sa constante est très élevée.

---

## Le facteur dominant : la hiérarchie mémoire

Sur une architecture x86_64 moderne, les temps d'accès mémoire typiques sont :

| Niveau | Latence approximative | Taille typique |  
|---|---|---|  
| Registre CPU | < 1 ns | quelques Ko |  
| Cache L1 | ~1 ns | 32-64 Ko |  
| Cache L2 | ~4 ns | 256-512 Ko |  
| Cache L3 | ~10-20 ns | 8-32 Mo |  
| RAM (DRAM) | ~50-100 ns | 16-128 Go |

Un accès à la RAM est environ **100 fois plus lent** qu'un accès au cache L1. Cela signifie qu'un algorithme qui fait 10 accès en cache L1 est plus rapide qu'un algorithme qui fait 1 seul accès en RAM.

C'est cette réalité matérielle qui explique la domination de `std::vector` dans la quasi-totalité des benchmarks :

- **`std::vector`** : éléments contigus → le prefetcher du CPU charge les lignes de cache suivantes proactivement → parcours à la vitesse du cache L1/L2.  
- **`std::deque`** : éléments contigus par blocs → bonne localité intra-bloc, sauts entre blocs → performances proches du vector pour les parcours, légèrement inférieures.  
- **`std::list`** : nœuds dispersés → chaque accès au nœud suivant est un saut imprévisible → cache misses systématiques → performances dominées par la latence mémoire.

---

## Mesurer plutôt que deviner

La seule façon fiable de choisir un conteneur pour du code critique en performance est de **mesurer**. Voici un micro-benchmark comparant l'insertion au milieu sur les trois conteneurs principaux :

```cpp
#include <vector>
#include <list>
#include <deque>
#include <chrono>
#include <print>

template <typename Container>  
auto bench_insertion_milieu(int n) {  
    Container c;

    auto debut = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < n; ++i) {
        auto pos = c.begin();
        // Avancer jusqu'au milieu
        std::advance(pos, static_cast<long>(c.size()) / 2);
        c.insert(pos, i);
    }

    auto fin = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(fin - debut);
}

int main() {
    constexpr int N = 50'000;

    auto t_vec  = bench_insertion_milieu<std::vector<int>>(N);
    auto t_list = bench_insertion_milieu<std::list<int>>(N);
    auto t_deq  = bench_insertion_milieu<std::deque<int>>(N);

    std::println("Insertion au milieu ({} éléments) :", N);
    std::println("  vector : {} µs", t_vec.count());
    std::println("  list   : {} µs", t_list.count());
    std::println("  deque  : {} µs", t_deq.count());
}
```

Résultats typiques (GCC 15, -O2, x86_64) :

```
Insertion au milieu (50000 éléments) :
  vector : 3 200 µs
  list   : 8 500 µs
  deque  : 4 100 µs
```

Malgré son insertion théoriquement O(n), `std::vector` est **plus de deux fois plus rapide** que `std::list` dont l'insertion est O(1). La raison : le benchmark inclut le parcours jusqu'au milieu (O(n) pour la liste avec des cache misses à chaque nœud) et le coût de l'allocation par nœud. Le `memmove` du vector sur des données contiguës est bien plus efficace.

> ⚠️ Ces chiffres sont indicatifs et varient selon l'architecture, le compilateur, la taille des éléments et les conditions du cache. Le message n'est pas que `std::vector` gagne toujours, mais que le Big O seul ne permet pas de prédire le résultat.

---

## Arbre de décision pour le choix d'un conteneur séquentiel

La complexité algorithmique est un critère parmi d'autres. Voici un arbre de décision qui intègre les considérations pratiques :

```
La taille est-elle connue à la compilation ?
├── OUI → La taille est-elle raisonnable pour la stack (< quelques Ko) ?
│         ├── OUI → std::array
│         └── NON → std::vector + reserve()
│
└── NON → Avez-vous besoin d'un accès par index O(1) ?
          ├── OUI → Les insertions en tête sont-elles fréquentes ?
          │         ├── OUI → std::deque
          │         └── NON → std::vector
          │
          └── NON → La stabilité des itérateurs/pointeurs est-elle critique ?
                    ├── OUI → Utilisez-vous splice() fréquemment ?
                    │         ├── OUI → std::list
                    │         └── NON → std::list (ou reconsidérer le design)
                    │
                    └── NON → std::vector
```

Dans le doute, la réponse est **`std::vector`**. Si le profiling révèle un problème, migrez vers un conteneur plus spécialisé en vous appuyant sur des mesures, pas sur l'analyse asymptotique seule.

---

## Le O(1) amorti de `std::vector::push_back`

Un point de subtilité souvent mal compris : le "O(1) amorti" de `push_back` signifie que **la moyenne** de n opérations `push_back` successives est O(1) par opération. Mais une opération individuelle peut coûter O(n) lorsqu'elle déclenche une réallocation.

Cela a des conséquences pour les systèmes temps réel ou à latence contrainte. Si une seule opération ne doit jamais dépasser un certain seuil de temps, le O(1) amorti n'est pas suffisant : la réallocation ponctuelle crée un **pic de latence**. Dans ce contexte, `reserve()` élimine le problème si la taille maximale est connue, ou `std::deque` peut être préférable car ses réallocations sont plus petites et plus fréquentes (allocation d'un bloc de taille fixe au lieu de copier tout le contenu).

---

## Complexité des opérations combinées

En pratique, une opération sur un conteneur n'est jamais isolée. Le choix du conteneur impacte la complexité de **séquences d'opérations** courantes. Voici quelques scénarios typiques et le conteneur optimal pour chacun :

**Remplissage puis parcours séquentiel (pattern le plus courant).**
Remplissage O(n) amorti + parcours O(n) = O(n) total. `std::vector` domine grâce à la localité de cache lors du parcours. Si la taille est connue, `reserve()` élimine les réallocations et le remplissage devient O(n) exact.

**File FIFO : ajout en fin, retrait en tête.**
`push_back` O(1) + `pop_front` O(1) = O(1) par opération. C'est le cas d'usage de `std::deque`. Avec `std::vector`, `pop_front` serait O(n) car il faut décaler tous les éléments.

**Filtrage : parcourir et supprimer les éléments qui ne satisfont pas un critère.**
Avec `std::vector` et `std::erase_if` (C++20) : O(n) — un seul parcours. Avec `std::list` et `remove_if` : O(n) également, mais avec des cache misses à chaque nœud. Le vector est plus rapide en pratique malgré une complexité identique.

**Maintien d'un ensemble trié avec insertions fréquentes.**
Insertion dans un vector trié avec `std::lower_bound` + `insert` : recherche O(log n) + insertion O(n) = O(n). Insertion dans une list triée : recherche O(n) + insertion O(1) = O(n). Les deux sont O(n), mais le vector est souvent plus rapide. Pour ce cas d'usage, envisagez plutôt `std::flat_set` (C++23, voir section 14.4) ou `std::set`.

---

## Bonnes pratiques

**Commencez par `std::vector`.** C'est le conteneur le plus rapide dans la majorité des cas réels, indépendamment de ce que la complexité théorique suggère. Ne changez que si un profiling démontre un problème.

**Ne choisissez pas un conteneur sur la base du Big O seul.** La complexité asymptotique est un guide, pas un oracle. Les constantes cachées, la localité de cache, et les coûts d'allocation dominent souvent le temps réel pour les tailles de données courantes (< 100 000 éléments).

**Utilisez `reserve()` sur `std::vector` dès que possible.** C'est l'optimisation la plus rentable : elle transforme le O(1) amorti en O(1) garanti (sans réallocation) et améliore la prédictibilité du temps d'exécution.

**Profilez avec des données réalistes.** Les micro-benchmarks sur des `int` ne reflètent pas le comportement avec des objets lourds (std::string, structures complexes). Le coût de copie/déplacement et la taille des éléments changent l'équilibre entre les conteneurs.

**Connaissez les complexités pour éviter les erreurs grossières.** Le Big O reste essentiel pour éviter les catastrophes : utiliser `std::find` sur un vector non trié pour chaque élément d'un autre vector produit du O(n²), là où un `std::unordered_set` donnerait du O(n). La théorie et la pratique convergent sur les ordres de grandeur.

**Documentez vos choix de conteneurs.** Quand vous choisissez un conteneur autre que `std::vector`, ajoutez un commentaire expliquant pourquoi. Cela aide les mainteneurs futurs à comprendre que le choix est délibéré et motivé par un profiling ou une contrainte spécifique.

⏭️ [Conteneurs Associatifs](/14-conteneurs-associatifs/README.md)
