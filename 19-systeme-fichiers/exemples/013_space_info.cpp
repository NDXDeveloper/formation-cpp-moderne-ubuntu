/* ============================================================================
   Section 19.1.3 : Espace disque
   Description : fs::space() pour informations d'espace disque
   Fichier source : 01.3-operations-fichiers.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

void print_space_info(const fs::path& p) {
    std::error_code ec;
    auto info = fs::space(p, ec);

    if (ec) {
        std::println("Erreur pour {} : {}", p.string(), ec.message());
        return;
    }

    constexpr auto Go = 1024.0 * 1024.0 * 1024.0;
    std::println("Filesystem contenant {} :", p.string());
    std::println("  Capacité   : {:.2f} Go", info.capacity / Go);
    std::println("  Libre      : {:.2f} Go", info.free / Go);
    std::println("  Disponible : {:.2f} Go", info.available / Go);
    std::println("  Utilisation : {:.1f}%",
        100.0 * (1.0 - static_cast<double>(info.available)
                      / static_cast<double>(info.capacity)));
}

int main() {
    print_space_info("/");
    print_space_info("/home");
    print_space_info("/tmp");
}
