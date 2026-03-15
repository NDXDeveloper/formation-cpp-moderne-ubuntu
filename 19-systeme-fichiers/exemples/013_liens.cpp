/* ============================================================================
   Section 19.1.3 : Liens symboliques et hard links
   Description : create_symlink(), create_hard_link(), read_symlink()
   Fichier source : 01.3-operations-fichiers.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    // Cleanup
    fs::remove("/tmp/python_ex013");
    fs::remove("/tmp/original_ex013.txt");
    fs::remove("/tmp/hardlink_ex013.txt");

    // Liens symboliques
    fs::create_symlink("/usr/bin/python3.12", "/tmp/python_ex013");
    fs::path cible = fs::read_symlink("/tmp/python_ex013");
    std::println("python -> {}", cible.string());

    // Hard links
    std::ofstream{"/tmp/original_ex013.txt"} << "hello";
    fs::create_hard_link("/tmp/original_ex013.txt", "/tmp/hardlink_ex013.txt");

    std::println("Équivalents : {}",
        fs::equivalent("/tmp/original_ex013.txt", "/tmp/hardlink_ex013.txt"));
    std::println("Hard links : {}", fs::hard_link_count("/tmp/original_ex013.txt"));

    // Nettoyage
    fs::remove("/tmp/python_ex013");
    fs::remove("/tmp/original_ex013.txt");
    fs::remove("/tmp/hardlink_ex013.txt");
}
