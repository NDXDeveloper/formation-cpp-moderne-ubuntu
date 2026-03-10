/* ============================================================================
   Section 2.5 : Premier programme
   Description : Programme mono-fichier avec aire_cercle, std::cout, cmath
   Fichier source : 05-premier-programme.md
   Compilation : g++ -std=c++23 -Wall -Wextra main_mono.cpp -o main_mono
   ============================================================================ */
#include <iostream>
#include <string>
#include <cmath>

#define APP_VERSION "1.0.0"

constexpr double PI = 3.14159265358979323846;

double aire_cercle(double rayon) {
    return PI * rayon * rayon;
}

int main() {
    std::string nom = "Ubuntu";
    double rayon = 5.0;
    double aire = aire_cercle(rayon);

    std::cout << "Bienvenue sur " << nom << " !" << std::endl;
    std::cout << "Version : " << APP_VERSION << std::endl;
    std::cout << "Aire d'un cercle de rayon " << rayon
              << " = " << aire << std::endl;
    std::cout << "Vérification avec cmath : sqrt(144) = "
              << std::sqrt(144.0) << std::endl;

    return 0;
}
