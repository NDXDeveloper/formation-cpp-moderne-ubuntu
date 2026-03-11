/* ============================================================================
   Section 13.1.2 : Comparaison de vecteurs
   Description : Comparaison lexicographique entre vecteurs (==, <) et
                 opérateur spaceship <=> (C++20)
   Fichier source : 01.2-methodes-essentielles.md
   ============================================================================ */
#include <vector>
#include <compare>
#include <print>

int main() {
    // Comparaisons classiques
    {
        std::vector<int> a{1, 2, 3};
        std::vector<int> b{1, 2, 3};
        std::vector<int> c{1, 2, 4};
        std::vector<int> d{1, 2};

        std::println("a == b : {}", a == b);  // true
        std::println("a == c : {}", a == c);  // false
        std::println("a < c  : {}", a < c);   // true
        std::println("a < d  : {}", a < d);   // false
        std::println("d < a  : {}", d < a);   // true
    }

    // Spaceship operator (C++20)
    {
        std::vector<int> a{1, 2, 3};
        std::vector<int> b{1, 2, 4};

        auto result = a <=> b;

        if (result < 0)      std::println("a < b");
        else if (result > 0) std::println("a > b");
        else                  std::println("a == b");
        // Sortie : a < b
    }
}
