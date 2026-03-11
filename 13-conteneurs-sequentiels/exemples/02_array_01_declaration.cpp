/* ============================================================================
   Section 13.2 : Déclaration et inclusion
   Description : Déclaration de std::array avec type/taille explicites,
                 CTAD (C++17) et initialisation uniforme
   Fichier source : 02-array.md
   ============================================================================ */
#include <array>
#include <print>

int main() {
    // Déclaration avec type et taille explicites
    std::array<int, 5> a{10, 20, 30, 40, 50};

    // Déduction de type et de taille (CTAD, C++17)
    std::array b{1.0, 2.0, 3.0};  // déduit std::array<double, 3>

    // Remplissage uniforme
    [[maybe_unused]] std::array<int, 4> zeros{};    // {0, 0, 0, 0} — value-initialized

    std::println("a : size={}, b : size={}", a.size(), b.size());
    // Sortie : a : size=5, b : size=3
}
