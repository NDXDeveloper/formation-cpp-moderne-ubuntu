/* ============================================================================
   Section 14.1 : Insertion d'éléments
   Description : operator[], insert, insert_or_assign, emplace, try_emplace
   Fichier source : 01-map-multimap.md
   ============================================================================ */
#include <map>
#include <string>
#include <vector>
#include <print>

int main() {
    // operator[] — accès ou création implicite
    std::map<std::string, int> scores;
    scores["Alice"] = 95;   // Crée la paire {"Alice", 95}
    scores["Alice"] = 100;  // Écrase la valeur existante
    int val = scores["Bob"]; // Attention : crée {"Bob", 0} si "Bob" n'existe pas !
    std::println("Alice={}, Bob={}, size={}", scores["Alice"], val, scores.size());

    std::println("---");

    // insert — insertion sans écrasement
    std::map<std::string, int> config;
    auto [it, success] = config.insert({"timeout", 30});
    // success == true : la paire a été insérée
    std::println("insert timeout: success={}", success);

    auto [it2, success2] = config.insert({"timeout", 60});
    // success2 == false : "timeout" existait déjà, la valeur 30 est conservée
    std::println("insert timeout again: success={}, value={}", success2, it2->second);

    std::println("---");

    // insert_or_assign (C++17) — insert ou mise à jour explicite
    auto [it3, inserted] = config.insert_or_assign("timeout", 60);
    // Si "timeout" existe : met à jour la valeur → 60, inserted == false
    std::println("insert_or_assign: inserted={}, value={}", inserted, it3->second);

    std::println("---");

    // emplace — construction en place
    std::map<std::string, std::vector<int>> data;
    data.emplace("mesures", std::vector<int>{10, 20, 30});
    std::println("mesures size={}", data["mesures"].size());

    std::println("---");

    // try_emplace (C++17) — emplace sans effet de bord
    std::map<std::string, std::string> cache;
    cache.try_emplace("key1", "valeur_couteuse_a_construire");
    cache.try_emplace("key1", "autre_valeur"); // ne fait rien
    std::println("key1={}", cache["key1"]);
}
