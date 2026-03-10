/* ============================================================================
   Section 7.4 : Classes abstraites — Fonctions virtuelles pures
   Description : Classe abstraite Forme avec fonctions virtuelles pures,
                 classe concrete Cercle qui implemente toutes les methodes.
                 Utilisation de std::numbers::pi.
   Fichier source : 04-classes-abstraites.md
   ============================================================================ */
#include <print>
#include <numbers>
#include <memory>
#include <vector>

class Forme {
public:
    virtual double aire() const = 0;
    virtual double perimetre() const = 0;
    virtual void dessiner() const = 0;
    virtual ~Forme() = default;
};

class Cercle : public Forme {
    double rayon_;
public:
    explicit Cercle(double rayon) : rayon_{rayon} {}

    double aire() const override {
        return std::numbers::pi * rayon_ * rayon_;
    }

    double perimetre() const override {
        return 2.0 * std::numbers::pi * rayon_;
    }

    void dessiner() const override {
        std::println("○ (rayon = {})", rayon_);
    }
};

class Carre : public Forme {
    double cote_;
public:
    explicit Carre(double cote) : cote_{cote} {}

    double aire() const override { return cote_ * cote_; }
    double perimetre() const override { return 4.0 * cote_; }
    void dessiner() const override { std::println("□ (côté = {})", cote_); }
};

int main() {
    // Forme f;  // ❌ Ne compile pas : Forme est abstraite

    std::vector<std::unique_ptr<Forme>> formes;
    formes.push_back(std::make_unique<Cercle>(5.0));
    formes.push_back(std::make_unique<Carre>(3.0));

    for (auto const& f : formes) {
        f->dessiner();
        std::println("  Aire      = {:.4f}", f->aire());
        std::println("  Périmètre = {:.4f}", f->perimetre());
    }
}
