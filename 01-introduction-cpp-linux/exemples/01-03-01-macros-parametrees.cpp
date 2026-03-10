/* ============================================================================
   Section 1.3.1 : Le préprocesseur — #include, #define, macros
   Description : Macros paramétrées — pièges du parenthésage et de
                 l'évaluation multiple. Comparaison avec constexpr.
   Fichier source : 03.1-preprocesseur.md
   Compilation : g++ -std=c++23 -o 01-03-01-macros-parametrees 01-03-01-macros-parametrees.cpp
   Sortie attendue :
     === Macro bien parenthésée ===
     CARRE(5) = 25
     MIN(3, 7) = 3
     === Macro MAL parenthésée (piège !) ===
     CARRE_MAUVAIS(3 + 1) = 7  (attendu 16, obtenu 7 !)
     === constexpr (sûr) ===
     carre(5) = 25
     carre(3 + 1) = 16  (correct)
   ============================================================================ */
#include <iostream>

// Macros paramétrées
#define CARRE(x) ((x) * (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Macro MAL parenthésée
#define CARRE_MAUVAIS(x) x * x

// Alternative constexpr C++ moderne
constexpr auto carre(auto x) { return x * x; }

int main() {
    std::cout << "=== Macro bien parenthésée ===" << std::endl;
    std::cout << "CARRE(5) = " << CARRE(5) << std::endl;
    std::cout << "MIN(3, 7) = " << MIN(3, 7) << std::endl;

    std::cout << "=== Macro MAL parenthésée (piège !) ===" << std::endl;
    int r = CARRE_MAUVAIS(3 + 1);
    // Expansion : 3 + 1 * 3 + 1 = 3 + 3 + 1 = 7 (pas 16 !)
    std::cout << "CARRE_MAUVAIS(3 + 1) = " << r
              << "  (attendu 16, obtenu " << r << " !)" << std::endl;

    std::cout << "=== constexpr (sûr) ===" << std::endl;
    std::cout << "carre(5) = " << carre(5) << std::endl;
    std::cout << "carre(3 + 1) = " << carre(3 + 1)
              << "  (correct)" << std::endl;

    return 0;
}
