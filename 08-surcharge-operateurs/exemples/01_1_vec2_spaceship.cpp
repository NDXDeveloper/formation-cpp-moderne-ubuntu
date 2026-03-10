/* ============================================================================
   Section 8.1 : Surcharge des opérateurs arithmétiques et de comparaison
   Description : Vec2 avec operator== et operator<=> defaultés (C++20) —
                 génération automatique des 6 opérateurs de comparaison
   Fichier source : 01-operateurs-arithmetiques.md
   ============================================================================ */
#include <compare>
#include <iostream>

class Vec2 {
    double x_;
    double y_;

public:
    Vec2(double x, double y) : x_{x}, y_{y} {}

    bool operator==(Vec2 const&) const = default;    // == et != auto-générés
    auto operator<=>(Vec2 const&) const = default;   // <, >, <=, >= auto-générés
};

int main() {
    Vec2 a{1.0, 2.0}, b{1.0, 3.0}, c{1.0, 2.0};
    std::cout << std::boolalpha;
    std::cout << "a == c: " << (a == c) << "\n";   // true
    std::cout << "a != b: " << (a != b) << "\n";   // true
    std::cout << "a < b:  " << (a < b)  << "\n";   // true
    std::cout << "a >= c: " << (a >= c) << "\n";    // true
}
