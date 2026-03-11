🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 14.4 — std::flat_map et std::flat_set (C++23) : Alternatives cache-friendly ⭐

## Module 5 — La Bibliothèque Standard (STL) | Niveau Intermédiaire

---

## Introduction

Les sections précédentes ont présenté deux familles de conteneurs associatifs : les conteneurs ordonnés basés sur des arbres (`std::map`, `std::set`) et les conteneurs non ordonnés basés sur des tables de hachage (`std::unordered_map`, `std::unordered_set`). Les deux partagent un défaut commun : une **mauvaise localité mémoire**. Les arbres allouent un nœud par élément, dispersé dans le heap. Les tables de hachage du standard utilisent du chaînage séparé, avec le même problème de dispersion.

C++23 introduit une troisième famille qui attaque frontalement ce problème : `std::flat_map` et `std::flat_set`. Ces conteneurs offrent la même interface ordonnée que `std::map` et `std::set`, mais reposent en interne sur des **conteneurs séquentiels triés** — par défaut, des `std::vector`. Le résultat : des données **contiguës en mémoire**, parfaitement adaptées au cache CPU, avec des performances en lecture qui peuvent surpasser significativement celles des conteneurs traditionnels.

```cpp
#include <flat_map>
#include <flat_set>
#include <print>

std::flat_map<std::string, int> population {
    {"Paris", 2'161'000},
    {"Lyon", 522'969},
    {"Marseille", 873'076}
};

// Interface identique à std::map
population["Toulouse"] = 504'078;

if (population.contains("Lyon")) {
    std::print("Lyon : {} habitants\n", population["Lyon"]);
}

// Parcours ordonné — garanti comme std::map
for (const auto& [ville, pop] : population) {
    std::print("{} : {}\n", ville, pop);
}
```

> 📎 *Cette section couvre l'utilisation pratique des conteneurs flat dans le contexte des conteneurs associatifs. Pour la présentation dans le cadre des nouveautés C++23, voir la section 12.9. Pour l'analyse approfondie des performances cache, voir la section 41.6.*

---

## Structure interne : des vecteurs triés

Contrairement à `std::map` qui stocke ses éléments dans un arbre de nœuds alloués individuellement, `std::flat_map` stocke ses données dans **deux conteneurs séquentiels distincts** maintenus en parallèle : un pour les clés, un pour les valeurs. Les deux sont triés par clé.

```
std::map (Red-Black Tree)              std::flat_map (vecteurs triés)
                                        
     [  "Lyon"  ]                       Clés (contigu)    Valeurs (contigu)
     /           \                      ┌──────────┐      ┌───────────┐
["Alice"]    ["Paris"]                  │ "Lyon"   │      │   522'969 │
            /        \                  │ "Mars."  │      │   873'076 │
     ["Marseille"] ["Toulouse"]         │ "Paris"  │      │ 2'161'000 │
                                        │ "Toul."  │      │   504'078 │
  5 nœuds × ~64 octets = ~320 octets    └──────────┘      └───────────┘
  dispersés dans le heap                  Contigu            Contigu
```

`std::flat_set` est plus simple encore : un seul vecteur trié contenant les éléments.

Cette organisation a une conséquence directe : quand le CPU parcourt les clés pour effectuer une recherche binaire, les données sont **alignées dans des lignes de cache consécutives**. Les prefetchers matériels anticipent les accès séquentiels, et les cache misses sont rares. C'est l'exact opposé de la navigation de pointeurs dans un arbre.

---

## Complexité : le compromis lecture vs écriture

La structure en vecteur trié modifie radicalement le profil de complexité par rapport à `std::map` :

| Opération | `std::map` | `std::flat_map` | Raison |
|---|---|---|---|
| **Recherche** | O(log n) | O(log n) | Recherche binaire dans les deux cas |
| **Insertion** | O(log n) | **O(n)** | Décalage des éléments dans le vecteur |
| **Suppression** | O(log n) | **O(n)** | Décalage des éléments dans le vecteur |
| **Parcours complet** | O(n) | O(n) | Mais beaucoup plus rapide en pratique |
| **Accès min/max** | O(log n) | **O(1)** | Premier/dernier élément du vecteur |

La recherche est en O(log n) dans les deux cas, mais le `flat_map` bénéficie d'un **facteur constant nettement inférieur** grâce à la localité mémoire. En pratique, un `std::flat_map` de quelques milliers d'éléments effectue une recherche binaire plus vite qu'un `std::map` de même taille, malgré la même complexité théorique.

Le prix est clair : l'insertion et la suppression sont en **O(n)** car elles nécessitent de décaler tous les éléments qui suivent le point d'insertion dans le vecteur, exactement comme un `std::vector::insert` au milieu. C'est le compromis fondamental des conteneurs flat.

---

## Conteneurs sous-jacents personnalisables

Par défaut, `std::flat_map` utilise `std::vector` pour stocker les clés et les valeurs. Mais le standard permet de spécifier d'autres conteneurs séquentiels :

```cpp
#include <flat_map>
#include <deque>
#include <vector>

// Par défaut : vector pour clés et valeurs
std::flat_map<int, std::string> default_fm;

// Deque pour les deux — stabilité des références lors des insertions en tête
std::flat_map<int, std::string,
              std::less<int>,
              std::deque<int>,
              std::deque<std::string>> deque_fm;
```

En pratique, `std::vector` est presque toujours le meilleur choix : c'est le conteneur qui offre la meilleure localité mémoire et les meilleures performances cache. L'utilisation de `std::deque` peut se justifier dans des cas très spécifiques où l'on souhaite éviter les réallocations massives, mais au prix d'une localité mémoire dégradée.

---

## Construction efficace : sorted_unique

L'un des scénarios les plus favorables aux conteneurs flat est la construction à partir de données déjà triées. Le tag `std::sorted_unique` (pour flat_map/flat_set) permet de signaler que les données sont déjà triées et sans doublons, évitant ainsi le tri interne :

```cpp
#include <flat_map>

// Données déjà triées et uniques
std::vector<int> keys {1, 2, 3, 4, 5};  
std::vector<std::string> values {"a", "b", "c", "d", "e"};  

// Construction en O(n) au lieu de O(n log n)
std::flat_map<int, std::string> fm(std::sorted_unique,
                                    std::move(keys),
                                    std::move(values));
```

Pour les données non triées, le constructeur standard effectue le tri automatiquement :

```cpp
std::flat_map<int, std::string> fm {
    {3, "c"}, {1, "a"}, {5, "e"}, {2, "b"}, {4, "d"}
};
// Trié automatiquement : {1:"a", 2:"b", 3:"c", 4:"d", 5:"e"}
```

### Le pattern "build once, read many"

Le scénario idéal pour un conteneur flat est le suivant :

1. **Phase de construction** — on insère tous les éléments (coûteux si insertions individuelles, efficace si construction en bloc).
2. **Phase de lecture** — on effectue des recherches intensives, des parcours, des requêtes par plage (très rapide grâce au cache).

```cpp
// Construction en bloc : trier un vecteur puis construire le flat_map
std::vector<std::pair<std::string, double>> raw_data = load_from_database();

// Trier par clé
std::ranges::sort(raw_data, {}, &std::pair<std::string, double>::first);

// Supprimer les doublons (garder le premier)
auto removed = std::ranges::unique(raw_data, {},
    &std::pair<std::string, double>::first);
raw_data.erase(removed.begin(), removed.end());

// Construire le flat_map à partir des données triées
std::flat_map<std::string, double> lookup(
    std::sorted_unique,
    raw_data.begin(), raw_data.end()
);

// Phase de lecture intensive — très performant
for (const auto& query : millions_of_queries) {
    if (auto it = lookup.find(query); it != lookup.end()) {
        process(it->second);
    }
}
```

---

## API : ce qui est identique et ce qui diffère

### Interface commune avec std::map

`std::flat_map` implémente la quasi-totalité de l'interface de `std::map` :

```cpp
std::flat_map<std::string, int> fm;

// Insertion — mêmes méthodes
fm["clé"] = 42;  
fm.insert({"autre", 10});  
fm.insert_or_assign("clé", 99);  
fm.emplace("test", 7);  
fm.try_emplace("nouveau", 33);  

// Recherche — identique
auto it = fm.find("clé");  
bool exists = fm.contains("test");     // C++20  
int val = fm.at("clé");                // Lance out_of_range si absent  
std::size_t n = fm.count("inexistant"); // 0  

// Requêtes par plage — identique
auto lo = fm.lower_bound("b");  
auto hi = fm.upper_bound("t");  

// Parcours ordonné — identique
for (const auto& [k, v] : fm) {
    std::print("{}: {}\n", k, v);
}

// Suppression
fm.erase("test");  
fm.erase(fm.find("autre"));  
```

### Méthodes spécifiques aux conteneurs flat

`std::flat_map` expose des méthodes qui n'existent pas dans `std::map`, liées à sa nature de wrapper sur des conteneurs séquentiels :

```cpp
// Accéder aux conteneurs sous-jacents (lecture seule)
const auto& keys   = fm.keys();    // Référence sur le vecteur de clés  
const auto& values = fm.values();  // Référence sur le vecteur de valeurs  

// extract — récupérer les conteneurs sous-jacents par mouvement
auto containers = std::move(fm).extract();
// containers.keys   : std::vector<std::string>
// containers.values : std::vector<int>
// fm est maintenant vide

// replace — remplacer les conteneurs sous-jacents
fm.replace(std::move(containers.keys), std::move(containers.values));
```

`extract` et `replace` permettent de manipuler les données brutes directement — utile pour des opérations en masse qui seraient inefficaces via l'interface associative (tri externe, filtrage, sérialisation).

### Ce qui manque

`std::flat_map` ne possède **pas** les méthodes de nœuds (`extract` par clé/itérateur retournant un `node_handle`, `merge` par transfert de nœuds) présentes dans `std::map` depuis C++17. Ces opérations n'ont pas de sens sans nœuds individuels.

---

## std::flat_set

`std::flat_set` est la contrepartie de `std::set` : un ensemble ordonné stocké dans un vecteur trié unique.

```cpp
#include <flat_set>
#include <print>

std::flat_set<int> primes {7, 2, 11, 3, 5, 2, 3};

for (int p : primes) {
    std::print("{} ", p);
}
// 2 3 5 7 11 — trié, sans doublons

// Même interface que std::set
primes.insert(13);  
bool has_five = primes.contains(5);  

// Requêtes par plage
auto from = primes.lower_bound(4);  
auto to   = primes.upper_bound(10);  
for (auto it = from; it != to; ++it) {  
    std::print("{} ", *it); // 5 7
}
```

Les mêmes compromis s'appliquent : recherche rapide grâce à la localité mémoire, insertion/suppression en O(n). La construction avec `std::sorted_unique` est disponible de la même manière.

### Opérations ensemblistes

Les algorithmes STL (`std::set_union`, `std::set_intersection`, etc.) fonctionnent avec `std::flat_set` puisque les éléments sont triés. Et grâce à la contiguïté mémoire, le parcours séquentiel qu'effectuent ces algorithmes est plus rapide qu'avec `std::set` :

```cpp
std::flat_set<int> A {1, 2, 3, 4, 5};  
std::flat_set<int> B {3, 4, 5, 6, 7};  

std::vector<int> result;  
std::set_intersection(  
    A.begin(), A.end(),
    B.begin(), B.end(),
    std::back_inserter(result)
);
// result = {3, 4, 5}
```

---

## flat_multimap et flat_multiset

C++23 inclut également `std::flat_multimap` et `std::flat_multiset`, qui autorisent les doublons. Comme leurs homologues basés sur les arbres, ils stockent plusieurs éléments avec la même clé, cette fois dans des vecteurs triés.

```cpp
#include <flat_set>

std::flat_multiset<int> ms {5, 3, 5, 1, 3, 5};
// Stocké comme un vecteur trié : {1, 3, 3, 5, 5, 5}

std::print("Nombre de 5 : {}\n", ms.count(5)); // 3
```

---

## Considérations mémoire

L'avantage mémoire des conteneurs flat est substantiel. Pour un `flat_map<int, int>` de n éléments :

| Conteneur | Mémoire par élément | Overhead | Mémoire pour 10 000 éléments |
|---|---|---|---|
| `std::map<int, int>` | ~48-64 octets | Pointeurs × 3 + couleur + alloc | ~500-640 Ko |
| `std::flat_map<int, int>` | ~8 octets | Aucun (contigu) | ~80 Ko |
| `std::unordered_map<int, int>` | ~50-70 octets | Hash bucket + chaînage | ~500-700 Ko |

Le ratio est de l'ordre de **6 à 8 fois moins de mémoire** pour le flat_map par rapport aux alternatives. Cet avantage s'amplifie avec le nombre d'éléments et se traduit directement en meilleure utilisation du cache.

> Le vecteur sous-jacent peut réserver de la capacité excédentaire (comme tout `std::vector`). Après la phase de construction, un appel à `shrink_to_fit()` via les conteneurs extraits peut libérer cette mémoire :
>
> ```cpp
> auto c = std::move(fm).extract();
> c.keys.shrink_to_fit();
> c.values.shrink_to_fit();
> fm.replace(std::move(c.keys), std::move(c.values));
> ```

---

## Pièges et limitations

### Coût d'insertion et de suppression

C'est la limitation principale. Chaque insertion ou suppression implique un décalage de tous les éléments suivants dans le vecteur — **O(n)** dans le pire cas. Pour un conteneur de 100 000 éléments, une insertion au début déplace 100 000 paires. Si les insertions et suppressions sont fréquentes et réparties aléatoirement, `std::map` sera nettement plus performant.

```
Insertion dans un flat_map de 8 éléments à la position 3 :

Avant : [A][B][C][D][E][F][G][H]
                 ↑ insertion ici
Après : [A][B][C][X][D][E][F][G][H]
                     ←←←←←←←←←←←  décalage de 5 éléments
```

### Invalidation des itérateurs et références

Toute insertion ou suppression peut invalider **tous** les itérateurs, références et pointeurs vers les éléments — comme pour un `std::vector`. C'est un changement majeur par rapport à `std::map`, où les itérateurs et références restent valides après insertion/suppression (sauf pour l'élément supprimé).

```cpp
std::flat_map<int, std::string> fm {{1, "a"}, {2, "b"}, {3, "c"}};

auto it = fm.find(2);  
fm.insert({0, "z"});   // Décalage → it est potentiellement invalide !  
// *it;                 // ❌ Comportement indéfini
```

### État d'exception et conteneurs désynchronisés

`std::flat_map` maintient deux conteneurs en parallèle (clés et valeurs). Si une opération lance une exception au milieu (par exemple, le déplacement d'une valeur échoue pendant une insertion), les deux conteneurs peuvent se retrouver dans un **état incohérent** — tailles différentes ou ordres désynchronisés.

Le standard définit cela comme un état valide mais non spécifié. En pratique, cela signifie que le flat_map reste destructible et assignable, mais son contenu est indéterminé. C'est un risque à garder en tête avec des types dont les opérations de copie ou de déplacement peuvent lancer des exceptions.

> ⭐ **Bonne pratique** : privilégiez des types `noexcept`-movable pour les clés et les valeurs d'un `flat_map`. Les types primitifs, `std::string` et la plupart des types de la STL satisfont cette condition.

### Support compilateur (mars 2026)

Le support de `std::flat_map` et `std::flat_set` varie selon les compilateurs et leurs bibliothèques standard :

| Compilateur | Bibliothèque | `<flat_map>` | `<flat_set>` |
|---|---|---|---|
| GCC 15 | libstdc++ | ✅ | ✅ |
| Clang 20 | libc++ | ✅ | ✅ |
| MSVC 17.10+ | MS STL | ✅ | ✅ |

Le support est désormais largement disponible dans les compilateurs récents. Vérifiez la version de votre bibliothèque standard si vous ciblez des environnements plus anciens.

---

## Guide de choix : quand utiliser un conteneur flat

### Cas favorables (préférer flat_map / flat_set)

- **Données construites une fois, lues intensivement** — tables de configuration, index de lookup, dictionnaires statiques.
- **Petits à moyens jeux de données** (jusqu'à quelques dizaines de milliers d'éléments) — le coût O(n) de l'insertion reste acceptable.
- **Parcours fréquents** — itération séquentielle, opérations ensemblistes, sérialisation.
- **Contraintes mémoire** — l'empreinte réduite libère de la place pour d'autres données dans le cache.
- **Boucles critiques en performance** — le gain en localité mémoire se mesure directement en cycles CPU.

### Cas défavorables (préférer map / unordered_map)

- **Insertions et suppressions fréquentes sur un grand conteneur** — le décalage O(n) dans le vecteur devient prohibitif.
- **Besoin de stabilité des itérateurs/références** — un flat_map invalide tout à chaque modification.
- **Très grands jeux de données (millions d'éléments)** avec des modifications continues — les arbres et tables de hachage sont conçus pour ce cas.
- **Types avec copie/mouvement coûteux ou susceptibles de lancer** — risque de désynchronisation des conteneurs internes.

### Règle empirique

> Si le ratio lectures/écritures est supérieur à 10:1 et que le conteneur fait moins de ~50 000 éléments, un conteneur flat est probablement le meilleur choix. Au-delà, mesurez avec un benchmark (section 14.5).

---

## Migration depuis std::map / std::set

Grâce à une interface quasi identique, la migration est souvent un simple changement de type :

```cpp
// Avant
#include <map>
std::map<std::string, Config> settings = load_config();

// Après
#include <flat_map>
std::flat_map<std::string, Config> settings = load_config();
```

Les points d'attention lors de la migration :

- Vérifier qu'aucune partie du code ne dépend de la **stabilité des itérateurs** après insertion/suppression.
- S'assurer que le profil d'accès est effectivement **dominé par les lectures**.
- Vérifier que les types stockés supportent le **déplacement sans exception** (`noexcept` move).
- Retirer tout code qui utilise les **node handles** (`extract` retournant un `node_type`, `merge`) — ces APIs n'existent pas dans les conteneurs flat.

---

## En bref

`std::flat_map` et `std::flat_set` (C++23) apportent la localité mémoire des vecteurs triés à l'interface des conteneurs associatifs ordonnés. Ils brillent dans les scénarios *read-heavy* : recherche binaire cache-friendly, parcours séquentiel rapide, empreinte mémoire réduite de 6 à 8 fois par rapport à `std::map`. Le compromis — insertion et suppression en O(n), invalidation totale des itérateurs — les rend inadaptés aux cas d'usage dominés par les écritures. Le pattern idéal est "construire une fois, lire des millions de fois". La section 14.5 met en perspective ces conteneurs avec des benchmarks comparatifs mesurés.

⏭️ [Comparaison de performances : ordered vs unordered vs flat](/14-conteneurs-associatifs/05-comparaison-performances.md)
