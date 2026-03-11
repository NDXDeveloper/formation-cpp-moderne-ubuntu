/* ============================================================================
   Section 13.1 : Parcours
   Description : Trois façons de parcourir un std::vector (range-based for,
                 par index, par itérateurs)
   Fichier source : 01-vector.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30, 40, 50};

    // Range-based for loop (C++11) — méthode préférée
    for (const auto& val : v) {
        std::print("{} ", val);
    }
    std::println("");  // 10 20 30 40 50

    // Par index — quand l'index est nécessaire
    for (std::size_t i = 0; i < v.size(); ++i) {
        std::print("[{}]={} ", i, v[i]);
    }
    std::println("");  // [0]=10 [1]=20 [2]=30 [3]=40 [4]=50

    // Par itérateurs — utile pour les algorithmes
    for (auto it = v.cbegin(); it != v.cend(); ++it) {
        std::print("{} ", *it);
    }
    std::println("");  // 10 20 30 40 50
}
