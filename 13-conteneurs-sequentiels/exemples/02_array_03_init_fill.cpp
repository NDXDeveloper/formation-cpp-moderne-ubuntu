/* ============================================================================
   Section 13.2 : Initialisation et fill
   Description : Subtilités de l'initialisation par agrégat (partielle, vide,
                 sans accolades) et remplissage avec fill()
   Fichier source : 02-array.md
   ============================================================================ */
#include <array>
#include <print>

int main() {
    // Initialisation par liste
    [[maybe_unused]] std::array<int, 4> a{1, 2, 3, 4};

    // Initialisation partielle — les restants sont value-initialized (0)
    [[maybe_unused]] std::array<int, 4> b{1, 2};
    // b = {1, 2, 0, 0}

    // Initialisation vide — tous à 0
    std::array<int, 4> c{};
    // c = {0, 0, 0, 0}

    std::println("c[0]={}", c[0]);  // 0 (garanti)

    // fill : remplit tout le tableau avec une valeur
    std::array<int, 5> d;
    d.fill(42);
    for (auto val : d) std::print("{} ", val);
    std::println("");
    // Sortie : 42 42 42 42 42
}
