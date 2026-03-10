/* ============================================================================
   Section 7.2 : Fonctions virtuelles et vtable — Resolution statique
   Description : Sans virtual, l'appel est resolu a la compilation en fonction
                 du type statique. afficher(Forme&) appelle toujours
                 Forme::dessiner(), meme si l'objet reel est un Cercle.
   Fichier source : 02-fonctions-virtuelles-vtable.md
   ============================================================================ */
#include <print>

class Forme {
public:
    void dessiner() const {
        std::println("Forme::dessiner()");
    }
};

class Cercle : public Forme {
public:
    void dessiner() const {
        std::println("Cercle::dessiner()");
    }
};

class Rectangle : public Forme {
public:
    void dessiner() const {
        std::println("Rectangle::dessiner()");
    }
};

void afficher(Forme const& f) {
    f.dessiner();   // Quelle version est appelée ?
}

int main() {
    Cercle c;
    Rectangle r;
    afficher(c);    // Forme::dessiner() — résolution statique
    afficher(r);    // Forme::dessiner() — résolution statique
}
