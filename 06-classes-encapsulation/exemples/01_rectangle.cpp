/* ============================================================================
   Section 6.1 : Définition de classes — Membres et méthodes
   Description : Anatomie d'une classe Rectangle — données membres avec
                 default member initializers, méthodes, instanciation sur la
                 pile et sur le tas
   Fichier source : 01-definition-classes.md
   ============================================================================ */
#include <iostream>

class Rectangle {
public:
    // --- Fonctions membres (méthodes) ---
    double area() const {
        return width_ * height_;
    }

    double perimeter() const {
        return 2.0 * (width_ + height_);
    }

    void resize(double w, double h) {
        width_ = w;
        height_ = h;
    }

private:
    // --- Données membres (attributs) ---
    double width_ = 0.0;    // Initialisation en place (C++11)
    double height_ = 0.0;
};

int main() {
    // Instanciation sur la pile
    Rectangle r;
    r.resize(4.0, 3.0);
    std::cout << r.area() << "\n";      // 12

    Rectangle a;
    a.resize(5.0, 2.0);
    Rectangle b;
    b.resize(10.0, 7.0);
    std::cout << a.area() << "\n";      // 10
    std::cout << b.area() << "\n";      // 70

    // Instanciation sur le tas
    Rectangle* p = new Rectangle;
    p->resize(3.0, 4.0);
    std::cout << p->area() << "\n";     // 12
    delete p;

    return 0;
}
// Sortie attendue :
// 12
// 10
// 70
// 12
