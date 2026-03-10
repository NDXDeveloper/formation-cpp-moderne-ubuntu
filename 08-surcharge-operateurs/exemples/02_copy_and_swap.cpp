/* ============================================================================
   Section 8.2 : Opérateurs d'affectation
   Description : Buffer avec copy-and-swap idiom — exception-safe,
                 self-assignment-safe, gestion unifiée copie/déplacement
   Fichier source : 02-operateurs-affectation.md
   ============================================================================ */
#include <algorithm>
#include <utility>
#include <iostream>

class Buffer {
    std::size_t taille_;
    int* donnees_;

public:
    explicit Buffer(std::size_t taille)
        : taille_{taille}
        , donnees_{new int[taille]{}} {}

    // Constructeur de copie
    Buffer(Buffer const& other)
        : taille_{other.taille_}
        , donnees_{new int[taille_]} {
        std::copy_n(other.donnees_, taille_, donnees_);
    }

    // Destructeur
    ~Buffer() {
        delete[] donnees_;
    }

    // Fonction swap membre (noexcept)
    void swap(Buffer& other) noexcept {
        std::swap(taille_, other.taille_);
        std::swap(donnees_, other.donnees_);
    }

    // operator= par copy-and-swap
    Buffer& operator=(Buffer other) {   // ← paramètre par VALEUR
        swap(other);                     // échange avec la copie locale
        return *this;
    }                                    // other (ancienne donnée) est détruit ici

    std::size_t taille() const { return taille_; }
    int& operator[](std::size_t i) { return donnees_[i]; }
    int const& operator[](std::size_t i) const { return donnees_[i]; }
};

int main() {
    Buffer a{100};
    Buffer b{200};

    a[0] = 42;
    b[0] = 99;

    // Copie (lvalue → constructeur de copie → swap)
    a = b;
    std::cout << "Après a = b:\n";
    std::cout << "  a[0] = " << a[0] << "\n";        // 99
    std::cout << "  a.taille() = " << a.taille() << "\n";  // 200

    // Déplacement (rvalue → constructeur de déplacement → swap)
    a = Buffer{300};
    std::cout << "Après a = Buffer{300}:\n";
    std::cout << "  a.taille() = " << a.taille() << "\n";  // 300

    // Self-assignment (safe par construction)
    a[0] = 77;
    a = a;
    std::cout << "Après a = a (self-assign):\n";
    std::cout << "  a[0] = " << a[0] << "\n";        // 77
    std::cout << "  a.taille() = " << a.taille() << "\n";  // 300
}
