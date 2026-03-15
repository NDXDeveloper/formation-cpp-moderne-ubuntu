/* ============================================================================
   Section 24.1.4 : Gestion des erreurs de parsing
   Description : Hiérarchie des exceptions (parse_error, type_error,
                 out_of_range), stratégies de capture, parsing sans exception,
                 std::expected, messages enrichis, validation structurelle,
                 pattern complet de chargement robuste
   Fichier source : 01.4-gestion-erreurs.md
   ============================================================================ */
#include <nlohmann/json.hpp>
#include <print>
#include <string>
#include <vector>
#include <optional>
#include <expected>
#include <format>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cassert>

using json = nlohmann::json;
namespace fs = std::filesystem;

// === ServerConfig pour les exemples de chargement ===
struct ServerConfig {
    std::string host;
    int port;
    int workers = 4;
};

void to_json(json& j, const ServerConfig& c) {
    j = json{{"host", c.host}, {"port", c.port}, {"workers", c.workers}};
}
void from_json(const json& j, ServerConfig& c) {
    j.at("host").get_to(c.host);
    j.at("port").get_to(c.port);
    c.workers = j.value("workers", 4);
}

// === byte_to_position (l.306-325) ===
struct FilePosition {
    std::size_t line;
    std::size_t column;
};

FilePosition byte_to_position(const std::string& content, std::size_t byte_offset) {
    std::size_t line = 1;
    std::size_t col = 1;

    for (std::size_t i = 0; i < byte_offset && i < content.size(); ++i) {
        if (content[i] == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }
    }

    return {line, col};
}

// === show_error_context (l.343-363) ===
void show_error_context(const std::string& content,
                        const std::string& filepath,
                        const json::parse_error& e) {
    auto [line, col] = byte_to_position(content, e.byte);

    std::istringstream stream(content);
    std::string current_line;
    for (std::size_t i = 0; i < line; ++i) {
        std::getline(stream, current_line);
    }

    std::print(stderr, "{}:{}:{}: erreur de syntaxe JSON\n",
               filepath, line, col);
    std::print(stderr, "  {} | {}\n", line, current_line);

    std::string marker(col - 1, ' ');
    std::print(stderr, "  {} | {}^\n",
               std::string(std::to_string(line).size(), ' '), marker);
}

// === report_config_error (l.286-298) ===
void report_config_error(const std::string& filepath,
                         const json::parse_error& e) {
    std::print(stderr, "\n╔══ Erreur de configuration ══════════════════\n");
    std::print(stderr, "║ Fichier : {}\n", filepath);
    std::print(stderr, "║ Position: octet {}\n", e.byte);
    std::print(stderr, "║\n");
    std::print(stderr, "║ Le fichier contient une erreur de syntaxe JSON.\n");
    std::print(stderr, "║ Vérifiez les virgules, guillemets et accolades\n");
    std::print(stderr, "║ autour de la position indiquée.\n");
    std::print(stderr, "║\n");
    std::print(stderr, "║ Détail technique : {}\n", e.what());
    std::print(stderr, "╚═════════════════════════════════════════════\n\n");
}

// === ValidationResult (l.383-427) ===
struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;

    void add_error(std::string msg) {
        valid = false;
        errors.push_back(std::move(msg));
    }
};

ValidationResult validate_server_config(const json& j) {
    ValidationResult result;

    if (!j.is_object()) {
        result.add_error("Le document racine doit être un objet JSON");
        return result;
    }

    if (!j.contains("host")) {
        result.add_error("Champ obligatoire 'host' manquant");
    } else if (!j["host"].is_string()) {
        result.add_error("'host' doit être une chaîne de caractères");
    }

    if (!j.contains("port")) {
        result.add_error("Champ obligatoire 'port' manquant");
    } else if (!j["port"].is_number_integer()) {
        result.add_error("'port' doit être un entier");
    } else {
        int port = j["port"];
        if (port < 1 || port > 65535) {
            result.add_error(
                std::format("'port' doit être entre 1 et 65535 (reçu: {})", port));
        }
    }

    if (j.contains("workers") && !j["workers"].is_number_unsigned()) {
        result.add_error("'workers' doit être un entier positif");
    }

    return result;
}

// === std::expected pattern (l.211-273) ===
enum class ConfigError {
    file_not_found,
    syntax_error,
    missing_field,
    type_mismatch
};

struct ConfigErrorInfo {
    ConfigError code;
    std::string detail;
};

std::expected<ServerConfig, ConfigErrorInfo>
load_config_expected(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(ConfigErrorInfo{
            ConfigError::file_not_found,
            std::format("Fichier introuvable : {}", path)
        });
    }

    try {
        json j = json::parse(file);
        return j.get<ServerConfig>();

    } catch (const json::parse_error& e) {
        return std::unexpected(ConfigErrorInfo{
            ConfigError::syntax_error,
            std::format("Syntaxe invalide (octet {}) : {}", e.byte, e.what())
        });

    } catch (const json::out_of_range& e) {
        return std::unexpected(ConfigErrorInfo{
            ConfigError::missing_field, e.what()
        });

    } catch (const json::type_error& e) {
        return std::unexpected(ConfigErrorInfo{
            ConfigError::type_mismatch, e.what()
        });
    }
}

// === Pattern complet : chargement robuste (l.488-551) ===
std::optional<ServerConfig> load_server_config(const fs::path& config_path) {
    if (!fs::exists(config_path)) {
        std::print(stderr, "Configuration introuvable : {}\n",
                   config_path.string());
        return std::nullopt;
    }

    std::string content;
    {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            std::print(stderr, "Impossible de lire : {}\n",
                       config_path.string());
            return std::nullopt;
        }
        content.assign(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
    }

    json j;
    try {
        j = json::parse(content);
    } catch (const json::parse_error& e) {
        auto [line, col] = byte_to_position(content, e.byte);
        std::print(stderr, "{}:{}:{}: erreur de syntaxe JSON\n",
                   config_path.string(), line, col);
        std::print(stderr, "  {}\n", e.what());
        return std::nullopt;
    }

    auto validation = validate_server_config(j);
    if (!validation.valid) {
        std::print(stderr, "Configuration invalide ({}) :\n",
                   config_path.string());
        for (const auto& err : validation.errors) {
            std::print(stderr, "  - {}\n", err);
        }
        return std::nullopt;
    }

    try {
        return j.get<ServerConfig>();
    } catch (const json::exception& e) {
        std::print(stderr, "Erreur de conversion inattendue : {}\n",
                   e.what());
        return std::nullopt;
    }
}

int main() {
    // === parse_error (l.37-47) ===
    std::print("=== parse_error ===\n");
    {
        try {
            json j = json::parse(R"({"host": "localhost", "port": 8080)");
        } catch (const json::parse_error& e) {
            std::print(stderr, "Erreur de syntaxe JSON\n");
            std::print(stderr, "  ID       : {}\n", e.id);
            std::print(stderr, "  Position : octet {}\n", e.byte);
        }
    }

    // === type_error (l.67-87) ===
    std::print("\n=== type_error ===\n");
    {
        json j = json::parse(R"({"port": "not_a_number"})");
        try {
            int port = j["port"].get<int>();
        } catch (const json::type_error& e) {
            std::print(stderr, "Type inattendu : {}\n", e.what());
        }

        json j2 = json::parse(R"({"items": "not_an_array"})");
        try {
            auto items = j2["items"].get<std::vector<int>>();
        } catch (const json::type_error& e) {
            std::print(stderr, "Type inattendu : {}\n", e.what());
        }
    }

    // === out_of_range (l.95-114) ===
    std::print("\n=== out_of_range ===\n");
    {
        json j = json::parse(R"({"host": "localhost"})");
        try {
            std::string db = j.at("database").get<std::string>();
        } catch (const json::out_of_range& e) {
            std::print(stderr, "Champ manquant : {}\n", e.what());
        }

        json arr = json::parse(R"([10, 20, 30])");
        try {
            int val = arr.at(5);
        } catch (const json::out_of_range& e) {
            std::print(stderr, "Index invalide : {}\n", e.what());
        }
    }

    // === Capture globale (l.165-173) ===
    std::print("\n=== Capture globale json::exception ===\n");
    {
        try {
            json j = json::parse("invalid");
        } catch (const json::exception& e) {
            std::print(stderr, "Erreur JSON [{}] : {}\n", e.id, e.what());
        }
    }

    // === Parsing sans exception (l.182-202) ===
    std::print("\n=== Parsing sans exception ===\n");
    {
        json j = json::parse("not json at all", nullptr, false);
        if (j.is_discarded()) {
            std::print("Message JSON invalide, ignoré\n");
        }

        // Navigation sans exception
        json j2 = json::parse(R"({"event_type": "click", "priority": 5})", nullptr, false);
        if (!j2.is_discarded()) {
            if (j2.contains("event_type") && j2["event_type"].is_string()) {
                std::string event_type = j2["event_type"].get<std::string>();
                int priority = j2.value("priority", 0);
                std::string source = j2.value("source", "unknown");
                std::print("event={}, priority={}, source={}\n",
                           event_type, priority, source);
            }
        }
    }

    // === std::expected (l.211-273) ===
    std::print("\n=== std::expected ===\n");
    {
        auto result = load_config_expected("nonexistent.json");
        if (!result) {
            const auto& err = result.error();
            std::print(stderr, "[{}] {}\n",
                static_cast<int>(err.code), err.detail);
        }

        // Tester avec un fichier valide temporaire
        {
            std::ofstream tmp("test_expected.json");
            tmp << R"({"host": "localhost", "port": 8080})";
        }
        auto result2 = load_config_expected("test_expected.json");
        if (result2) {
            std::print("Config chargée : {}:{}\n", result2->host, result2->port);
        }
        fs::remove("test_expected.json");
    }

    // === byte_to_position + show_error_context ===
    std::print("\n=== byte_to_position + show_error_context ===\n");
    {
        std::string content = R"({
    "host": "localhost",
    "port": 8080
    "workers": 4
})";
        try {
            json j = json::parse(content);
        } catch (const json::parse_error& e) {
            auto [line, col] = byte_to_position(content, e.byte);
            std::print("Position : ligne {}, colonne {}\n", line, col);
            show_error_context(content, "inline", e);
        }
    }

    // === report_config_error ===
    std::print("\n=== report_config_error ===\n");
    {
        try {
            json j = json::parse(R"({"host": "localhost", )");
        } catch (const json::parse_error& e) {
            report_config_error("config.json", e);
        }
    }

    // === ValidationResult (l.383-447) ===
    std::print("\n=== Validation structurelle ===\n");
    {
        // Valide
        json j_ok = json::parse(R"({"host": "localhost", "port": 8080, "workers": 4})");
        auto v = validate_server_config(j_ok);
        std::print("Config valide : {}\n", v.valid);

        // Invalide : port manquant, workers incorrect
        json j_bad = json::parse(R"({"host": "localhost", "workers": -1})");
        auto v2 = validate_server_config(j_bad);
        std::print("Config invalide : {} ({} erreur(s))\n",
                   !v2.valid, v2.errors.size());
        for (const auto& err : v2.errors) {
            std::print("  - {}\n", err);
        }

        // Port hors limites
        json j_range = json::parse(R"({"host": "localhost", "port": 99999})");
        auto v3 = validate_server_config(j_range);
        std::print("Port hors limites : {}\n", !v3.valid);
        for (const auto& err : v3.errors) {
            std::print("  - {}\n", err);
        }
    }

    // === Pattern complet : chargement robuste (l.488-551) ===
    std::print("\n=== Chargement robuste ===\n");
    {
        // Fichier inexistant
        auto r1 = load_server_config("does_not_exist.json");
        std::print("Fichier inexistant : {}\n", !r1.has_value());

        // Fichier valide
        {
            std::ofstream tmp("test_robust.json");
            tmp << R"({"host": "0.0.0.0", "port": 443, "workers": 8})";
        }
        auto r2 = load_server_config("test_robust.json");
        if (r2) {
            std::print("Chargé : {}:{} ({} workers)\n",
                       r2->host, r2->port, r2->workers);
        }
        fs::remove("test_robust.json");

        // Fichier avec erreur de syntaxe
        {
            std::ofstream tmp("test_syntax.json");
            tmp << R"({"host": "localhost" "port": 8080})";
        }
        auto r3 = load_server_config("test_syntax.json");
        std::print("Syntaxe invalide : {}\n", !r3.has_value());
        fs::remove("test_syntax.json");

        // Fichier avec validation échouée
        {
            std::ofstream tmp("test_valid.json");
            tmp << R"({"port": "not_an_int"})";
        }
        auto r4 = load_server_config("test_valid.json");
        std::print("Validation échouée : {}\n", !r4.has_value());
        fs::remove("test_valid.json");
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
