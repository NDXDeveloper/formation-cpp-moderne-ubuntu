/* ============================================================================
   Section 14.1 : Comparaison personnalisée
   Description : Ordre décroissant, comparaison insensible à la casse,
                 transparent comparators (C++14)
   Fichier source : 01-map-multimap.md
   ============================================================================ */
#include <map>
#include <string>
#include <string_view>
#include <algorithm>
#include <print>

struct CaseInsensitiveCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        return std::lexicographical_compare(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](char ca, char cb) {
                return std::tolower(static_cast<unsigned char>(ca))
                     < std::tolower(static_cast<unsigned char>(cb));
            }
        );
    }
};

int main() {
    // Ordre décroissant
    std::map<int, std::string, std::greater<int>> desc_map {
        {1, "un"}, {3, "trois"}, {2, "deux"}
    };
    for (const auto& [k, v] : desc_map) {
        std::print("{}: {}\n", k, v);
    }
    // 3: trois
    // 2: deux
    // 1: un

    std::println("---");

    // Comparaison insensible à la casse
    std::map<std::string, int, CaseInsensitiveCompare> ci_map;
    ci_map["Hello"] = 1;
    ci_map["hello"] = 2; // Écrase la précédente (même clé au sens du comparateur)
    // ci_map.size() == 1
    std::println("ci_map.size() == {}", ci_map.size());
    std::println("ci_map[Hello] = {}", ci_map["Hello"]);

    std::println("---");

    // Transparent comparators et recherche hétérogène (C++14)
    std::map<std::string, int, std::less<>> config {
        {"timeout", 30},
        {"retries", 3},
        {"buffer_size", 4096}
    };

    // Recherche directe avec un const char* — pas de std::string temporaire
    auto it = config.find("timeout");
    std::println("timeout = {}", it->second);

    // Fonctionne aussi avec std::string_view
    std::string_view key = "retries";
    auto it2 = config.find(key);
    std::println("retries = {}", it2->second);
}
