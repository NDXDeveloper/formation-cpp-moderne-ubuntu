/* ============================================================================
   Section 14.4 : Construction efficace avec sorted_unique
   Description : Construction O(n) depuis données triées, construction standard
   Fichier source : 04-flat-map-flat-set.md
   ============================================================================ */
#include <flat_map>
#include <vector>
#include <string>
#include <print>

int main() {
    // Construction avec sorted_unique — O(n)
    std::vector<int> keys {1, 2, 3, 4, 5};
    std::vector<std::string> values {"a", "b", "c", "d", "e"};

    std::flat_map<int, std::string> fm(std::sorted_unique,
                                        std::move(keys),
                                        std::move(values));
    for (const auto& [k, v] : fm) {
        std::print("{}:{} ", k, v);
    }
    std::println("");

    std::println("---");

    // Construction standard — tri automatique
    std::flat_map<int, std::string> fm2 {
        {3, "c"}, {1, "a"}, {5, "e"}, {2, "b"}, {4, "d"}
    };
    // Trié automatiquement : {1:"a", 2:"b", 3:"c", 4:"d", 5:"e"}
    for (const auto& [k, v] : fm2) {
        std::print("{}:{} ", k, v);
    }
    std::println("");
}
