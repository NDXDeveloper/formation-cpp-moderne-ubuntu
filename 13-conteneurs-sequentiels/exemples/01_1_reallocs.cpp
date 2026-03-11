/* ============================================================================
   Section 13.1.1 : Amortissement
   Description : Compteur de réallocations lors de push_back successifs
                 sur un million d'éléments (croissance géométrique)
   Fichier source : 01.1-fonctionnement-interne.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v;
    int nb_reallocs = 0;
    std::size_t ancienne_cap = 0;

    for (int i = 0; i < 1'000'000; ++i) {
        v.push_back(i);
        if (v.capacity() != ancienne_cap) {
            ++nb_reallocs;
            ancienne_cap = v.capacity();
        }
    }

    std::println("Éléments insérés : 1 000 000");
    std::println("Réallocations    : {}", nb_reallocs);
    std::println("Capacité finale  : {}", v.capacity());
}
