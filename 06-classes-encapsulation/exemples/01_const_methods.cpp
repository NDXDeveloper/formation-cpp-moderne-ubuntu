/* ============================================================================
   Section 6.1 : Définition de classes — Membres et méthodes
   Description : Méthodes const — garantir la lecture seule, utilisation
                 avec des références const
   Fichier source : 01-definition-classes.md
   ============================================================================ */
#include <iostream>

class Circle {
public:
    explicit Circle(double radius) : radius_(radius) {}

    // Méthode const — ne modifie pas l'objet
    double area() const {
        return 3.14159265358979 * radius_ * radius_;
    }

    // Méthode non-const — modifie l'objet
    void scale(double factor) {
        radius_ *= factor;
    }

private:
    double radius_;
};

void print_info(const Circle& c) {
    std::cout << c.area() << "\n";    // OK — area() est const
    // c.scale(2.0);                  // ERREUR — scale() n'est pas const
}

int main() {
    Circle c(5.0);
    print_info(c);   // 78.5398...

    c.scale(2.0);    // OK — c n'est pas const
    print_info(c);   // 314.159...

    return 0;
}
// Sortie attendue :
// 78.5398
// 314.159
