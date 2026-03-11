/* ============================================================================
   Section 13.2 : std::to_array (C++20)
   Description : Conversion d'un tableau C en std::array avec déduction
                 automatique du type et de la taille
   Fichier source : 02-array.md
   ============================================================================ */
#include <array>
#include <print>

int main() {
    // Conversion d'un tableau C en std::array
    int raw[] = {10, 20, 30, 40};
    auto a = std::to_array(raw);
    // a est std::array<int, 4>

    // Fonctionne aussi avec des littéraux
    auto b = std::to_array({1, 2, 3});
    // b est std::array<int, 3>

    // Avec des types move-only
    auto c = std::to_array<std::string>({"hello", "world"});

    std::println("a.size()={}, b.size()={}, c.size()={}", a.size(), b.size(), c.size());
    // Sortie : a.size()=4, b.size()=3, c.size()=2
}
