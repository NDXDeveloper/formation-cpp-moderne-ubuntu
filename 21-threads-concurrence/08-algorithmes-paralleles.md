🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 21.8 Algorithmes parallèles appliqués à la concurrence

> 📎 *Cette section met en pratique les politiques d'exécution parallèle dans un contexte multi-thread. Pour la couverture détaillée des politiques d'exécution et des algorithmes parallèles de la STL, voir la **section 15.7**.*

## Paralléliser sans gérer de threads

Tout au long de ce chapitre, vous avez appris à créer des threads, les synchroniser, protéger les données partagées, coordonner les arrêts. Cette machinerie est indispensable pour les architectures concurrentes complexes — serveurs, pools de workers, pipelines de données. Mais pour un large éventail de problèmes, elle est disproportionnée.

Si votre besoin se résume à « appliquer une opération sur chaque élément d'une collection, en parallèle », la STL offre une solution immédiate depuis C++17 : les **politiques d'exécution**. Un seul argument supplémentaire transforme un algorithme séquentiel en algorithme parallèle, sans thread à créer, sans mutex à gérer, sans join à orchestrer.

```cpp
#include <algorithm>
#include <execution>
#include <vector>

std::vector<double> data(10'000'000);

// Séquentiel
std::sort(data.begin(), data.end());

// Parallèle — une seule modification
std::sort(std::execution::par, data.begin(), data.end());
```

L'implémentation gère en interne le partitionnement des données, la création des threads (ou leur réutilisation via un pool), et la synchronisation des résultats. Vous n'avez qu'à garantir que l'opération elle-même est thread-safe.

---

## Rappel des politiques d'exécution

Le header `<execution>` définit quatre politiques :

| Politique | Objet | Comportement |
|-----------|-------|-------------|
| Séquentielle | `std::execution::seq` | Exécution dans le thread appelant, comme sans politique |
| Parallèle | `std::execution::par` | Exécution répartie sur plusieurs threads |
| Parallèle non-séquencée | `std::execution::par_unseq` | Parallèle + vectorisation SIMD possible |
| Non-séquencée | `std::execution::unseq` | Vectorisation SIMD dans un seul thread (C++20) |

En contexte de concurrence, les deux politiques pertinentes sont `par` et `par_unseq`. La différence pratique :

- **`par`** : votre opération peut contenir des appels synchronisants (acquérir un mutex, par exemple). L'implémentation exécute les éléments sur différents threads mais respecte un ordonnancement qui tolère la synchronisation.
- **`par_unseq`** : votre opération ne doit contenir **aucun** appel synchronisant — pas de mutex, pas de lock, pas d'allocation qui pourrait verrouiller en interne. L'implémentation peut entrelacer les opérations sur un même thread (vectorisation), ce qui rendrait un deadlock inévitable si un verrou était acquis.

---

## Quand utiliser les algorithmes parallèles vs les threads manuels

Les algorithmes parallèles et la gestion manuelle des threads ne sont pas en compétition — ils répondent à des besoins différents :

| Besoin | Solution |
|--------|----------|
| Appliquer une transformation à une grande collection | `std::execution::par` + algorithme STL |
| Trier un gros vecteur | `std::sort(par, ...)` |
| Réduire/agréger des données | `std::reduce(par, ...)` |
| Serveur avec connections concurrentes | Threads manuels / pool de workers |
| Pipeline producteur/consommateur | Threads + queues bloquantes |
| Tâches hétérogènes en parallèle | `std::async` ou `std::jthread` |
| Boucle de traitement avec arrêt propre | `std::jthread` + `stop_token` |

La règle est simple : si votre problème est un **data parallelism** (même opération sur beaucoup d'éléments), les algorithmes parallèles sont la solution la plus concise et souvent la plus performante. Si votre problème est un **task parallelism** (tâches hétérogènes, coordination, communication), les primitives du chapitre sont nécessaires.

---

## Application pratique : traitement de données en pipeline

Un scénario courant combine les deux approches : des threads gèrent l'architecture globale (lecture, écriture, coordination), et les algorithmes parallèles accélèrent les phases de calcul intensif à l'intérieur de chaque étape.

```cpp
#include <algorithm>
#include <execution>
#include <numeric>
#include <vector>
#include <thread>
#include <print>
#include <cmath>
#include <mutex>
#include <queue>
#include <condition_variable>

struct Batch {
    std::vector<double> samples;
};

// Étape de traitement : normalisation + filtrage
// Utilise les algorithmes parallèles pour le calcul intensif
Batch process_batch(Batch batch) {
    // 1. Calculer la moyenne (réduction parallèle)
    double sum = std::reduce(std::execution::par,
                             batch.samples.begin(),
                             batch.samples.end(),
                             0.0);
    double mean = sum / static_cast<double>(batch.samples.size());

    // 2. Normaliser chaque échantillon (transformation parallèle)
    std::for_each(std::execution::par, 
                  batch.samples.begin(), batch.samples.end(),
                  [mean](double& val) {
                      val -= mean;
                  });

    // 3. Filtrer les outliers (suppression parallèle)
    auto new_end = std::remove_if(std::execution::par,
                                  batch.samples.begin(),
                                  batch.samples.end(),
                                  [](double val) {
                                      return std::abs(val) > 3.0;
                                  });
    batch.samples.erase(new_end, batch.samples.end());

    return batch;
}
```

Dans cet exemple, l'architecture globale (lecture des batches, dispatch, écriture des résultats) est gérée par des threads ou un pool de workers. Mais le traitement de chaque batch — qui porte sur des millions d'échantillons — exploite les algorithmes parallèles pour saturer les cœurs disponibles sans aucune gestion manuelle de threads.

---

## std::reduce vs std::accumulate

En contexte parallèle, `std::reduce` remplace `std::accumulate` pour les opérations de réduction. La différence est fondamentale :

- **`std::accumulate`** : applique l'opération strictement de gauche à droite. Séquentiel par nature. N'accepte pas de politique d'exécution.
- **`std::reduce`** : autorise l'implémentation à grouper et réordonner les opérations. Compatible avec les politiques d'exécution.

```cpp
#include <numeric>
#include <execution>
#include <vector>

std::vector<double> data(10'000'000, 1.0);

// Séquentiel strict (gauche à droite)
double sum1 = std::accumulate(data.begin(), data.end(), 0.0);

// Parallèle (ordre des opérations non garanti)
double sum2 = std::reduce(std::execution::par, data.begin(), data.end(), 0.0);
```

> ⚠️ **Précision flottante** : `std::reduce` ne garantit pas le même résultat que `std::accumulate` pour les types à virgule flottante. L'addition flottante n'est pas associative — `(a + b) + c` peut différer de `a + (b + c)` en raison des arrondis. En parallèle, l'ordre des additions est indéterminé, donc le résultat peut varier légèrement d'une exécution à l'autre. Pour les entiers, les deux donnent le même résultat.

### std::transform_reduce : map-reduce en une passe

`std::transform_reduce` combine une transformation et une réduction en une seule opération parallélisable. C'est l'équivalent du pattern *map-reduce* :

```cpp
#include <numeric>
#include <execution>
#include <vector>
#include <print>

struct Employee {
    std::string name;
    double salary;
    bool active;
};

void compute_total_salary(const std::vector<Employee>& employees) {
    // Map : extraire le salaire si actif (sinon 0)
    // Reduce : sommer
    double total = std::transform_reduce(
        std::execution::par,
        employees.begin(), employees.end(),
        0.0,                                          // Valeur initiale (reduce)
        std::plus<>{},                                // Opération de réduction
        [](const Employee& e) -> double {             // Transformation (map)
            return e.active ? e.salary : 0.0;
        }
    );

    std::println("Masse salariale active : {:.2f}", total);
}
```

Sans `transform_reduce`, il faudrait soit deux passes (filtrer puis sommer), soit une boucle manuelle avec un mutex pour le total — les deux étant moins performants.

---

## Contraintes de thread-safety sur les opérations

Quand vous passez un callable (lambda, foncteur) à un algorithme parallèle, ce callable sera invoqué depuis **plusieurs threads simultanément**. Il doit donc respecter des contraintes strictes :

### Avec std::execution::par

Les invocations se font sur différents threads mais ne sont pas entrelacées sur un même thread. Vous pouvez utiliser des primitives de synchronisation si nécessaire :

```cpp
std::mutex mtx;  
std::vector<std::string> errors;  

std::for_each(std::execution::par,
              items.begin(), items.end(),
              [&](const Item& item) {
                  auto result = validate(item);
                  if (!result.ok) {
                      // ✅ OK avec par : le mutex est autorisé
                      std::lock_guard lock(mtx);
                      errors.push_back(result.message);
                  }
              });
```

### Avec std::execution::par_unseq

Les invocations peuvent être entrelacées sur un même thread (vectorisation). Tout appel bloquant ou synchronisant est interdit :

```cpp
// ❌ DEADLOCK potentiel avec par_unseq
std::for_each(std::execution::par_unseq,
              items.begin(), items.end(),
              [&](const Item& item) {
                  std::lock_guard lock(mtx);  // Interdit ! Peut deadlock
                  results.push_back(process(item));
              });

// ✅ Correct avec par_unseq : aucune synchronisation
std::for_each(std::execution::par_unseq,
              items.begin(), items.end(),
              [](Item& item) {
                  item.value = compute(item.input);  // Écriture locale à l'élément
              });
```

### Règle d'or

- L'opération **ne doit pas modifier d'état partagé** sans synchronisation.
- Avec `par` : la synchronisation est autorisée (mutex, atomic).
- Avec `par_unseq` : **aucune** synchronisation. L'opération doit être pure ou ne modifier que l'élément courant.

Le pattern le plus sûr est de structurer le calcul pour qu'il soit **embarrassingly parallel** — chaque élément est traité indépendamment, sans état partagé :

```cpp
// Pattern idéal : chaque élément est indépendant
std::vector<Input> inputs = load_data();  
std::vector<Output> outputs(inputs.size());  

std::transform(std::execution::par_unseq,
               inputs.begin(), inputs.end(),
               outputs.begin(),
               [](const Input& in) {
                   return compute(in);  // Fonction pure, pas d'état partagé
               });
```

---

## Performances : quand la parallélisation paie

La parallélisation a un coût fixe : partitionnement des données, création/réveil de threads (ou dispatch vers un pool), synchronisation des résultats. Ce coût est amorti seulement si le volume de données ou le temps de calcul par élément est suffisant.

### Seuils indicatifs

Voici des ordres de grandeur mesurés sur un poste typique (8 cœurs, GCC 14+, libstdc++ avec TBB) :

| Algorithme | Seuil approximatif pour que `par` soit rentable |
|------------|--------------------------------------------------|
| `std::sort` | > 50 000 éléments |
| `std::for_each` (opération légère) | > 100 000 éléments |
| `std::for_each` (opération lourde) | > 1 000 éléments |
| `std::reduce` | > 100 000 éléments |
| `std::transform` | > 100 000 éléments (opération légère) |

Ces seuils varient considérablement selon le matériel, le compilateur, et la complexité de l'opération. **Mesurez toujours** avec un benchmark réaliste avant de conclure qu'une parallélisation est bénéfique.

```cpp
#include <algorithm>
#include <execution>
#include <vector>
#include <chrono>
#include <print>

void benchmark_sort() {
    constexpr int N = 10'000'000;
    std::vector<int> data(N);
    std::ranges::generate(data, std::rand);

    auto data_par = data;  // Copie pour le test parallèle

    // Séquentiel
    auto t0 = std::chrono::steady_clock::now();
    std::sort(data.begin(), data.end());
    auto t1 = std::chrono::steady_clock::now();

    // Parallèle
    auto t2 = std::chrono::steady_clock::now();
    std::sort(std::execution::par, data_par.begin(), data_par.end());
    auto t3 = std::chrono::steady_clock::now();

    auto seq_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    auto par_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();

    std::println("Séquentiel : {:.1f} ms", seq_ms);
    std::println("Parallèle  : {:.1f} ms", par_ms);
    std::println("Speedup    : {:.1f}x", seq_ms / par_ms);
}
```

Sur 10 millions d'entiers avec 8 cœurs, le speedup typique est de 3-5x pour le tri (le tri n'est pas parfaitement parallélisable en raison de la phase de fusion).

---

## Prérequis d'implémentation sur Linux

Les algorithmes parallèles de la STL ne sont pas une fonctionnalité « gratuite » — ils nécessitent un backend de threading. Les deux principaux compilateurs sur Linux ont des approches différentes :

### GCC (libstdc++)

Depuis GCC 9, les algorithmes parallèles sont implémentés via **Intel TBB** (Threading Building Blocks). TBB doit être installé et lié explicitement :

```bash
# Installation
sudo apt install libtbb-dev

# Compilation — le flag -ltbb est indispensable
g++ -std=c++23 -O2 main.cpp -ltbb -o main
```

Sans `-ltbb`, le code compile mais les algorithmes s'exécutent **séquentiellement** sans avertissement. C'est un piège courant.

### Clang (libc++)

Le support des algorithmes parallèles dans libc++ est historiquement en retard par rapport à libstdc++. Vérifiez l'état du support pour votre version de Clang. Une option est d'utiliser libstdc++ avec Clang :

```bash
clang++ -std=c++23 -O2 -stdlib=libstdc++ main.cpp -ltbb -o main
```

> 📎 *Pour les détails d'installation et les subtilités de compatibilité, voir la **section 15.7.3** (Précautions et limitations pratiques).*

---

## Combiner threads manuels et algorithmes parallèles

Un point important : les algorithmes parallèles utilisent en interne un pool de threads (celui de TBB, typiquement). Si vous lancez un algorithme parallèle depuis **plusieurs threads simultanément**, les pools sont partagés et TBB gère automatiquement la distribution du travail sans surcharger le système.

Cela signifie que vous pouvez combiner les deux approches sans craindre une explosion du nombre de threads :

```cpp
#include <algorithm>
#include <execution>
#include <thread>
#include <vector>
#include <print>
#include <cmath>

struct Dataset {
    std::vector<double> values;
    double result;
};

void process_dataset(Dataset& ds) {
    // Chaque thread utilise les algorithmes parallèles en interne
    // TBB partage le pool de threads entre tous les appelants
    std::for_each(std::execution::par,
                  ds.values.begin(), ds.values.end(),
                  [](double& v) { v = std::sqrt(std::abs(v)); });

    ds.result = std::reduce(std::execution::par,
                            ds.values.begin(), ds.values.end(),
                            0.0);
}

int main() {
    std::vector<Dataset> datasets(4);
    for (auto& ds : datasets) {
        ds.values.resize(5'000'000);
        std::ranges::generate(ds.values, [] {
            return static_cast<double>(std::rand()) / RAND_MAX;
        });
    }

    // Threads de haut niveau pour le task parallelism
    std::vector<std::jthread> threads;
    for (auto& ds : datasets) {
        threads.emplace_back([&ds] {
            process_dataset(ds);  // Data parallelism en interne
        });
    }

    // Les jthreads se joignent automatiquement
    threads.clear();

    for (int i = 0; i < 4; ++i) {
        std::println("Dataset {} : somme = {:.2f}", i, datasets[i].result);
    }
}
```

Ce pattern — task parallelism en surface, data parallelism en profondeur — est courant dans les applications de traitement de données, les simulations scientifiques, et les services de calcul.

---

## Résumé

| Aspect | Détail |
|--------|--------|
| Header | `<execution>`, `<algorithm>`, `<numeric>` |
| Standard | C++17 (politiques), C++20 (`unseq`) |
| Politiques principales | `par` (parallèle), `par_unseq` (parallèle + vectorisation) |
| Backend Linux (GCC) | Intel TBB — lier avec `-ltbb` |
| `std::reduce` | Remplacement parallèle de `std::accumulate` |
| `std::transform_reduce` | Map-reduce en une passe |
| Contrainte `par` | Synchronisation autorisée (mutex, atomic) |
| Contrainte `par_unseq` | Aucune synchronisation — opérations pures uniquement |
| Seuil de rentabilité | Typiquement > 50 000 - 100 000 éléments (mesurer !) |
| Combinaison avec threads | Sûre — TBB partage son pool entre appelants |
| Cas d'usage idéal | Data parallelism sur de grandes collections |

---

## Conclusion du chapitre

Ce chapitre a couvert l'ensemble des outils de programmation concurrente disponibles en C++ moderne, des briques les plus basses aux abstractions les plus hautes :

- **`std::thread`** et **`std::jthread`** pour créer et gérer des threads d'exécution.
- **`std::mutex`**, **`std::lock_guard`**, **`std::unique_lock`**, **`std::scoped_lock`** pour protéger les données partagées.
- **`std::condition_variable`** pour coordonner les threads par signalisation.
- **`std::atomic`** pour les opérations indivisibles sans verrou.
- **`std::async`**, **`std::future`**, **`std::promise`** pour la programmation asynchrone orientée résultat.
- Les **principes de thread-safety** et les **patterns de conception** pour structurer le code concurrent.
- Les **algorithmes parallèles** de la STL pour paralléliser sans gérer de threads.

Le fil conducteur est une progression vers l'abstraction : commencez par le niveau le plus haut qui résout votre problème (algorithmes parallèles, `std::async`), et ne descendez vers les primitives bas niveau (`std::thread`, mutex, atomiques) que quand le contrôle supplémentaire est nécessaire. Chaque couche d'abstraction élimine des catégories entières de bugs — et en concurrence, les bugs sont les plus coûteux à trouver et à corriger.

⏭️ [Networking et Communication](/22-networking/README.md)
