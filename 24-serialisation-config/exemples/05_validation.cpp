/* ============================================================================
   Section 24.5 : Bonnes pratiques : Validation de schémas
   Description : Les trois niveaux de validation (syntaxe, structure,
                 sémantique), ConfigValidator générique, application à
                 nlohmann/json, yaml-cpp et toml++, validation sémantique
                 (cohérence, formats), pipeline complet, mode strict/tolérant
   Fichier source : 05-validation-schemas.md
   ============================================================================ */
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>
#include <toml++/toml.hpp>
#include <print>
#include <string>
#include <vector>
#include <format>
#include <functional>
#include <optional>
#include <fstream>
#include <regex>

using json = nlohmann::json;
using namespace std::string_literals;

// === ServerConfig pour le pipeline ===
struct ServerConfig {
    std::string host;
    int port;
    int workers = 4;
    struct Tls {
        std::string cert_path;
        std::string key_path;
        bool verify_client = false;
    };
    std::optional<Tls> tls;
};

void from_json(const json& j, ServerConfig::Tls& t) {
    j.at("cert_path").get_to(t.cert_path);
    j.at("key_path").get_to(t.key_path);
    t.verify_client = j.value("verify_client", false);
}

void from_json(const json& j, ServerConfig& c) {
    j.at("host").get_to(c.host);
    j.at("port").get_to(c.port);
    c.workers = j.value("workers", 4);
    if (j.contains("tls") && !j["tls"].is_null()) {
        c.tls = j["tls"].get<ServerConfig::Tls>();
    }
}

// === ConfigValidator générique (l.242-335) ===
class ConfigValidator {
public:
    struct Error {
        std::string path;
        std::string message;
    };

    bool valid() const { return errors_.empty(); }
    const std::vector<Error>& errors() const { return errors_; }
    const std::vector<Error>& warnings() const { return warnings_; }

    ConfigValidator& require(bool condition,
                             const std::string& path,
                             const std::string& message) {
        if (!condition) {
            errors_.push_back({path, message});
        }
        return *this;
    }

    template <typename T>
    ConfigValidator& in_range(const std::string& path,
                              T value, T min, T max) {
        if (value < min || value > max) {
            errors_.push_back({path,
                std::format("doit être entre {} et {} (reçu : {})",
                    min, max, value)});
        }
        return *this;
    }

    ConfigValidator& not_empty(const std::string& path,
                               const std::string& value) {
        if (value.empty()) {
            errors_.push_back({path, "ne peut pas être vide"});
        }
        return *this;
    }

    ConfigValidator& one_of(const std::string& path,
                            const std::string& value,
                            std::initializer_list<std::string> allowed) {
        for (const auto& a : allowed) {
            if (value == a) return *this;
        }

        std::string joined;
        for (const auto& a : allowed) {
            if (!joined.empty()) joined += ", ";
            joined += "'" + a + "'";
        }
        errors_.push_back({path,
            std::format("doit être l'une de [{}] (reçu : '{}')",
                joined, value)});
        return *this;
    }

    ConfigValidator& check(const std::string& path,
                           std::function<bool()> predicate,
                           const std::string& message) {
        if (!predicate()) {
            errors_.push_back({path, message});
        }
        return *this;
    }

    ConfigValidator& warn(bool condition,
                          const std::string& path,
                          const std::string& message) {
        if (!condition) {
            warnings_.push_back({path, message});
        }
        return *this;
    }

    void report(const std::string& context = "") const {
        for (const auto& w : warnings_) {
            std::print(stderr, "  warning {} : {}\n", w.path, w.message);
        }
        if (!valid()) {
            std::string header = context.empty()
                ? "Validation échouée"
                : std::format("Validation échouée pour {}", context);
            std::print(stderr, "{} ({} erreur(s)) :\n",
                header, errors_.size());
            for (const auto& err : errors_) {
                std::print(stderr, "  - {} : {}\n", err.path, err.message);
            }
        }
    }

private:
    std::vector<Error> errors_;
    std::vector<Error> warnings_;
};

// === Application à nlohmann/json (l.340-373) ===
ConfigValidator validate_json_config(const json& j) {
    ConfigValidator v;

    v.require(j.is_object(), "/", "le document racine doit être un objet");
    if (!j.is_object()) return v;

    v.require(j.contains("server"), "server", "section obligatoire manquante");
    if (!j.contains("server")) return v;

    const auto& server = j["server"];
    v.require(server.contains("host"), "server.host", "champ obligatoire")
     .require(server.contains("port"), "server.port", "champ obligatoire");

    if (server.contains("host") && server["host"].is_string()) {
        v.not_empty("server.host", server["host"].get<std::string>());
    } else if (server.contains("host")) {
        v.require(false, "server.host", "doit être une chaîne");
    }

    if (server.contains("port") && server["port"].is_number_integer()) {
        v.in_range("server.port", server["port"].get<int>(), 1, 65535);
    } else if (server.contains("port")) {
        v.require(false, "server.port", "doit être un entier");
    }

    if (server.contains("log_level") && server["log_level"].is_string()) {
        v.one_of("server.log_level",
                 server["log_level"].get<std::string>(),
                 {"trace", "debug", "info", "warn", "error", "fatal"});
    }

    return v;
}

// === Application à yaml-cpp (l.378-410) ===
ConfigValidator validate_yaml_config(const YAML::Node& root) {
    ConfigValidator v;

    v.require(root.IsMap(), "/", "le document racine doit être une map");
    if (!root.IsMap()) return v;

    v.require(root["server"].IsDefined(), "server",
              "section obligatoire manquante");
    if (!root["server"]) return v;

    const auto& server = root["server"];

    v.require(server["host"].IsDefined(), "server.host",
              "champ obligatoire");
    v.require(server["port"].IsDefined(), "server.port",
              "champ obligatoire");

    if (server["host"] && server["host"].IsScalar()) {
        v.not_empty("server.host", server["host"].as<std::string>());
    }

    if (server["port"] && server["port"].IsScalar()) {
        try {
            int port = server["port"].as<int>();
            v.in_range("server.port", port, 1, 65535);
        } catch (const YAML::BadConversion&) {
            v.require(false, "server.port", "doit être un entier");
        }
    }

    return v;
}

// === Application à toml++ (l.416-441) ===
ConfigValidator validate_toml_config(const toml::table& config) {
    ConfigValidator v;

    v.require(config.contains("server"), "server",
              "section obligatoire manquante");
    if (!config.contains("server")) return v;

    auto* server = config["server"].as_table();
    v.require(server != nullptr, "server", "doit être une table");
    if (!server) return v;

    auto host = (*server)["host"].value<std::string>();
    auto port = (*server)["port"].value<int64_t>();

    v.require(host.has_value(), "server.host", "champ obligatoire");
    v.require(port.has_value(), "server.port", "champ obligatoire");

    if (host) {
        v.not_empty("server.host", *host);
    }
    if (port) {
        v.in_range<int64_t>("server.port", *port, 1, 65535);
    }

    return v;
}

// === Validation sémantique (l.454-512) ===
void validate_tls_consistency(ConfigValidator& v,
                              const ServerConfig& config) {
    if (config.port == 443 && !config.tls.has_value()) {
        v.require(false, "server",
            "le port 443 nécessite une configuration TLS");
    }

    if (config.tls.has_value()) {
        v.warn(config.port == 443 || config.port == 8443,
            "server.port",
            "un port TLS standard (443 ou 8443) est recommandé "
            "quand TLS est activé");
    }
}

void validate_formats(ConfigValidator& v, const ServerConfig& config) {
    v.check("server.host",
        [&]{
            static const std::regex host_re(
                R"(^(\d{1,3}\.){3}\d{1,3}$|^[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?)*$)");
            return std::regex_match(config.host, host_re);
        },
        std::format("'{}' n'est pas un hostname ou une IP valide",
            config.host));
}

// === Clés inconnues (l.587-604) ===
void reject_unknown_keys(ConfigValidator& v,
                         const json& node,
                         const std::vector<std::string>& known,
                         const std::string& context,
                         bool strict) {
    if (!node.is_object()) return;
    for (auto& [key, _] : node.items()) {
        bool is_known = false;
        for (const auto& k : known) {
            if (key == k) { is_known = true; break; }
        }
        if (!is_known) {
            if (strict) {
                v.require(false, context + "." + key, "clé inconnue");
            } else {
                v.warn(false, context + "." + key, "clé inconnue (ignorée)");
            }
        }
    }
}

// === Pipeline complet (l.524-571) ===
std::optional<ServerConfig> load_and_validate_config(
        const std::string& path) {

    json j;
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::print(stderr, "Fichier introuvable : {}\n", path);
            return std::nullopt;
        }
        j = json::parse(file);
    } catch (const json::parse_error& e) {
        std::print(stderr, "Syntaxe JSON invalide dans {} :\n  {}\n",
            path, e.what());
        return std::nullopt;
    }

    auto structural = validate_json_config(j);
    if (!structural.valid()) {
        structural.report(path);
        return std::nullopt;
    }

    ServerConfig config;
    try {
        config = j["server"].get<ServerConfig>();
    } catch (const json::exception& e) {
        std::print(stderr, "Erreur de conversion : {}\n", e.what());
        return std::nullopt;
    }

    ConfigValidator semantic;
    validate_tls_consistency(semantic, config);
    validate_formats(semantic, config);

    if (!semantic.valid()) {
        semantic.report(path);
        return std::nullopt;
    }

    // Afficher les warnings même si valide
    if (!semantic.warnings().empty()) {
        for (const auto& w : semantic.warnings()) {
            std::print(stderr, "  warning {} : {}\n", w.path, w.message);
        }
    }

    return config;
}

int main() {
    // === ConfigValidator — JSON ===
    std::print("=== Validation JSON ===\n");
    {
        // Valide
        json j = json::parse(R"({"server": {"host": "localhost", "port": 8080}})");
        auto v = validate_json_config(j);
        std::print("JSON valide : {}\n", v.valid());

        // Invalide
        json j2 = json::parse(R"({"server": {"port": "not_int"}})");
        auto v2 = validate_json_config(j2);
        std::print("JSON invalide : {} ({} erreur(s))\n", !v2.valid(), v2.errors().size());
        for (const auto& err : v2.errors()) {
            std::print("  - {} : {}\n", err.path, err.message);
        }

        // Port hors limites
        json j3 = json::parse(R"({"server": {"host": "localhost", "port": 99999}})");
        auto v3 = validate_json_config(j3);
        std::print("Port hors limites : {}\n", !v3.valid());

        // log_level invalide
        json j4 = json::parse(R"({"server": {"host": "localhost", "port": 8080, "log_level": "verbose"}})");
        auto v4 = validate_json_config(j4);
        std::print("log_level invalide : {}\n", !v4.valid());
        for (const auto& err : v4.errors()) {
            std::print("  - {} : {}\n", err.path, err.message);
        }
    }

    // === ConfigValidator — YAML ===
    std::print("\n=== Validation YAML ===\n");
    {
        YAML::Node root = YAML::Load(R"(
server:
  host: localhost
  port: 8080
)");
        auto v = validate_yaml_config(root);
        std::print("YAML valide : {}\n", v.valid());

        YAML::Node bad = YAML::Load("not_a_map");
        auto v2 = validate_yaml_config(bad);
        std::print("YAML non-map : {} ({} erreur(s))\n", !v2.valid(), v2.errors().size());
    }

    // === ConfigValidator — TOML ===
    std::print("\n=== Validation TOML ===\n");
    {
        toml::table config = toml::parse(R"(
[server]
host = "localhost"
port = 8080
)");
        auto v = validate_toml_config(config);
        std::print("TOML valide : {}\n", v.valid());

        toml::table bad = toml::parse(R"(
[server]
host = "localhost"
port = 99999
)");
        auto v2 = validate_toml_config(bad);
        std::print("TOML port hors limites : {}\n", !v2.valid());
    }

    // === Validation sémantique ===
    std::print("\n=== Validation sémantique ===\n");
    {
        // Port 443 sans TLS
        ServerConfig cfg1{.host = "localhost", .port = 443};
        ConfigValidator v1;
        validate_tls_consistency(v1, cfg1);
        std::print("443 sans TLS invalide : {}\n", !v1.valid());

        // Port non-standard avec TLS → warning
        ServerConfig cfg2{
            .host = "localhost", .port = 9090,
            .tls = ServerConfig::Tls{"/cert", "/key", false}
        };
        ConfigValidator v2;
        validate_tls_consistency(v2, cfg2);
        std::print("TLS port non-standard : valide={}, warnings={}\n",
                   v2.valid(), v2.warnings().size());

        // Format hostname
        ServerConfig cfg3{.host = "valid-host.example.com", .port = 8080};
        ConfigValidator v3;
        validate_formats(v3, cfg3);
        std::print("Hostname valide : {}\n", v3.valid());

        ServerConfig cfg4{.host = "invalid host!", .port = 8080};
        ConfigValidator v4;
        validate_formats(v4, cfg4);
        std::print("Hostname invalide : {}\n", !v4.valid());
    }

    // === Clés inconnues (strict/tolérant) ===
    std::print("\n=== Clés inconnues ===\n");
    {
        json j = json::parse(R"({"host": "localhost", "port": 8080, "tieout": 30})");

        // Mode tolérant
        ConfigValidator v1;
        reject_unknown_keys(v1, j, {"host", "port", "workers"}, "server", false);
        std::print("Tolérant : valide={}, warnings={}\n",
                   v1.valid(), v1.warnings().size());

        // Mode strict
        ConfigValidator v2;
        reject_unknown_keys(v2, j, {"host", "port", "workers"}, "server", true);
        std::print("Strict : valide={}, erreurs={}\n",
                   v2.valid(), v2.errors().size());
    }

    // === Pipeline complet ===
    std::print("\n=== Pipeline complet ===\n");
    {
        // Créer un fichier de test valide
        {
            std::ofstream f("test_pipeline.json");
            f << R"({"server": {"host": "0.0.0.0", "port": 8080, "workers": 4}})";
        }
        auto r1 = load_and_validate_config("test_pipeline.json");
        if (r1) {
            std::print("Pipeline OK : {}:{}\n", r1->host, r1->port);
        }
        std::remove("test_pipeline.json");

        // Fichier inexistant
        auto r2 = load_and_validate_config("nonexistent.json");
        std::print("Fichier manquant : {}\n", !r2.has_value());

        // Fichier invalide structurellement
        {
            std::ofstream f("test_invalid.json");
            f << R"({"server": {"port": "not_an_int"}})";
        }
        auto r3 = load_and_validate_config("test_invalid.json");
        std::print("Structure invalide : {}\n", !r3.has_value());
        std::remove("test_invalid.json");
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
