/* ============================================================================
   Section 39.1 : Creation de paquets DEB (Debian/Ubuntu)
   Description : Exemple minimal pour demo CPack
   Fichier source : 01-paquets-deb.md
   ============================================================================ */

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--version") {
        std::cout << "syswatch-cpack 1.2.0" << std::endl;
        return 0;
    }
    std::cout << "syswatch-cpack: demo CPack" << std::endl;
    return 0;
}
