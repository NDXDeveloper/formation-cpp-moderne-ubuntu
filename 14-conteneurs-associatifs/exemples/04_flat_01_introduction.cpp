/* ============================================================================
   Section 14.4 : Introduction à std::flat_map (C++23)
   Description : Déclaration, insertion, recherche, parcours ordonné
   Fichier source : 04-flat-map-flat-set.md
   ============================================================================ */
#include <flat_map>
#include <print>
#include <string>

int main() {
    std::flat_map<std::string, int> population {
        {"Paris", 2'161'000},
        {"Lyon", 522'969},
        {"Marseille", 873'076}
    };

    // Interface identique à std::map
    population["Toulouse"] = 504'078;

    if (population.contains("Lyon")) {
        std::print("Lyon : {} habitants\n", population["Lyon"]);
    }

    // Parcours ordonné — garanti comme std::map
    for (const auto& [ville, pop] : population) {
        std::print("{} : {}\n", ville, pop);
    }
}
