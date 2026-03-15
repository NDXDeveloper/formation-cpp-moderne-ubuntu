/* ============================================================================
   Section 19.1.1 : directory_iterator : parcours à un seul niveau
   Description : Parcours d'un répertoire avec tri alphabétique
   Fichier source : 01.1-parcours-repertoires.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

int main() {
    fs::path dir = "/var/log";
    std::vector<fs::directory_entry> entries;

    for (const auto& entry : fs::directory_iterator(dir)) {
        entries.push_back(entry);
    }

    // Tri alphabétique par nom de fichier
    std::ranges::sort(entries, [](const auto& a, const auto& b) {
        return a.path().filename() < b.path().filename();
    });

    std::println("Contenu de {} (trié) :", dir.string());
    for (const auto& entry : entries) {
        std::println("  {}", entry.path().filename().string());
    }
}
