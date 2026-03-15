/* ============================================================================
   Section 19.1.1 : Patterns courants
   Description : Filtrage par extension, calcul de taille, fichiers récents
   Fichier source : 01.1-parcours-repertoires.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <vector>
#include <chrono>
#include <cstdint>
#include <fstream>

namespace fs = std::filesystem;

auto find_by_extension(const fs::path& dir, const std::string& ext)
    -> std::vector<fs::path>
{
    std::vector<fs::path> results;

    for (const auto& entry : fs::recursive_directory_iterator(
            dir, fs::directory_options::skip_permission_denied))
    {
        if (entry.is_regular_file() && entry.path().extension() == ext) {
            results.push_back(entry.path());
        }
    }

    return results;
}

auto directory_size(const fs::path& dir) -> std::uintmax_t {
    std::uintmax_t total = 0;

    for (const auto& entry : fs::recursive_directory_iterator(
            dir, fs::directory_options::skip_permission_denied))
    {
        if (entry.is_regular_file() && !entry.is_symlink()) {
            total += entry.file_size();
        }
    }

    return total;
}

int main() {
    // Créer une arborescence de test
    fs::path projet = "/tmp/test_patterns_ex";
    fs::remove_all(projet);
    fs::create_directories(projet / "src");
    fs::create_directories(projet / "include");
    std::ofstream{projet / "src" / "main.cpp"} << "int main() {}";
    std::ofstream{projet / "src" / "utils.cpp"} << "void f() {}";
    std::ofstream{projet / "include" / "utils.h"} << "#pragma once";
    std::ofstream{projet / "README.md"} << "# Projet";

    // Filtrer par extension
    auto cpp_files = find_by_extension(projet, ".cpp");
    std::println("Fichiers .cpp trouvés : {}", cpp_files.size());
    for (const auto& f : cpp_files) {
        std::println("  {}", f.lexically_relative(projet).string());
    }

    // Taille totale
    auto taille = directory_size(projet);
    std::println("Taille totale : {} octets", taille);

    // Fichiers récents (modifiés dans les dernières 24h)
    using namespace std::chrono_literals;
    auto now = fs::file_time_type::clock::now();
    int recents = 0;

    for (const auto& entry : fs::recursive_directory_iterator(projet)) {
        if (entry.is_regular_file()) {
            auto age = now - entry.last_write_time();
            if (age <= 24h) ++recents;
        }
    }
    std::println("Fichiers modifiés dans les dernières 24h : {}", recents);

    // Nettoyage
    fs::remove_all(projet);
}
