/* ============================================================================
   Section 39.1.3 : Construction avec dpkg-deb
   Description : Point d'entree syswatch — monitoring systeme simplifie
   Fichier source : 01.3-dpkg-deb.md
   ============================================================================ */

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--version") {
        std::cout << "syswatch 1.2.0" << std::endl;
        return 0;
    }
    std::cout << "syswatch: monitoring systeme demarre" << std::endl;
    return 0;
}
