/* ============================================================================
   Section 8.2 : Opérateurs d'affectation
   Description : Opérateurs bit-à-bit composés (|=, &=) pour un type
                 Permissions basé sur des flags bitmask
   Fichier source : 02-operateurs-affectation.md
   ============================================================================ */
#include <cstdint>
#include <iostream>

class Permissions {
    std::uint8_t bits_;

public:
    enum Flag : std::uint8_t {
        Lecture   = 0b001,
        Ecriture  = 0b010,
        Execution = 0b100
    };

    explicit Permissions(std::uint8_t bits = 0) : bits_{bits} {}

    Permissions& operator|=(Flag f) {
        bits_ |= f;
        return *this;
    }

    Permissions& operator&=(std::uint8_t mask) {
        bits_ &= mask;
        return *this;
    }

    bool a(Flag f) const { return (bits_ & f) != 0; }

    friend Permissions operator|(Permissions p, Flag f) {
        p |= f;
        return p;
    }

    friend std::ostream& operator<<(std::ostream& os, Permissions const& p) {
        os << (p.a(Lecture) ? "r" : "-")
           << (p.a(Ecriture) ? "w" : "-")
           << (p.a(Execution) ? "x" : "-");
        return os;
    }
};

int main() {
    Permissions p;
    p |= Permissions::Lecture;
    p |= Permissions::Execution;
    std::cout << "Permissions : " << p << "\n";   // r-x

    // Ajout écriture
    p |= Permissions::Ecriture;
    std::cout << "Après |= Ecriture : " << p << "\n";  // rwx

    // Retrait exécution via masque
    p &= ~Permissions::Execution;
    std::cout << "Après &= ~Execution : " << p << "\n";  // rw-
}
