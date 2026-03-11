/* ============================================================================
   Section 13.1.2 : Itérateurs
   Description : Itérateurs de base, inverses et arithmétique sur les
                 itérateurs à accès aléatoire de std::vector
   Fichier source : 01.2-methodes-essentielles.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    // Itérateurs de base
    {
        std::vector<int> v{10, 20, 30, 40, 50};
        for (auto it = v.begin(); it != v.end(); ++it) {
            *it *= 2;
        }
        for (auto it = v.cbegin(); it != v.cend(); ++it) {
            std::print("{} ", *it);
        }
        std::println("");
        // Sortie : 20 40 60 80 100
    }

    // Itérateurs inverses
    {
        std::vector<int> v{10, 20, 30, 40, 50};
        for (auto it = v.rbegin(); it != v.rend(); ++it) {
            std::print("{} ", *it);
        }
        std::println("");
        // Sortie : 50 40 30 20 10

        for (auto it = v.crbegin(); it != v.crend(); ++it) {
            std::print("{} ", *it);
        }
        std::println("");
        // Sortie : 50 40 30 20 10
    }

    // Arithmétique sur itérateurs
    {
        std::vector<int> v{10, 20, 30, 40, 50};
        auto it = v.begin();

        it += 3;
        std::println("begin()+3 → {}", *it);  // 40

        it -= 2;
        std::println("puis -2  → {}", *it);   // 20

        std::println("begin()[4] → {}", v.begin()[4]);  // 50

        auto dist = v.end() - v.begin();
        std::println("distance = {}", dist);   // 5

        std::println("begin < end ? {}", v.begin() < v.end());  // true
    }
}
