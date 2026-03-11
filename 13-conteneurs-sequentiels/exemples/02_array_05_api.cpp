/* ============================================================================
   Section 13.2 : API de std::array
   Description : Accès aux éléments ([], at, front, back, data), itérateurs
                 (avant, inverse) et méthodes utilitaires (size, fill, swap)
   Fichier source : 02-array.md
   ============================================================================ */
#include <array>
#include <print>
#include <stdexcept>

int main() {
    // Accès aux éléments
    {
        std::array<int, 5> a{10, 20, 30, 40, 50};

        std::println("a[2] = {}", a[2]);       // 30
        std::println("a.at(2) = {}", a.at(2)); // 30
        try {
            [[maybe_unused]] auto val = a.at(10);
        } catch (const std::out_of_range& e) {
            std::println("Exception : {}", e.what());
        }
        std::println("front={}, back={}", a.front(), a.back());
        // Sortie : front=10, back=50

        int* raw = a.data();
        std::println("data()[3] = {}", raw[3]);  // 40
    }

    // Itérateurs
    {
        std::array<int, 5> a{50, 40, 30, 20, 10};
        for (auto it = a.cbegin(); it != a.cend(); ++it) {
            std::print("{} ", *it);
        }
        std::println("");  // 50 40 30 20 10

        for (auto it = a.crbegin(); it != a.crend(); ++it) {
            std::print("{} ", *it);
        }
        std::println("");  // 10 20 30 40 50
    }

    // Méthodes utilitaires
    {
        std::array<int, 5> a{10, 20, 30, 40, 50};
        std::println("size     = {}", a.size());
        std::println("max_size = {}", a.max_size());
        std::println("empty    = {}", a.empty());

        a.fill(0);
        std::array<int, 5> b{1, 2, 3, 4, 5};
        a.swap(b);

        for (auto val : a) std::print("{} ", val);
        std::println("");
        // Sortie : 1 2 3 4 5
    }
}
