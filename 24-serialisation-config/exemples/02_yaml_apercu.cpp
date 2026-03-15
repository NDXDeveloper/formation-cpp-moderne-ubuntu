/* ============================================================================
   Section 24.2 : YAML : Parsing avec yaml-cpp
   Description : Aperçu de l'API — chargement fichier, accès scalaires,
                 valeur par défaut, test d'existence, itération séquence,
                 YAML::convert<T>, gestion des erreurs
   Fichier source : 02-yaml-cpp.md
   ============================================================================ */
#include <yaml-cpp/yaml.h>
#include <print>
#include <string>
#include <vector>

// === Conversion types utilisateur : Endpoint (l.340-373) ===
struct Endpoint {
    std::string path;
    std::string method;
    int timeout_ms;
};

namespace YAML {
template <>
struct convert<Endpoint> {
    static Node encode(const Endpoint& e) {
        Node node;
        node["path"] = e.path;
        node["method"] = e.method;
        node["timeout_ms"] = e.timeout_ms;
        return node;
    }

    static bool decode(const Node& node, Endpoint& e) {
        if (!node.IsMap()) return false;
        if (!node["path"] || !node["method"]) return false;

        e.path = node["path"].as<std::string>();
        e.method = node["method"].as<std::string>();
        e.timeout_ms = node["timeout_ms"].as<int>(5000);
        return true;
    }
};
}  // namespace YAML

int main() {
    // === Chargement depuis un fichier (l.74-113) ===
    std::print("=== Chargement fichier ===\n");
    {
        YAML::Node config = YAML::LoadFile("config.yaml");

        std::string host = config["server"]["host"].as<std::string>();
        int port = config["server"]["port"].as<int>();
        std::print("Serveur : {}:{}\n", host, port);

        int workers = config["server"]["workers"].as<int>(4);
        std::print("Workers : {}\n", workers);

        if (config["database"]) {
            std::string db = config["database"]["name"].as<std::string>();
            std::print("Base de données : {}\n", db);
        }

        if (config["logging"]["outputs"]) {
            for (const auto& output : config["logging"]["outputs"]) {
                if (output.IsScalar()) {
                    std::print("Log output : {}\n", output.as<std::string>());
                } else if (output.IsMap()) {
                    for (const auto& kv : output) {
                        std::print("Log output : {} → {}\n",
                            kv.first.as<std::string>(),
                            kv.second.as<std::string>());
                    }
                }
            }
        }
    }

    // === YAML::convert<Endpoint> (l.376-391) ===
    std::print("\n=== convert<Endpoint> ===\n");
    {
        YAML::Node root = YAML::Load(R"(
api:
  path: /health
  method: GET
  timeout_ms: 1000
endpoints:
  - path: /health
    method: GET
  - path: /api/data
    method: POST
    timeout_ms: 3000
)");
        auto ep = root["api"].as<Endpoint>();
        std::print("{} {} ({}ms)\n", ep.method, ep.path, ep.timeout_ms);

        auto endpoints = root["endpoints"].as<std::vector<Endpoint>>();
        std::print("Endpoints : {}\n", endpoints.size());
        for (const auto& e : endpoints) {
            std::print("  {} {} ({}ms)\n", e.method, e.path, e.timeout_ms);
        }

        // Sérialisation
        Endpoint new_ep{"/metrics", "GET", 2000};
        YAML::Node node;
        node["endpoint"] = new_ep;
        std::print("Sérialisé : {}\n", YAML::Dump(node));
    }

    // === Gestion des erreurs (l.405-432) ===
    std::print("\n=== Gestion des erreurs ===\n");
    {
        // ParserException
        try {
            YAML::Node config = YAML::Load("{ invalid yaml: [}");
        } catch (const YAML::ParserException& e) {
            std::print(stderr, "Erreur de syntaxe YAML\n");
            std::print(stderr, "  Ligne   : {}\n", e.mark.line + 1);
            std::print(stderr, "  Colonne : {}\n", e.mark.column + 1);
        }

        // BadConversion
        YAML::Node config = YAML::Load("port: not_a_number");
        try {
            int port = config["port"].as<int>();
        } catch (const YAML::BadConversion& e) {
            std::print(stderr, "Conversion impossible : {}\n", e.what());
        }
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
