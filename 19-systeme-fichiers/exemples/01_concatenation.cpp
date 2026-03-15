/* ============================================================================
   Section 19.1 : Concaténation de chemins avec /
   Description : Opérateur / et += pour construire des chemins
   Fichier source : 01-std-filesystem.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    fs::path base = "/home/user/projet";
    fs::path src = base / "src";
    fs::path main_file = src / "main.cpp";

    std::println("{}", main_file.string());
    // /home/user/projet/src/main.cpp

    // append() est l'équivalent méthode de l'opérateur /
    fs::path config = base;
    config /= "config";
    config /= "settings.yaml";
    std::println("{}", config.string());
    // /home/user/projet/config/settings.yaml

    // Distinction / vs +=
    fs::path p = "/home/user/fichier";

    // operator/ : ajoute un séparateur
    auto a = p / ".bak";
    std::println("{}", a.string());
    // /home/user/fichier/.bak

    // operator+= : concatène directement
    auto b = p;
    b += ".bak";
    std::println("{}", b.string());
    // /home/user/fichier.bak
}
