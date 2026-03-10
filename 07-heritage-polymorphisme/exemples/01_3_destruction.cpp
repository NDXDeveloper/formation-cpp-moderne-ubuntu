/* ============================================================================
   Section 7.1.3 : Heritage virtuel — Construction et destruction completes
   Description : Avec heritage virtuel, le destructeur de la base virtuelle
                 n'est appele qu'une seule fois (en dernier).
                 Ordre : construction base virtuelle -> intermediaires -> derivee,
                 destruction dans l'ordre inverse.
   Fichier source : 01.3-heritage-virtuel.md
   ============================================================================ */
#include <print>

class Peripherique {
public:
    explicit Peripherique(int id) { std::println("Peripherique()"); }
    ~Peripherique() { std::println("~Peripherique()"); }
};

class PeripheriqueEntree : virtual public Peripherique {
public:
    explicit PeripheriqueEntree(int id) : Peripherique{id} {
        std::println("PeripheriqueEntree()");
    }
    ~PeripheriqueEntree() { std::println("~PeripheriqueEntree()"); }
};

class PeripheriqueSortie : virtual public Peripherique {
public:
    explicit PeripheriqueSortie(int id) : Peripherique{id} {
        std::println("PeripheriqueSortie()");
    }
    ~PeripheriqueSortie() { std::println("~PeripheriqueSortie()"); }
};

class PeripheriqueES : public PeripheriqueEntree, public PeripheriqueSortie {
public:
    PeripheriqueES(int id)
        : Peripherique{id}, PeripheriqueEntree{id}, PeripheriqueSortie{id} {
        std::println("PeripheriqueES()");
    }
    ~PeripheriqueES() { std::println("~PeripheriqueES()"); }
};

int main() {
    PeripheriqueES pes{42};
}
