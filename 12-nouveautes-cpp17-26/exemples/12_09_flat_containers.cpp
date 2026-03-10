/* ============================================================================
   Section 12.9 : std::flat_map et std::flat_set (C++23)
   Description : Conteneurs ordonnes a memoire contigue - flat_map basique,
                 flat_set, construction sorted_unique, keys/values/extract
   Fichier source : 09-flat-containers.md
   ============================================================================ */
#include <flat_map>
#include <flat_set>
#include <string>
#include <vector>
#include <print>

int main() {
    // === flat_map basique (lignes 47-78) ===
    std::print("=== flat_map basic ===\n");
    std::flat_map<std::string, int> scores;
    scores["Alice"] = 95;
    scores["Bob"] = 87;
    scores["Clara"] = 92;
    scores.insert({"Dave", 88});
    scores.emplace("Eve", 91);

    if (auto it = scores.find("Bob"); it != scores.end()) {
        std::print("Bob : {}\n", it->second);
    }

    for (const auto& [name, score] : scores) {
        std::print("{} : {}\n", name, score);
    }

    if (scores.contains("Alice")) {
        std::print("Alice trouvee\n");
    }

    // === flat_set (lignes 83-98) ===
    std::print("\n=== flat_set ===\n");
    std::flat_set<std::string> tags;
    tags.insert("cpp");
    tags.insert("linux");
    tags.insert("devops");
    tags.insert("cpp");    // Ignore

    for (const auto& tag : tags) {
        std::print("{} ", tag);
    }
    std::print("\n");
    // cpp devops linux

    // === sorted_unique construction (lignes 104-116) ===
    std::print("\n=== sorted_unique ===\n");
    std::vector<std::string> keys = {"Alice", "Bob", "Clara"};
    std::vector<int> values = {95, 87, 92};

    std::flat_map<std::string, int> fm_su(std::sorted_unique,
                                           std::move(keys),
                                           std::move(values));
    for (const auto& [name, score] : fm_su) {
        std::print("{} : {}\n", name, score);
    }

    // sorted_unique flat_set
    std::vector<int> sorted_data = {1, 3, 5, 7, 9};
    std::flat_set<int> s(std::sorted_unique, std::move(sorted_data));
    for (int v : s) std::print("{} ", v);
    std::print("\n");

    // === keys/values/extract (lignes 181-194) ===
    std::print("\n=== keys/values/extract ===\n");
    std::flat_map<std::string, int> fm2 = {{"Alice", 95}, {"Bob", 87}, {"Clara", 92}};

    const auto& k = fm2.keys();
    const auto& v = fm2.values();

    std::print("Cles : ");
    for (const auto& key : k) std::print("{} ", key);
    std::print("\n");

    std::print("Valeurs : ");
    for (int val : v) std::print("{} ", val);
    std::print("\n");

    auto [extracted_keys, extracted_values] = std::move(fm2).extract();
    std::print("Extracted keys: ");
    for (const auto& ek : extracted_keys) std::print("{} ", ek);
    std::print("\n");
    std::print("fm2 apres extract, size: {}\n", fm2.size());
}
