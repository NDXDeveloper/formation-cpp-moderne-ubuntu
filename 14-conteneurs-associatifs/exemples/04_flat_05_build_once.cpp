/* ============================================================================
   Section 14.4 : Pattern "build once, read many"
   Description : Construction en bloc depuis données non triées, sorted_unique
   Fichier source : 04-flat-map-flat-set.md
   ============================================================================ */
#include <flat_map>
#include <vector>
#include <string>
#include <algorithm>
#include <ranges>
#include <print>

int main() {
    // Simuler load_from_database()
    std::vector<std::pair<std::string, double>> raw_data {
        {"gamma", 3.3}, {"alpha", 1.1}, {"beta", 2.2},
        {"alpha", 9.9}, {"beta", 8.8}
    };

    // Trier par clé
    std::ranges::sort(raw_data, {}, &std::pair<std::string, double>::first);

    // Supprimer les doublons (garder le premier)
    auto removed = std::ranges::unique(raw_data, {},
        &std::pair<std::string, double>::first);
    raw_data.erase(removed.begin(), removed.end());

    // Construire le flat_map à partir des données triées
    std::flat_map<std::string, double> lookup(
        std::sorted_unique,
        raw_data.begin(), raw_data.end()
    );

    // Phase de lecture intensive — très performant
    for (const auto& [k, v] : lookup) {
        std::print("{} : {}\n", k, v);
    }

    // Vérifier la recherche
    if (auto it = lookup.find("beta"); it != lookup.end()) {
        std::println("beta trouvé : {}", it->second);
    }
}
