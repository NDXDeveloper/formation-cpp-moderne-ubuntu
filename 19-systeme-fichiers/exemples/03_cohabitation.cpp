/* ============================================================================
   Section 19.3 : Cohabitation des API dans un même projet
   Description : std::filesystem + std::fstream + POSIX dans un même programme
   Fichier source : 03-comparaison-api.md
   ============================================================================ */
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>

namespace fs = std::filesystem;

void example(const fs::path& dir) {
    // std::filesystem pour la structure
    fs::create_directories(dir / "data");

    // std::fstream pour l'écriture textuelle
    std::ofstream meta(dir / "data" / "metadata.json");
    meta << R"({"version": 1, "format": "binary"})";
    meta.close();

    // POSIX pour l'écriture durable du fichier de données
    fs::path data_path = dir / "data" / "payload.bin";
    int fd = open(data_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd >= 0) {
        const char* payload = "binary data here";
        write(fd, payload, strlen(payload));
        fsync(fd);
        close(fd);
    }
}

int main() {
    fs::path dir = "/tmp/cohabitation_ex03";
    fs::remove_all(dir);

    example(dir);

    // Vérifier les résultats
    std::println("Structure créée :");
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        auto rel = entry.path().lexically_relative(dir);
        std::println("  {} ({} octets)", rel.string(),
            entry.is_regular_file() ? entry.file_size() : 0);
    }

    // Nettoyage
    fs::remove_all(dir);
}
