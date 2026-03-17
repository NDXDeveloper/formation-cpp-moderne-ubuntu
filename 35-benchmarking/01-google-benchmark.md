🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 35.1 Google Benchmark : Micro-benchmarking

## Installation et intégration CMake

Google Benchmark suit le même modèle d'intégration que Google Test. L'approche recommandée est FetchContent pour les mêmes raisons — reproductibilité et cohérence des flags de compilation (section 33.1).

### FetchContent (recommandé)

```cmake
# benchmarks/CMakeLists.txt
include(FetchContent)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.5
    GIT_SHALLOW    ON
)

# Désactiver les tests internes de Google Benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

add_executable(mon_projet_benchmarks
    bench_sort.cpp
    bench_containers.cpp
)

target_link_libraries(mon_projet_benchmarks
    PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main  # Fournit le main()
        mon_projet_lib
)
```

Comme pour GTest, `benchmark::benchmark_main` fournit un `main()` prêt à l'emploi qui initialise le framework, parse les options de ligne de commande et exécute tous les benchmarks enregistrés. Si vous avez besoin d'un `main()` personnalisé (initialisation de ressources globales), liez `benchmark::benchmark` seul et écrivez votre propre point d'entrée.

### Installation système

```bash
sudo apt install libbenchmark-dev
```

Le `CMakeLists.txt` utilise alors `find_package` :

```cmake
find_package(benchmark REQUIRED)

target_link_libraries(mon_projet_benchmarks
    PRIVATE benchmark::benchmark benchmark::benchmark_main)
```

### Structure de projet

Les benchmarks vivent dans un répertoire séparé des tests unitaires — ils ont un cycle de vie et des contraintes différents :

```
mon_projet/
├── src/                  # Code de production
├── include/
├── tests/                # Tests unitaires (GTest)
└── benchmarks/           # Benchmarks (Google Benchmark)
    ├── CMakeLists.txt
    ├── bench_sort.cpp
    └── bench_containers.cpp
```

### Flags de compilation pour les benchmarks

Contrairement aux tests unitaires (compilés en `-O0` avec couverture), les benchmarks doivent être compilés avec les **mêmes optimisations que la production** — sinon on mesure une réalité sans rapport avec le code déployé :

```bash
cmake -B build-bench -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_BENCHMARKS=ON
```

Un benchmark compilé en `-O0` mesure le surcoût de l'absence d'optimisation, pas la performance réelle du code. C'est une erreur fréquente qui invalide complètement les résultats.

## Anatomie d'un benchmark

### Le benchmark le plus simple

```cpp
#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>

static void BM_VectorSort(benchmark::State& state) {
    std::vector<int> data = {5, 3, 8, 1, 9, 2, 7, 4, 6, 0};

    for (auto _ : state) {
        auto copy = data;
        std::sort(copy.begin(), copy.end());
    }
}

BENCHMARK(BM_VectorSort);
```

Décomposons chaque élément.

**`benchmark::State& state`** — L'objet `state` contrôle le nombre d'itérations du benchmark. Google Benchmark détermine automatiquement combien d'itérations sont nécessaires pour obtenir une mesure stable — typiquement des milliers à des millions pour les opérations rapides.

**`for (auto _ : state)`** — Cette boucle est le cœur de la mesure. Le framework chronomètre uniquement les itérations de cette boucle. Tout ce qui est avant la boucle (construction de `data`) est du setup, exécuté une seule fois et non mesuré.

**`BENCHMARK(BM_VectorSort)`** — Enregistre la fonction comme benchmark. Par convention, les fonctions benchmark sont préfixées par `BM_`.

### Exécution et sortie

```bash
./mon_projet_benchmarks
```

La sortie console est un tableau structuré :

```
-------------------------------------------------------------
Benchmark                   Time             CPU   Iterations
-------------------------------------------------------------
BM_VectorSort            54.3 ns         54.2 ns     12845672
```

Trois colonnes de résultats :

- **Time** — Temps réel (wall clock) moyen par itération. Inclut le temps passé en attente (I/O, scheduling).
- **CPU** — Temps CPU moyen par itération. Exclut le temps d'attente. Pour du code purement calculatoire, Time et CPU sont quasi identiques. Un écart significatif indique des attentes I/O ou des context switches.
- **Iterations** — Nombre d'itérations exécutées. Déterminé automatiquement par le framework. Un nombre élevé (millions) indique une opération rapide ; un nombre faible (dizaines) indique une opération lente.

## Empêcher l'optimisation : DoNotOptimize et ClobberMemory

Comme présenté dans l'introduction du chapitre, le compilateur peut éliminer le code dont le résultat n'est pas consommé. Google Benchmark fournit deux outils pour empêcher cela sans introduire de surcoût parasite.

### benchmark::DoNotOptimize

`DoNotOptimize` force le compilateur à considérer qu'une valeur est utilisée, même si elle ne l'est pas :

```cpp
static void BM_StringCreation(benchmark::State& state) {
    for (auto _ : state) {
        std::string s("Hello, World!");
        benchmark::DoNotOptimize(s);
    }
}
```

Sans `DoNotOptimize(s)`, le compilateur pourrait constater que `s` n'est jamais lu et éliminer entièrement sa construction. Avec `DoNotOptimize`, le compilateur est contraint de maintenir la construction de `s` mais n'ajoute aucune instruction supplémentaire à l'exécution — c'est une barrière purement logique pour l'optimiseur.

La règle pratique : appliquez `DoNotOptimize` sur le **résultat** de l'opération mesurée — la valeur de retour d'une fonction, l'objet construit, le pointeur obtenu.

### benchmark::ClobberMemory

`ClobberMemory` force le compilateur à considérer que toute la mémoire accessible pourrait avoir été modifiée. C'est une barrière plus large que `DoNotOptimize`, utilisée quand l'opération mesurée modifie un état en mémoire plutôt que de produire une valeur de retour :

```cpp
static void BM_VectorPushBack(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> v;
        v.reserve(100);
        for (int i = 0; i < 100; ++i) {
            v.push_back(i);
        }
        benchmark::ClobberMemory();
    }
}
```

`ClobberMemory()` empêche le compilateur de constater que le vecteur `v` n'est jamais lu après le remplissage et d'éliminer la boucle. En pratique, `DoNotOptimize` sur la variable suffit souvent — `ClobberMemory` est nécessaire quand la modification porte sur des structures de données complexes ou des pointeurs indirects.

### Combiner les deux

Pour les cas complexes, les deux outils se combinent :

```cpp
static void BM_MapInsert(benchmark::State& state) {
    for (auto _ : state) {
        std::map<int, int> m;
        for (int i = 0; i < 1000; ++i) {
            m[i] = i * 2;
        }
        benchmark::DoNotOptimize(m.size());
        benchmark::ClobberMemory();
    }
}
```

## Séparer le setup de la mesure

Tout le code avant la boucle `for (auto _ : state)` est exécuté avant la mesure. Mais parfois le setup doit être répété à chaque itération (recréer des données fraîches) tout en étant exclu du chronométrage. Les méthodes `PauseTiming()` et `ResumeTiming()` permettent cela :

```cpp
static void BM_Sort1000Elements(benchmark::State& state) {
    for (auto _ : state) {
        // Pause le chronomètre pendant le setup
        state.PauseTiming();
        std::vector<int> data(1000);
        std::iota(data.begin(), data.end(), 0);
        std::shuffle(data.begin(), data.end(), 
                     std::mt19937{std::random_device{}()});
        state.ResumeTiming();

        // Seul le tri est mesuré
        std::sort(data.begin(), data.end());
        benchmark::DoNotOptimize(data.data());
    }
}
```

> ⚠️ **Utilisez `PauseTiming/ResumeTiming` avec parcimonie.** Chaque appel a un coût propre (lecture de l'horloge système). Pour des opérations très courtes (quelques nanosecondes), ce surcoût peut dominer la mesure. Préférez préparer les données avant la boucle quand c'est possible, ou utiliser une fixture (voir plus bas).

## Benchmarks paramétrés

Comme les tests paramétrés de GTest (section 33.2.3), Google Benchmark permet d'exécuter un même benchmark avec différentes valeurs de paramètres. C'est essentiel pour comprendre comment la performance évolue avec la taille des données.

### Avec Arg

```cpp
static void BM_VectorSort(benchmark::State& state) {
    const int n = state.range(0);  // Récupère le paramètre

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> data(n);
        std::iota(data.begin(), data.end(), 0);
        std::ranges::reverse(data);
        state.ResumeTiming();

        std::sort(data.begin(), data.end());
        benchmark::DoNotOptimize(data.data());
    }

    // Métrique personnalisée : éléments traités par seconde
    state.SetItemsProcessed(state.iterations() * n);
}

BENCHMARK(BM_VectorSort)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);
```

La sortie compare les performances pour chaque taille :

```
-------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations
-------------------------------------------------------------------
BM_VectorSort/100              312 ns          311 ns      2247191  
BM_VectorSort/1000            4521 ns         4518 ns       154832  
BM_VectorSort/10000          62.4 us         62.3 us        11234  
BM_VectorSort/100000          843 us          842 us          831  
```

Le suffixe `/100`, `/1000`, etc. identifie le paramètre. On voit immédiatement que le tri croît de manière quasi-linéarithmique (O(n log n) confirmé).

### Avec Range et RangeMultiplier

Pour explorer des tailles par puissances de 2 (typique pour les analyses de complexité) :

```cpp
BENCHMARK(BM_VectorSort)
    ->RangeMultiplier(2)
    ->Range(64, 65536);
```

Génère des benchmarks pour 64, 128, 256, 512, ..., 65536. Le `RangeMultiplier` définit le facteur entre chaque point. Cette progression géométrique est idéale pour visualiser la complexité algorithmique sur un graphe logarithmique.

### Avec DenseRange

Pour explorer une plage linéaire avec un pas fixe :

```cpp
BENCHMARK(BM_CacheEffect)
    ->DenseRange(1, 10, 1);  // 1, 2, 3, ..., 10
```

### Paramètres multiples

Pour les benchmarks avec deux axes de variation (taille des données et nombre de threads, par exemple) :

```cpp
static void BM_MatrixMultiply(benchmark::State& state) {
    const int rows = state.range(0);
    const int cols = state.range(1);

    for (auto _ : state) {
        auto result = mp::matrix_multiply(rows, cols);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_MatrixMultiply)
    ->Args({64, 64})
    ->Args({128, 128})
    ->Args({256, 256})
    ->Args({512, 512});
```

`state.range(0)` et `state.range(1)` récupèrent les paramètres dans l'ordre. La sortie identifie chaque combinaison :

```
BM_MatrixMultiply/64/64         12.4 us  
BM_MatrixMultiply/128/128       98.7 us  
BM_MatrixMultiply/256/256        812 us  
BM_MatrixMultiply/512/512       6.91 ms  
```

## Métriques personnalisées

Au-delà du temps par itération, Google Benchmark permet de rapporter des métriques spécifiques au domaine.

### SetItemsProcessed

Rapporte un débit en éléments par seconde :

```cpp
static void BM_ParseJson(benchmark::State& state) {
    std::string json = generate_json(state.range(0));

    for (auto _ : state) {
        auto result = mp::parse_json(json);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
```

La sortie ajoute une colonne `items_per_second` :

```
BM_ParseJson/1000     4.21 us    ...    237.529M items/s
```

### SetBytesProcessed

Rapporte un débit en octets par seconde — idéal pour les opérations I/O, la compression ou le hashing :

```cpp
static void BM_SHA256(benchmark::State& state) {
    const int size = state.range(0);
    std::vector<uint8_t> data(size, 0x42);

    for (auto _ : state) {
        auto hash = mp::sha256(data);
        benchmark::DoNotOptimize(hash);
    }
    state.SetBytesProcessed(state.iterations() * size);
}

BENCHMARK(BM_SHA256)
    ->RangeMultiplier(4)
    ->Range(256, 1 << 20);  // 256 B → 1 MB
```

La sortie affiche le débit :

```
BM_SHA256/256          142 ns    ...    1.71746G/s  
BM_SHA256/1024         487 ns    ...    2.00429G/s  
BM_SHA256/1048576     412 us    ...    2.42548G/s  
```

### Counters personnalisés

Pour des métriques arbitraires :

```cpp
static void BM_Compress(benchmark::State& state) {
    std::string data = generate_data(state.range(0));
    std::string compressed;

    for (auto _ : state) {
        compressed = mp::compress(data);
        benchmark::DoNotOptimize(compressed);
    }

    state.counters["ratio"] = benchmark::Counter(
        static_cast<double>(data.size()) / compressed.size(),
        benchmark::Counter::kAvgThreads
    );
    state.SetBytesProcessed(state.iterations() * data.size());
}
```

Le compteur `ratio` apparaît comme une colonne supplémentaire dans la sortie.

## Fixtures de benchmark

Comme les fixtures GTest (section 33.2.2), Google Benchmark permet de factoriser le setup dans une classe fixture. La syntaxe diffère légèrement :

```cpp
class DatabaseBenchmark : public benchmark::Fixture {  
public:  
    void SetUp(const benchmark::State& state) override {
        db = mp::Database::open_in_memory();
        db->seed(state.range(0));  // Insérer N enregistrements
    }

    void TearDown(const benchmark::State& /*state*/) override {
        db.reset();
    }

protected:
    std::unique_ptr<mp::Database> db;
};

BENCHMARK_DEFINE_F(DatabaseBenchmark, SelectAll)(benchmark::State& state) {
    for (auto _ : state) {
        auto results = db->select_all();
        benchmark::DoNotOptimize(results.size());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

BENCHMARK_REGISTER_F(DatabaseBenchmark, SelectAll)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

BENCHMARK_DEFINE_F(DatabaseBenchmark, FindById)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = db->find_by_id(42);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_REGISTER_F(DatabaseBenchmark, FindById)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);
```

La mécanique est en trois temps : `BENCHMARK_DEFINE_F` déclare le benchmark, `BENCHMARK_REGISTER_F` l'enregistre avec ses paramètres. Le `SetUp` est appelé avant chaque configuration de paramètres (pas avant chaque itération), ce qui amortit le coût de l'initialisation.

## Comparer des implémentations

L'un des usages les plus courants de Google Benchmark est la comparaison directe de plusieurs approches pour le même problème. La convention est de nommer les benchmarks de manière parallèle :

```cpp
static void BM_SearchLinear(benchmark::State& state) {
    const int n = state.range(0);
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 0);
    int target = n - 1;  // Pire cas

    for (auto _ : state) {
        auto it = std::find(data.begin(), data.end(), target);
        benchmark::DoNotOptimize(it);
    }
}

static void BM_SearchBinary(benchmark::State& state) {
    const int n = state.range(0);
    std::vector<int> data(n);
    std::iota(data.begin(), data.end(), 0);
    int target = n - 1;

    for (auto _ : state) {
        bool found = std::binary_search(data.begin(), data.end(), target);
        benchmark::DoNotOptimize(found);
    }
}

static void BM_SearchUnorderedSet(benchmark::State& state) {
    const int n = state.range(0);
    std::unordered_set<int> data;
    for (int i = 0; i < n; ++i) data.insert(i);
    int target = n - 1;

    for (auto _ : state) {
        auto it = data.find(target);
        benchmark::DoNotOptimize(it);
    }
}

BENCHMARK(BM_SearchLinear)->RangeMultiplier(10)->Range(10, 1'000'000);  
BENCHMARK(BM_SearchBinary)->RangeMultiplier(10)->Range(10, 1'000'000);  
BENCHMARK(BM_SearchUnorderedSet)->RangeMultiplier(10)->Range(10, 1'000'000);  
```

La sortie met en évidence les caractéristiques de chaque approche :

```
BM_SearchLinear/10               5.23 ns  
BM_SearchLinear/100              41.7 ns  
BM_SearchLinear/1000              389 ns  
BM_SearchLinear/1000000        389412 ns  

BM_SearchBinary/10               6.12 ns  
BM_SearchBinary/100              12.4 ns  
BM_SearchBinary/1000             18.7 ns  
BM_SearchBinary/1000000          31.2 ns  

BM_SearchUnorderedSet/10         8.91 ns  
BM_SearchUnorderedSet/100        9.24 ns  
BM_SearchUnorderedSet/1000       9.31 ns  
BM_SearchUnorderedSet/1000000    12.8 ns  
```

Les résultats confirment les complexités théoriques — linéaire pour `find`, logarithmique pour `binary_search`, quasi-constante pour `unordered_set::find` — et quantifient les constantes cachées : `unordered_set` est plus lent que `binary_search` pour N=10 à cause du surcoût du hashing, mais domine largement pour les grandes tailles.

## Benchmarks multi-threads

Google Benchmark supporte le benchmarking de code concurrent via l'option `Threads` :

```cpp
static void BM_MutexContention(benchmark::State& state) {
    static std::mutex mtx;
    static int shared_counter = 0;

    for (auto _ : state) {
        std::lock_guard lock(mtx);
        shared_counter++;
        benchmark::DoNotOptimize(shared_counter);
    }
}

BENCHMARK(BM_MutexContention)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8);
```

Le framework crée le nombre de threads spécifié et exécute le benchmark en parallèle. La sortie montre comment la contention dégrade le débit :

```
BM_MutexContention/threads:1     18.3 ns  
BM_MutexContention/threads:2     85.4 ns  
BM_MutexContention/threads:4      231 ns  
BM_MutexContention/threads:8      587 ns  
```

L'augmentation non linéaire du temps par opération révèle le coût de la contention sur le mutex — exactement le type d'information nécessaire pour décider entre un mutex global et des approches lock-free (section 42.4) ou des structures par thread.

## Options de ligne de commande

L'exécutable de benchmark accepte de nombreuses options qui s'avèrent précieuses au quotidien.

### Filtrage

```bash
# Exécuter uniquement les benchmarks de tri
./mon_projet_benchmarks --benchmark_filter="Sort"

# Exclure un benchmark spécifique (regex)
./mon_projet_benchmarks --benchmark_filter="^(?!.*Slow)"
```

### Répétitions et statistiques

```bash
# 5 répétitions avec statistiques (mean, median, stddev)
./mon_projet_benchmarks --benchmark_repetitions=5 \
                         --benchmark_report_aggregates_only=true
```

Avec les répétitions, la sortie inclut des lignes supplémentaires :

```
BM_VectorSort/1000_mean        4518 ns  
BM_VectorSort/1000_median      4502 ns  
BM_VectorSort/1000_stddev       127 ns  
BM_VectorSort/1000_cv          2.81 %  
```

Le coefficient de variation (`cv`) est particulièrement utile : au-dessus de 5%, les résultats sont trop bruités pour être fiables. La section 35.2 détaille comment réduire ce bruit.

### Format de sortie

```bash
# Sortie JSON (pour analyse automatisée)
./mon_projet_benchmarks --benchmark_format=json \
                         --benchmark_out=results.json

# Sortie CSV
./mon_projet_benchmarks --benchmark_format=csv \
                         --benchmark_out=results.csv

# Sortie console + JSON simultanément
./mon_projet_benchmarks --benchmark_out=results.json \
                         --benchmark_out_format=json
```

Le format JSON est le plus riche — il contient les métadonnées de l'environnement (CPU, OS, compilateur) en plus des résultats, ce qui facilite la comparaison entre machines.

### Unité de temps

```bash
# Forcer l'affichage en microsecondes
./mon_projet_benchmarks --benchmark_time_unit=us
```

Par défaut, Google Benchmark choisit automatiquement l'unité la plus lisible (ns, us, ms, s). Le forcer peut être utile pour aligner la sortie de benchmarks de durées très différentes.

## Comparaison entre deux runs

Google Benchmark fournit un outil `compare.py` (dans le répertoire `tools/` du dépôt) qui compare deux fichiers JSON et calcule le changement relatif :

```bash
# Exécuter le benchmark avant modification
./mon_projet_benchmarks --benchmark_out=before.json \
                         --benchmark_out_format=json

# ... modifier le code ...

# Exécuter le benchmark après modification
./mon_projet_benchmarks --benchmark_out=after.json \
                         --benchmark_out_format=json

# Comparer
python3 tools/compare.py benchmarks before.json after.json
```

La sortie montre le changement relatif pour chaque benchmark :

```
Benchmark                        Time       CPU
----------------------------------------------
BM_VectorSort/1000            -0.1523   -0.1518    (amélioration 15%)  
BM_VectorSort/10000           -0.2104   -0.2098    (amélioration 21%)  
BM_SearchLinear/1000          +0.0034   +0.0031    (stable)  
```

Un changement négatif indique une amélioration (le code est plus rapide), un changement positif indique une régression. Les valeurs proches de zéro (< 2-3%) sont généralement du bruit statistique et ne doivent pas être interprétées comme des changements réels — la section 35.3 détaille les seuils d'interprétation.

## Pièges courants

### Mesurer le setup au lieu du code

```cpp
// ❌ La création du vecteur est mesurée avec le tri
static void BM_SortBad(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> data(10000);
        std::iota(data.begin(), data.end(), 0);
        std::ranges::reverse(data);
        std::sort(data.begin(), data.end());
    }
}

// ✅ Seul le tri est mesuré
static void BM_SortGood(benchmark::State& state) {
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);
    std::ranges::reverse(data);

    for (auto _ : state) {
        auto copy = data;
        std::sort(copy.begin(), copy.end());
    }
}
```

Dans la version corrigée, le vecteur initial est préparé avant la boucle. Chaque itération copie le vecteur (coût mesurable mais amortissable si la copie est rapide par rapport au tri) puis le trie. Si le coût de la copie est un problème, utilisez `PauseTiming/ResumeTiming`.

### Compiler en Debug

Un benchmark compilé avec `-O0` (mode Debug) mesure le surcoût des appels de fonctions non inlinés, des variables non optimisées en registres et des boucles non déroulées. Les résultats n'ont aucun rapport avec les performances en production. Compilez **toujours** les benchmarks en Release (`-O2` ou `-O3`).

### Ignorer le warmup du cache

La première itération d'un benchmark est souvent plus lente que les suivantes — le code et les données ne sont pas encore dans le cache CPU. Google Benchmark gère cela automatiquement en effectuant un warmup avant la mesure. Cependant, si vous utilisez `PauseTiming/ResumeTiming` pour recréer des données à chaque itération, le warmup du cache de données est annulé à chaque fois, ce qui peut fausser les résultats vers le haut.

### Benchmarker des opérations trop courtes

Une opération de 1 nanoseconde est à la limite de résolution des horloges système. Google Benchmark gère les opérations courtes en regroupant automatiquement les itérations, mais les résultats sous 1-2 nanosecondes doivent être interprétés avec prudence. Pour les opérations ultra-courtes, envisagez de mesurer un lot (100 ou 1000 opérations) et de diviser le résultat.

### Oublier DoNotOptimize

Sans `DoNotOptimize`, le compilateur peut éliminer l'opération mesurée. Un benchmark qui affiche 0 nanosecondes ou un temps anormalement bas est presque toujours victime de cette optimisation. En cas de doute, inspectez l'assembleur généré (`-S` ou via Compiler Explorer, section 48.4.4) pour vérifier que l'opération est bien présente.

---


⏭️ [Mesure de performance fiable](/35-benchmarking/02-mesure-fiable.md)
