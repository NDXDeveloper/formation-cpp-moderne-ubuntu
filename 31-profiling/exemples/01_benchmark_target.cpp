/* ============================================================================
   Section 31.1 / 31.2 / 31.3 : perf, gprof, flamegraphs
   Description : Programme cible pour le profiling — contient trois hotspots
                 identifiables (tri, copie de strings, somme) pour tester
                 perf record/report, perf stat, gprof, et les flamegraphs
   Fichier source : 01-perf.md, 02-gprof.md, 03-flamegraphs.md
   ============================================================================ */

#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <cstdio>
#include <string>

// Hotspot 1 : tri intensif (CPU-bound)
void tri_intensif(std::vector<int>& data) {
    std::sort(data.begin(), data.end());
}

// Hotspot 2 : copie de strings (allocation-heavy, cache-unfriendly)
void copie_strings(int n) {
    std::vector<std::string> v;
    for (int i = 0; i < n; ++i) {
        v.push_back(std::string(100, 'x') + std::to_string(i));
    }
}

// Fonction légère (devrait être négligeable dans le profil)
int somme(const std::vector<int>& data) {
    return std::accumulate(data.begin(), data.end(), 0);
}

int main() {
    constexpr int N = 2'000'000;
    std::mt19937 rng(42);

    std::vector<int> data(N);
    std::generate(data.begin(), data.end(), rng);

    // Phase 1 : tri
    tri_intensif(data);

    // Phase 2 : copie de strings
    copie_strings(200'000);

    // Phase 3 : somme
    int s = somme(data);
    std::printf("Somme: %d, Premier: %d, Dernier: %d\n",
                s, data[0], data[N-1]);
    return 0;
}
