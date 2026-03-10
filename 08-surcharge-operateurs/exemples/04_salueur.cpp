/* ============================================================================
   Section 8.4 : Opérateur d'appel de fonction (operator())
   Description : Foncteur de base — Salueur avec état (formule configurable)
   Fichier source : 04-operateur-appel.md
   ============================================================================ */
#include <string>
#include <print>

class Salueur {
    std::string formule_;

public:
    explicit Salueur(std::string formule)
        : formule_{std::move(formule)} {}

    void operator()(std::string const& nom) const {
        std::println("{}, {} !", formule_, nom);
    }
};

int main() {
    Salueur bonjour{"Bonjour"};
    Salueur salut{"Salut"};

    bonjour("Alice");    // Bonjour, Alice !
    salut("Bob");        // Salut, Bob !
}
