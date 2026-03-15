/* ============================================================================
   Section 19.1.3 : Modification des métadonnées
   Description : file_size(), resize_file(), last_write_time(), clock_cast
   Fichier source : 01.3-operations-fichiers.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <chrono>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    // Préparer un fichier de test
    fs::path p = "/tmp/test_meta_ex013.txt";
    std::ofstream{p} << "Contenu de test pour les métadonnées";

    // Taille de fichier
    auto taille = fs::file_size(p);
    std::println("Taille : {} octets", taille);

    // Tronquer le fichier
    fs::resize_file(p, 10);
    std::println("Après troncation : {} octets", fs::file_size(p));

    // Étendre le fichier (rempli de zéros)
    fs::resize_file(p, 100);
    std::println("Après extension : {} octets", fs::file_size(p));

    // Date de dernière modification
    auto ftime = fs::last_write_time(p);

    // Modifier la date (touch)
    auto now = fs::file_time_type::clock::now();
    fs::last_write_time(p, now);

    // C++20 : conversion via clock_cast
    auto sys_time = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
    std::println("Dernière modification : {}", sys_time);

    // Taille d'un fichier connu
    auto bash_size = fs::file_size("/usr/bin/bash");
    std::println("bash : {} octets ({:.1f} Ko)", bash_size,
        static_cast<double>(bash_size) / 1024);

    // Nettoyage
    fs::remove(p);
}
