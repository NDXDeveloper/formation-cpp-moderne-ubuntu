/* ============================================================================
   Section 45.1 : Buffer overflows et protection
   Description : Acces securise avec .at() et std::span (C++20)
   Fichier source : 01-buffer-overflows.md
   ============================================================================ */

#include <vector>
#include <span>
#include <stdexcept>
#include <print>

void safe_access() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // .at() verifie les bornes et leve std::out_of_range
    try {
        int b = v.at(10);
    } catch (const std::out_of_range& e) {
        std::print(stderr, "Acces hors bornes detecte : {}\n", e.what());
    }
}

void process_modern(std::span<const int> data) {
    for (int value : data) {
        std::print("{} ", value);
    }
    std::print("\n");
}

int main() {
    safe_access();

    std::vector<int> v = {1, 2, 3, 4, 5};
    process_modern(v);  // Conversion implicite vector -> span
}
