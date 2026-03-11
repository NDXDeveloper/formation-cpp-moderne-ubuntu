/* ============================================================================
   Section 17.5 : Alternatives modernes : std::expected (C++23), codes d'erreur
   Description : std::expected construction et inspection, value_or,
                 interface monadique (and_then, transform, or_else),
                 std::expected<void, E>, combinaison expected/exceptions,
                 comparaison avec std::optional et std::error_code
   Fichier source : 05-alternatives-modernes.md
   ============================================================================ */

#include <expected>
#include <optional>
#include <print>
#include <string>
#include <fstream>
#include <sstream>
#include <functional>
#include <system_error>
#include <cerrno>
#include <cstdio>

// === Enum d'erreur pour les exemples ===
enum class ConfigError {
    fichier_introuvable,
    permission_refusee,
    format_invalide,
    cle_manquante
};

std::string to_string(ConfigError e) {
    switch (e) {
        case ConfigError::fichier_introuvable: return "fichier introuvable";
        case ConfigError::permission_refusee:  return "permission refusée";
        case ConfigError::format_invalide:     return "format invalide";
        case ConfigError::cle_manquante:       return "clé manquante";
    }
    return "inconnu";
}

// === std::expected : construction et inspection ===
std::expected<std::string, ConfigError> lire_config(const std::string& chemin) {
    std::ifstream f(chemin);
    if (!f.is_open()) {
        return std::unexpected(ConfigError::fichier_introuvable);
    }

    std::ostringstream ss;
    ss << f.rdbuf();
    std::string contenu = ss.str();

    if (contenu.empty()) {
        return std::unexpected(ConfigError::format_invalide);
    }

    return contenu;
}

// === Pipeline avec l'interface monadique ===
enum class PipelineError {
    fichier_introuvable,
    format_invalide,
    port_hors_limites
};

std::string to_string(PipelineError e) {
    switch (e) {
        case PipelineError::fichier_introuvable: return "fichier introuvable";
        case PipelineError::format_invalide:     return "format invalide";
        case PipelineError::port_hors_limites:   return "port hors limites";
    }
    return "inconnu";
}

struct Config {
    int port;
    std::string log_level;
};

std::expected<std::string, PipelineError> lire_fichier(const std::string& chemin) {
    std::ifstream f(chemin);
    if (!f.is_open()) return std::unexpected(PipelineError::fichier_introuvable);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::expected<Config, PipelineError> parser(const std::string& contenu) {
    // Parsing simplifié : on cherche "port:" et "log_level:"
    auto pos_port = contenu.find("port:");
    if (pos_port == std::string::npos)
        return std::unexpected(PipelineError::format_invalide);

    int port = 0;
    try {
        port = std::stoi(contenu.substr(pos_port + 5));
    } catch (...) {
        return std::unexpected(PipelineError::format_invalide);
    }

    std::string log_level = "info";  // défaut
    auto pos_log = contenu.find("log_level:");
    if (pos_log != std::string::npos) {
        auto start = pos_log + 10;
        auto end = contenu.find('\n', start);
        log_level = contenu.substr(start, end - start);
        // Trim spaces
        while (!log_level.empty() && log_level.front() == ' ') log_level.erase(0, 1);
        while (!log_level.empty() && log_level.back() == ' ') log_level.pop_back();
    }

    return Config{port, log_level};
}

std::expected<Config, PipelineError> valider(Config cfg) {
    if (cfg.port < 1 || cfg.port > 65535) {
        return std::unexpected(PipelineError::port_hors_limites);
    }
    return cfg;
}

// === std::expected<void, E> ===
enum class OpError { echec };

std::expected<void, OpError> operation_sans_retour(bool reussit) {
    if (!reussit) {
        return std::unexpected(OpError::echec);
    }
    return {};  // succès, pas de valeur
}

// === std::optional : l'absence sans explication ===
std::optional<std::string> trouver_env(const std::string& nom) {
    if (const char* val = std::getenv(nom.c_str())) {
        return std::string(val);
    }
    return std::nullopt;
}

// === Pont expected ↔ exception ===
template <typename F, typename... Args>
auto capturer_exception(F&& f, Args&&... args)
    -> std::expected<std::invoke_result_t<F, Args...>, std::string>
{
    try {
        return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    } catch (const std::exception& e) {
        return std::unexpected(std::string(e.what()));
    }
}

int main() {
    std::print("=== 1. std::expected : construction et inspection ===\n");
    {
        auto r1 = lire_config("/tmp/test_config_inexistant.yaml");
        if (!r1) {
            std::print("  Erreur : {}\n", to_string(r1.error()));
        }

        // Créer un fichier temporaire pour tester le succès
        {
            std::ofstream tmp("/tmp/test_expected_config.txt");
            tmp << "port: 8080\nlog_level: debug\n";
        }
        auto r2 = lire_config("/tmp/test_expected_config.txt");
        if (r2) {
            std::print("  Succès : {} octets lus\n", r2.value().size());
        }
    }

    std::print("\n=== 2. value_or ===\n");
    {
        auto config = lire_config("/fichier/inexistant")
                          .value_or("port: 8080\nlog_level: info");
        std::print("  Config (avec fallback) : {}\n",
                   config.substr(0, config.find('\n')));
    }

    std::print("\n=== 3. Interface monadique : and_then, transform ===\n");
    {
        // Pipeline complet sur fichier existant
        auto resultat = lire_fichier("/tmp/test_expected_config.txt")
                            .and_then(parser)
                            .and_then(valider)
                            .transform([](const Config& cfg) {
                                return "Serveur sur port " + std::to_string(cfg.port)
                                     + " (log: " + cfg.log_level + ")";
                            });

        if (resultat) {
            std::print("  Pipeline OK : {}\n", *resultat);
        } else {
            std::print("  Pipeline erreur : {}\n", to_string(resultat.error()));
        }
    }

    std::print("\n=== 4. or_else : récupération d'erreur ===\n");
    {
        auto config = lire_fichier("/fichier/inexistant")
                          .or_else([](PipelineError err) -> std::expected<std::string, PipelineError> {
                              if (err == PipelineError::fichier_introuvable) {
                                  std::print("  → Fallback : config par défaut\n");
                                  return std::string("port: 3000\nlog_level: warn");
                              }
                              return std::unexpected(err);
                          })
                          .and_then(parser)
                          .and_then(valider);

        if (config) {
            std::print("  Config récupérée : port={}, log={}\n",
                       config->port, config->log_level);
        }
    }

    std::print("\n=== 5. Pipeline avec erreur en milieu de chaîne ===\n");
    {
        // Créer un fichier avec un port invalide
        {
            std::ofstream tmp("/tmp/test_expected_bad.txt");
            tmp << "port: 99999\nlog_level: debug\n";
        }

        auto resultat = lire_fichier("/tmp/test_expected_bad.txt")
                            .and_then(parser)
                            .and_then(valider);

        if (!resultat) {
            std::print("  Erreur détectée : {}\n", to_string(resultat.error()));
        }
    }

    std::print("\n=== 6. std::expected<void, E> ===\n");
    {
        auto r1 = operation_sans_retour(true);
        std::print("  Opération réussie : {}\n", r1.has_value());

        auto r2 = operation_sans_retour(false);
        std::print("  Opération échouée : {}\n", !r2.has_value());
    }

    std::print("\n=== 7. std::optional : absence sans explication ===\n");
    {
        if (auto home = trouver_env("HOME")) {
            std::print("  HOME = {}\n", *home);
        } else {
            std::print("  HOME non définie\n");
        }

        if (auto var = trouver_env("VARIABLE_INEXISTANTE_XYZ")) {
            std::print("  Trouvée : {}\n", *var);
        } else {
            std::print("  VARIABLE_INEXISTANTE_XYZ : non définie (nullopt)\n");
        }
    }

    std::print("\n=== 8. Pont exception → expected ===\n");
    {
        auto r1 = capturer_exception([] {
            return std::stoi("42");
        });
        std::print("  stoi(\"42\") → {}\n", *r1);

        auto r2 = capturer_exception([] {
            return std::stoi("pas_un_nombre");
        });
        if (!r2) {
            std::print("  stoi(\"pas_un_nombre\") → erreur : {}\n", r2.error());
        }
    }

    // Nettoyage des fichiers temporaires
    std::remove("/tmp/test_expected_config.txt");
    std::remove("/tmp/test_expected_bad.txt");

    std::print("\nProgramme terminé.\n");
    return 0;
}
