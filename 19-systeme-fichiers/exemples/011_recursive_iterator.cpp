/* ============================================================================
   Section 19.1.1 : recursive_directory_iterator
   Description : Parcours récursif avec contrôle de profondeur et exclusions
   Fichier source : 01.1-parcours-repertoires.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <unordered_set>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    // Créer une arborescence de test
    fs::path racine = "/tmp/test_recursive_ex";
    fs::remove_all(racine);
    fs::create_directories(racine / "src");
    fs::create_directories(racine / "include" / "projet");
    fs::create_directories(racine / "build" / "obj");
    fs::create_directories(racine / ".git");
    std::ofstream{racine / "src" / "main.cpp"};
    std::ofstream{racine / "src" / "utils.cpp"};
    std::ofstream{racine / "include" / "projet" / "config.h"};
    std::ofstream{racine / "build" / "CMakeCache.txt"};
    std::ofstream{racine / "CMakeLists.txt"};
    std::ofstream{racine / "README.md"};

    // Parcours récursif avec indentation
    std::println("=== Parcours récursif ===");
    auto it = fs::recursive_directory_iterator(racine);
    for (const auto& entry : it) {
        std::string indent(it.depth() * 2, ' ');
        std::println("{}{}", indent, entry.path().filename().string());
    }

    // Parcours avec exclusion de répertoires
    std::println("\n=== Parcours avec exclusions (build, .git) ===");
    const std::unordered_set<std::string> exclusions = {
        "build", ".git", "node_modules"
    };

    auto it2 = fs::recursive_directory_iterator(
        racine, fs::directory_options::skip_permission_denied);

    for (auto& entry : it2) {
        if (entry.is_directory()
            && exclusions.contains(entry.path().filename().string()))
        {
            it2.disable_recursion_pending();
            continue;
        }

        if (entry.is_regular_file()) {
            std::println("{}", entry.path().lexically_relative(racine).string());
        }
    }

    // Nettoyage
    fs::remove_all(racine);
}
