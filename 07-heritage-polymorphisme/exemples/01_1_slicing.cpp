/* ============================================================================
   Section 7.1.1 : Heritage simple — Probleme du slicing
   Description : Demonstration du slicing (copie par valeur d'un objet derive
                 dans une variable de type base) et comment l'eviter
   Fichier source : 01.1-heritage-simple.md
   ============================================================================ */
#include <print>
#include <string>

class Animal {
    std::string nom_;
public:
    Animal(std::string nom) : nom_{std::move(nom)} {}
    void parler() const { std::println("{} fait du bruit", nom_); }
    std::string const& nom() const { return nom_; }
};

class Chien : public Animal {
    std::string race_;
public:
    Chien(std::string nom, std::string race)
        : Animal{std::move(nom)}, race_{std::move(race)} {}

    void parler() const { std::println("{} aboie ({})", nom(), race_); }
};

int main() {
    Chien rex{"Rex", "Berger Allemand"};

    std::println("=== Appel direct sur Chien ===");
    rex.parler();         // "Rex aboie (Berger Allemand)"

    std::println("\n=== Slicing : copie par valeur ===");
    Animal a = rex;       // SLICING : seule la partie Animal est copiée
    a.parler();           // "Rex fait du bruit" — la partie Chien est perdue

    std::println("\n=== Pas de slicing : reference et pointeur ===");
    Animal& ref = rex;    // Référence — pas de copie, pas de slicing
    Animal* ptr = &rex;   // Pointeur — pas de copie, pas de slicing
    ref.parler();         // "Rex fait du bruit" (name hiding, pas virtual)
    ptr->parler();        // "Rex fait du bruit" (name hiding, pas virtual)
}
