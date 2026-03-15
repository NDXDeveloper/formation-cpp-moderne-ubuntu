/* ============================================================================
   Section 19.1.3 : Copie de fichiers
   Description : copy_file(), copy_options et copy() récursive
   Fichier source : 01.3-operations-fichiers.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    // Setup
    fs::remove("/tmp/hostname_backup_ex");

    fs::path source = "/etc/hostname";
    fs::path dest = "/tmp/hostname_backup_ex";

    // Copie simple
    bool copied = fs::copy_file(source, dest);
    std::println("Copié : {}", copied);

    // Deuxième appel : échoue car dest existe
    std::error_code ec;
    fs::copy_file(source, dest, ec);
    if (ec) {
        std::println("Erreur attendue : {}", ec.message());
    }

    // Écraser si la destination existe
    fs::copy_file(source, dest, fs::copy_options::overwrite_existing);
    std::println("Écrasement OK");

    // Ne rien faire si la destination existe
    bool skipped = fs::copy_file(source, dest, fs::copy_options::skip_existing);
    std::println("Copié (skip) : {}", skipped);  // false

    // Nettoyage
    fs::remove(dest);
}
