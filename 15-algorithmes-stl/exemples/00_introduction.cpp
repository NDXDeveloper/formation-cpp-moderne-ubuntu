/* ============================================================================
   Section 15.0 : Introduction — Algorithmes de la STL
   Description : Exemples d'introduction du chapitre couvrant count_if,
                 transform, find_if, remove-erase idiom, C++20 std::erase,
                 et les conventions classiques vs Ranges.
   Fichier source : README.md
   ============================================================================ */

#include <algorithm>
#include <ranges>
#include <vector>
#include <print>

int main() {
    // === Conventions classiques vs Ranges ===
    {
        std::vector<int> v = {5, 2, 8, 1, 9, 3};

        // Classique : paire d'itérateurs
        std::sort(v.begin(), v.end());
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 2 3 5 8 9

        auto it = std::find(v.begin(), v.end(), 8);
        if (it != v.end()) std::print("find(8) OK\n");

        // C++20 Ranges
        std::vector<int> v2 = {5, 2, 8, 1, 9, 3};
        std::ranges::sort(v2);

        auto it2 = std::ranges::find(v2, 8);
        if (it2 != v2.end()) std::print("ranges::find(8) OK\n");
    }

    // === Foncteurs, lambdas et prédicats ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // Compter les nombres pairs
        auto n = std::count_if(v.begin(), v.end(), [](int x) {
            return x % 2 == 0;
        });
        std::print("Nombres pairs : {}\n", n);
        // n == 5

        // Transformer : mettre au carré
        std::vector<int> squares(v.size());
        std::transform(v.begin(), v.end(), squares.begin(), [](int x) {
            return x * x;
        });
        for (int x : squares) std::print("{} ", x);
        std::println("");
        // 1 4 9 16 25 36 49 64 81 100

        // Trouver le premier élément supérieur à 7
        auto it = std::find_if(v.begin(), v.end(), [](int x) {
            return x > 7;
        });
        std::print("Premier > 7 : {}\n", *it);
        // *it == 8
    }

    // === Remove-erase idiom ===
    {
        std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};
        v.erase(std::remove(v.begin(), v.end(), 2), v.end());
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 3 5 7
    }

    // === C++20 std::erase / std::erase_if ===
    {
        std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};
        std::erase(v, 2);
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 3 5 7

        std::erase_if(v, [](int x) { return x > 5; });
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 3 5
    }
}
