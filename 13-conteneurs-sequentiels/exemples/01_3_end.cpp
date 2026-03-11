/* ============================================================================
   Section 13.1.3 : end() invalide après modification
   Description : Démonstration du piège de stocker end() avant une boucle
                 qui modifie la taille du vecteur — solution par index
   Fichier source : 01.3-invalidation-iterateurs.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v{1, 2, 3, 4, 5};
    v.reserve(100);  // pas de réallocation possible

    // ✅ CORRECT : recalculer end() ou travailler avec la taille initiale
    auto taille_initiale = v.size();
    for (std::size_t i = 0; i < taille_initiale; ++i) {
        v.push_back(v[i] * 10);
    }

    for (auto val : v) std::print("{} ", val);
    // Sortie : 1 2 3 4 5 10 20 30 40 50
}
