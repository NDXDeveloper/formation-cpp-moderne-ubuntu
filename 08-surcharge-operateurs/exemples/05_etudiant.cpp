/* ============================================================================
   Section 8.5 : Opérateur spaceship <=> (C++20)
   Description : = default — comparaison automatique membre par membre
                 (lexicographique dans l'ordre de déclaration)
   Fichier source : 05-operateur-spaceship.md
   ============================================================================ */
#include <compare>
#include <string>
#include <print>

class Etudiant {
    std::string nom_;
    std::string prenom_;
    int annee_naissance_;

public:
    Etudiant(std::string nom, std::string prenom, int annee)
        : nom_{std::move(nom)}, prenom_{std::move(prenom)}, annee_naissance_{annee} {}

    auto operator<=>(Etudiant const&) const = default;
    bool operator==(Etudiant const&) const = default;
};

int main() {
    Etudiant a{"Dupont", "Alice", 2001};
    Etudiant b{"Dupont", "Bob", 1999};
    Etudiant c{"Dupont", "Alice", 2001};

    std::println("{}", a == c);    // true
    std::println("{}", a != b);    // true
    std::println("{}", a < b);     // true  (même nom, "Alice" < "Bob")
    std::println("{}", a >= c);    // true
}
