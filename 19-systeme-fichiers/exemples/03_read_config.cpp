/* ============================================================================
   Section 19.3 : Scénario 1 — Lire un fichier de configuration
   Description : Lecture simple avec std::fstream (fichier petit, lu une fois)
   Fichier source : 03-comparaison-api.md
   ============================================================================ */
#include <fstream>
#include <string>
#include <sstream>
#include <print>
#include <filesystem>

std::string read_config(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Impossible d'ouvrir " + path.string());
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int main() {
    namespace fs = std::filesystem;

    // Créer un fichier de config de test
    fs::path config = "/tmp/config_ex03.yaml";
    {
        std::ofstream out(config);
        out << "server:\n  host: localhost\n  port: 8080\n";
    }

    // Lire la config
    auto content = read_config(config);
    std::println("Configuration lue ({} octets) :", content.size());
    std::print("{}", content);

    // Test avec fichier inexistant
    try {
        read_config("/tmp/inexistant_ex03.yaml");
    } catch (const std::runtime_error& e) {
        std::println("Erreur attendue : {}", e.what());
    }

    // Nettoyage
    fs::remove(config);
}
