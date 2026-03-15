/* ============================================================================
   Section 19.1 : Normalisation et chemins relatifs
   Description : lexically_normal() et lexically_relative()
   Fichier source : 01-std-filesystem.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    // Normalisation syntaxique
    fs::path p = "/home/user/./projet/../projet/src//main.cpp";
    std::println("Brut       : {}", p.string());
    std::println("Normalisé  : {}", p.lexically_normal().string());
    // Normalisé  : /home/user/projet/src/main.cpp

    // Chemins relatifs
    fs::path fichier = "/home/user/projet/src/main.cpp";
    fs::path base = "/home/user/projet";

    std::println("{}", fichier.lexically_relative(base).string());
    // src/main.cpp

    std::println("{}", base.lexically_relative(fichier).string());
    // ../..
}
