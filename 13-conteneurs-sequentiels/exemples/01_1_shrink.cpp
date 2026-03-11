/* ============================================================================
   Section 13.1.1 : Réduire la capacité avec shrink_to_fit()
   Description : Démonstration de shrink_to_fit() pour libérer la mémoire
                 excédentaire après clear()
   Fichier source : 01.1-fonctionnement-interne.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v(10'000);

    v.clear();  // size=0, mais la capacité reste à 10000
    std::println("Après clear : size={}, capacity={}", v.size(), v.capacity());
    // Sortie : Après clear : size=0, capacity=10000

    v.shrink_to_fit();  // demande de réduire la capacité
    std::println("Après shrink : size={}, capacity={}", v.size(), v.capacity());
    // Sortie probable : Après shrink : size=0, capacity=0
}
