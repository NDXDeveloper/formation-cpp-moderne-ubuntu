/* ============================================================================
   Section 19.1 : Performance et considérations système
   Description : Réduction des appels stat() redondants avec fs::status()
   Fichier source : 01-std-filesystem.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <system_error>

namespace fs = std::filesystem;

int main() {
    fs::path p = "/etc/hostname";

    // ❌ Trois appels stat() pour le même fichier
    if (fs::exists(p) && fs::is_regular_file(p) && !fs::is_empty(p)) {
        auto taille = fs::file_size(p);  // Encore un stat()
        std::println("Approche 1 - Taille : {}", taille);
    }

    // ✅ Un seul appel stat(), puis interrogation du résultat
    std::error_code ec;
    auto st = fs::status(p, ec);
    if (!ec && fs::is_regular_file(st)) {
        auto taille = fs::file_size(p);  // Inévitable : file_size n'accepte pas un status
        std::println("Approche 2 - Taille : {}", taille);
    }
}
