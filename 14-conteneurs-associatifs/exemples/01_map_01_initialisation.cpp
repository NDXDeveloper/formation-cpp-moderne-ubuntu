/* ============================================================================
   Section 14.1 : Déclarer et initialiser un std::map
   Description : Initialisation par liste, vide+insertion, plage d'itérateurs
   Fichier source : 01-map-multimap.md
   ============================================================================ */
#include <map>
#include <string>
#include <vector>
#include <print>

int main() {
    // Initialisation par liste
    std::map<std::string, int> population {
        {"Paris",    2'161'000},
        {"Lyon",       522'969},
        {"Marseille",  873'076},
        {"Toulouse",   504'078}
    };
    // Les éléments sont automatiquement triés par clé (ordre alphabétique)
    for (const auto& [ville, pop] : population) {
        std::print("{} : {}\n", ville, pop);
    }

    std::println("---");

    // Initialisation vide puis insertion progressive
    std::map<int, std::string> codes_http;
    codes_http[200] = "OK";
    codes_http[404] = "Not Found";
    codes_http[500] = "Internal Server Error";
    for (const auto& [code, msg] : codes_http) {
        std::print("{} : {}\n", code, msg);
    }

    std::println("---");

    // Initialisation depuis une plage d'itérateurs
    std::vector<std::pair<std::string, double>> raw_data {
        {"alpha", 1.1}, {"gamma", 3.3}, {"beta", 2.2}
    };
    std::map<std::string, double> sorted_data(raw_data.begin(), raw_data.end());
    // sorted_data est trié : alpha, beta, gamma
    for (const auto& [k, v] : sorted_data) {
        std::print("{} : {}\n", k, v);
    }
}
