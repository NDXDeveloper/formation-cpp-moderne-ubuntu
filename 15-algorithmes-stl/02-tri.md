🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 15.2 — Tri : std::sort, std::stable_sort

## Chapitre 15 — Algorithmes de la STL

---

## Introduction

Le tri est probablement l'opération algorithmique la plus étudiée en informatique, et pour cause : une fois les données triées, de nombreuses opérations deviennent drastiquement plus efficaces (recherche dichotomique en O(log n), fusion en O(n), déduplication en O(n)…). La STL propose plusieurs algorithmes de tri, chacun adapté à un besoin différent : tri complet, tri stable, tri partiel, ou simple sélection du n-ième élément.

Tous les algorithmes de tri se trouvent dans l'en-tête `<algorithm>`.

```cpp
#include <algorithm>
#include <vector>
#include <string>
```

---

## std::sort — Le tri par défaut

`std::sort` est l'algorithme de tri généraliste de la STL. Il trie les éléments dans l'intervalle `[first, last)` en ordre croissant par défaut :

```cpp
std::vector<int> v = {42, 17, 93, 5, 68, 31, 7};

std::sort(v.begin(), v.end());
// v == {5, 7, 17, 31, 42, 68, 93}
```

### Caractéristiques fondamentales

**Complexité** : O(n log n) comparaisons en moyenne *et* dans le pire cas. Le standard C++ (depuis C++11) garantit cette borne supérieure. Les implémentations modernes utilisent généralement un algorithme hybride — Introsort — qui combine quicksort, heapsort et insertion sort pour garantir le pire cas sans sacrifier les performances en moyenne.

**Stabilité** : `std::sort` n'est **pas stable**. Deux éléments considérés comme équivalents par le comparateur peuvent voir leur ordre relatif modifié après le tri. Si la stabilité est nécessaire, il faut utiliser `std::stable_sort` (voir plus loin).

**Exigence sur les itérateurs** : `std::sort` requiert des itérateurs **random-access**. Cela signifie qu'il fonctionne avec `std::vector`, `std::array`, `std::deque` et les tableaux C bruts, mais **pas** avec `std::list` ni `std::forward_list` (qui possèdent leur propre méthode membre `.sort()`).

### Tri avec comparateur personnalisé

Par défaut, `std::sort` utilise `operator<`. On peut fournir un comparateur pour modifier l'ordre :

```cpp
std::vector<int> v = {42, 17, 93, 5, 68, 31, 7};

// Tri décroissant avec std::greater
std::sort(v.begin(), v.end(), std::greater<int>{});
// v == {93, 68, 42, 31, 17, 7, 5}

// Tri décroissant avec une lambda (équivalent)
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a > b;
});
```

Pour des objets complexes, le comparateur projette sur le critère de tri souhaité :

```cpp
struct Server {
    std::string hostname;
    int cpu_load;      // pourcentage
    int memory_mb;
};

std::vector<Server> servers = {
    {"web-01",  72, 4096},
    {"web-02",  45, 8192},
    {"db-01",   91, 16384},
    {"cache-01", 23, 2048},
    {"web-03",  45, 4096}
};

// Trier par charge CPU croissante
std::sort(servers.begin(), servers.end(), [](const Server& a, const Server& b) {
    return a.cpu_load < b.cpu_load;
});
// cache-01(23), web-02(45), web-03(45), web-01(72), db-01(91)
```

### Le comparateur doit définir un ordre strict faible

Le comparateur passé à `std::sort` doit respecter les propriétés d'un **strict weak ordering** :

- **Irréflexivité** : `comp(a, a)` doit être `false`.
- **Asymétrie** : si `comp(a, b)` est `true`, alors `comp(b, a)` doit être `false`.
- **Transitivité** : si `comp(a, b)` et `comp(b, c)`, alors `comp(a, c)`.

Violer ces règles provoque un **comportement indéfini**. Le programme peut crasher, boucler indéfiniment, ou corrompre silencieusement les données. C'est l'un des pièges les plus vicieux du C++, car le bug peut apparaître uniquement sur certains jeux de données ou certaines implémentations.

L'erreur la plus classique : utiliser `<=` au lieu de `<` :

```cpp
// ⚠️ INCORRECT — viole l'irréflexivité (comp(a, a) == true quand a == a)
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a <= b;  // BUG : comportement indéfini
});

// ✅ CORRECT
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a < b;
});
```

### Tri sur un sous-intervalle

Puisque `std::sort` opère sur une paire d'itérateurs, on peut trier un sous-ensemble de la séquence :

```cpp
std::vector<int> v = {9, 7, 5, 3, 1, 8, 6, 4, 2, 0};

// Trier uniquement les 5 premiers éléments
std::sort(v.begin(), v.begin() + 5);
// v == {1, 3, 5, 7, 9, 8, 6, 4, 2, 0}
//       ^^^^^^^^^^^^^^  triés   ^^^^^^^^^^ inchangés
```

### Version Ranges (C++20)

```cpp
std::vector<int> v = {42, 17, 93, 5, 68};

// Passage direct du conteneur
std::ranges::sort(v);

// Tri décroissant
std::ranges::sort(v, std::greater{});

// Tri d'objets avec projection — pas besoin de lambda
std::ranges::sort(servers, {}, &Server::cpu_load);

// Tri décroissant par mémoire
std::ranges::sort(servers, std::greater{}, &Server::memory_mb);
```

Les projections éliminent une grande partie des comparateurs lambda écrits à la main. La combinaison comparateur + projection couvre la quasi-totalité des besoins de tri.

---

## std::stable_sort — Préserver l'ordre des équivalents

`std::stable_sort` garantit que deux éléments considérés comme **équivalents** par le comparateur conservent leur **ordre relatif d'origine**. C'est essentiel dans de nombreux cas pratiques.

```cpp
struct Task {
    std::string name;
    int priority;     // 1 = haute, 3 = basse
    int creation_id;  // ordre d'arrivée
};

std::vector<Task> tasks = {
    {"backup",   2, 1},
    {"deploy",   1, 2},
    {"cleanup",  2, 3},
    {"monitor",  1, 4},
    {"archive",  2, 5}
};

// Trier par priorité, en préservant l'ordre d'arrivée au sein de chaque priorité
std::stable_sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
    return a.priority < b.priority;
});

// Résultat garanti :
// deploy(1, #2), monitor(1, #4),        ← priorité 1, ordre d'arrivée préservé
// backup(2, #1), cleanup(2, #3), archive(2, #5)  ← priorité 2, ordre d'arrivée préservé
```

Avec `std::sort` (non stable), les tâches de même priorité pourraient apparaître dans n'importe quel ordre — `archive` pourrait précéder `backup`, par exemple.

### Coût de la stabilité

La stabilité a un prix. `std::stable_sort` a une complexité de **O(n log n)** s'il dispose de suffisamment de mémoire auxiliaire, mais peut dégrader à **O(n log² n)** si la mémoire est limitée. Il utilise typiquement un **merge sort** (tri fusion), qui nécessite O(n) mémoire supplémentaire dans le cas optimal.

| Critère | `std::sort` | `std::stable_sort` |
|---|---|---|
| Complexité temps | O(n log n) garanti | O(n log n) à O(n log² n) |
| Mémoire auxiliaire | O(log n) (pile de récursion) | O(n) idéalement |
| Stabilité | Non | Oui |
| Algorithme typique | Introsort | Merge sort |

En pratique, `std::sort` est plus rapide sur la majorité des cas. Utilisez `std::stable_sort` uniquement quand la stabilité est effectivement requise — le cas classique étant le **tri multi-critères par étapes successives** (trier d'abord par un critère secondaire, puis stable-sort par le critère principal).

### Version Ranges

```cpp
std::ranges::stable_sort(tasks, {}, &Task::priority);
```

---

## std::partial_sort — Trier seulement les N premiers

Quand on n'a besoin que des N plus petits (ou plus grands) éléments dans l'ordre, trier l'intégralité de la séquence est un gaspillage. `std::partial_sort` place les N premiers éléments triés au début, et laisse le reste dans un ordre non spécifié :

```cpp
std::vector<int> v = {42, 17, 93, 5, 68, 31, 7, 84, 12};

// Obtenir les 3 plus petits éléments, triés
std::partial_sort(v.begin(), v.begin() + 3, v.end());
// v == {5, 7, 12, ...le reste dans un ordre quelconque...}
//       ^^^^^^^^^ triés
```

La signature est `partial_sort(first, middle, last)` : les éléments de `[first, middle)` seront triés, les éléments de `[middle, last)` seront dans un ordre indéterminé.

**Complexité** : O(n log k), où n est la taille totale et k le nombre d'éléments à trier (la distance entre `first` et `middle`). C'est nettement mieux que O(n log n) quand k est petit par rapport à n — typiquement pour un « top 10 » sur des millions d'éléments.

```cpp
struct LogEntry {
    std::string message;
    int severity;
    std::string timestamp;
};

std::vector<LogEntry> logs = /* ...milliers d'entrées... */;

// Obtenir les 10 entrées les plus critiques
std::partial_sort(logs.begin(), logs.begin() + 10, logs.end(),
    [](const LogEntry& a, const LogEntry& b) {
        return a.severity > b.severity;  // sévérité décroissante
    }
);

// Les 10 premières entrées sont les plus critiques, triées
for (int i = 0; i < 10; ++i) {
    std::print("[{}] {}\n", logs[i].severity, logs[i].message);
}
```

### std::partial_sort_copy — Trier dans un autre conteneur

Si on veut conserver la séquence d'origine intacte, `std::partial_sort_copy` copie les N plus petits éléments triés dans un conteneur de destination :

```cpp
std::vector<int> v = {42, 17, 93, 5, 68, 31, 7, 84, 12};  
std::vector<int> top3(3);  

std::partial_sort_copy(v.begin(), v.end(), top3.begin(), top3.end());
// top3 == {5, 7, 12}
// v est inchangé
```

---

## std::nth_element — Sélection du N-ième élément

`std::nth_element` résout un problème plus simple que le tri : placer le N-ième élément à sa position finale (celle qu'il aurait si la séquence était entièrement triée), avec la garantie que tous les éléments avant lui sont ≤ et tous ceux après lui sont ≥, mais sans trier ni l'un ni l'autre de ces sous-ensembles.

```cpp
std::vector<int> v = {42, 17, 93, 5, 68, 31, 7, 84, 12};

// Placer la médiane (index 4) à sa position correcte
std::nth_element(v.begin(), v.begin() + 4, v.end());
// v[4] == 31 (la médiane)
// v[0..3] contiennent {5, 7, 12, 17} dans un ordre quelconque
// v[5..8] contiennent {42, 68, 84, 93} dans un ordre quelconque
```

**Complexité** : O(n) en moyenne — c'est l'algorithme de sélection, bien plus rapide qu'un tri complet. Il est basé sur une variante de quickselect (Introselect dans les implémentations modernes).

Cas d'usage typiques :

```cpp
std::vector<double> latencies = /* ...milliers de mesures... */;

// Trouver le percentile 95 (P95)
auto p95_pos = latencies.begin() + static_cast<int>(latencies.size() * 0.95);  
std::nth_element(latencies.begin(), p95_pos, latencies.end());  
double p95 = *p95_pos;  
std::print("P95 latency : {:.2f} ms\n", p95);  

// Trouver la médiane
auto mid = latencies.begin() + latencies.size() / 2;  
std::nth_element(latencies.begin(), mid, latencies.end());  
double median = *mid;  
std::print("Médiane : {:.2f} ms\n", median);  
```

`std::nth_element` est souvent la réponse quand on pense avoir besoin d'un tri mais qu'on ne veut qu'une statistique d'ordre (médiane, percentile, k-ième plus petit).

---

## std::is_sorted et std::is_sorted_until

Avant d'appliquer un algorithme de recherche dichotomique ou de fusionner deux séquences, il peut être utile de **vérifier** si les données sont déjà triées :

```cpp
std::vector<int> v1 = {1, 3, 5, 7, 9};  
std::vector<int> v2 = {1, 3, 8, 2, 9};  

bool sorted1 = std::is_sorted(v1.begin(), v1.end());  // true  
bool sorted2 = std::is_sorted(v2.begin(), v2.end());  // false  
```

`std::is_sorted_until` renvoie un itérateur vers le premier élément qui rompt l'ordre de tri :

```cpp
std::vector<int> v = {1, 3, 5, 8, 2, 9, 11};

auto it = std::is_sorted_until(v.begin(), v.end());  
std::print("Trié jusqu'à l'index {} (valeur {})\n",  
           std::distance(v.begin(), it), *it);
// Trié jusqu'à l'index 4 (valeur 2)
// La sous-séquence {1, 3, 5, 8} est triée, la rupture intervient à 2
```

Ces algorithmes sont en O(n) et peuvent servir de garde-fous défensifs, par exemple dans un `assert` :

```cpp
void process_sorted_data(const std::vector<int>& data) {
    assert(std::is_sorted(data.begin(), data.end()) &&
           "process_sorted_data requiert des données triées");
    // ... utilisation de lower_bound, binary_search, etc.
}
```

Versions Ranges :

```cpp
bool ok = std::ranges::is_sorted(v);  
auto it = std::ranges::is_sorted_until(v);  
```

---

## Tri et opérateur spaceship (C++20)

Depuis C++20, l'opérateur spaceship `<=>` simplifie la définition de l'ordre pour les types personnalisés. Un seul `operator<=>` defaulté génère automatiquement tous les opérateurs de comparaison, rendant le type directement compatible avec `std::sort` :

```cpp
struct Version {
    int major;
    int minor;
    int patch;

    auto operator<=>(const Version&) const = default;
};

std::vector<Version> versions = {
    {2, 1, 0}, {1, 9, 3}, {2, 0, 1}, {1, 9, 3}, {3, 0, 0}
};

std::sort(versions.begin(), versions.end());
// {1,9,3}, {1,9,3}, {2,0,1}, {2,1,0}, {3,0,0}
```

La comparaison se fait membre par membre dans l'ordre de déclaration (lexicographique). Pour un tri sur un seul champ, les projections Ranges restent la solution la plus directe.

---

## Tri de tableaux C et std::array

Les algorithmes de tri fonctionnent avec tout ce qui expose des itérateurs random-access, y compris les tableaux C bruts via `std::begin` / `std::end` :

```cpp
int arr[] = {42, 17, 93, 5, 68};

std::sort(std::begin(arr), std::end(arr));
// arr == {5, 17, 42, 68, 93}
```

Et naturellement avec `std::array` :

```cpp
std::array<int, 5> a = {42, 17, 93, 5, 68};

std::sort(a.begin(), a.end());
// ou en C++20
std::ranges::sort(a);
```

---

## Tri de std::list et std::forward_list

`std::sort` ne fonctionne **pas** avec `std::list` et `std::forward_list`, car ces conteneurs ne fournissent pas d'itérateurs random-access. Il faut utiliser leur **méthode membre** `.sort()`, qui implémente un merge sort adapté aux listes chaînées (manipulation de pointeurs, sans copie d'éléments) :

```cpp
std::list<int> lst = {42, 17, 93, 5, 68};

// std::sort(lst.begin(), lst.end());  // ❌ Erreur de compilation

lst.sort();                             // ✅ Méthode membre — merge sort
// lst == {5, 17, 42, 68, 93}

lst.sort(std::greater<int>{});          // Tri décroissant
```

La méthode membre est stable par nature (merge sort) et opère en O(n log n) sans mémoire auxiliaire significative.

---

## Choisir le bon algorithme de tri

```
           Besoin de trier toute la séquence ?
                   /                \
                 Oui                Non
                 /                    \
     Stabilité requise ?        Combien d'éléments ?
        /        \                 /           \
      Oui       Non          Les N premiers    Un seul (N-ième)
       |          |               |                  |
 stable_sort    sort        partial_sort        nth_element
```

| Algorithme | Complexité | Stable | Mémoire | Cas d'usage |
|---|---|---|---|---|
| `sort` | O(n log n) | Non | O(log n) | Tri général, cas par défaut |
| `stable_sort` | O(n log n) à O(n log² n) | Oui | O(n) | Tri multi-critères, préserver l'ordre |
| `partial_sort` | O(n log k) | Non | O(1) | Top-N, classement partiel |
| `nth_element` | O(n) moyen | Non | O(1) | Médiane, percentiles, sélection |
| `is_sorted` | O(n) | — | O(1) | Vérification, assertions |

Règles générales :

- **Par défaut**, utilisez `std::sort`. C'est le plus rapide dans le cas général.
- **Si la stabilité compte** (même priorité = même ordre qu'à l'arrivée), passez à `std::stable_sort`.
- **Si vous n'avez besoin que des k premiers** résultats triés, `std::partial_sort` économise du travail.
- **Si vous n'avez besoin que d'un seul élément** (médiane, percentile), `std::nth_element` est optimal en O(n).
- **Si les données sont dans une `std::list`**, utilisez la méthode membre `.sort()`.

---

## Synthèse

La STL offre un spectre complet d'algorithmes de tri, du tri généraliste (`sort`) à la sélection chirurgicale du N-ième élément (`nth_element`). Le choix se fait selon trois axes : faut-il trier toute la séquence ou seulement une partie, la stabilité est-elle requise, et quel type d'itérateurs le conteneur fournit-il. Les versions Ranges (C++20) avec projections simplifient considérablement le tri d'objets complexes, éliminant la majorité des lambdas comparateurs écrites à la main. Enfin, la règle du strict weak ordering n'est pas une recommandation mais une obligation — la violer produit un comportement indéfini que rien ne signalera avant le crash ou la corruption.

⏭️ [Transformation : std::transform, std::accumulate](/15-algorithmes-stl/03-transformation.md)
