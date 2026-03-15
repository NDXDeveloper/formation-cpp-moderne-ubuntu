/* ============================================================================
   Section 24.1 : JSON - Lecture/Écriture avec nlohmann/json
   Description : Aperçu de l'API — création, accès, itération, conversion,
                 parsing et dump
   Fichier source : 01-json-nlohmann.md
   ============================================================================ */
#include <nlohmann/json.hpp>
#include <print>
#include <string>
#include <vector>
#include <map>
#include <fstream>

using json = nlohmann::json;

// === Conversion types utilisateur (l.115-129) ===
struct ServerConfig {
    std::string host;
    int port;
    bool tls;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServerConfig, host, port, tls)

int main() {
    // === Création directe (l.23-46) ===
    std::print("=== Création directe ===\n");
    json config = {
        {"server", {
            {"host", "0.0.0.0"},
            {"port", 8080},
            {"tls", true}
        }},
        {"workers", 4},
        {"tags", {"production", "eu-west"}}
    };

    std::string host = config["server"]["host"];
    int port = config["server"]["port"];
    std::print("host={}, port={}\n", host, port);

    for (auto& [key, value] : config["server"].items()) {
        std::print("{} = {}\n", key, value.dump());
    }

    // === Accès (l.76-94) ===
    std::print("\n=== Accès ===\n");
    int timeout = config.value("timeout", 30);
    std::print("timeout (défaut) = {}\n", timeout);

    if (config.contains("workers")) {
        int w = config["workers"];
        std::print("workers = {}\n", w);
    }

    try {
        auto val = config.at("nonexistent");
    } catch (const json::out_of_range& e) {
        std::print(stderr, "Clé manquante : {}\n", e.what());
    }

    // === Conversion C++ <-> JSON (l.100-111) ===
    std::print("\n=== Conversion conteneurs ===\n");
    std::vector<int> scores = {95, 87, 72, 100};
    json j_scores = scores;

    std::map<std::string, double> prices = {{"BTC", 68500.0}, {"ETH", 3800.0}};
    json j_prices = prices;

    auto restored_scores = j_scores.get<std::vector<int>>();
    auto restored_prices = j_prices.get<std::map<std::string, double>>();
    std::print("scores: {} éléments, prices: {} éléments\n",
               restored_scores.size(), restored_prices.size());

    // === Conversion types utilisateur (l.115-129) ===
    std::print("\n=== Conversion ServerConfig ===\n");
    ServerConfig cfg{"localhost", 443, true};
    json j = cfg;
    std::print("Sérialisé : {}\n", j.dump());

    auto cfg2 = j.get<ServerConfig>();
    std::print("Désérialisé : {}:{} tls={}\n", cfg2.host, cfg2.port, cfg2.tls);

    // === Parsing et dump (l.137-158) ===
    std::print("\n=== Parsing et dump ===\n");
    json j1 = json::parse(R"({"status": "ok", "code": 200})");
    std::print("status={}, code={}\n",
               j1["status"].get<std::string>(), j1["code"].get<int>());

    // Parsing sans exception
    std::string input = "invalid json{";
    json j3 = json::parse(input, nullptr, false);
    if (j3.is_discarded()) {
        std::print(stderr, "JSON invalide\n");
    }

    // dump
    std::string compact = j1.dump();
    std::string pretty = j1.dump(4);
    std::print("compact: {}\n", compact);
    std::print("pretty:\n{}\n", pretty);
}
