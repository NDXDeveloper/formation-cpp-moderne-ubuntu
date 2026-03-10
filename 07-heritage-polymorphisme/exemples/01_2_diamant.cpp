/* ============================================================================
   Section 7.1.2 : Heritage multiple — Probleme du diamant
   Description : Demonstration du probleme du diamant : duplication du
                 sous-objet commun (Peripherique construit deux fois),
                 ambiguite d'acces et station meteo avec donnees ambigues
   Fichier source : 01.2-heritage-multiple.md
   ============================================================================ */
#include <print>

// --- Diamant : double construction ---

class Peripherique {
protected:
    int id_;
public:
    explicit Peripherique(int id) : id_{id} {
        std::println("Peripherique::Peripherique({})", id);
    }
    int id() const { return id_; }
};

class PeripheriqueEntree : public Peripherique {
public:
    explicit PeripheriqueEntree(int id) : Peripherique{id} {}
    void lire() { std::println("Lecture depuis périphérique {}", id_); }
};

class PeripheriqueSortie : public Peripherique {
public:
    explicit PeripheriqueSortie(int id) : Peripherique{id} {}
    void ecrire() { std::println("Écriture vers périphérique {}", id_); }
};

class PeripheriqueES : public PeripheriqueEntree, public PeripheriqueSortie {
public:
    PeripheriqueES(int id)
        : PeripheriqueEntree{id}, PeripheriqueSortie{id} {}
};

// --- Ambiguïté sur données membres ---

class CapteurTemperature {
protected:
    double valeur_ = 0.0;
public:
    void mesurer() { valeur_ = 22.5; }
    double valeur() const { return valeur_; }
};

class CapteurHumidite {
protected:
    double valeur_ = 0.0;
public:
    void mesurer() { valeur_ = 65.0; }
    double valeur() const { return valeur_; }
};

class StationMeteo : public CapteurTemperature, public CapteurHumidite {
public:
    void rapport() {
        CapteurTemperature::mesurer();
        CapteurHumidite::mesurer();
        std::println("Température : {}°C", CapteurTemperature::valeur());
        std::println("Humidité    : {}%",  CapteurHumidite::valeur());
    }
};

int main() {
    std::println("=== Diamant : double construction ===");
    PeripheriqueES pes{42};

    std::println("\n=== Acces qualifie (pas d'ambiguite) ===");
    pes.PeripheriqueEntree::id();
    pes.PeripheriqueSortie::id();
    pes.lire();
    pes.ecrire();

    std::println("\n=== StationMeteo : donnees ambigues ===");
    StationMeteo s;
    s.rapport();
}
