/* ============================================================================
   Section 16.1 : Templates de fonctions — NTTP avec auto (C++17)
   Description : Paramètre non-type avec auto, déduit automatiquement
   Fichier source : 01-templates-fonctions.md
   ============================================================================ */
#include <print>
#include <typeinfo>
#include <array>

template <auto N>
void afficher_valeur() {
    std::print("Valeur : {}, type : {}\n", N, typeid(N).name());
}

int main() {
    afficher_valeur<42>();     // N est int
    afficher_valeur<'A'>();    // N est char
    afficher_valeur<true>();   // N est bool

    // std::array utilise un NTTP
    std::array<int, 5> tableau;
    std::print("array size = {}\n", tableau.size());   // 5
}
