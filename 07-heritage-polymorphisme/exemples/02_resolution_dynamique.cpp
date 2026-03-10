/* ============================================================================
   Section 7.2 : Fonctions virtuelles et vtable — Resolution dynamique
   Description : Avec virtual, l'appel est resolu a l'execution en fonction
                 du type reel de l'objet (dispatch dynamique via vtable).
                 Demonstration des conditions du dispatch dynamique.
   Fichier source : 02-fonctions-virtuelles-vtable.md
   ============================================================================ */
#include <print>

class Forme {
public:
    virtual void dessiner() const {
        std::println("Forme::dessiner()");
    }
    virtual ~Forme() = default;
};

class Cercle : public Forme {
public:
    void dessiner() const override {
        std::println("Cercle::dessiner()");
    }
};

class Rectangle : public Forme {
public:
    void dessiner() const override {
        std::println("Rectangle::dessiner()");
    }
};

void afficher(Forme const& f) {
    f.dessiner();   // dispatch dynamique
}

int main() {
    Cercle c;
    Rectangle r;

    std::println("=== Dispatch dynamique via reference ===");
    afficher(c);    // Cercle::dessiner()
    afficher(r);    // Rectangle::dessiner()

    std::println("\n=== Conditions du dispatch ===");
    Forme& ref = c;
    Forme* ptr = &c;
    Forme  val = c;   // slicing !

    ref.dessiner();           // Dispatch dynamique → Cercle::dessiner()
    ptr->dessiner();          // Dispatch dynamique → Cercle::dessiner()
    val.dessiner();           // Résolution statique → Forme::dessiner() (slicing)
    ref.Forme::dessiner();    // Résolution statique → Forme::dessiner() (qualifié)
}
