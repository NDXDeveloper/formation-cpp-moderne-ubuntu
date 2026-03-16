/* ============================================================================
   Section 30.2 : Heap profiling avec Massif
   Description : Programme avec consommation mémoire croissante pour
                 démontrer le profiling avec Massif
   Fichier source : 02-massif.md
   Compilation : g++-15 -std=c++23 -g -O0 -o massif_demo 05_massif_demo.cpp
   Exécution  : valgrind --tool=massif ./massif_demo
                ms_print massif.out.*
   ============================================================================ */

#include <vector>
#include <string>
#include <cstdio>

// Simule un cache qui grandit sans borne
std::vector<std::string> cache;

void inserer_cache(int id) {
    // Chaque entrée fait ~100 octets
    cache.push_back(std::string(100, 'x') + std::to_string(id));
}

void traiter_requetes(int n) {
    for (int i = 0; i < n; ++i) {
        inserer_cache(i);
    }
}

int main() {
    std::printf("Phase 1 : 1000 insertions\n");
    traiter_requetes(1000);

    std::printf("Phase 2 : 2000 insertions supplémentaires\n");
    traiter_requetes(2000);

    std::printf("Cache final : %zu entrées\n", cache.size());
    return 0;
}
