/* ============================================================================
   Section 13.1.2 : Idiome erase-remove et std::erase_if (C++20)
   Description : Suppression d'éléments par critère — comparaison entre
                 l'idiome pré-C++20 et les fonctions C++20
   Fichier source : 01.2-methodes-essentielles.md
   ============================================================================ */
#include <vector>
#include <algorithm>
#include <print>

int main() {
    // Pré-C++20 : idiome erase-remove
    {
        std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        v.erase(
            std::remove_if(v.begin(), v.end(), [](int n) { return n % 2 == 0; }),
            v.end()
        );
        for (auto val : v) std::print("{} ", val);
        std::println("");
        // Sortie : 1 3 5 7 9
    }

    // C++20 : std::erase_if
    {
        std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::erase_if(v, [](int n) { return n % 2 == 0; });
        for (auto val : v) std::print("{} ", val);
        std::println("");
        // Sortie : 1 3 5 7 9
    }

    // C++20 : std::erase par valeur
    {
        std::vector<int> v{1, 2, 3, 2, 5, 2};
        std::erase(v, 2);
        for (auto val : v) std::print("{} ", val);
        std::println("");
        // Sortie : 1 3 5
    }
}
