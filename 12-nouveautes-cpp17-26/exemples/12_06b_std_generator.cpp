/* ============================================================================
   Section 12.6 : Coroutines (C++20) - std::generator (C++23)
   Description : Generateur fibonacci utilisant std::generator avec
                 integration pipeline ranges (views::take)
   Fichier source : 06-coroutines.md
   ============================================================================ */
#include <generator>
#include <print>
#include <ranges>

// === fibonacci avec std::generator (lignes 255-262) ===
std::generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}

// === main (lignes 265-271) ===
int main() {
    for (int n : fibonacci() | std::views::take(10)) {
        std::print("{} ", n);
    }
    std::print("\n");
    // Sortie : 0 1 1 2 3 5 8 13 21 34
}
