/* ============================================================================
   Section 2.6.4 : Standard (-std=c++20/23)
   Description : Démo des ranges C++20 — filtre sur un vector
   Fichier source : 06.4-standard.md
   Compilation : g++ -std=c++20 -Wall -Wextra ranges_demo.cpp -o ranges_demo
   ============================================================================ */
#include <vector>
#include <algorithm>
#include <ranges>
#include <iostream>

int main() {
    std::vector<int> v = {5, 3, 1, 4, 2};
    auto pairs = v | std::views::filter([](int n){ return n % 2 == 0; });

    std::cout << "Nombres pairs : ";
    for (int n : pairs) {
        std::cout << n << " ";
    }
    std::cout << std::endl;

    return 0;
}
