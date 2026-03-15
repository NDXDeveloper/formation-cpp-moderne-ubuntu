/* ============================================================================
   Section 24.3 : TOML : Alternative moderne (toml++)
   Description : Parsing fichier/chaîne, navigation, extraction typée,
                 value_or, at_path, itération tables/tableaux, tableaux de
                 tables, conversion types utilisateur, dates/heures natives,
                 écriture TOML, formateurs, validation
   Fichier source : 03-toml.md
   ============================================================================ */
#include <toml++/toml.hpp>
#include <print>
#include <string>
#include <vector>
#include <optional>
#include <format>
#include <fstream>
#include <sstream>

using namespace std::string_literals;

// === Types métier (l.371-385) ===
struct TlsConfig {
    std::string cert_path;
    std::string key_path;
    bool verify_client;
};

struct ServerConfig {
    std::string host;
    int port;
    int workers;
    bool debug;
    std::optional<TlsConfig> tls;
    std::vector<std::string> allowed_origins;
};

TlsConfig parse_tls(const toml::table& tbl) {
    return TlsConfig{
        .cert_path = tbl["cert_path"].value_or(""s),
        .key_path = tbl["key_path"].value_or(""s),
        .verify_client = tbl["verify_client"].value_or(false)
    };
}

ServerConfig parse_server_config(const toml::table& root) {
    ServerConfig cfg;
    cfg.host    = root.at_path("server.host").value_or("localhost"s);
    cfg.port    = static_cast<int>(
                      root.at_path("server.port").value_or(int64_t{8080}));
    cfg.workers = static_cast<int>(
                      root.at_path("server.workers").value_or(int64_t{4}));
    cfg.debug   = root.at_path("server.debug").value_or(false);

    if (auto* tls_tbl = root.at_path("server.tls").as_table()) {
        cfg.tls = parse_tls(*tls_tbl);
    }

    if (auto* origins = root.at_path("server.allowed_origins").as_array()) {
        for (const auto& item : *origins) {
            if (auto val = item.value<std::string>()) {
                cfg.allowed_origins.push_back(*val);
            }
        }
    }

    return cfg;
}

// === Validation (l.558-598) ===
struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;
    void add(std::string msg) {
        valid = false;
        errors.push_back(std::move(msg));
    }
};

ValidationResult validate_config(const toml::table& config) {
    ValidationResult v;

    if (!config.contains("server")) {
        v.add("Section [server] obligatoire manquante");
        return v;
    }

    auto* server = config["server"].as_table();
    if (!server) {
        v.add("[server] doit être une table");
        return v;
    }

    if (auto port = (*server)["port"].value<int64_t>()) {
        if (*port < 1 || *port > 65535) {
            v.add(std::format("server.port hors limites : {}", *port));
        }
    } else {
        v.add("server.port obligatoire (entier entre 1 et 65535)");
    }

    if (!(*server)["host"].value<std::string>()) {
        v.add("server.host obligatoire (chaîne)");
    }

    return v;
}

// === required<T> helper (l.436-448) ===
template <typename T>
T required(const toml::table& tbl, std::string_view key) {
    auto val = tbl[key].template value<T>();
    if (!val) {
        throw std::runtime_error(
            std::format("Champ obligatoire '{}' manquant ou type invalide", key));
    }
    return *val;
}

int main() {
    // === Parsing fichier (l.153-174) ===
    std::print("=== Parsing fichier ===\n");
    {
        try {
            toml::table config = toml::parse_file("config.toml");
            std::print("Type du nœud racine : table\n");
            std::print("Nombre de clés racine : {}\n", config.size());
        } catch (const toml::parse_error& err) {
            std::print(stderr, "Erreur de parsing TOML :\n  {}\n", err.what());
            return 1;
        }
    }

    // === Parsing chaîne (l.181-186) ===
    std::print("\n=== Parsing chaîne ===\n");
    {
        toml::table config = toml::parse(R"(
    [server]
    host = "localhost"
    port = 8080
)");
        auto host = config["server"]["host"].value<std::string>();
        if (host) std::print("host={}\n", *host);
    }

    // === Parsing flux (l.189-194) ===
    std::print("\n=== Parsing flux ===\n");
    {
        std::ifstream file("config.toml");
        toml::table config = toml::parse(file, "config.toml"s);
        auto host = config.at_path("server.host").value<std::string>();
        if (host) std::print("host={}\n", *host);
    }

    // === Navigation et extraction (l.244-283) ===
    std::print("\n=== Extraction typée ===\n");
    {
        toml::table config = toml::parse_file("config.toml");

        // value<T>() → optional
        std::optional<std::string> host = config["server"]["host"].value<std::string>();
        std::optional<int64_t> port = config["server"]["port"].value<int64_t>();
        if (host && port) {
            std::print("Serveur : {}:{}\n", *host, *port);
        }

        // value_or()
        std::string h = config["server"]["host"].value_or("localhost"s);
        int64_t p     = config["server"]["port"].value_or(int64_t{8080});
        int64_t w     = config["server"]["workers"].value_or(int64_t{4});
        bool debug    = config["server"]["debug"].value_or(false);
        std::print("host={}, port={}, workers={}, debug={}\n", h, p, w, debug);

        // at_path
        auto cert = config.at_path("server.tls.cert_path").value<std::string>();
        if (cert) std::print("cert_path={}\n", *cert);
    }

    // === Itération sur table (l.303-313) ===
    std::print("\n=== Itération table ===\n");
    {
        toml::table config = toml::parse_file("config.toml");
        for (auto& [key, value] : config) {
            std::print("{} (type={})\n",
                std::string_view{key},
                static_cast<int>(value.type()));
        }
    }

    // === Itération sur tableau (l.317-326) ===
    std::print("\n=== Itération tableau endpoints ===\n");
    {
        toml::table config = toml::parse_file("config.toml");
        if (auto* endpoints = config["server"]["endpoints"].as_array()) {
            for (const auto& ep : *endpoints) {
                if (const auto* tbl = ep.as_table()) {
                    std::string path = (*tbl)["path"].value_or(""s);
                    std::string method = (*tbl)["method"].value_or(""s);
                    std::print("{} {}\n", method, path);
                }
            }
        }
    }

    // === Conversion types utilisateur (l.395-430) ===
    std::print("\n=== ServerConfig ===\n");
    {
        toml::table config = toml::parse_file("config.toml");
        ServerConfig server = parse_server_config(config);
        std::print("{}:{} ({} workers)\n", server.host, server.port, server.workers);
        if (server.tls) {
            std::print("TLS : {}\n", server.tls->cert_path);
        }
    }

    // === required<T> helper (l.436-448) ===
    std::print("\n=== required<T> ===\n");
    {
        toml::table config = toml::parse(R"(
[server]
host = "localhost"
port = 8080
)");
        auto* server = config["server"].as_table();
        try {
            std::string host = required<std::string>(*server, "host");
            int64_t port = required<int64_t>(*server, "port");
            std::print("host={}, port={}\n", host, port);
        } catch (const std::exception& e) {
            std::print(stderr, "Erreur : {}\n", e.what());
        }

        // Test champ manquant
        try {
            std::string missing = required<std::string>(*server, "missing_key");
        } catch (const std::exception& e) {
            std::print("Erreur attendue : {}\n", e.what());
        }
    }

    // === Dates et heures (l.454-485) ===
    std::print("\n=== Dates et heures ===\n");
    {
        toml::table config = toml::parse_file("config.toml");

        auto created = config.at_path("deployment.created_at").value<toml::date_time>();
        auto release = config.at_path("deployment.release_date").value<toml::date>();
        auto backup  = config.at_path("deployment.backup_time").value<toml::time>();

        if (created) {
            std::print("Créé le {}-{:02}-{:02} à {:02}:{:02}:{:02}\n",
                created->date.year, created->date.month, created->date.day,
                created->time.hour, created->time.minute, created->time.second);
        }
        if (release) {
            std::print("Release : {}-{:02}-{:02}\n",
                release->year, release->month, release->day);
        }
        if (backup) {
            std::print("Backup quotidien à {:02}:{:02}\n",
                backup->hour, backup->minute);
        }
    }

    // === Écriture TOML (l.493-518) ===
    std::print("\n=== Écriture TOML ===\n");
    {
        toml::table config;
        config.insert("title", "Mon Application");

        toml::table server;
        server.insert("host", "0.0.0.0");
        server.insert("port", 8080);
        server.insert("debug", false);
        config.insert("server", std::move(server));

        toml::array ports;
        ports.push_back(8080);
        ports.push_back(8443);
        config.insert("ports", std::move(ports));

        config.insert("created", toml::date_time{
            toml::date{2026, 3, 14},
            toml::time{10, 30, 0}
        });

        // Formateurs via operator<<
        std::ostringstream oss;
        oss << toml::toml_formatter{config};
        std::print("TOML:\n{}\n", oss.str());

        // JSON formatter
        std::ostringstream oss_json;
        oss_json << toml::json_formatter{config};
        std::print("JSON:\n{}\n", oss_json.str());
    }

    // === Validation (l.558-598) ===
    std::print("\n=== Validation ===\n");
    {
        toml::table config = toml::parse_file("config.toml");
        auto v = validate_config(config);
        std::print("Config valide : {}\n", v.valid);

        // Test invalide
        toml::table bad = toml::parse(R"(
[server]
host = "localhost"
)");
        auto v2 = validate_config(bad);
        std::print("Config invalide : {} ({} erreur(s))\n", !v2.valid, v2.errors.size());
        for (const auto& err : v2.errors) {
            std::print("  - {}\n", err);
        }

        // Port hors limites
        toml::table bad2 = toml::parse(R"(
[server]
host = "localhost"
port = 99999
)");
        auto v3 = validate_config(bad2);
        std::print("Port hors limites : {}\n", !v3.valid);
    }

    // === Erreur de parsing (l.153-174) ===
    std::print("\n=== Erreur de parsing ===\n");
    {
        try {
            toml::table config = toml::parse("invalid = [toml");
        } catch (const toml::parse_error& err) {
            std::print(stderr, "Erreur TOML : {}\n", err.what());
            std::print(stderr, "  Ligne {}, colonne {}\n",
                err.source().begin.line, err.source().begin.column);
        }
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
