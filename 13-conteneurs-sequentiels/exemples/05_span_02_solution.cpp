/* ============================================================================
   Section 13.5 : Solution avec std::span
   Description : Une seule fonction accepte vector, array, tableau C et
                 sous-vues grâce à std::span — zéro copie, zéro allocation
   Fichier source : 05-span.md
   ============================================================================ */
#include <span>
#include <vector>
#include <array>
#include <numeric>
#include <print>

// UNE seule fonction, accepte tout conteneur contigu
int somme(std::span<const int> donnees) {
    return std::accumulate(donnees.begin(), donnees.end(), 0);
}

int main() {
    int tab[] = {1, 2, 3, 4, 5};
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::array<int, 5> arr{1, 2, 3, 4, 5};

    std::println("C array : {}", somme(tab));
    std::println("vector  : {}", somme(vec));
    std::println("array   : {}", somme(arr));
    std::println("partiel : {}", somme({vec.data() + 1, 3}));
    // Somme de {2, 3, 4} = 9
}
