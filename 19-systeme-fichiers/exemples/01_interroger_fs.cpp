/* ============================================================================
   Section 19.1 : Interroger le filesystem
   Description : Existence, type, taille, métadonnées et espace disque
   Fichier source : 01-std-filesystem.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <chrono>

namespace fs = std::filesystem;

int main() {
    fs::path p = "/etc/hostname";

    // Existence et type
    std::println("Existe : {}", fs::exists(p));
    std::println("Fichier régulier  : {}", fs::is_regular_file(p));
    std::println("Répertoire        : {}", fs::is_directory(p));
    std::println("Lien symbolique   : {}", fs::is_symlink(p));
    std::println("Vide              : {}", fs::is_empty(p));

    // Taille et métadonnées
    if (fs::exists(p) && fs::is_regular_file(p)) {
        auto taille = fs::file_size(p);
        std::println("Taille : {} octets", taille);

        auto ftime = fs::last_write_time(p);
        (void)ftime;  // Utilisé uniquement pour illustrer l'API

        auto info = fs::space("/");
        std::println("Espace total     : {} Go", info.capacity / (1024*1024*1024));
        std::println("Espace libre     : {} Go", info.free / (1024*1024*1024));
        std::println("Espace dispo     : {} Go", info.available / (1024*1024*1024));
    }
}
