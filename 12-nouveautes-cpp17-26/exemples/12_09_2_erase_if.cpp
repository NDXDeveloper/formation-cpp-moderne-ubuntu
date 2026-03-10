/* ============================================================================
   Section 12.9.2 : Cas d'usage et limites - erase_if
   Description : Pattern erase dans une boucle et std::erase_if sur
                 un flat_map - suppression conditionnelle optimisee
   Fichier source : 09.2-cas-usage-limites.md
   ============================================================================ */
#include <flat_map>
#include <string>
#include <print>

int main() {
    // === erase loop pattern (lignes 236-243) ===
    std::print("=== erase loop ===\n");
    std::flat_map<std::string, int> fm = {{"a", 1}, {"b", 2}, {"c", 3}};

    for (auto it = fm.begin(); it != fm.end(); ) {
        if (it->second < 2) {
            it = fm.erase(it);
        } else {
            ++it;
        }
    }

    std::print("After erase loop: ");
    for (const auto& [k, v] : fm) {
        std::print("{}={} ", k, v);
    }
    std::print("\n");
    // b=2 c=3

    // === erase_if pattern (lignes 250-253) ===
    std::print("\n=== erase_if ===\n");
    std::flat_map<std::string, int> fm2 = {{"a", 1}, {"b", 2}, {"c", 3}};
    std::erase_if(fm2, [](const auto& pair) {
        return pair.second < 2;
    });

    std::print("After erase_if: ");
    for (const auto& [k, v] : fm2) {
        std::print("{}={} ", k, v);
    }
    std::print("\n");
    // b=2 c=3
}
