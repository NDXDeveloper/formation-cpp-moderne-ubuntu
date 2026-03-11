🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 14.5 — Comparaison de performances : ordered vs unordered vs flat

## Module 5 — La Bibliothèque Standard (STL) | Niveau Intermédiaire

---

## Introduction

Les sections précédentes ont présenté trois familles de conteneurs associatifs, chacune avec ses complexités théoriques et ses compromis structurels. Mais la complexité algorithmique ne raconte qu'une partie de l'histoire. Un O(1) avec un facteur constant élevé peut être plus lent qu'un O(log n) avec une excellente localité mémoire. Un O(n) sur des données contiguës peut battre un O(log n) sur des nœuds dispersés dans le heap.

Cette section confronte les conteneurs dans des **scénarios mesurés** représentatifs de cas d'usage réels. L'objectif n'est pas de déclarer un vainqueur universel — il n'en existe pas — mais de construire une intuition fiable sur le comportement réel de chaque conteneur, pour guider le choix en situation concrète.

> Les résultats présentés ici sont des ordres de grandeur typiques observés sur du matériel moderne (processeur x86-64, Ubuntu, GCC 15, -O2). Les performances absolues varient selon le matériel, le compilateur, les options d'optimisation et la nature des données. La démarche — mesurer sur son propre workload — compte davantage que les chiffres exacts.

---

## Méthodologie de benchmark

Comparer des conteneurs de manière fiable exige quelques précautions. Les résultats naïfs (mesurer un seul appel, oublier le warmup, ignorer les effets de cache) sont souvent trompeurs.

### Principes essentiels

- **Utiliser une bibliothèque de micro-benchmarking** comme Google Benchmark (chapitre 35) plutôt que des mesures manuelles avec `std::chrono`. Ces bibliothèques gèrent le warmup, les itérations multiples, la stabilisation statistique et la prévention des optimisations du compilateur.
- **Mesurer des scénarios distincts** : insertion, recherche de clés existantes, recherche de clés absentes, suppression, parcours complet, accès mixte lecture/écriture. Chaque conteneur a un profil différent selon l'opération.
- **Varier la taille du conteneur** : le comportement relatif change selon que le conteneur tient dans le cache L1 (quelques Ko), L2 (quelques centaines de Ko), L3 (quelques Mo), ou déborde en mémoire principale.
- **Utiliser des données réalistes** : des clés séquentielles (`0, 1, 2, ...`) favorisent artificiellement certaines fonctions de hachage. Des clés aléatoires ou issues de données réelles donnent des résultats plus représentatifs.
- **Compiler en mode optimisé** (`-O2` ou `-O3`). Les performances en mode debug (`-O0`) ne reflètent pas le comportement en production.

### Structure type d'un benchmark

```cpp
#include <benchmark/benchmark.h>
#include <map>
#include <unordered_map>
#include <flat_map>
#include <random>

// Génération de données aléatoires (hors de la boucle mesurée)
static std::vector<int> generate_keys(std::size_t n) {
    std::mt19937 rng(42); // Seed fixe pour reproductibilité
    std::vector<int> keys(n);
    std::iota(keys.begin(), keys.end(), 0);
    std::shuffle(keys.begin(), keys.end(), rng);
    return keys;
}

static void BM_MapFind(benchmark::State& state) {
    auto keys = generate_keys(state.range(0));
    std::map<int, int> m;
    for (int k : keys) m[k] = k;

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(0, state.range(0) - 1);

    for (auto _ : state) {
        int key = dist(rng);
        auto it = m.find(key);
        benchmark::DoNotOptimize(it);
    }
}
BENCHMARK(BM_MapFind)->Range(64, 1 << 20);

// Répéter pour unordered_map, flat_map...
```

---

## Benchmark 1 : Recherche par clé (int → int)

La recherche est l'opération la plus fréquente pour les conteneurs associatifs. Ce benchmark mesure le temps de `find()` sur des clés entières aléatoires, pour des tailles allant de 64 à 1 million d'éléments.

### Résultats typiques (ns/opération, clé int)

```
Taille        std::map    std::unordered_map    std::flat_map
──────────    ────────    ──────────────────    ─────────────
64                18                  12                  8
256               28                  14                 12
1 000             38                  15                 18
10 000            55                  16                 28
100 000           85                  18                 38
1 000 000        120                  22                 52
```

### Analyse

**`std::unordered_map` domine** sur toute la gamme. Sa performance est quasi-constante : elle ne dépend que du coût du hash et de la longueur du bucket (typiquement 1 élément). La légère augmentation avec la taille est due aux cache misses sur les nœuds de la liste chaînée qui deviennent plus probables quand le conteneur dépasse la taille du cache.

**`std::flat_map` est le plus rapide sur les petites tailles** (jusqu'à ~500-1000 éléments). La recherche binaire sur un vecteur contigu qui tient dans le cache L1/L2 bénéficie pleinement du prefetching matériel. Au-delà, la profondeur logarithmique de la recherche binaire augmente et les accès mémoire deviennent moins prédictibles.

**`std::map` est systématiquement le plus lent** pour la recherche pure. Chaque comparaison dans l'arbre implique une indirection de pointeur — un cache miss potentiel à chaque niveau. Pour un arbre d'un million d'éléments (~20 niveaux), c'est jusqu'à 20 cache misses en série.

### Facteur décisif : la taille du cache

Le graphe de performance montre typiquement des "marches" correspondant aux seuils de cache :

- **< 32 Ko** (cache L1) : `flat_map` domine largement — tout est en cache, la localité est maximale.
- **32 Ko – 256 Ko** (cache L2) : `flat_map` reste compétitif, `unordered_map` prend l'avantage.
- **256 Ko – 8 Mo** (cache L3) : `unordered_map` domine, `flat_map` reste correct, `map` décroche.
- **> 8 Mo** (mémoire principale) : `unordered_map` conserve son avantage, les deux autres souffrent des latences mémoire, mais `flat_map` reste meilleur que `map` grâce au prefetching séquentiel de la recherche binaire.

---

## Benchmark 2 : Recherche par clé (string → int)

Avec des clés `std::string`, le coût de la comparaison et du hachage change la donne. La comparaison de chaînes est en O(k) (k = longueur), le hachage aussi.

### Résultats typiques (ns/opération, clé string ~20 chars)

```
Taille        std::map    std::unordered_map    std::flat_map
──────────    ────────    ──────────────────    ─────────────
64                35                  30                 20
1 000             65                  32                 45
10 000           100                  35                 65
100 000          140                  40                 85
```

### Analyse

L'écart entre `unordered_map` et `map` se réduit par rapport aux clés entières. La raison : le hachage d'une chaîne de 20 caractères coûte plus cher que le hachage d'un entier, ce qui augmente le coût fixe de chaque opération sur `unordered_map`. La comparaison lexicographique dans `std::map` s'arrête souvent après quelques caractères (short-circuit), ce qui la rend compétitive.

`std::flat_map` reste avantageux sur les petites tailles grâce à la localité, mais les chaînes elles-mêmes sont souvent allouées sur le heap (au-delà de la Small String Optimization), ce qui réduit partiellement l'avantage de la contiguïté des vecteurs.

> ⭐ **Rappel** : utilisez `std::less<>` (transparent comparator) pour les maps et flat_maps à clés string, afin d'éviter la construction de `std::string` temporaires lors des recherches avec des `const char*` ou `std::string_view`. L'impact peut être de 20 à 40% sur les recherches.

---

## Benchmark 3 : Insertion

### Résultats typiques (ns/opération, insertion de n éléments, clé int)

```
Taille        std::map    std::unordered_map    std::flat_map
──────────    ────────    ──────────────────    ─────────────
64                45                  35                 25
1 000             65                  40                350
10 000            90                  45              3 500
100 000          130                  50             35 000
```

### Analyse

Le coût d'insertion en O(n) de `std::flat_map` est spectaculairement visible à partir de quelques milliers d'éléments. Chaque insertion décale en moyenne la moitié des éléments du vecteur. À 100 000 éléments, une seule insertion est environ **700 fois plus lente** que dans `std::unordered_map`.

`std::map` et `std::unordered_map` maintiennent des coûts d'insertion raisonnables à toute taille, avec l'avantage au second.

> C'est la raison pour laquelle les conteneurs flat ne conviennent pas aux scénarios d'insertion continue. Pour le pattern "build once, read many", on construit un vecteur non trié, on le trie une seule fois en O(n log n), puis on construit le flat_map avec `sorted_unique` — le coût total est bien inférieur à n insertions individuelles.

### Insertion en masse : construction initiale

```
Construction de n éléments (total, pas par élément)

Taille     map (insert ×n)   umap (insert ×n)   flat_map (sort+construct)
────────   ───────────────   ────────────────   ────────────────────────
10 000           0.9 ms            0.5 ms                0.3 ms
100 000         14.0 ms            6.0 ms                3.5 ms
1 000 000      210.0 ms           75.0 ms               45.0 ms
```

Quand le flat_map est construit en bloc (tri + construction), il est **le plus rapide** des trois, car le tri d'un vecteur est une opération très cache-friendly. C'est le scénario où le conteneur flat exprime tout son potentiel.

---

## Benchmark 4 : Parcours complet (itération)

Le parcours séquentiel de tous les éléments met en lumière l'impact de la localité mémoire de la manière la plus directe.

### Résultats typiques (µs pour un parcours complet de 100 000 éléments int → int)

```
Conteneur                Temps      Facteur
──────────────────────   ──────     ───────
std::flat_map              25 µs    1.0×  (référence)  
std::vector<pair> trié     25 µs    1.0×  
std::unordered_map        180 µs    7.2×  
std::map                  320 µs   12.8×  
```

### Analyse

Le parcours est le scénario où les conteneurs flat écrasent la concurrence. Itérer sur un vecteur contigu est l'opération la plus favorable au cache CPU : le prefetcher matériel anticipe chaque accès, et les données arrivent dans le cache avant même d'être demandées.

`std::map` est le plus lent : le parcours in-order de l'arbre suit une séquence de pointeurs gauche/droite/parent qui provoque un cache miss quasi systématique à chaque nœud.

`std::unordered_map` se situe entre les deux : le parcours traverse les buckets séquentiellement (accès au tableau contigu), mais suit les listes chaînées dans chaque bucket (indirections dispersées).

> Ce benchmark explique pourquoi `std::flat_map` est si attractif pour les workloads de type "construire un index, puis le parcourir en boucle" — agrégation, reporting, sérialisation.

---

## Benchmark 5 : Suppression

### Résultats typiques (ns/opération, suppression aléatoire, 10 000 éléments, clé int)

```
Conteneur                 Temps
──────────────────────    ──────
std::unordered_map          20 ns  
std::map                    55 ns  
std::flat_map            1 800 ns  
```

La suppression confirme le pattern de l'insertion : les conteneurs flat paient le décalage O(n) dans le vecteur. Pour un usage où les suppressions sont fréquentes et réparties aléatoirement, les conteneurs traditionnels sont largement préférables.

---

## Benchmark 6 : Empreinte mémoire

Au-delà du temps CPU, la mémoire consommée influence la performance globale (pression sur le cache, pression sur l'allocateur, consommation totale du processus).

### Mesure typique (octets/élément, clé int, valeur int, plateforme 64 bits)

```
Conteneur                 Octets/élément    Pour 100 000 éléments
──────────────────────    ──────────────    ─────────────────────
std::flat_map                      8                    0.8 Mo  
std::vector<pair> trié             8                    0.8 Mo  
std::map                         ~56                    5.6 Mo  
std::unordered_map               ~72                    7.2 Mo  
```

`std::unordered_map` est le plus gourmand : aux nœuds chaînés s'ajoute le tableau de buckets (un pointeur par bucket, souvent surdimensionné pour maintenir le facteur de charge). `std::map` consomme moins grâce à l'absence de tableau de buckets, mais chaque nœud porte trois pointeurs et un bit de couleur. Les conteneurs flat ne consomment que la mémoire strictement nécessaire aux données — un facteur 7 à 9 par rapport aux alternatives pour des clés et valeurs de petite taille.

L'impact est double : empreinte mémoire réduite du processus, et surtout meilleure utilisation du cache — pour un budget cache donné, un flat_map y loge 7 à 9 fois plus d'éléments.

---

## Synthèse : quel conteneur pour quel profil

### Tableau décisionnel

| Profil d'usage | Meilleur choix | Pourquoi |
|---|---|---|
| Recherche pure, pas d'ordre | `unordered_map` | O(1) amorti, rapide à toute taille |
| Read-heavy, petit/moyen jeu | `flat_map` | Localité cache, mémoire minimale |
| Lecture + écriture équilibrées | `map` | O(log n) garanti, itérateurs stables |
| Construction en bloc puis lectures | `flat_map` | Construction rapide + lectures optimales |
| Insertions/suppressions continues | `map` ou `unordered_map` | O(log n) ou O(1) par opération |
| Parcours séquentiels fréquents | `flat_map` | Itération ~10× plus rapide que map |
| Requêtes par plage | `map` ou `flat_map` | lower/upper_bound disponibles |
| Très grands jeux (> 1M éléments) | `unordered_map` | Scalabilité O(1), indépendant de n |
| Mémoire contrainte | `flat_map` | 7-9× moins de mémoire |
| Latence prévisible (pas de pics) | `map` | Pas de rehashing ni de réallocation |

### Arbre de décision rapide

```
L'ordre des clés est-il nécessaire ?
├── NON → std::unordered_map (ou unordered_set)
│         Le conteneur le plus rapide pour la recherche pure.
│
└── OUI → Le profil est-il dominé par les lectures (ratio > 10:1) ?
           ├── OUI → Le jeu de données est-il < ~50 000 éléments ?
           │         ├── OUI → std::flat_map / flat_set ⭐
           │         │         Performance cache optimale.
           │         └── NON → Benchmarker flat_map vs map sur vos données.
           │                   flat_map reste souvent compétitif jusqu'à ~500K.
           │
           └── NON → std::map / std::set
                     Insertions/suppressions O(log n), itérateurs stables.
```

---

## Le vecteur trié : la solution DIY avant C++23

Avant la standardisation des conteneurs flat en C++23, le pattern du **vecteur trié** avec recherche binaire était une pratique courante pour obtenir les mêmes avantages de localité mémoire :

```cpp
#include <vector>
#include <algorithm>

std::vector<std::pair<int, std::string>> data;

// Insertion en vrac
data.push_back({3, "c"});  
data.push_back({1, "a"});  
data.push_back({2, "b"});  

// Tri une seule fois
std::ranges::sort(data, {}, &std::pair<int, std::string>::first);

// Recherche binaire
auto it = std::ranges::lower_bound(data, 2,
    {}, &std::pair<int, std::string>::first);

if (it != data.end() && it->first == 2) {
    std::print("Trouvé : {}\n", it->second);
}
```

`std::flat_map` encapsule exactement ce pattern dans une interface propre et type-safe, avec la gestion automatique du tri et de l'unicité. Si vous maintenez du code C++17 qui utilise un vecteur trié comme conteneur associatif, la migration vers `std::flat_map` en C++23 est le chemin naturel.

---

## Alternatives hors standard

Les conteneurs de la STL ne sont pas toujours les plus performants en pratique. Plusieurs bibliothèques tierces offrent des implémentations sensiblement plus rapides, basées sur des structures de données que le standard ne peut pas adopter (à cause des contraintes de stabilité des itérateurs et références) :

| Bibliothèque | Conteneur | Technique | Avantage typique |
|---|---|---|---|
| Abseil (Google) | `absl::flat_hash_map` | Open addressing, Swiss table | 2-3× plus rapide que `std::unordered_map` |
| Boost 1.84+ | `boost::unordered_flat_map` | Open addressing | 2-3× plus rapide que `std::unordered_map` |
| ankerl | `ankerl::unordered_dense::map` | Robin Hood hashing | 2-4× plus rapide, mémoire réduite |
| Folly (Meta) | `folly::F14FastMap` | SIMD probing | Très rapide sur grands jeux de données |

Ces bibliothèques sont largement utilisées en production dans les systèmes à haute performance. Si le benchmark de votre application montre que le conteneur associatif est un goulot d'étranglement, les évaluer est une étape pertinente avant d'optimiser ailleurs.

> L'intégration de ces bibliothèques se fait typiquement via Conan (section 27.2) ou vcpkg (section 27.3), et elles sont compatibles avec l'interface de la STL — le remplacement est souvent transparent.

---

## L'erreur la plus fréquente : optimiser sans mesurer

Le piège classique est de choisir un conteneur sur la base de la complexité théorique sans mesurer le comportement réel. Quelques exemples courants de surprises :

- **Remplacer `std::map` par `std::unordered_map` pour "gagner en performance"** — et constater que la fonction de hachage des clés string est plus coûteuse que la comparaison, annulant le gain.
- **Utiliser `std::unordered_map` pour 50 éléments** — alors qu'un simple `std::vector` avec recherche linéaire O(n) serait plus rapide grâce à la localité mémoire et à l'absence d'overhead de hachage.
- **Choisir `std::flat_map` pour un cache avec des évictions fréquentes** — et découvrir que chaque suppression décale des milliers d'éléments.
- **Ignorer l'impact de l'allocateur** — les conteneurs à nœuds (`map`, `unordered_map`) sollicitent intensément l'allocateur heap. Un allocateur par pool ou par arène peut changer radicalement leurs performances.

La règle est simple : **profilez d'abord, optimisez ensuite**. Les outils de profiling (chapitre 31) et de micro-benchmarking (chapitre 35) sont les alliés indispensables pour prendre des décisions éclairées.

---

## En bref

Aucun conteneur associatif ne domine dans tous les scénarios. `std::unordered_map` offre la meilleure recherche pure à grande échelle. `std::flat_map` excelle en lecture sur les petits et moyens jeux de données grâce à sa localité mémoire, et consomme 7 à 9 fois moins de mémoire. `std::map` offre des performances prévisibles et la stabilité des itérateurs. Le choix se fait en croisant le profil d'accès (ratio lectures/écritures), la taille des données, le besoin d'ordre, et — surtout — la mesure sur des données représentatives. La complexité algorithmique guide l'intuition ; le benchmark tranche.

⏭️ [Algorithmes de la STL](/15-algorithmes-stl/README.md)
