🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 15.3 — Transformation : std::transform, std::accumulate

## Chapitre 15 — Algorithmes de la STL

---

## Introduction

Les algorithmes de recherche (15.1) et de tri (15.2) réorganisent ou inspectent les données existantes. Les algorithmes de **transformation** et de **réduction** vont plus loin : ils produisent de **nouvelles valeurs** à partir des éléments d'une séquence. Transformer chaque élément, calculer une somme, agréger des résultats, générer une séquence numérique — ces opérations constituent le cœur du traitement de données en C++.

Les algorithmes de transformation se trouvent dans deux en-têtes :

- **`<algorithm>`** — `std::transform`, `std::for_each`, `std::generate`…
- **`<numeric>`** — `std::accumulate`, `std::reduce`, `std::transform_reduce`, `std::iota`, `std::partial_sum`, `std::inner_product`…

```cpp
#include <algorithm>
#include <numeric>
#include <vector>
#include <string>
```

---

## std::transform — Appliquer une fonction à chaque élément

`std::transform` est le « map » fonctionnel de la STL. Il applique une fonction à chaque élément d'une séquence source et écrit le résultat dans une séquence de destination.

### Forme unaire : transformer chaque élément

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};  
std::vector<int> squares(v.size());  

std::transform(v.begin(), v.end(), squares.begin(), [](int x) {
    return x * x;
});
// squares == {1, 4, 9, 16, 25}
```

La destination doit avoir une taille suffisante pour accueillir les résultats. Dans l'exemple ci-dessus, `squares` est pré-alloué avec `v.size()` éléments. Écrire au-delà de la capacité provoque un comportement indéfini.

Alternative avec un `back_inserter` pour insérer dynamiquement :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};  
std::vector<int> squares;  

std::transform(v.begin(), v.end(), std::back_inserter(squares), [](int x) {
    return x * x;
});
// squares == {1, 4, 9, 16, 25}
```

`std::back_inserter` appelle `push_back` pour chaque résultat, ce qui évite de pré-allouer. C'est pratique mais légèrement moins performant à cause des réallocations potentielles. Un `squares.reserve(v.size())` avant le `transform` élimine ce surcoût.

### Transformation in-place

La source et la destination peuvent être identiques — `std::transform` fonctionne alors en place :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

std::transform(v.begin(), v.end(), v.begin(), [](int x) {
    return x * 2;
});
// v == {2, 4, 6, 8, 10}
```

### Forme binaire : combiner deux séquences

La deuxième surcharge de `std::transform` prend deux séquences en entrée et applique une opération binaire élément par élément :

```cpp
std::vector<int> a = {1, 2, 3, 4, 5};  
std::vector<int> b = {10, 20, 30, 40, 50};  
std::vector<int> sums(a.size());  

std::transform(a.begin(), a.end(), b.begin(), sums.begin(),
    [](int x, int y) { return x + y; }
);
// sums == {11, 22, 33, 44, 55}
```

La deuxième séquence doit avoir au moins autant d'éléments que la première. Le standard ne vérifie pas cette condition — si `b` est plus courte que `a`, c'est un comportement indéfini silencieux.

Un cas d'usage courant — calculer des différences entre deux séries de mesures :

```cpp
std::vector<double> before = {100.0, 200.0, 150.0, 300.0};  
std::vector<double> after  = {110.0, 195.0, 160.0, 310.0};  
std::vector<double> deltas(before.size());  

std::transform(before.begin(), before.end(), after.begin(), deltas.begin(),
    [](double b, double a) { return a - b; }
);
// deltas == {10.0, -5.0, 10.0, 10.0}
```

### Changer le type de sortie

La fonction passée à `std::transform` peut renvoyer un type différent de celui des éléments source. C'est ce qui en fait un véritable « map » :

```cpp
std::vector<int> codes = {200, 404, 500, 301, 200};  
std::vector<std::string> labels;  

std::transform(codes.begin(), codes.end(), std::back_inserter(labels),
    [](int code) -> std::string {
        switch (code) {
            case 200: return "OK";
            case 301: return "Redirect";
            case 404: return "Not Found";
            case 500: return "Server Error";
            default:  return "Unknown";
        }
    }
);
// labels == {"OK", "Not Found", "Server Error", "Redirect", "OK"}
```

### Version Ranges (C++20)

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};  
std::vector<int> squares(v.size());  

std::ranges::transform(v, squares.begin(), [](int x) { return x * x; });
```

Avec projection sur un objet complexe :

```cpp
struct Employee {
    std::string name;
    double salary;
};

std::vector<Employee> team = {
    {"Alice", 75000.0}, {"Bob", 62000.0}, {"Carol", 88000.0}
};

// Extraire les noms
std::vector<std::string> names(team.size());  
std::ranges::transform(team, names.begin(), &Employee::name);  
// names == {"Alice", "Bob", "Carol"}
```

---

## std::for_each — Appliquer un effet à chaque élément

`std::for_each` ressemble à `std::transform`, mais il n'écrit pas dans une destination. Il applique une fonction à chaque élément, généralement pour ses **effets de bord** (affichage, accumulation dans une variable externe, modification in-place via référence) :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

std::for_each(v.begin(), v.end(), [](int x) {
    std::print("{} ", x);
});
// Affiche : 1 2 3 4 5
```

Modification in-place (le paramètre doit être une référence) :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

std::for_each(v.begin(), v.end(), [](int& x) {
    x *= 3;
});
// v == {3, 6, 9, 12, 15}
```

En pratique, depuis C++11, la range-based `for` loop couvre la plupart des usages de `std::for_each` de manière plus lisible :

```cpp
for (auto& x : v) {
    x *= 3;
}
```

`std::for_each` reste pertinent dans deux cas : quand on a besoin de la valeur de retour (il renvoie l'objet fonction, ce qui permet de récupérer un état accumulé), et quand on l'utilise avec les **politiques d'exécution parallèle** (section 15.7), ce que la range-based `for` loop ne supporte pas :

```cpp
#include <execution>

std::for_each(std::execution::par, v.begin(), v.end(), [](int& x) {
    x *= 3;  // exécuté en parallèle sur plusieurs cœurs
});
```

### std::for_each_n (C++17)

Variante qui traite les N premiers éléments seulement :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

std::for_each_n(v.begin(), 4, [](int& x) { x *= 10; });
// v == {10, 20, 30, 40, 5, 6, 7, 8}
```

---

## std::accumulate — Réduction séquentielle

`std::accumulate` (dans `<numeric>`) est l'algorithme de **réduction** (fold) de la STL. Il parcourt une séquence et combine progressivement tous les éléments en une seule valeur, en partant d'une valeur initiale :

```cpp
#include <numeric>

std::vector<int> v = {1, 2, 3, 4, 5};

int sum = std::accumulate(v.begin(), v.end(), 0);
// sum == 15  (0 + 1 + 2 + 3 + 4 + 5)

int product = std::accumulate(v.begin(), v.end(), 1, std::multiplies<int>{});
// product == 120  (1 * 1 * 2 * 3 * 4 * 5)
```

Le troisième paramètre est la **valeur initiale**. Son type détermine le type du résultat — un piège classique :

```cpp
std::vector<double> v = {1.5, 2.5, 3.5};

// ⚠️ PIÈGE : valeur initiale int → résultat int, troncatures à chaque étape
int bad = std::accumulate(v.begin(), v.end(), 0);
// bad == 6 (1.5→1, +2.5→3, +3.5→6 — troncatures successives)

// ✅ CORRECT : valeur initiale double → résultat double
double good = std::accumulate(v.begin(), v.end(), 0.0);
// good == 7.5
```

Le type de la valeur initiale pilote toute la chaîne de calcul. Passer `0` au lieu de `0.0` est une erreur extrêmement courante avec des séquences de flottants.

### Opération personnalisée

Le quatrième paramètre est une opération binaire. Le premier argument est l'accumulateur, le second est l'élément courant :

```cpp
std::vector<std::string> words = {"Hello", " ", "World", "!"};

std::string sentence = std::accumulate(words.begin(), words.end(), std::string{});
// sentence == "Hello World!"
```

Un cas plus élaboré — construire un résumé statistique :

```cpp
struct Stats {
    double sum = 0.0;
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
    int count = 0;
};

std::vector<double> data = {3.14, 2.71, 1.41, 1.73, 2.23};

Stats result = std::accumulate(data.begin(), data.end(), Stats{},
    [](Stats acc, double val) {
        acc.sum += val;
        acc.min = std::min(acc.min, val);
        acc.max = std::max(acc.max, val);
        acc.count++;
        return acc;
    }
);

std::print("Somme: {:.2f}, Min: {:.2f}, Max: {:.2f}, Moyenne: {:.2f}\n",
           result.sum, result.min, result.max, result.sum / result.count);
// Somme: 11.22, Min: 1.41, Max: 3.14, Moyenne: 2.24
```

### Limitation de std::accumulate : strictement séquentiel

`std::accumulate` est **séquentiel par définition**. Il traite les éléments de gauche à droite, un par un. Il ne peut pas être parallélisé, car le standard garantit cet ordre d'évaluation. Pour une réduction parallélisable, C++17 introduit `std::reduce`.

---

## std::reduce (C++17) — Réduction parallélisable

`std::reduce` est le successeur parallélisable de `std::accumulate`. Il produit le même résultat pour les opérations **associatives et commutatives** (addition, multiplication, min, max…), mais autorise l'implémentation à réordonner et regrouper les opérations, ce qui permet la parallélisation :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

int sum = std::reduce(v.begin(), v.end(), 0);
// sum == 15

int product = std::reduce(v.begin(), v.end(), 1, std::multiplies<int>{});
// product == 120
```

La différence fondamentale avec `std::accumulate` :

```
std::accumulate (gauche à droite, garanti) :
    ((((0 + 1) + 2) + 3) + 4) + 5

std::reduce (ordre non spécifié) :
    Peut calculer (1 + 2) et (3 + 4) en parallèle,
    puis (3 + 7) et (0 + 5), etc.
```

Avec une politique d'exécution parallèle :

```cpp
#include <execution>

std::vector<int> v = /* ...millions d'éléments... */;

int sum = std::reduce(std::execution::par, v.begin(), v.end(), 0);
// Réduit en parallèle sur plusieurs cœurs
```

### Quand l'ordre compte : accumulate vs reduce

Si l'opération n'est **pas** associative ou commutative, `std::reduce` peut donner des résultats différents de `std::accumulate`. L'exemple le plus parlant est la concaténation de chaînes :

```cpp
std::vector<std::string> words = {"A", "B", "C", "D"};

// accumulate : garanti gauche-à-droite
std::string s1 = std::accumulate(words.begin(), words.end(), std::string{});
// s1 == "ABCD" — toujours

// reduce : ordre non spécifié
std::string s2 = std::reduce(words.begin(), words.end(), std::string{});
// s2 pourrait être "ABCD", "BACD", "CDAB"... résultat non déterministe
```

La concaténation de chaînes est associative mais **pas commutative** (« AB » ≠ « BA »). `std::reduce` est donc inapproprié ici. La règle est simple : si changer l'ordre de calcul change le résultat, utilisez `std::accumulate`. Sinon, préférez `std::reduce` qui ouvre la porte à la parallélisation.

Pour les flottants, l'addition n'est techniquement pas associative (à cause des erreurs d'arrondi), mais la différence est généralement négligeable. `std::reduce` est le choix standard pour les sommes numériques quand la reproductibilité bit-à-bit n'est pas critique.

---

## std::transform_reduce (C++17) — Map + Reduce fusionnés

`std::transform_reduce` combine une transformation et une réduction en une seule opération. C'est l'équivalent d'un `transform` suivi d'un `reduce`, mais sans créer de séquence intermédiaire :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};

// Somme des carrés — sans conteneur intermédiaire
int sum_sq = std::transform_reduce(
    v.begin(), v.end(),
    0,                                    // valeur initiale
    std::plus<>{},                        // opération de réduction
    [](int x) { return x * x; }          // transformation
);
// sum_sq == 55  (1 + 4 + 9 + 16 + 25)
```

La forme binaire calcule un produit scalaire (ou toute opération similaire sur deux séquences) :

```cpp
std::vector<double> prices    = {10.0, 25.0, 5.0};  
std::vector<int>    quantities = {3, 2, 10};  

// Montant total : somme des (prix × quantité)
double total = std::transform_reduce(
    prices.begin(), prices.end(),
    quantities.begin(),
    0.0                   // valeur initiale
);
// total == 130.0  (30.0 + 50.0 + 50.0)
// Utilise + et * par défaut (produit scalaire)
```

Avec des opérations personnalisées :

```cpp
std::vector<double> predictions = {2.5, 3.0, 4.5, 5.0};  
std::vector<double> actuals     = {2.7, 2.8, 4.6, 5.2};  

// Somme des erreurs absolues
double mae = std::transform_reduce(
    predictions.begin(), predictions.end(),
    actuals.begin(),
    0.0,                                                  // init
    std::plus<>{},                                        // reduce
    [](double pred, double actual) {                      // transform
        return std::abs(pred - actual);
    }
);
// mae == 0.7  (0.2 + 0.2 + 0.1 + 0.2)
```

`std::transform_reduce` est parallélisable et accepte les politiques d'exécution :

```cpp
double total = std::transform_reduce(
    std::execution::par,
    prices.begin(), prices.end(),
    quantities.begin(),
    0.0
);
```

---

## std::inner_product — Produit scalaire (pré-C++17)

Avant C++17, le produit scalaire et les réductions combinées se faisaient avec `std::inner_product`. Il reste disponible mais `std::transform_reduce` est préférable en code moderne, car il supporte la parallélisation :

```cpp
std::vector<double> a = {1.0, 2.0, 3.0};  
std::vector<double> b = {4.0, 5.0, 6.0};  

double dot = std::inner_product(a.begin(), a.end(), b.begin(), 0.0);
// dot == 32.0  (1*4 + 2*5 + 3*6)
```

Tout comme `std::accumulate`, `std::inner_product` est strictement séquentiel.

---

## std::partial_sum — Sommes partielles (prefix sum)

`std::partial_sum` calcule les sommes cumulatives d'une séquence. Chaque élément de la sortie est la somme de tous les éléments source jusqu'à cette position :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};  
std::vector<int> cumul(v.size());  

std::partial_sum(v.begin(), v.end(), cumul.begin());
// cumul == {1, 3, 6, 10, 15}
//           1  1+2  1+2+3  1+2+3+4  1+2+3+4+5
```

Avec une opération personnalisée :

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};  
std::vector<int> running_max(v.size());  

std::partial_sum(v.begin(), v.end(), running_max.begin(),
    [](int acc, int val) { return std::max(acc, val); }
);
// running_max == {1, 2, 3, 4, 5}  (le max ne fait que croître ici)
```

Cas d'usage courant — calculer un budget cumulé pour détecter le mois de dépassement :

```cpp
std::vector<double> monthly_expenses = {
    1200.0, 1350.0, 980.0, 1500.0, 1100.0, 1450.0
};
std::vector<double> cumulative(monthly_expenses.size());

std::partial_sum(monthly_expenses.begin(), monthly_expenses.end(),
                 cumulative.begin());
// cumulative == {1200, 2550, 3530, 5030, 6130, 7580}

double budget = 5000.0;  
auto it = std::find_if(cumulative.begin(), cumulative.end(),  
    [budget](double total) { return total > budget; }
);

if (it != cumulative.end()) {
    auto month = std::distance(cumulative.begin(), it) + 1;
    std::print("Budget dépassé au mois {} (cumul : {:.0f})\n", month, *it);
    // Budget dépassé au mois 4 (cumul : 5030)
}
```

### std::inclusive_scan et std::exclusive_scan (C++17)

C++17 introduit des variantes parallélisables de `std::partial_sum` :

- **`std::inclusive_scan`** — inclut l'élément courant dans le cumul (comme `partial_sum`).
- **`std::exclusive_scan`** — exclut l'élément courant (chaque position contient la somme des éléments *précédents*).

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};  
std::vector<int> inc(v.size()), exc(v.size());  

std::inclusive_scan(v.begin(), v.end(), inc.begin());
// inc == {1, 3, 6, 10, 15}

std::exclusive_scan(v.begin(), v.end(), exc.begin(), 0);
// exc == {0, 1, 3, 6, 10}
```

La différence est subtile mais importante en algorithmique parallèle et dans certains calculs (offsets, positions cumulées, allocation par tranches).

Ces deux algorithmes acceptent les politiques d'exécution :

```cpp
std::inclusive_scan(std::execution::par, v.begin(), v.end(), inc.begin());
```

---

## std::iota — Générer une séquence numérique

`std::iota` (dans `<numeric>`) remplit une séquence avec des valeurs incrémentales à partir d'une valeur de départ :

```cpp
std::vector<int> indices(10);

std::iota(indices.begin(), indices.end(), 0);
// indices == {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}

std::iota(indices.begin(), indices.end(), 100);
// indices == {100, 101, 102, 103, 104, 105, 106, 107, 108, 109}
```

Cas d'usage fréquent — créer un vecteur d'indices pour un tri indirect (trier les indices selon les valeurs d'un autre vecteur) :

```cpp
std::vector<std::string> names = {"Charlie", "Alice", "Bob", "Dave"};  
std::vector<int> order(names.size());  

std::iota(order.begin(), order.end(), 0);
// order == {0, 1, 2, 3}

std::sort(order.begin(), order.end(), [&names](int a, int b) {
    return names[a] < names[b];
});
// order == {1, 2, 0, 3}  (indices triés par nom alphabétique)

for (int i : order) {
    std::print("{}\n", names[i]);
}
// Alice, Bob, Charlie, Dave — sans modifier le vecteur original
```

---

## std::generate et std::generate_n — Remplissage par générateur

`std::generate` remplit une séquence en appelant un callable (sans argument) pour chaque élément :

```cpp
std::vector<int> v(10);

// Remplir avec des puissances de 2
int power = 1;  
std::generate(v.begin(), v.end(), [&power]() {  
    int result = power;
    power *= 2;
    return result;
});
// v == {1, 2, 4, 8, 16, 32, 64, 128, 256, 512}
```

`std::generate_n` génère un nombre précis d'éléments :

```cpp
std::vector<int> v;  
v.reserve(5);  

std::generate_n(std::back_inserter(v), 5, [n = 0]() mutable {
    int current = n++;
    return current * current;  // 0, 1, 4, 9, 16
});
```

Un cas d'usage courant — générer des données aléatoires :

```cpp
#include <random>

std::vector<double> samples(1000);  
std::mt19937 gen(42);  // seed fixe pour reproductibilité  
std::normal_distribution<double> dist(0.0, 1.0);  

std::generate(samples.begin(), samples.end(), [&]() {
    return dist(gen);
});
// samples contient 1000 valeurs suivant une loi normale N(0,1)
```

---

## Résumé : choisir le bon algorithme

| Besoin | Algorithme | En-tête | Parallélisable |
|---|---|---|---|
| Transformer chaque élément | `std::transform` | `<algorithm>` | Oui (C++17) |
| Effet de bord sur chaque élément | `std::for_each` | `<algorithm>` | Oui (C++17) |
| Réduction séquentielle | `std::accumulate` | `<numeric>` | Non |
| Réduction parallélisable | `std::reduce` | `<numeric>` | Oui |
| Transformation + réduction | `std::transform_reduce` | `<numeric>` | Oui |
| Produit scalaire (legacy) | `std::inner_product` | `<numeric>` | Non |
| Sommes cumulatives | `std::partial_sum` | `<numeric>` | Non |
| Sommes cumulatives parallèles | `std::inclusive_scan` / `std::exclusive_scan` | `<numeric>` | Oui |
| Séquence incrémentale | `std::iota` | `<numeric>` | Non |
| Remplissage par générateur | `std::generate` | `<algorithm>` | Non |

La tendance en C++ moderne est claire : les algorithmes de `<numeric>` introduits en C++17 (`reduce`, `transform_reduce`, `inclusive_scan`, `exclusive_scan`) sont les versions parallélisables de leurs prédécesseurs (`accumulate`, `inner_product`, `partial_sum`). Pour du nouveau code, préférez les versions C++17 même sans parallélisation immédiate — elles ouvrent la porte à l'optimisation future par simple ajout d'une politique d'exécution.

---

## Synthèse

`std::transform` et `std::accumulate` forment le duo fondamental du traitement de données en C++ : transformer les éléments un à un, puis les agréger en un résultat unique. C++17 étend ce modèle avec `std::reduce` et `std::transform_reduce` qui fusionnent transformation et réduction en une seule passe parallélisable. Les algorithmes cumulatifs (`partial_sum`, `inclusive_scan`, `exclusive_scan`) couvrent les cas où le résultat intermédiaire à chaque étape est lui-même significatif. Enfin, `std::iota` et `std::generate` complètent le tableau en permettant de construire des séquences à partir de rien. Ensemble, ces algorithmes remplacent la grande majorité des boucles `for` manuelles par des expressions déclaratives dont l'intention est immédiatement lisible.

⏭️ [Manipulation : std::copy, std::move, std::remove_if](/15-algorithmes-stl/04-manipulation.md)
