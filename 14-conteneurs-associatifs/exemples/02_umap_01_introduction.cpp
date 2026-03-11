/* ============================================================================
   Section 14.2 : Introduction à std::unordered_map
   Description : Déclaration, recherche O(1), parcours non ordonné
   Fichier source : 02-unordered-map.md
   ============================================================================ */
#include <unordered_map>
#include <string>
#include <print>

int main() {
    std::unordered_map<std::string, int> population {
        {"Paris",     2'161'000},
        {"Lyon",        522'969},
        {"Marseille",   873'076},
        {"Toulouse",    504'078}
    };

    // Recherche en O(1) amorti
    if (auto it = population.find("Lyon"); it != population.end()) {
        std::print("Lyon : {} habitants\n", it->second);
    }

    // Parcours — ordre NON garanti
    for (const auto& [ville, pop] : population) {
        std::print("{} : {}\n", ville, pop);
    }
    // Peut afficher dans n'importe quel ordre
}
