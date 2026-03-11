/* ============================================================================
   Section 14.2 : std::unordered_multimap
   Description : Clés dupliquées, count, equal_range
   Fichier source : 02-unordered-map.md
   ============================================================================ */
#include <unordered_map>
#include <string>
#include <print>

int main() {
    std::unordered_multimap<std::string, int> scores {
        {"Alice", 95}, {"Bob", 87}, {"Alice", 88}, {"Alice", 92}
    };

    // count retourne le nombre d'éléments avec cette clé
    std::print("Scores d'Alice : {}\n", scores.count("Alice")); // 3

    // equal_range retourne la plage d'éléments
    auto [begin, end] = scores.equal_range("Alice");
    for (auto it = begin; it != end; ++it) {
        std::print("  {}\n", it->second);
    }
}
