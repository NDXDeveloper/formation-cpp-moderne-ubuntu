/* ============================================================================
   Section 14.1 : Recherche et accès
   Description : find, contains (C++20), at avec exception
   Fichier source : 01-map-multimap.md
   ============================================================================ */
#include <map>
#include <string>
#include <print>
#include <stdexcept>

int main() {
    std::map<std::string, int> population {
        {"Paris", 2'161'000}, {"Lyon", 522'969},
        {"Marseille", 873'076}, {"Toulouse", 504'078}
    };

    // find — la méthode de recherche standard
    auto it = population.find("Lyon");
    if (it != population.end()) {
        std::print("Lyon : {} habitants\n", it->second);
    } else {
        std::print("Ville non trouvée\n");
    }

    // contains (C++20) — test d'existence simplifié
    if (population.contains("Marseille")) {
        std::print("Marseille est dans la map\n");
    }

    // at — accès avec vérification
    try {
        [[maybe_unused]] int pop = population.at("Bordeaux");
    } catch (const std::out_of_range& e) {
        std::print("Erreur : {}\n", e.what());
    }
}
