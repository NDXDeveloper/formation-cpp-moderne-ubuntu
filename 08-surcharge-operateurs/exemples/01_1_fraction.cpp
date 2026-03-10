/* ============================================================================
   Section 8.1 : Surcharge des opérateurs arithmétiques et de comparaison
   Description : Classe Fraction complète — opérateurs arithmétiques composés
                 et binaires, comparaison, flux, conversion implicite int
   Fichier source : 01-operateurs-arithmetiques.md
   ============================================================================ */
#include <numeric>   // std::gcd
#include <ostream>
#include <stdexcept>
#include <iostream>

class Fraction {
    int num_;
    int den_;

    void simplifier() {
        if (den_ < 0) { num_ = -num_; den_ = -den_; }
        int g = std::gcd(std::abs(num_), den_);
        num_ /= g;
        den_ /= g;
    }

public:
    Fraction(int num, int den = 1) : num_{num}, den_{den} {
        if (den == 0) throw std::invalid_argument{"Dénominateur nul"};
        simplifier();
    }

    int numerateur() const { return num_; }
    int denominateur() const { return den_; }

    // --- Opérateurs composés (membres) ---

    Fraction& operator+=(Fraction const& rhs) {
        num_ = num_ * rhs.den_ + rhs.num_ * den_;
        den_ = den_ * rhs.den_;
        simplifier();
        return *this;
    }

    Fraction& operator-=(Fraction const& rhs) {
        num_ = num_ * rhs.den_ - rhs.num_ * den_;
        den_ = den_ * rhs.den_;
        simplifier();
        return *this;
    }

    Fraction& operator*=(Fraction const& rhs) {
        num_ *= rhs.num_;
        den_ *= rhs.den_;
        simplifier();
        return *this;
    }

    Fraction& operator/=(Fraction const& rhs) {
        if (rhs.num_ == 0) throw std::invalid_argument{"Division par zéro"};
        num_ *= rhs.den_;
        den_ *= rhs.num_;
        simplifier();
        return *this;
    }

    // --- Unaire ---

    Fraction operator-() const {
        return Fraction{-num_, den_};
    }

    // --- Comparaison (hidden friends) ---

    friend bool operator==(Fraction const& a, Fraction const& b) {
        return a.num_ == b.num_ && a.den_ == b.den_;
    }

    friend bool operator<(Fraction const& a, Fraction const& b) {
        return a.num_ * b.den_ < b.num_ * a.den_;
    }

    friend bool operator!=(Fraction const& a, Fraction const& b) { return !(a == b); }
    friend bool operator>(Fraction const& a, Fraction const& b)  { return b < a; }
    friend bool operator<=(Fraction const& a, Fraction const& b) { return !(b < a); }
    friend bool operator>=(Fraction const& a, Fraction const& b) { return !(a < b); }

    // --- Flux ---

    friend std::ostream& operator<<(std::ostream& os, Fraction const& f) {
        os << f.num_;
        if (f.den_ != 1) os << "/" << f.den_;
        return os;
    }
};

// --- Opérateurs binaires (fonctions libres) ---

Fraction operator+(Fraction lhs, Fraction const& rhs) { lhs += rhs; return lhs; }
Fraction operator-(Fraction lhs, Fraction const& rhs) { lhs -= rhs; return lhs; }
Fraction operator*(Fraction lhs, Fraction const& rhs) { lhs *= rhs; return lhs; }
Fraction operator/(Fraction lhs, Fraction const& rhs) { lhs /= rhs; return lhs; }

int main() {
    Fraction a{1, 3};
    Fraction b{2, 5};

    std::cout << "a     = " << a << "\n";         // 1/3
    std::cout << "b     = " << b << "\n";         // 2/5
    std::cout << "a + b = " << (a + b) << "\n";   // 11/15
    std::cout << "a * b = " << (a * b) << "\n";   // 2/15
    std::cout << "-a    = " << (-a) << "\n";       // -1/3
    std::cout << std::boolalpha;
    std::cout << "a < b  = " << (a < b) << "\n";  // true
    std::cout << "a == a = " << (a == a) << "\n";  // true

    // Conversion implicite int → Fraction via constructeur
    Fraction c = a + 2;     // 7/3
    Fraction d = 3 * b;     // 6/5

    std::cout << "c = a + 2 = " << c << "\n";     // 7/3
    std::cout << "d = 3 * b = " << d << "\n";     // 6/5
}
