/* ============================================================================
   Section 19.1 : Gestion des erreurs : deux approches
   Description : Comparaison exceptions vs std::error_code
   Fichier source : 01-std-filesystem.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <system_error>

namespace fs = std::filesystem;

void approche_exceptions() {
    try {
        auto taille = fs::file_size("/chemin/inexistant");
        std::println("Taille : {}", taille);
    } catch (const fs::filesystem_error& e) {
        std::println("Erreur filesystem : {}", e.what());
        std::println("  path1 : {}", e.path1().string());
        std::println("  code  : {}", e.code().message());
    }
}

void approche_error_code() {
    std::error_code ec;
    auto taille = fs::file_size("/chemin/inexistant", ec);

    if (ec) {
        std::println("Erreur : {} (code {})", ec.message(), ec.value());
    } else {
        std::println("Taille : {}", taille);
    }
}

int main() {
    approche_exceptions();
    approche_error_code();
}
