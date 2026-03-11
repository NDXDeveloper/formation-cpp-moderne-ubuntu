/* ============================================================================
   Section 14.1 : Parcours et itération
   Description : Parcours ordonné, inverse, requêtes par plage, equal_range
   Fichier source : 01-map-multimap.md
   ============================================================================ */
#include <map>
#include <string>
#include <print>

int main() {
    // Parcours ordonné classique
    std::map<std::string, int> population {
        {"Paris", 2'161'000}, {"Lyon", 522'969},
        {"Marseille", 873'076}, {"Toulouse", 504'078}
    };

    for (const auto& [ville, pop] : population) {
        std::print("{} : {}\n", ville, pop);
    }
    // Affiche dans l'ordre alphabétique :
    // Lyon : 522969
    // Marseille : 873076
    // Paris : 2161000
    // Toulouse : 504078

    std::println("---");

    // Parcours inverse
    for (auto it = population.rbegin(); it != population.rend(); ++it) {
        std::print("{} : {}\n", it->first, it->second);
    }
    // Toulouse, Paris, Marseille, Lyon

    std::println("---");

    // Requêtes par plage : lower_bound et upper_bound
    std::map<int, std::string> events {
        {1789, "Révolution française"},
        {1804, "Premier Empire"},
        {1848, "Deuxième République"},
        {1870, "Troisième République"},
        {1914, "Première Guerre mondiale"},
        {1939, "Seconde Guerre mondiale"},
        {1958, "Cinquième République"}
    };

    // Événements du XIXe siècle [1800, 1900)
    auto from = events.lower_bound(1800); // Premier élément >= 1800
    auto to   = events.lower_bound(1900); // Premier élément >= 1900

    for (auto it = from; it != to; ++it) {
        std::print("{} : {}\n", it->first, it->second);
    }
    // 1804 : Premier Empire
    // 1848 : Deuxième République
    // 1870 : Troisième République

    std::println("---");

    // equal_range
    auto [begin, end] = events.equal_range(1848);
    for (auto it = begin; it != end; ++it) {
        std::print("{} : {}\n", it->first, it->second);
    }
}
