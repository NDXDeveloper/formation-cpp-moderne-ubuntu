/* ============================================================================
   Section 8.1 : Surcharge des opérateurs arithmétiques et de comparaison
   Description : Type Vec2 complet — opérateurs composés, binaires, unaires,
                 comparaison pré-C++20, multiplication scalaire, flux
   Fichier source : 01-operateurs-arithmetiques.md
   ============================================================================ */
#include <cmath>
#include <iostream>

class Vec2 {
    double x_;
    double y_;

public:
    Vec2() : x_{0.0}, y_{0.0} {}
    Vec2(double x, double y) : x_{x}, y_{y} {}

    double x() const { return x_; }
    double y() const { return y_; }
    double norme() const { return std::sqrt(x_ * x_ + y_ * y_); }

    // Opérateurs composés (membres)
    Vec2& operator+=(Vec2 const& rhs) {
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
    }

    Vec2& operator-=(Vec2 const& rhs) {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        return *this;
    }

    Vec2& operator*=(double scalaire) {
        x_ *= scalaire;
        y_ *= scalaire;
        return *this;
    }

    // Unaires
    Vec2 operator-() const { return Vec2{-x_, -y_}; }
    Vec2 operator+() const { return *this; }

    // Flux (hidden friend)
    friend std::ostream& operator<<(std::ostream& os, Vec2 const& v) {
        os << "(" << v.x_ << ", " << v.y_ << ")";
        return os;
    }

    friend std::istream& operator>>(std::istream& is, Vec2& v) {
        is >> v.x_ >> v.y_;
        return is;
    }
};

// Opérateurs binaires (fonctions libres)
Vec2 operator+(Vec2 lhs, Vec2 const& rhs) { lhs += rhs; return lhs; }
Vec2 operator-(Vec2 lhs, Vec2 const& rhs) { lhs -= rhs; return lhs; }

// Multiplication scalaire — deux sens
Vec2 operator*(Vec2 v, double scalaire) { v *= scalaire; return v; }
Vec2 operator*(double scalaire, Vec2 v) { v *= scalaire; return v; }

// Comparaison pré-C++20
bool operator==(Vec2 const& lhs, Vec2 const& rhs) {
    return lhs.x() == rhs.x() && lhs.y() == rhs.y();
}
bool operator!=(Vec2 const& lhs, Vec2 const& rhs) { return !(lhs == rhs); }

bool operator<(Vec2 const& lhs, Vec2 const& rhs) {
    if (lhs.x() != rhs.x()) return lhs.x() < rhs.x();
    return lhs.y() < rhs.y();
}
bool operator>(Vec2 const& lhs, Vec2 const& rhs)  { return rhs < lhs; }
bool operator<=(Vec2 const& lhs, Vec2 const& rhs) { return !(rhs < lhs); }
bool operator>=(Vec2 const& lhs, Vec2 const& rhs) { return !(lhs < rhs); }

int main() {
    Vec2 a{1, 2};

    // Opérateurs binaires et symétrie
    std::cout << "a = " << a << "\n";
    std::cout << "a + (3,4) = " << (a + Vec2{3, 4}) << "\n";
    std::cout << "(3,4) + a = " << (Vec2{3, 4} + a) << "\n";

    // Unaire
    Vec2 v{3.0, 4.0};
    std::cout << "-v = " << (-v) << "\n";
    std::cout << "norme(v) = " << v.norme() << "\n";

    // Multiplication scalaire
    std::cout << "v * 2.0 = " << (v * 2.0) << "\n";
    std::cout << "2.0 * v = " << (2.0 * v) << "\n";

    // Comparaison
    Vec2 c1{1.0, 2.0}, c2{1.0, 3.0}, c3{1.0, 2.0};
    std::cout << std::boolalpha;
    std::cout << "c1 == c3: " << (c1 == c3) << "\n";
    std::cout << "c1 != c2: " << (c1 != c2) << "\n";
    std::cout << "c1 < c2: " << (c1 < c2) << "\n";

    // Chaînage flux
    Vec2 fa{1.0, 2.0}, fb{3.0, 4.0};
    std::cout << "a = " << fa << ", b = " << fb << "\n";
}
