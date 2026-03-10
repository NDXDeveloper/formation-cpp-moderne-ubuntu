/* ============================================================================
   Section 8.5 : Opérateur spaceship <=> (C++20)
   Description : Comparaison tripartite de base — résultat à trois valeurs
                 (less, equal, greater) et catégories de comparaison
   Fichier source : 05-operateur-spaceship.md
   ============================================================================ */
#include <compare>
#include <print>
#include <limits>

int main() {
    // Comparaison tripartite de base
    int a = 3, b = 5;
    auto resultat = a <=> b;

    if (resultat < 0)  std::println("a < b");     // ✅ — ce cas
    if (resultat == 0) std::println("a == b");
    if (resultat > 0)  std::println("a > b");

    // Les entiers ont un strong_ordering
    auto r = 42 <=> 17;   // std::strong_ordering::greater
    static_assert(std::is_same_v<decltype(r), std::strong_ordering>);
    std::println("42 <=> 17 : greater = {}", r > 0);   // true

    // Les flottants ont un partial_ordering à cause de NaN
    double x = 1.0, nan = std::numeric_limits<double>::quiet_NaN();
    auto r2 = x <=> nan;   // std::partial_ordering::unordered
    std::println("1.0 <=> NaN : unordered = {}", r2 != std::partial_ordering::less
        && r2 != std::partial_ordering::equivalent
        && r2 != std::partial_ordering::greater);  // true
}
