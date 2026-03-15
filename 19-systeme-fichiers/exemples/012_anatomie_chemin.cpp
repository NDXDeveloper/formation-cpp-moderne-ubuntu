/* ============================================================================
   Section 19.1.2 : Anatomie complète d'un chemin Linux
   Description : Itération composante par composante et distinction absolu/relatif
   Fichier source : 01.2-manipulation-chemins.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    // Itération des composantes
    fs::path p = "/home/user/projet/src/main.cpp";

    std::println("Composantes de {} :", p.string());
    for (const auto& composante : p) {
        std::println("  [{}]", composante.string());
    }

    // Distinction absolu / relatif
    fs::path absolu = "/var/log/syslog";
    fs::path relatif = "build/release/app";
    fs::path relatif_dot = "./config/settings.yaml";

    std::println("\n{} est absolu : {}", absolu.string(), absolu.is_absolute());
    std::println("{} est relatif : {}", relatif.string(), relatif.is_relative());
    std::println("{} est relatif : {}", relatif_dot.string(), relatif_dot.is_relative());
}
