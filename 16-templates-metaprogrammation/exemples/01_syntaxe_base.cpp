/* ============================================================================
   Section 16.1 : Templates de fonctions — Syntaxe de base et instanciation
   Description : Template maximum avec déduction automatique et instanciation
                 explicite, paramètres template multiples, NTTP
   Fichier source : 01-templates-fonctions.md
   ============================================================================ */
#include <string>
#include <print>

// Syntaxe de base
template <typename T>
T maximum(T a, T b) {
    return (a > b) ? a : b;
}

// Paramètres template multiples
template <typename T, typename U>
auto addition(T a, U b) -> decltype(a + b) {
    return a + b;
}

// Version C++14
template <typename T, typename U>
auto addition2(T a, U b) {
    return a + b;
}

// Paramètres template non-type
template <typename T, int N>
T multiplier(T valeur) {
    return valeur * N;
}

int main() {
    // Déduction automatique
    int a = maximum(3, 7);
    double b = maximum(3.14, 2.71);
    std::string c = maximum(
        std::string("alpha"),
        std::string("beta")
    );
    std::print("maximum(3, 7) = {}\n", a);           // 7
    std::print("maximum(3.14, 2.71) = {}\n", b);     // 3.14
    std::print("maximum(alpha, beta) = {}\n", c);     // beta

    // Instanciation explicite
    auto r1 = maximum<int>(3, 7);
    auto r2 = maximum<long>(3, 7);
    auto r3 = maximum<double>(10, 20);
    std::print("maximum<int>(3,7) = {}\n", r1);
    std::print("maximum<long>(3,7) = {}\n", r2);
    std::print("maximum<double>(10,20) = {}\n", r3);

    // Paramètres multiples
    auto m1 = addition(3, 2.5);
    auto m2 = addition(1.0f, 100L);
    std::print("addition(3, 2.5) = {}\n", m1);       // 5.5
    std::print("addition(1.0f, 100L) = {}\n", m2);   // 101

    // NTTP
    auto n1 = multiplier<int, 3>(10);
    auto n2 = multiplier<double, 5>(2.4);
    std::print("multiplier<int,3>(10) = {}\n", n1);     // 30
    std::print("multiplier<double,5>(2.4) = {}\n", n2); // 12
}
