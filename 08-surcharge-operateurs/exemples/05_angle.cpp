/* ============================================================================
   Section 8.5 : Opérateur spaceship <=> (C++20)
   Description : Comparaison avec transformation — Angle normalisé modulo 360°,
                 partial_ordering car double peut être NaN
   Fichier source : 05-operateur-spaceship.md
   ============================================================================ */
#include <compare>
#include <cmath>
#include <print>

class Angle {
    double degres_;   // stocké en degrés, peut être > 360

public:
    explicit Angle(double deg) : degres_{deg} {}

    // Normaliser avant de comparer
    std::partial_ordering operator<=>(Angle const& rhs) const {
        auto norm = [](double d) { return std::fmod(d, 360.0); };
        return norm(degres_) <=> norm(rhs.degres_);
    }

    bool operator==(Angle const& rhs) const {
        return (*this <=> rhs) == 0;
    }

    double degres() const { return degres_; }
};

int main() {
    Angle a{30};
    Angle b{390};    // 390 % 360 = 30
    Angle c{45};

    std::println("Angle({}) == Angle({}) → {}", a.degres(), b.degres(), a == b);  // true
    std::println("Angle({}) <  Angle({}) → {}", a.degres(), c.degres(), a < c);   // true
    std::println("Angle({}) >  Angle({}) → {}", c.degres(), a.degres(), c > a);   // true
}
