/* ============================================================================
   Section 13.1.3 : Suppression en boucle
   Description : Pattern correct de suppression d'éléments pendant un parcours
                 en utilisant le retour de erase()
   Fichier source : 01.3-invalidation-iterateurs.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};

    // ✅ CORRECT : utiliser le retour de erase
    for (auto it = v.begin(); it != v.end(); /* pas de ++it ici */) {
        if (*it % 2 == 0) {
            it = v.erase(it);  // erase retourne l'itérateur suivant valide
        } else {
            ++it;              // avancer manuellement seulement si pas de suppression
        }
    }

    for (auto val : v) std::print("{} ", val);
    // Sortie : 1 3 5 7
}
