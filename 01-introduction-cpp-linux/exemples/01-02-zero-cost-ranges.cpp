/* ============================================================================
   Section 1.2 : Pourquoi C++ pour le DevOps et le System Programming
   Description : Démonstration du zero-cost abstraction avec ranges et lambdas
                 Filtre les éléments actifs et extrait leurs valeurs
   Fichier source : 02-pourquoi-cpp-devops.md
   Compilation : g++ -std=c++23 -o 01-02-zero-cost-ranges 01-02-zero-cost-ranges.cpp
   Sortie attendue : 10 30 50
   ============================================================================ */
#include <iostream>
#include <vector>
#include <ranges>

struct Item {
    bool active;
    int value;
};

int main() {
    std::vector<Item> data = {
        {true, 10}, {false, 20}, {true, 30}, {false, 40}, {true, 50}
    };

    auto result = data
        | std::views::filter([](auto& x) { return x.active; })
        | std::views::transform([](auto& x) { return x.value; })
        | std::ranges::to<std::vector>();

    for (auto v : result) {
        std::cout << v << " ";
    }
    std::cout << "\n";

    return 0;
}
