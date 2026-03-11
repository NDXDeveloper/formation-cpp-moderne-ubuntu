/* ============================================================================
   Section 13.1.1 : Contrôler la capacité avec reserve()
   Description : Démonstration de reserve() pour pré-allouer la capacité
                 et éviter toute réallocation
   Fichier source : 01.1-fonctionnement-interne.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v;
    v.reserve(1'000'000);  // alloue pour 1M éléments, size reste 0

    std::println("size={}, capacity={}", v.size(), v.capacity());
    // Sortie : size=0, capacity=1000000

    for (int i = 0; i < 1'000'000; ++i) {
        v.push_back(i);    // aucune réallocation
    }

    std::println("size={}, capacity={}", v.size(), v.capacity());
    // Sortie : size=1000000, capacity=1000000
}
