/* ============================================================================
   Section 19.1 : La classe centrale : std::filesystem::path
   Description : Construction d'un path depuis différents types de chaînes
   Fichier source : 01-std-filesystem.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    // Depuis un littéral string
    fs::path p1 = "/home/user/projet/src/main.cpp";

    // Depuis une std::string
    std::string dir = "/var/log";
    fs::path p2(dir);

    // Depuis une string_view (C++17)
    std::string_view sv = "/etc/nginx/nginx.conf";
    fs::path p3(sv);

    // Path vide (valide, mais ne pointe vers rien)
    fs::path p4;

    std::println("p1 = {}", p1.string());
    std::println("p4 est vide : {}", p4.empty());
}
