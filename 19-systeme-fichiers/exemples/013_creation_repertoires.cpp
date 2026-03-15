/* ============================================================================
   Section 19.1.3 : Création de répertoires
   Description : create_directory(), create_directories() et permissions
   Fichier source : 01.3-operations-fichiers.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::remove_all("/tmp/mon_app_ex013");

    // Crée un seul niveau
    bool created = fs::create_directory("/tmp/mon_app_ex013");
    std::println("Créé : {}", created);

    // Idempotent : retourne false si déjà existant
    bool again = fs::create_directory("/tmp/mon_app_ex013");
    std::println("Déjà existant, créé : {}", again);

    // Parent manquant : exception
    try {
        fs::create_directory("/tmp/absent_ex013/sous_repertoire");
    } catch (const fs::filesystem_error& e) {
        std::println("Erreur : {}", e.what());
    }

    // Créer une arborescence complète (mkdir -p)
    bool dirs = fs::create_directories("/tmp/mon_app_ex013/build/release/x86_64");
    std::println("Arborescence créée : {}", dirs);

    // Nettoyage
    fs::remove_all("/tmp/mon_app_ex013");
}
