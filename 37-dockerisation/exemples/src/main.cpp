/* ============================================================================
   Section 37.2 : Multi-stage builds
   Description : Point d'entrée du programme de démonstration Docker
   Fichier source : 02-multi-stage-builds.md
   ============================================================================ */
#include "myapp/app.hpp"
#include <iostream>
int main() {
    std::cout << greet("Docker") << std::endl;
    return 0;
}
