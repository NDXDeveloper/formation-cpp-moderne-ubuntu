/* ============================================================================
   Section 13.1 : Suppression d'éléments
   Description : Méthodes de suppression (pop_back, erase, clear) sur un
                 std::vector
   Fichier source : 01-vector.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30, 40, 50};

    // Supprimer le dernier élément
    v.pop_back();                          // {10, 20, 30, 40}

    // Supprimer un élément par itérateur
    v.erase(v.begin() + 1);               // {10, 30, 40}

    // Supprimer une plage
    v.erase(v.begin(), v.begin() + 2);    // {40}

    // Vider complètement
    v.clear();                             // size=0, capacity inchangée

    std::println("size={}, empty={}", v.size(), v.empty());
    // Sortie : size=0, empty=true
}
