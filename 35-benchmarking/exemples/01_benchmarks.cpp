/* ============================================================================
   Section 35.1 : Google Benchmark — Micro-benchmarking
   Description : Exemples de benchmarks illustrant les concepts clés :
                 benchmark simple, DoNotOptimize, ClobberMemory,
                 benchmarks paramétrés avec Arg, et SetItemsProcessed
   Fichier source : 01-google-benchmark.md
   ============================================================================ */

#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <map>

// === Benchmark le plus simple ===
static void BM_VectorSort(benchmark::State& state) {
    std::vector<int> data = {5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
    for (auto _ : state) {
        auto copy = data;
        std::sort(copy.begin(), copy.end());
    }
}
BENCHMARK(BM_VectorSort);

// === DoNotOptimize ===
static void BM_StringCreation(benchmark::State& state) {
    for (auto _ : state) {
        std::string s("Hello, World!");
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_StringCreation);

// === ClobberMemory ===
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
BENCHMARK(BM_VectorPushBack);

// === Benchmarks paramétrés ===
static void BM_VectorSortParam(benchmark::State& state) {
    const int n = state.range(0);
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<int> data(n);
        std::iota(data.begin(), data.end(), 0);
        std::reverse(data.begin(), data.end());
        state.ResumeTiming();
        std::sort(data.begin(), data.end());
        benchmark::DoNotOptimize(data.data());
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_VectorSortParam)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// === Map insert ===
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
BENCHMARK(BM_MapInsert);
