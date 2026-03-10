/* ============================================================================
   Section 7.2 : Fonctions virtuelles et vtable — RTTI (typeid)
   Description : typeid accede au pointeur RTTI stocke dans la vtable pour
                 retourner le type dynamique reel de l'objet.
   Fichier source : 02-fonctions-virtuelles-vtable.md
   ============================================================================ */
#include <typeinfo>
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

void identifier(Forme const& f) {
    std::println("Type réel : {}", typeid(f).name());
}

int main() {
    Cercle c;
    Rectangle r;
    identifier(c);   // affiche le nom mangé de Cercle
    identifier(r);   // affiche le nom mangé de Rectangle
}
