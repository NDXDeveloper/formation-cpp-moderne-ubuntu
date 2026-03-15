/* ============================================================================
   Section 19.1.3 : Pattern d'écriture atomique (write-then-rename)
   Description : Écriture atomique via fichier temporaire + rename()
   Fichier source : 01.3-operations-fichiers.md
   ============================================================================ */
#include <filesystem>
#include <fstream>
#include <print>

namespace fs = std::filesystem;

void atomic_write(const fs::path& target, const std::string& content) {
    // 1. Écrire dans un fichier temporaire dans le MÊME répertoire
    fs::path tmp = target;
    tmp += ".tmp";

    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Impossible de créer " + tmp.string());
        }
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        out.flush();
    }

    // 3. Renommage atomique
    fs::rename(tmp, target);
}

int main() {
    fs::path config = "/tmp/mon_app_ex013/config.yaml";
    fs::create_directories(config.parent_path());

    atomic_write(config, "server:\n  port: 8080\n  host: localhost\n");
    std::println("Configuration écrite de manière atomique");

    // Vérifier le contenu
    std::ifstream in(config);
    std::string line;
    while (std::getline(in, line)) {
        std::println("  {}", line);
    }

    // Nettoyage
    fs::remove_all("/tmp/mon_app_ex013");
}
