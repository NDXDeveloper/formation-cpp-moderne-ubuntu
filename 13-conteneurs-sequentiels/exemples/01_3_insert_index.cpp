/* ============================================================================
   Section 13.1.3 : Insertion pendant un parcours
   Description : Insertion sûre par index pendant un parcours, évitant
                 l'invalidation des itérateurs
   Fichier source : 01.3-invalidation-iterateurs.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v{1, 2, 3};

    // ✅ CORRECT : travailler par index
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i] == 2) {
            v.insert(v.begin() + static_cast<std::ptrdiff_t>(i), 99);
            ++i;  // sauter l'élément inséré pour éviter une boucle infinie
        }
    }

    for (auto val : v) std::print("{} ", val);
    // Sortie : 1 99 2 3
}
