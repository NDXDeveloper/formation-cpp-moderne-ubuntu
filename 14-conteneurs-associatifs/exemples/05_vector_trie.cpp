/* ============================================================================
   Section 14.5 : Le vecteur trié — la solution DIY avant C++23
   Description : Vecteur trié avec recherche binaire via std::ranges
   Fichier source : 05-comparaison-performances.md
   ============================================================================ */
#include <vector>
#include <algorithm>
#include <ranges>
#include <string>
#include <print>

int main() {
    std::vector<std::pair<int, std::string>> data;

    // Insertion en vrac
    data.push_back({3, "c"});
    data.push_back({1, "a"});
    data.push_back({2, "b"});

    // Tri une seule fois
    std::ranges::sort(data, {}, &std::pair<int, std::string>::first);

    // Recherche binaire
    auto it = std::ranges::lower_bound(data, 2,
        {}, &std::pair<int, std::string>::first);

    if (it != data.end() && it->first == 2) {
        std::print("Trouvé : {}\n", it->second);
    }
}
