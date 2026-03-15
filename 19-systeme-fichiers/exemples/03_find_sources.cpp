/* ============================================================================
   Section 19.3 : Scénario 5 — Parcourir une arborescence
   Description : Recherche récursive de fichiers .cpp avec std::filesystem
   Fichier source : 03-comparaison-api.md
   ============================================================================ */
#include <filesystem>
#include <vector>
#include <print>
#include <fstream>

namespace fs = std::filesystem;

auto find_sources(const fs::path& root) -> std::vector<fs::path> {
    std::vector<fs::path> result;

    for (const auto& entry : fs::recursive_directory_iterator(
            root, fs::directory_options::skip_permission_denied))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
            result.push_back(entry.path());
        }
    }

    return result;
}

int main() {
    // Créer une arborescence de test
    fs::path projet = "/tmp/projet_ex03";
    fs::remove_all(projet);
    fs::create_directories(projet / "src" / "core");
    fs::create_directories(projet / "src" / "utils");
    fs::create_directories(projet / "tests");
    fs::create_directories(projet / "include");

    std::ofstream{projet / "src" / "core" / "main.cpp"} << "int main() {}";
    std::ofstream{projet / "src" / "core" / "app.cpp"} << "void run() {}";
    std::ofstream{projet / "src" / "utils" / "logger.cpp"} << "void log() {}";
    std::ofstream{projet / "tests" / "test_app.cpp"} << "void test() {}";
    std::ofstream{projet / "include" / "app.h"} << "#pragma once";
    std::ofstream{projet / "README.md"} << "# Projet";

    // Rechercher les .cpp
    auto sources = find_sources(projet);
    std::println("Fichiers .cpp trouvés : {}", sources.size());
    for (const auto& f : sources) {
        std::println("  {}", f.lexically_relative(projet).string());
    }

    // Nettoyage
    fs::remove_all(projet);
}
