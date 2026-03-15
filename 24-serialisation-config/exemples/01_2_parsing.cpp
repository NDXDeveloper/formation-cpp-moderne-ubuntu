/* ============================================================================
   Section 24.1.2 : Parsing de fichiers JSON
   Description : Parsing fichier/chaîne, navigation DOM, accès sécurisé,
                 extraction typée, itération, JSON Pointer, SAX, JSONC
   Fichier source : 01.2-parsing.md
   ============================================================================ */
#include <nlohmann/json.hpp>
#include <fstream>
#include <print>
#include <string>
#include <vector>
#include <sstream>

using json = nlohmann::json;

// === SAX handler (l.391-443) ===
struct StatsHandler : json::json_sax_t {
    int object_count = 0;
    int array_count = 0;
    int string_count = 0;
    int number_count = 0;

    bool null() override { return true; }
    bool boolean(bool) override { return true; }
    bool number_integer(json::number_integer_t) override {
        ++number_count; return true;
    }
    bool number_unsigned(json::number_unsigned_t) override {
        ++number_count; return true;
    }
    bool number_float(json::number_float_t, const json::string_t&) override {
        ++number_count; return true;
    }
    bool string(json::string_t&) override {
        ++string_count; return true;
    }
    bool binary(json::binary_t&) override { return true; }
    bool start_object(std::size_t) override {
        ++object_count; return true;
    }
    bool end_object() override { return true; }
    bool key(json::string_t&) override { return true; }
    bool start_array(std::size_t) override {
        ++array_count; return true;
    }
    bool end_array() override { return true; }
    bool parse_error(std::size_t position, const std::string& last_token,
                     const json::exception& ex) override {
        std::print(stderr, "Erreur parsing position {} : {}\n",
                   position, ex.what());
        return false;
    }
};

int main() {
    // === Parsing fichier (l.23-41) ===
    std::print("=== Parsing fichier ===\n");
    {
        std::ifstream file("config.json");
        if (!file.is_open()) {
            std::print(stderr, "Impossible d'ouvrir config.json\n");
            return 1;
        }
        json config = json::parse(file);
        std::print("Host : {}\n", config["server"]["host"].get<std::string>());
        std::print("Port : {}\n", config["server"]["port"].get<int>());
    }

    // === Parsing chaîne (l.58-69) ===
    std::print("\n=== Parsing chaîne ===\n");
    {
        std::string response = R"({
            "status": "success",
            "data": {
                "user_id": 42,
                "username": "alice",
                "roles": ["admin", "dev"]
            }
        })";
        json j = json::parse(response);
        std::print("user={}, roles={}\n",
                   j["data"]["username"].get<std::string>(),
                   j["data"]["roles"].size());
    }

    // === Navigation DOM (l.108-131) ===
    std::print("\n=== Navigation DOM ===\n");
    {
        json j = json::parse(R"({
            "database": {
                "host": "db.example.com",
                "port": 5432,
                "replicas": ["replica-1", "replica-2", "replica-3"]
            }
        })");
        std::string db_host = j["database"]["host"];
        std::string first_replica = j["database"]["replicas"][0];
        std::print("db_host={}, first_replica={}\n", db_host, first_replica);
    }

    // === Accès sécurisé .at() (l.140-145) ===
    std::print("\n=== Accès .at() ===\n");
    {
        json j = json::parse(R"({"database": {"host": "db.example.com"}})");
        try {
            std::string host = j.at("database").at("host");
            int port = j.at("database").at("port");  // n'existe pas
        } catch (const json::out_of_range& e) {
            std::print(stderr, "Champ manquant : {}\n", e.what());
        }
    }

    // === Accès .value() (l.155-159) ===
    std::print("\n=== Accès .value() ===\n");
    {
        json j = json::parse(R"({"database": {"host": "db.example.com"}})");
        int timeout = j["database"].value("timeout", 30);
        std::string host = j["database"].value("host", "localhost");
        std::print("timeout={}, host={}\n", timeout, host);
    }

    // === Extraction typée (l.186-200) ===
    std::print("\n=== Extraction typée ===\n");
    {
        json j = json::parse(R"({
            "name": "monitoring-agent",
            "version": 3,
            "latency_ms": 12.5,
            "active": true,
            "tags": ["infra", "prod"],
            "metadata": null
        })");
        std::string name = j["name"].get<std::string>();
        int version      = j["version"].get<int>();
        double latency   = j["latency_ms"].get<double>();
        bool active      = j["active"].get<bool>();
        auto tags        = j["tags"].get<std::vector<std::string>>();
        std::print("name={}, v={}, lat={}, active={}, tags={}\n",
                   name, version, latency, active, tags.size());
    }

    // === Itération objet (l.229-241) ===
    std::print("\n=== Itération objet ===\n");
    {
        json server = json::parse(R"({
            "host": "0.0.0.0",
            "port": 8080,
            "workers": 4
        })");
        for (auto& [key, value] : server.items()) {
            std::print("{} → {}\n", key, value.dump());
        }
    }

    // === Itération tableau (l.248-252) ===
    std::print("\n=== Itération tableau ===\n");
    {
        json replicas = json::parse(R"(["replica-1", "replica-2", "replica-3"])");
        for (const auto& replica : replicas) {
            std::print("Replica : {}\n", replica.get<std::string>());
        }
    }

    // === flatten (l.268-280) ===
    std::print("\n=== flatten ===\n");
    {
        json nested = json::parse(R"({
            "server": {
                "host": "0.0.0.0",
                "ports": [8080, 8443]
            }
        })");
        for (auto& [pointer, value] : nested.flatten().items()) {
            std::print("{} = {}\n", pointer, value.dump());
        }
    }

    // === JSON Pointer (l.290-306) ===
    std::print("\n=== JSON Pointer ===\n");
    {
        json config = json::parse(R"({
            "services": {
                "api": {
                    "endpoints": [
                        {"path": "/health", "method": "GET"},
                        {"path": "/data",   "method": "POST"}
                    ]
                }
            }
        })");
        std::string method = config["/services/api/endpoints/1/method"_json_pointer];
        std::string same = config["services"]["api"]["endpoints"][1]["method"];
        std::print("method={}, same={}\n", method, same);
    }

    // === Parsing strict vs tolérant (l.319-348) ===
    std::print("\n=== Parsing strict/tolérant ===\n");
    {
        try {
            json j = json::parse("{ invalid json }");
        } catch (const json::parse_error& e) {
            std::print(stderr, "Erreur de parsing : position octet {}\n", e.byte);
        }

        std::string input = "not json at all";
        json j3 = json::parse(input, nullptr, false);
        if (j3.is_discarded()) {
            std::print("JSON invalide (sans exception), document ignoré\n");
        }
    }

    // === Callback de parsing (l.358-372) ===
    std::print("\n=== Callback de parsing ===\n");
    {
        auto filter = [](int depth, json::parse_event_t event, json& parsed) {
            if (event == json::parse_event_t::key) {
                const std::string& key = parsed.get<std::string>();
                if (!key.empty() && key[0] == '_') {
                    return false;
                }
            }
            return true;
        };
        std::string input = R"({"name": "app", "_internal": "secret", "version": 2})";
        json j = json::parse(input, filter);
        std::print("Filtré : {}\n", j.dump());
    }

    // === SAX handler (l.448-459) ===
    std::print("\n=== SAX handler ===\n");
    {
        std::string data = R"({"a": 1, "b": [2, 3], "c": {"d": "hello"}})";
        std::istringstream stream(data);
        StatsHandler handler;
        bool success = json::sax_parse(stream, &handler);
        if (success) {
            std::print("Objets={}, Tableaux={}, Chaînes={}, Nombres={}\n",
                       handler.object_count, handler.array_count,
                       handler.string_count, handler.number_count);
        }
    }

    // === Unicode (l.483-494) ===
    std::print("\n=== Unicode ===\n");
    {
        json j = json::parse(R"({"ville": "Héricourt-en-Caux", "emoji": "🚀"})");
        std::string ville = j["ville"];
        std::string emoji = j["emoji"];
        std::print("ville={}, emoji={}\n", ville, emoji);

        json j2 = json::parse(R"({"text": "\u00e9t\u00e9"})");
        std::string text = j2["text"];
        std::print("text={}\n", text);
    }

    // === JSONC - commentaires (l.507-527) ===
    std::print("\n=== JSONC commentaires ===\n");
    {
        std::string jsonc = R"({
            // Serveur principal
            "host": "0.0.0.0",
            "port": 8080,      /* port HTTP */
            "features": [
                "logging",
                "metrics"
            ]
        })";
        json j = json::parse(jsonc, nullptr, true, true);
        std::print("host={}, features={}\n",
                   j["host"].get<std::string>(), j["features"].size());
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
