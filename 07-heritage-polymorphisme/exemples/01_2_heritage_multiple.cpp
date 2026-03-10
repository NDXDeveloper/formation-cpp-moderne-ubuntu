/* ============================================================================
   Section 7.1.2 : Heritage multiple — Syntaxe et disposition memoire
   Description : Heritage multiple Document (Imprimable + Serialisable),
                 disposition memoire Voiture (Moteur + Chassis) avec
                 verification des adresses des sous-objets
   Fichier source : 01.2-heritage-multiple.md
   ============================================================================ */
#include <print>
#include <string>

class Imprimable {
public:
    void imprimer() const {
        std::println("(impression du contenu)");
    }
};

class Serialisable {
public:
    void serialiser() const {
        std::println("(sérialisation en binaire)");
    }
};

class Document : public Imprimable, public Serialisable {
    std::string titre_;
public:
    explicit Document(std::string titre) : titre_{std::move(titre)} {}
    std::string const& titre() const { return titre_; }
};

// ---

class Moteur {
protected:
    int puissance_cv_;
public:
    explicit Moteur(int cv) : puissance_cv_{cv} {}
};

class Chassis {
protected:
    double poids_kg_;
public:
    explicit Chassis(double kg) : poids_kg_{kg} {}
};

class Voiture : public Moteur, public Chassis {
    std::string modele_;
public:
    Voiture(std::string modele, int cv, double kg)
        : Moteur{cv}, Chassis{kg}, modele_{std::move(modele)} {}
};

int main() {
    std::println("=== Document : heritage multiple ===");
    Document doc{"Rapport Q1"};
    doc.imprimer();     // hérité de Imprimable
    doc.serialiser();   // hérité de Serialisable

    std::println("\n=== Voiture : adresses des sous-objets ===");
    Voiture v{"Clio", 90, 1100.0};
    Moteur*  pm = &v;
    Chassis* pc = &v;

    std::println("Voiture : {}", static_cast<void*>(&v));
    std::println("Moteur  : {}", static_cast<void*>(pm));
    std::println("Chassis : {}", static_cast<void*>(pc));
    std::println("(Moteur == Voiture, Chassis decale de sizeof(Moteur))");
}
