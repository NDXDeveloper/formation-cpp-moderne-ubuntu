/* ============================================================================
   Section 19.1 : Les opérations fondamentales
   Description : Création, copie, déplacement, suppression et liens symboliques
   Fichier source : 01-std-filesystem.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    // --- Création ---
    fs::create_directory("/tmp/mon_projet_ex");
    fs::create_directories("/tmp/mon_projet_ex/build/release");
    std::ofstream{"/tmp/mon_projet_ex/config.yaml"};
    std::println("Création OK");

    // --- Copie ---
    std::ofstream{"/tmp/mon_projet_ex/source.txt"} << "hello";
    fs::copy_file("/tmp/mon_projet_ex/source.txt", "/tmp/mon_projet_ex/dest.txt");
    fs::copy_file("/tmp/mon_projet_ex/source.txt", "/tmp/mon_projet_ex/dest.txt",
        fs::copy_options::overwrite_existing);
    std::println("Copie OK");

    // --- Déplacement ---
    fs::rename("/tmp/mon_projet_ex/dest.txt", "/tmp/mon_projet_ex/nouveau.txt");
    std::println("Renommage OK");

    // --- Liens symboliques ---
    fs::remove("/tmp/python_ex_link");
    fs::create_symlink("/usr/bin/python3.12", "/tmp/python_ex_link");
    fs::path cible = fs::read_symlink("/tmp/python_ex_link");
    std::println("python -> {}", cible.string());

    fs::file_status st = fs::status("/tmp/python_ex_link");
    std::println("Type : fichier régulier = {}", st.type() == fs::file_type::regular);

    fs::file_status lst = fs::symlink_status("/tmp/python_ex_link");
    std::println("Type : lien symbolique = {}", lst.type() == fs::file_type::symlink);

    // --- Suppression ---
    fs::remove("/tmp/python_ex_link");
    fs::remove("/tmp/mon_projet_ex/source.txt");
    auto nb = fs::remove_all("/tmp/mon_projet_ex");
    std::println("{} éléments supprimés", nb);
}
