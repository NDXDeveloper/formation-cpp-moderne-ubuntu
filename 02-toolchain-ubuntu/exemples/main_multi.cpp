/* ============================================================================
   Section 2.5 : Premier programme (multi-fichiers)
   Description : Programme multi-fichiers utilisant mathutils
   Fichier source : 05-premier-programme.md / 05.1-compilation-etapes.md
   Compilation : g++ main_multi.cpp mathutils.cpp -o main_multi
   ============================================================================ */
#include <iostream>
#include <string>
#include <cmath>
#include "mathutils.hpp"

#define APP_VERSION "1.0.0"

int main() {
    std::string nom = "Ubuntu";
    double rayon = 5.0;

    std::cout << "Bienvenue sur " << nom << " !" << std::endl;
    std::cout << "Version : " << APP_VERSION << std::endl;
    std::cout << "Aire d'un cercle de rayon " << rayon
              << " = " << aire_cercle(rayon) << std::endl;
    std::cout << "Périmètre : " << perimetre_cercle(rayon) << std::endl;
    std::cout << "sqrt(144) = " << std::sqrt(144.0) << std::endl;

    return 0;
}
