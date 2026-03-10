/* ============================================================================
   Section 2.7.2 : Formatage type-safe et performant
   Description : Utilisation de std::format pour construire des chaînes sans afficher
   Fichier source : 07.2-formatage-type-safe.md
   Compilation : g++ -std=c++23 -Wall -Wextra format_demo.cpp -o format_demo
   ============================================================================ */
#include <format>
#include <string>
#include <vector>
#include <print>

int main() {
    // std::format retourne une std::string
    int code = 404;
    int ligne = 42;
    std::string message = std::format("Erreur {} à la ligne {}", code, ligne);
    std::println("Message : {}", message);

    // Construction de lignes formatées dans un vector
    std::vector<std::string> lignes;
    for (int i = 0; i < 5; ++i) {
        lignes.push_back(std::format("Ligne {:03d} : données", i));
    }

    std::println("\nLignes construites avec std::format :");
    for (const auto& l : lignes) {
        std::println("  {}", l);
    }

    return 0;
}
