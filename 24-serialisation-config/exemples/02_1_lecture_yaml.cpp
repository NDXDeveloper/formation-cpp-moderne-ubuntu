/* ============================================================================
   Section 24.2.1 : Lecture de fichiers de configuration
   Description : Chargement fichier/chaîne, multi-documents, navigation DOM,
                 accès par index, test de type, extraction typée avec défaut,
                 extraction conteneurs, YAML::convert<T> complet (TlsConfig,
                 ServerConfig), itération map/séquence/récursive, ancres,
                 validation structurelle, détection clés inconnues
   Fichier source : 02.1-lecture-config.md
   ============================================================================ */
#include <yaml-cpp/yaml.h>
#include <print>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <format>
#include <filesystem>

// === Types métier (l.242-254) ===
struct TlsConfig {
    std::string cert_path;
    std::string key_path;
    bool verify_client;
};

struct ServerConfig {
    std::string host;
    int port;
    int workers;
    std::optional<TlsConfig> tls;
    std::vector<std::string> allowed_origins;
};

namespace YAML {
template <>
struct convert<TlsConfig> {
    static bool decode(const Node& node, TlsConfig& t) {
        if (!node.IsMap()) return false;
        if (!node["cert_path"] || !node["key_path"]) return false;
        t.cert_path = node["cert_path"].as<std::string>();
        t.key_path = node["key_path"].as<std::string>();
        t.verify_client = node["verify_client"].as<bool>(false);
        return true;
    }
    static Node encode(const TlsConfig& t) {
        Node node;
        node["cert_path"] = t.cert_path;
        node["key_path"] = t.key_path;
        node["verify_client"] = t.verify_client;
        return node;
    }
};

template <>
struct convert<ServerConfig> {
    static bool decode(const Node& node, ServerConfig& s) {
        if (!node.IsMap()) return false;
        if (!node["host"] || !node["port"]) return false;
        s.host = node["host"].as<std::string>();
        s.port = node["port"].as<int>();
        s.workers = node["workers"].as<int>(4);
        if (node["tls"] && !node["tls"].IsNull()) {
            s.tls = node["tls"].as<TlsConfig>();
        } else {
            s.tls = std::nullopt;
        }
        if (node["allowed_origins"]) {
            s.allowed_origins =
                node["allowed_origins"].as<std::vector<std::string>>();
        }
        return true;
    }
    static Node encode(const ServerConfig& s) {
        Node node;
        node["host"] = s.host;
        node["port"] = s.port;
        node["workers"] = s.workers;
        if (s.tls.has_value()) {
            node["tls"] = *s.tls;
        }
        node["allowed_origins"] = s.allowed_origins;
        return node;
    }
};
}  // namespace YAML

// === Itération récursive (l.399-431) ===
void dump_node(const YAML::Node& node, int depth = 0) {
    std::string indent(depth * 2, ' ');
    switch (node.Type()) {
        case YAML::NodeType::Scalar:
            std::print("{}(scalar) {}\n", indent, node.as<std::string>());
            break;
        case YAML::NodeType::Sequence:
            std::print("{}(sequence, {} éléments)\n", indent, node.size());
            for (const auto& item : node) {
                dump_node(item, depth + 1);
            }
            break;
        case YAML::NodeType::Map:
            std::print("{}(map, {} clés)\n", indent, node.size());
            for (const auto& kv : node) {
                std::print("{}  clé: {}\n", indent,
                    kv.first.as<std::string>());
                dump_node(kv.second, depth + 1);
            }
            break;
        case YAML::NodeType::Null:
            std::print("{}(null)\n", indent);
            break;
        case YAML::NodeType::Undefined:
            std::print("{}(undefined)\n", indent);
            break;
    }
}

// === Validation structurelle (l.542-615) ===
struct ConfigValidation {
    bool valid = true;
    std::vector<std::string> errors;

    void require_key(const YAML::Node& parent, const std::string& key,
                     const std::string& context) {
        if (!parent[key] || parent[key].IsNull()) {
            valid = false;
            errors.push_back(
                std::format("Champ obligatoire '{}' manquant dans {}", key, context));
        }
    }

    void require_type(const YAML::Node& node, const std::string& name,
                      YAML::NodeType::value expected) {
        if (!node.IsDefined()) return;
        bool type_ok = false;
        std::string expected_name;
        switch (expected) {
            case YAML::NodeType::Scalar:
                type_ok = node.IsScalar();
                expected_name = "scalaire";
                break;
            case YAML::NodeType::Sequence:
                type_ok = node.IsSequence();
                expected_name = "séquence";
                break;
            case YAML::NodeType::Map:
                type_ok = node.IsMap();
                expected_name = "map";
                break;
            default: break;
        }
        if (!type_ok) {
            valid = false;
            errors.push_back(
                std::format("'{}' doit être de type {} (reçu: {})",
                    name, expected_name, static_cast<int>(node.Type())));
        }
    }
};

// === Détection clés inconnues (l.623-644) ===
void warn_unknown_keys(const YAML::Node& node,
                       const std::vector<std::string>& known_keys,
                       const std::string& context) {
    if (!node.IsMap()) return;
    for (const auto& kv : node) {
        std::string key = kv.first.as<std::string>();
        bool found = false;
        for (const auto& known : known_keys) {
            if (key == known) { found = true; break; }
        }
        if (!found) {
            std::print(stderr, "Clé inconnue '{}' dans {} (ignorée)\n",
                key, context);
        }
    }
}

int main() {
    // === Chargement simple (l.23-33) ===
    std::print("=== Chargement fichier ===\n");
    {
        YAML::Node config = YAML::LoadFile("config.yaml");
        std::print("Type du nœud racine : {}\n",
            config.IsMap() ? "map" : "autre");
        std::print("Nombre de clés racine : {}\n", config.size());
    }

    // === Chargement depuis chaîne (l.57-66) ===
    std::print("\n=== Chargement chaîne ===\n");
    {
        std::string yaml_content = R"(
server:
  host: localhost
  port: 8080
)";
        YAML::Node config = YAML::Load(yaml_content);
        std::string host = config["server"]["host"].as<std::string>();
        std::print("host={}\n", host);
    }

    // === Multi-documents (l.70-92) ===
    std::print("\n=== Multi-documents ===\n");
    {
        std::string yaml_multi = R"(---
kind: Deployment
metadata:
  name: api
---
kind: Service
metadata:
  name: api-svc
)";
        std::vector<YAML::Node> docs = YAML::LoadAll(yaml_multi);
        std::print("Nombre de documents : {}\n", docs.size());
        for (std::size_t i = 0; i < docs.size(); ++i) {
            std::string kind = docs[i]["kind"].as<std::string>("Unknown");
            std::string name = docs[i]["metadata"]["name"].as<std::string>("unnamed");
            std::print("Document {} : {} / {}\n", i + 1, kind, name);
        }
    }

    // === Navigation (l.100-125) ===
    std::print("\n=== Navigation ===\n");
    {
        YAML::Node config = YAML::LoadFile("config.yaml");
        YAML::Node db_node = config["database"]["host"];
        YAML::Node missing = config["nonexistent"];
        std::print("Existe : {}\n", missing.IsDefined() ? "oui" : "non");

        // Accès avec défaut
        std::string host = config["database"]["host"].as<std::string>("default");
        std::print("db_host={}\n", host);
    }

    // === Accès par index (l.131-144) ===
    std::print("\n=== Accès par index ===\n");
    {
        YAML::Node config = YAML::Load(R"(
ports:
  - 8080
  - 8443
  - 9090
)");
        for (std::size_t i = 0; i < config["ports"].size(); ++i) {
            std::print("Port {} : {}\n", i, config["ports"][i].as<int>());
        }
    }

    // === Test de type (l.152-164) ===
    std::print("\n=== Test de type ===\n");
    {
        YAML::Node config = YAML::Load(R"(
server:
  port: 8080
  tags:
    - prod
    - eu
  empty: ~
)");
        YAML::Node port_node = config["server"]["port"];
        if (port_node.IsScalar()) std::print("port est un scalaire\n");

        YAML::Node tags_node = config["server"]["tags"];
        if (tags_node.IsSequence()) std::print("tags est une séquence ({} éléments)\n", tags_node.size());

        YAML::Node empty_node = config["server"]["empty"];
        if (empty_node.IsNull()) std::print("empty est null\n");

        YAML::Node undef = config["server"]["nope"];
        if (!undef.IsDefined()) std::print("nope n'est pas défini\n");
    }

    // === Extraction typée (l.172-202) ===
    std::print("\n=== Extraction typée ===\n");
    {
        YAML::Node config = YAML::Load(R"(
app:
  name: monitoring-agent
  version: 3
  sampling_rate: 0.75
  debug: true
  description: ~
)");
        std::string name = config["app"]["name"].as<std::string>();
        int version = config["app"]["version"].as<int>();
        double rate = config["app"]["sampling_rate"].as<double>();
        bool debug = config["app"]["debug"].as<bool>();
        std::print("name={}, version={}, rate={}, debug={}\n",
                   name, version, rate, debug);

        // Avec valeur par défaut
        int port = config["app"]["port"].as<int>(8080);
        std::string log_level = config["app"]["log_level"].as<std::string>("info");
        std::print("port={}, log_level={}\n", port, log_level);
    }

    // === Extraction conteneurs (l.208-234) ===
    std::print("\n=== Extraction conteneurs ===\n");
    {
        YAML::Node config = YAML::Load(R"(
allowed_origins:
  - https://app.example.com
  - https://admin.example.com
  - https://localhost:3000
port_range:
  - 8000
  - 8100
tags:
  env: production
  region: eu-west
  team: platform
)");
        auto origins = config["allowed_origins"].as<std::vector<std::string>>();
        auto ports = config["port_range"].as<std::vector<int>>();
        auto tags = config["tags"].as<std::map<std::string, std::string>>();
        std::print("origins={}, ports={}, tags={}\n",
                   origins.size(), ports.size(), tags.size());
    }

    // === Extraction types utilisateur (l.241-343) ===
    std::print("\n=== ServerConfig ===\n");
    {
        YAML::Node root = YAML::Load(R"(
server:
  host: 0.0.0.0
  port: 443
  workers: 8
  tls:
    cert_path: /etc/ssl/server.crt
    key_path: /etc/ssl/server.key
    verify_client: true
  allowed_origins:
    - https://app.example.com
    - https://admin.example.com
)");
        auto server = root["server"].as<ServerConfig>();
        std::print("{}:{} ({} workers)\n", server.host, server.port, server.workers);
        if (server.tls) {
            std::print("TLS actif : {}\n", server.tls->cert_path);
        }
        std::print("Origins : {}\n", server.allowed_origins.size());
    }

    // === Itération map (l.354-367) ===
    std::print("\n=== Itération map ===\n");
    {
        YAML::Node config = YAML::Load(R"(
environment:
  APP_NAME: my-service
  APP_PORT: "8080"
  APP_DEBUG: "false"
  DB_HOST: db.internal
)");
        for (const auto& kv : config["environment"]) {
            std::string key = kv.first.as<std::string>();
            std::string value = kv.second.as<std::string>();
            std::print("{} = {}\n", key, value);
        }
    }

    // === Itération séquence (l.377-392) ===
    std::print("\n=== Itération séquence ===\n");
    {
        YAML::Node config = YAML::Load(R"(
services:
  - name: api
    port: 8080
  - name: worker
    port: 9090
  - name: scheduler
    port: 7070
)");
        for (const auto& service : config["services"]) {
            std::string name = service["name"].as<std::string>();
            int port = service["port"].as<int>();
            std::print("Service {} sur le port {}\n", name, port);
        }
    }

    // === Itération récursive (l.398-431) ===
    std::print("\n=== Itération récursive ===\n");
    {
        YAML::Node small = YAML::Load(R"(
name: test
items:
  - alpha
  - beta
)");
        dump_node(small);
    }

    // === Ancres et alias (l.440-465) ===
    std::print("\n=== Ancres et alias ===\n");
    {
        YAML::Node config = YAML::Load(R"(
defaults: &defaults
  timeout: 30
  retries: 3
  circuit_breaker:
    threshold: 5
    reset_after: 60
services:
  api: *defaults
  worker: *defaults
)");
        int api_timeout = config["services"]["api"]["timeout"].as<int>();
        int worker_retries = config["services"]["worker"]["retries"].as<int>();
        std::print("api_timeout={}, worker_retries={}\n", api_timeout, worker_retries);
    }

    // === Validation structurelle (l.542-615) ===
    std::print("\n=== Validation structurelle ===\n");
    {
        YAML::Node root = YAML::Load(R"(
server:
  host: 0.0.0.0
  port: 8080
)");
        ConfigValidation v;
        v.require_key(root, "server", "racine");
        if (root["server"]) {
            const auto& server = root["server"];
            v.require_type(server, "server", YAML::NodeType::Map);
            v.require_key(server, "host", "server");
            v.require_key(server, "port", "server");
        }
        std::print("Validation OK : {}\n", v.valid);

        // Invalide
        YAML::Node bad = YAML::Load("server: missing_data");
        ConfigValidation v2;
        v2.require_key(bad, "server", "racine");
        v2.require_type(bad["server"], "server", YAML::NodeType::Map);
        std::print("Validation KO : {} ({} erreur(s))\n", !v2.valid, v2.errors.size());
    }

    // === Détection clés inconnues (l.623-644) ===
    std::print("\n=== Détection clés inconnues ===\n");
    {
        YAML::Node config = YAML::Load(R"(
server:
  host: localhost
  port: 8080
  tieout: 30
)");
        warn_unknown_keys(config["server"],
            {"host", "port", "workers", "tls", "allowed_origins"},
            "server");
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
