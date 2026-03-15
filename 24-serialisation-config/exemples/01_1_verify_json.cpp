/* ============================================================================
   Section 24.1.1 : Installation et intégration
   Description : Vérification de l'installation nlohmann/json — création,
                 sérialisation et parsing
   Fichier source : 01.1-installation.md
   ============================================================================ */
// verify_json.cpp
#include <nlohmann/json.hpp>
#include <print>

using json = nlohmann::json;

int main() {
    // Création
    json j = {
        {"library", "nlohmann/json"},
        {"version", 3},
        {"header_only", true}
    };

    // Sérialisation
    std::print("JSON généré :\n{}\n", j.dump(2));

    // Parsing
    auto parsed = json::parse(R"({"status": "ok", "code": 200})");
    std::print("Status : {}\n", parsed["status"].get<std::string>());
    std::print("Code   : {}\n", parsed["code"].get<int>());

    return 0;
}
