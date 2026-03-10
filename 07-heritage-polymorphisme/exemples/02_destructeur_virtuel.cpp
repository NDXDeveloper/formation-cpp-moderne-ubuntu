/* ============================================================================
   Section 7.2 : Fonctions virtuelles et vtable — Destructeur virtuel
   Description : Sans destructeur virtuel, delete via pointeur de base est un
                 comportement indefini. Avec virtual ~Forme(), la chaine
                 complete de destruction est appelee.
   Fichier source : 02-fonctions-virtuelles-vtable.md
   ============================================================================ */
#include <print>
#include <vector>
#include <memory>

// --- Cas problematique (destructeur NON virtuel) ---

class FormeNonVirtual {
public:
    ~FormeNonVirtual() { std::println("~FormeNonVirtual()"); }
};

class CercleNonVirtual : public FormeNonVirtual {
    std::vector<double> points_{1.0, 2.0, 3.0};
public:
    ~CercleNonVirtual() { std::println("~CercleNonVirtual()"); }
};

// --- Cas correct (destructeur virtuel) ---

class Forme {
public:
    virtual ~Forme() { std::println("~Forme()"); }
};

class Cercle : public Forme {
    std::vector<double> points_{1.0, 2.0, 3.0};
public:
    ~Cercle() override { std::println("~Cercle()"); }
};

int main() {
    std::println("=== Sans destructeur virtuel (UB!) ===");
    FormeNonVirtual* f1 = new CercleNonVirtual{};
    delete f1;   // UB : ~CercleNonVirtual() n'est PAS appelé

    std::println("\n=== Avec destructeur virtuel (correct) ===");
    Forme* f2 = new Cercle{};
    delete f2;   // OK : ~Cercle() puis ~Forme()

    std::println("\n=== Avec unique_ptr (recommande) ===");
    auto f3 = std::make_unique<Cercle>();
    // destruction automatique à la fin du scope
}
