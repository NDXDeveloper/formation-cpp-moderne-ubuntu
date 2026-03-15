/* ============================================================================
   Section 19.4 : Vérification de l'espace disque avant écriture
   Description : has_enough_space() avec marge de sécurité
   Fichier source : 04-permissions-droits.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <cstdint>

namespace fs = std::filesystem;

bool has_enough_space(const fs::path& target, std::uintmax_t required_bytes) {
    std::error_code ec;
    auto info = fs::space(target.parent_path(), ec);
    if (ec) {
        std::println("Impossible de vérifier l'espace : {}", ec.message());
        return false;
    }

    // Garder une marge de sécurité de 10%
    auto margin = required_bytes / 10;
    if (info.available < required_bytes + margin) {
        std::println("Espace insuffisant : {} Mo disponibles, {} Mo requis",
            info.available / (1024 * 1024),
            (required_bytes + margin) / (1024 * 1024));
        return false;
    }

    return true;
}

int main() {
    fs::path target = "/tmp/test_space_ex04.bin";

    // Test avec une taille raisonnable (1 Mo)
    std::println("1 Mo : {}", has_enough_space(target, 1024 * 1024) ? "OK" : "insuffisant");

    // Test avec une taille énorme (1 To)
    std::uintmax_t one_tb = 1024ULL * 1024 * 1024 * 1024;
    std::println("1 To : {}", has_enough_space(target, one_tb) ? "OK" : "insuffisant");

    // Afficher l'espace disponible
    auto info = fs::space("/tmp");
    std::println("\nEspace sur /tmp :");
    std::println("  Disponible : {} Mo", info.available / (1024 * 1024));
}
