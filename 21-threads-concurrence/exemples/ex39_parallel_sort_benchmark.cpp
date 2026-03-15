/* ============================================================================
   Section 21.8 : Algorithmes parallèles - Benchmark sort séquentiel vs parallèle
   Description : Comparaison des temps de tri avec et sans execution::par
   Fichier source : 08-algorithmes-paralleles.md
   Note : Sans TBB installé, par s'exécute séquentiellement (pas de speedup)
   ============================================================================ */

#include <algorithm>
#include <execution>
#include <vector>
#include <chrono>
#include <print>
#include <ranges>
#include <random>

int main() {
    constexpr int N = 10'000'000;

    std::mt19937 gen(42);
    std::vector<int> data(N);
    std::ranges::generate(data, gen);

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
    std::println("(Sans TBB, le speedup sera ~1.0x)");
}
