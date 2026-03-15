/* ============================================================================
   Section 19.1.1 : directory_entry : le cache intelligent
   Description : Utilisation du cache directory_entry pour éviter les stat() redondants
   Fichier source : 01.1-parcours-repertoires.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::path dir = "/var/log";

    for (const auto& entry : fs::directory_iterator(dir)) {
        // Ces appels exploitent le cache du directory_entry
        // Pas d'appel stat() supplémentaire pour le type
        if (entry.is_regular_file()) {
            std::println("{} — {} octets",
                entry.path().filename().string(),
                entry.file_size());
        } else if (entry.is_directory()) {
            std::println("{}/", entry.path().filename().string());
        } else if (entry.is_symlink()) {
            std::println("{} -> (lien)", entry.path().filename().string());
        }
    }
}
