/* ============================================================================
   Section 19.1 : Décomposition d'un chemin
   Description : Extraction des composantes d'un chemin avec fs::path
   Fichier source : 01-std-filesystem.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::path p = "/home/user/projet/build/app.tar.gz";

    std::println("Chemin complet  : {}", p.string());
    std::println("root_path()     : {}", p.root_path().string());
    std::println("root_name()     : {}", p.root_name().string());
    std::println("root_directory() : {}", p.root_directory().string());
    std::println("relative_path() : {}", p.relative_path().string());
    std::println("parent_path()   : {}", p.parent_path().string());
    std::println("filename()      : {}", p.filename().string());
    std::println("stem()          : {}", p.stem().string());
    std::println("extension()     : {}", p.extension().string());
}
