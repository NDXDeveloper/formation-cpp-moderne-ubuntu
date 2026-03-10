/* ============================================================================
   Section 8.1 : Surcharge des opérateurs arithmétiques et de comparaison
   Description : Hidden friends — opérateurs définis inline dans la classe,
                 visibles uniquement par ADL
   Fichier source : 01-operateurs-arithmetiques.md
   ============================================================================ */
#include <iostream>

class Vec2 {
    double x_, y_;
public:
    Vec2(double x, double y) : x_{x}, y_{y} {}

    friend Vec2 operator+(Vec2 lhs, Vec2 const& rhs) {
        lhs.x_ += rhs.x_;
        lhs.y_ += rhs.y_;
        return lhs;
    }

    friend bool operator==(Vec2 const& lhs, Vec2 const& rhs) {
        return lhs.x_ == rhs.x_ && lhs.y_ == rhs.y_;
    }

    friend std::ostream& operator<<(std::ostream& os, Vec2 const& v) {
        os << "(" << v.x_ << ", " << v.y_ << ")";
        return os;
    }
};

int main() {
    Vec2 a{1.0, 2.0}, b{3.0, 4.0};
    std::cout << "a + b = " << (a + b) << "\n";         // (4, 6)
    std::cout << std::boolalpha;
    std::cout << "a == a: " << (a == a) << "\n";         // true
    std::cout << "a == b: " << (a == b) << "\n";         // false
}
