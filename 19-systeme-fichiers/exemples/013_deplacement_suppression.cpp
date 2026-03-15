/* ============================================================================
   Section 19.1.3 : Déplacement, renommage et suppression
   Description : rename(), remove(), remove_all() et move cross-filesystem
   Fichier source : 01.3-operations-fichiers.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <fstream>
#include <cstdint>

namespace fs = std::filesystem;

void move_cross_filesystem(const fs::path& source, const fs::path& dest) {
    std::error_code ec;
    fs::rename(source, dest, ec);

    if (ec) {
        // Probable cross-filesystem — fallback sur copy + remove
        fs::copy(source, dest, fs::copy_options::recursive
                              | fs::copy_options::overwrite_existing);
        fs::remove_all(source);
        std::println("Déplacé (via copie) : {} → {}", source.string(), dest.string());
    } else {
        std::println("Déplacé (atomique)  : {} → {}", source.string(), dest.string());
    }
}

int main() {
    // Setup
    fs::create_directories("/tmp/ex013_move");
    std::ofstream{"/tmp/ex013_move/ancien.txt"} << "hello";

    // Renommer
    fs::rename("/tmp/ex013_move/ancien.txt", "/tmp/ex013_move/nouveau.txt");
    std::println("Renommage OK");

    // Supprimer un fichier
    bool removed = fs::remove("/tmp/ex013_move/nouveau.txt");
    std::println("Supprimé : {}", removed);

    // Supprimer un inexistant : retourne false
    bool absent = fs::remove("/tmp/ex013_move/inexistant");
    std::println("Inexistant, supprimé : {}", absent);

    // Supprimer un répertoire NON vide : échoue
    fs::create_directories("/tmp/ex013_move/sub");
    std::ofstream{"/tmp/ex013_move/sub/f.txt"} << "x";
    std::error_code ec;
    fs::remove("/tmp/ex013_move/sub", ec);
    if (ec) {
        std::println("Erreur attendue : {}", ec.message());
    }

    // remove_all() récursif
    std::uintmax_t count = fs::remove_all("/tmp/ex013_move");
    std::println("{} éléments supprimés", count);
}
