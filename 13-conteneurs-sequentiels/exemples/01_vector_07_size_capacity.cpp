/* ============================================================================
   Section 13.1 : Size vs Capacity
   Description : Démonstration de la distinction entre size() et capacity()
                 lors de push_back successifs (doublement de la capacité)
   Fichier source : 01-vector.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v;

    std::println("Départ : size={}, capacity={}", v.size(), v.capacity());

    for (int i = 0; i < 10; ++i) {
        v.push_back(i);
        std::println("Après push_back({}) : size={}, capacity={}",
                      i, v.size(), v.capacity());
    }
}
