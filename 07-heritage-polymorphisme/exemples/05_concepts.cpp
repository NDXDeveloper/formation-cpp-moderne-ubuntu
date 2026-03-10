/* ============================================================================
   Section 7.5 : Cout du polymorphisme — Concepts C++20
   Description : Les Concepts definissent un contrat d'interface verifie a la
                 compilation, sans vtable ni surcout a l'execution.
   Fichier source : 05-cout-polymorphisme.md
   ============================================================================ */
#include <concepts>
#include <print>
#include <numbers>

template <typename T>
concept Dessinable = requires(T const& t) {
    { t.dessiner() } -> std::same_as<void>;
    { t.aire() } -> std::convertible_to<double>;
};

template <Dessinable T>
void afficher(T const& forme) {
    forme.dessiner();   // appel direct, inlinable
    std::println("  Aire = {:.2f}", forme.aire());
}

class Cercle {
    double rayon_;
public:
    explicit Cercle(double r) : rayon_{r} {}
    double aire() const { return std::numbers::pi * rayon_ * rayon_; }
    void dessiner() const { std::println("○ (rayon = {})", rayon_); }
};

class Triangle {
    double base_, hauteur_;
public:
    Triangle(double b, double h) : base_{b}, hauteur_{h} {}
    double aire() const { return 0.5 * base_ * hauteur_; }
    void dessiner() const { std::println("△ (base={}, h={})", base_, hauteur_); }
};

int main() {
    Cercle c{5.0};
    Triangle t{6.0, 4.0};

    afficher(c);
    afficher(t);
}
