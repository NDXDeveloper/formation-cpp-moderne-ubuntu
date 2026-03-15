/* ============================================================================
   Section 19.1.1 : Parcours et Ranges C++20
   Description : Filtrage avec std::views::filter et transform
   Fichier source : 01.1-parcours-repertoires.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <ranges>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    // Créer une arborescence de test
    fs::path dir = "/tmp/test_ranges_ex";
    fs::remove_all(dir);
    fs::create_directories(dir);
    std::ofstream{dir / "main.cpp"};
    std::ofstream{dir / "utils.cpp"};
    std::ofstream{dir / "utils.h"};
    std::ofstream{dir / "config.h"};
    std::ofstream{dir / "README.md"};

    // Filtrer les fichiers .h avec une vue ranges
    auto headers = fs::directory_iterator(dir)
        | std::views::filter([](const auto& e) {
            return e.is_regular_file() && e.path().extension() == ".h";
        })
        | std::views::transform([](const auto& e) {
            return e.path().filename().string();
        });

    std::println("Fichiers .h trouvés :");
    for (const auto& name : headers) {
        std::println("  {}", name);
    }

    // Nettoyage
    fs::remove_all(dir);
}
