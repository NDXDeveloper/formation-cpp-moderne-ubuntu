/* ============================================================================
   Section 14.1 : Suppression d'éléments
   Description : erase (clé, itérateur, plage), extract (C++17), merge (C++17)
   Fichier source : 01-map-multimap.md
   ============================================================================ */
#include <map>
#include <string>
#include <print>

int main() {
    // erase par clé
    std::map<std::string, int> population {
        {"Paris", 2'161'000}, {"Lyon", 522'969},
        {"Marseille", 873'076}, {"Toulouse", 504'078}
    };
    std::size_t removed = population.erase("Toulouse");
    // removed == 1 si la clé existait, 0 sinon
    std::println("removed={}, size={}", removed, population.size());

    // erase par itérateur
    auto it = population.find("Lyon");
    if (it != population.end()) {
        population.erase(it); // O(1) amorti une fois l'itérateur obtenu
    }
    std::println("after erase Lyon: size={}", population.size());

    std::println("---");

    // erase par plage
    std::map<int, std::string> events {
        {1789, "Révolution française"},
        {1804, "Premier Empire"},
        {1848, "Deuxième République"},
        {1870, "Troisième République"},
        {1914, "Première Guerre mondiale"},
        {1939, "Seconde Guerre mondiale"},
        {1958, "Cinquième République"}
    };
    // Supprimer tous les événements avant 1900
    events.erase(events.begin(), events.lower_bound(1900));
    for (const auto& [y, e] : events) {
        std::print("{} : {}\n", y, e);
    }

    std::println("---");

    // extract (C++17) — retirer un nœud sans le détruire
    std::map<std::string, int> source {{"Alice", 95}, {"Bob", 87}};
    std::map<std::string, int> dest;

    auto node = source.extract("Alice");
    if (!node.empty()) {
        node.key() = "Alicia"; // On peut modifier la clé !
        dest.insert(std::move(node));
    }
    // source contient {"Bob", 87}
    // dest contient {"Alicia", 95}
    std::print("source: ");
    for (const auto& [k, v] : source) std::print("{}={} ", k, v);
    std::print("\ndest: ");
    for (const auto& [k, v] : dest) std::print("{}={} ", k, v);
    std::println("");

    std::println("---");

    // merge (C++17) — fusion de maps
    std::map<int, std::string> m1 {{1, "a"}, {3, "c"}, {5, "e"}};
    std::map<int, std::string> m2 {{2, "b"}, {3, "x"}, {4, "d"}};

    m1.merge(m2);
    // m1 contient {1:"a", 2:"b", 3:"c", 4:"d", 5:"e"}
    // m2 contient {3:"x"} — la clé 3 n'a pas pu être transférée (doublon)
    std::print("m1: ");
    for (const auto& [k, v] : m1) std::print("{}:{} ", k, v);
    std::print("\nm2: ");
    for (const auto& [k, v] : m2) std::print("{}:{} ", k, v);
    std::println("");
}
