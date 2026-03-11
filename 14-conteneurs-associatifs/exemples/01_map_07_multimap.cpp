/* ============================================================================
   Section 14.1 : std::multimap
   Description : Construction, insertion, equal_range, count, suppression
   Fichier source : 01-map-multimap.md
   ============================================================================ */
#include <map>
#include <string>
#include <print>

int main() {
    // Construction
    std::multimap<std::string, std::string> index {
        {"fruit",  "pomme"},
        {"fruit",  "banane"},
        {"fruit",  "cerise"},
        {"légume", "carotte"},
        {"légume", "poireau"}
    };

    // Insertion — réussit toujours
    index.insert({"fruit", "fraise"});  // Toujours accepté
    index.insert({"fruit", "fraise"});  // Aussi accepté — doublon exact autorisé

    // equal_range — accéder aux éléments d'une clé
    auto [begin, end] = index.equal_range("fruit");
    for (auto it = begin; it != end; ++it) {
        std::print("{}\n", it->second);
    }
    // pomme, banane, cerise, fraise, fraise (ordre d'insertion pour clés égales)

    std::println("---");

    // count
    std::size_t nb_fruits = index.count("fruit"); // 5
    std::println("nb_fruits = {}", nb_fruits);

    // erase par clé — supprime TOUS les éléments ayant cette clé
    index.erase("fruit"); // Supprime les 5 entrées "fruit"
    std::println("after erase fruit: size={}", index.size());

    // erase par itérateur — supprime un seul élément
    auto it = index.find("légume"); // Pointe sur le premier "légume"
    if (it != index.end()) {
        index.erase(it); // Supprime uniquement cet élément
    }
    std::println("after erase one légume: size={}", index.size());
    for (const auto& [k, v] : index) {
        std::print("  {} : {}\n", k, v);
    }
}
