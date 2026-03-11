/* ============================================================================
   Section 13.5 : Le problème que std::span résout
   Description : Avant C++20 — trois versions de la même fonction pour
                 accepter différentes sources de données
   Fichier source : 05-span.md
   ============================================================================ */
#include <vector>
#include <array>
#include <numeric>
#include <print>

// Version 1 : pointeur + taille (API C classique)
int somme_c(const int* data, std::size_t taille) {
    int total = 0;
    for (std::size_t i = 0; i < taille; ++i) {
        total += data[i];
    }
    return total;
}

// Version 2 : std::vector
int somme_vec(const std::vector<int>& v) {
    return std::accumulate(v.begin(), v.end(), 0);
}

// Version 3 : template pour tout conteneur (verbeux)
template <typename Container>
int somme_generic(const Container& c) {
    return std::accumulate(c.begin(), c.end(), 0);
}

int main() {
    int tab[] = {1, 2, 3, 4, 5};
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::array<int, 5> arr{1, 2, 3, 4, 5};

    std::println("C     : {}", somme_c(tab, 5));
    std::println("vector: {}", somme_vec(vec));
    std::println("array : {}", somme_generic(arr));
}
