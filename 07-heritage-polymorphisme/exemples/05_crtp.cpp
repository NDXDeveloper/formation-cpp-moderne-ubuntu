/* ============================================================================
   Section 7.5 : Cout du polymorphisme — CRTP (polymorphisme statique)
   Description : Curiously Recurring Template Pattern : polymorphisme sans
                 vtable ni vptr, resolution a la compilation, inlinable.
   Fichier source : 05-cout-polymorphisme.md
   ============================================================================ */
#include <print>
#include <numbers>

template <typename Derived>
class FormeBase {
public:
    void dessiner() const {
        // Appel statique — résolu à la compilation, inlinable
        static_cast<Derived const*>(this)->dessiner_impl();
    }

    double aire() const {
        return static_cast<Derived const*>(this)->aire_impl();
    }
};

class Cercle : public FormeBase<Cercle> {
    double rayon_;
public:
    explicit Cercle(double r) : rayon_{r} {}

    void dessiner_impl() const {
        std::println("○ (rayon = {})", rayon_);
    }

    double aire_impl() const {
        return std::numbers::pi * rayon_ * rayon_;
    }
};

class Carre : public FormeBase<Carre> {
    double cote_;
public:
    explicit Carre(double c) : cote_{c} {}

    void dessiner_impl() const {
        std::println("□ (côté = {})", cote_);
    }

    double aire_impl() const {
        return cote_ * cote_;
    }
};

// Fonction template — accepte tout type satisfaisant l'interface CRTP
template <typename T>
void afficher(FormeBase<T> const& forme) {
    forme.dessiner();
    std::println("  Aire = {:.2f}", forme.aire());
}

int main() {
    Cercle c{5.0};
    Carre  s{3.0};

    afficher(c);
    afficher(s);
}
