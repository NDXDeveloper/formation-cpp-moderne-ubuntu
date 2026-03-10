/* ============================================================================
   Section 7.1.3 : Heritage virtuel — Resolution du diamant
   Description : Heritage virtuel pour resoudre le probleme du diamant.
                 Un seul sous-objet Peripherique est partage, un seul appel
                 au constructeur, acces non ambigu a id().
   Fichier source : 01.3-heritage-virtuel.md
   ============================================================================ */
#include <print>

class Peripherique {
protected:
    int id_;
public:
    explicit Peripherique(int id) : id_{id} {
        std::println("Peripherique::Peripherique({})", id);
    }
    int id() const { return id_; }
};

class PeripheriqueEntree : virtual public Peripherique {
public:
    explicit PeripheriqueEntree(int id) : Peripherique{id} {}
    void lire() { std::println("Lecture depuis périphérique {}", id_); }
};

class PeripheriqueSortie : virtual public Peripherique {
public:
    explicit PeripheriqueSortie(int id) : Peripherique{id} {}
    void ecrire() { std::println("Écriture vers périphérique {}", id_); }
};

class PeripheriqueES : public PeripheriqueEntree, public PeripheriqueSortie {
public:
    PeripheriqueES(int id)
        : Peripherique{id}          // la classe la plus dérivée initialise la base virtuelle
        , PeripheriqueEntree{id}
        , PeripheriqueSortie{id} {}
};

int main() {
    std::println("=== Construction (un seul appel) ===");
    PeripheriqueES pes{42};
    std::println("ID = {}", pes.id());   // Plus d'ambiguïté

    std::println("\n=== Utilisation ===");
    pes.lire();
    pes.ecrire();
}
