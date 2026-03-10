/* ============================================================================
   Section 7.1.2 : Heritage multiple — Ambiguite de noms et resolution
   Description : Ambiguite quand deux bases ont une methode de meme nom,
                 resolution par qualification explicite, redefinition dans
                 la derivee, et using pour surcharges differentes
   Fichier source : 01.2-heritage-multiple.md
   ============================================================================ */
#include <print>
#include <string>

// --- Ambiguité et résolution par redéfinition ---

class Ecran {
public:
    void allumer() { std::println("Ecran allumé"); }
};

class Clavier {
public:
    void allumer() { std::println("Clavier rétro-éclairé"); }
};

class Laptop : public Ecran, public Clavier {
public:
    void allumer() {
        Ecran::allumer();    // allumer l'écran
        Clavier::allumer();  // puis le clavier
    }
};

// --- Résolution par using (signatures différentes) ---

class Capteur {
public:
    void lire(int canal) { std::println("Capteur canal {}", canal); }
};

class Fichier {
public:
    void lire(std::string const& chemin) { std::println("Fichier {}", chemin); }
};

class SystemeAcquisition : public Capteur, public Fichier {
public:
    using Capteur::lire;   // rend visible lire(int)
    using Fichier::lire;   // rend visible lire(string)
};

int main() {
    std::println("=== Laptop : resolution par redefinition ===");
    Laptop pc;
    pc.allumer();

    std::println("\n=== Laptop : qualification explicite ===");
    pc.Ecran::allumer();
    pc.Clavier::allumer();

    std::println("\n=== SystemeAcquisition : using avec signatures differentes ===");
    SystemeAcquisition sys;
    sys.lire(3);                 // Capteur::lire(int)
    sys.lire("/tmp/data.csv");   // Fichier::lire(string)
}
