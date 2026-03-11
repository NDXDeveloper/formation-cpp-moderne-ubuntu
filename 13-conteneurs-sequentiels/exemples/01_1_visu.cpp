/* ============================================================================
   Section 13.1.1 : Visualiser le comportement interne
   Description : Observation des réallocations, changements d'adresse du bloc
                 interne, et relation taille/capacité lors de push_back
   Fichier source : 01.1-fonctionnement-interne.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v;
    const int* ancien_bloc = nullptr;

    std::println("{:<12} {:<8} {:<10} {:<18} {}",
                 "Opération", "size", "capacity", "data()", "Réalloc?");
    std::println("{:-<65}", "");

    for (int i = 0; i < 20; ++i) {
        v.push_back(i * 10);
        bool realloc = (v.data() != ancien_bloc);
        if (realloc || i < 5 || v.size() == v.capacity()) {
            std::println("push_back({:<3}) {:<8} {:<10} {:<18} {}",
                         i * 10, v.size(), v.capacity(),
                         static_cast<const void*>(v.data()),
                         realloc ? "OUI" : "non");
        }
        ancien_bloc = v.data();
    }

    std::println("{:-<65}", "");
    std::println("\nReserve puis remplissage :");
    std::vector<int> v2;
    v2.reserve(20);
    const int* bloc_fixe = v2.data();
    for (int i = 0; i < 20; ++i) {
        v2.push_back(i);
    }
    std::println("Adresse constante : {} (changée : {})",
                 bloc_fixe == v2.data() ? "oui" : "non",
                 bloc_fixe != v2.data() ? "oui" : "non");
}
