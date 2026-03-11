/* ============================================================================
   Section 16.1 : Templates de fonctions — Surcharge et valeurs par défaut
   Description : Coexistence template/non-template, valeurs par défaut des
                 paramètres template, et récursion de templates
   Fichier source : 01-templates-fonctions.md
   ============================================================================ */
#include <string>
#include <print>
#include <cstring>

// Surcharge non-template pour const char*
const char* maximum_str(const char* a, const char* b) {
    return (std::strcmp(a, b) > 0) ? a : b;
}

// Template générique
template <typename T>
T maximum(T a, T b) {
    return (a > b) ? a : b;
}

// Valeur par défaut pour le type
template <typename T = int>
T zero() {
    return T{};
}

// Valeur par défaut pour un NTTP
template <typename T, int Precision = 2>
void afficher(T valeur) {
    std::print("{:.{}f}\n", static_cast<double>(valeur), Precision);
}

// Récursion de templates
template <typename T, int N>
constexpr T puissance(T base) {
    if constexpr (N == 0) {
        return T{1};
    } else if constexpr (N < 0) {
        return T{1} / puissance<T, -N>(base);
    } else {
        return base * puissance<T, N - 1>(base);
    }
}

int main() {
    // Surcharge
    auto r1 = maximum(3, 7);
    auto r2 = maximum_str("hello", "world");
    std::print("maximum(3, 7) = {}\n", r1);
    std::print("maximum_str(hello, world) = {}\n", r2);

    // Valeurs par défaut
    auto a = zero();
    auto b = zero<double>();
    std::print("zero() = {}\n", a);           // 0
    std::print("zero<double>() = {}\n", b);   // 0

    // Afficher avec Precision
    afficher(3.14159);                  // 3.14
    afficher<double, 4>(3.14159);       // 3.1416

    // Récursion
    constexpr auto p1 = puissance<double, 3>(2.0);
    constexpr auto p2 = puissance<int, 0>(42);
    constexpr auto p3 = puissance<double, -2>(2.0);
    std::print("puissance<double,3>(2.0) = {}\n", p1);   // 8
    std::print("puissance<int,0>(42) = {}\n", p2);        // 1
    std::print("puissance<double,-2>(2.0) = {}\n", p3);   // 0.25
}
