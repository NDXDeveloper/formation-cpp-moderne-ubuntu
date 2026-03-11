/* ============================================================================
   Section 13.2 : constexpr et std::array
   Description : Utilisation de std::array dans des contextes constexpr pour
                 des calculs à la compilation (table de carrés, static_assert)
   Fichier source : 02-array.md
   ============================================================================ */
#include <array>
#include <algorithm>
#include <print>

constexpr std::array<int, 5> creer_carre() {
    std::array<int, 5> a{};
    for (int i = 0; i < 5; ++i) {
        a[i] = (i + 1) * (i + 1);
    }
    return a;
}

constexpr auto carres = creer_carre();
// carres = {1, 4, 9, 16, 25} — calculé à la COMPILATION

// Recherche à la compilation (C++20 : constexpr std::find)
constexpr bool contient_16 = std::find(carres.begin(), carres.end(), 16) != carres.end();

int main() {
    static_assert(contient_16, "16 devrait être dans la table des carrés");

    for (auto val : carres) std::print("{} ", val);
    std::println("");
    // Sortie : 1 4 9 16 25
}
