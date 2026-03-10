/* ============================================================================
   Section 7.5 : Cout du polymorphisme — std::variant + std::visit
   Description : Polymorphisme avec ensemble ferme de types, stockage par
                 valeur (contiguïte memoire), dispatch a la compilation.
   Fichier source : 05-cout-polymorphisme.md
   ============================================================================ */
#include <variant>
#include <vector>
#include <print>
#include <numbers>

class Cercle {
    double rayon_;
public:
    explicit Cercle(double r) : rayon_{r} {}
    double aire() const { return std::numbers::pi * rayon_ * rayon_; }
    void dessiner() const { std::println("○ (rayon = {})", rayon_); }
};

class Rectangle {
    double largeur_, hauteur_;
public:
    Rectangle(double l, double h) : largeur_{l}, hauteur_{h} {}
    double aire() const { return largeur_ * hauteur_; }
    void dessiner() const { std::println("□ ({}x{})", largeur_, hauteur_); }
};

using Forme = std::variant<Cercle, Rectangle>;

void dessiner_forme(Forme const& f) {
    std::visit([](auto const& forme) { forme.dessiner(); }, f);
}

double aire_forme(Forme const& f) {
    return std::visit([](auto const& forme) { return forme.aire(); }, f);
}

int main() {
    std::vector<Forme> formes;
    formes.emplace_back(Cercle{5.0});
    formes.emplace_back(Rectangle{3.0, 4.0});

    for (auto const& f : formes) {
        dessiner_forme(f);
        std::println("  Aire = {:.2f}", aire_forme(f));
    }
}
