/* ============================================================================
   Section 19.1.2 : Normalisation syntaxique vs physique
   Description : lexically_normal(), canonical(), weakly_canonical()
   Fichier source : 01.2-manipulation-chemins.md
   ============================================================================ */
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

int main() {
    // Normalisation syntaxique
    fs::path p1 = "/home/user/./projet/../projet/src//main.cpp";
    std::println("{}", p1.lexically_normal().string());
    // /home/user/projet/src/main.cpp

    // Trailing slash préservé
    fs::path p2 = "/var/log/";
    std::println("{}", p2.lexically_normal().string());
    // /var/log/

    // Remontées au-delà de la racine : tronquées
    fs::path p3 = "/home/../../etc/passwd";
    std::println("{}", p3.lexically_normal().string());
    // /etc/passwd

    // Chemin relatif : les .. initiaux sont préservés
    fs::path p4 = "../../src/main.cpp";
    std::println("{}", p4.lexically_normal().string());
    // ../../src/main.cpp

    // Conversion relatif → absolu
    fs::path relatif = "src/main.cpp";
    fs::path abs = fs::absolute(relatif);
    std::println("absolute()  : {}", abs.string());

    fs::path wcanon = fs::weakly_canonical("src/../build/output.bin");
    std::println("weakly_canonical() : {}", wcanon.string());
}
