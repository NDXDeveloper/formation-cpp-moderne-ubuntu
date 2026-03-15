/* ============================================================================
   Section 19.1.3 : Gestion des fichiers temporaires
   Description : make_temp_path() avec PID et timestamp
   Fichier source : 01.3-operations-fichiers.md
   ============================================================================ */
#include <filesystem>
#include <fstream>
#include <print>
#include <format>
#include <chrono>
#include <unistd.h>

namespace fs = std::filesystem;

auto make_temp_path(const std::string& prefix, const std::string& suffix = "")
    -> fs::path
{
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto pid = getpid();
    auto name = std::format("{}_{}_{}{}",prefix, pid, now, suffix);
    return fs::temp_directory_path() / name;
}

int main() {
    auto tmp = make_temp_path("mon_app", ".dat");
    std::println("Fichier temporaire : {}", tmp.string());

    // Créer et utiliser le fichier
    {
        std::ofstream out(tmp);
        out << "données temporaires\n";
    }

    // Vérifier
    std::println("Existe : {}", fs::exists(tmp));
    std::println("Taille : {} octets", fs::file_size(tmp));

    // Nettoyage
    fs::remove(tmp);
    std::println("Nettoyé : {}", !fs::exists(tmp));
}
