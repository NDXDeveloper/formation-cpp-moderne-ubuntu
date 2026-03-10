/* ============================================================================
   Section 1.0 : Introduction au chapitre
   Description : Premier exemple de la formation — affichage avec std::print
   Fichier source : README.md
   Compilation : g++ -std=c++23 -o 01-00-bienvenue 01-00-bienvenue.cpp
   Sortie attendue : Bienvenue dans la formation C++ moderne !
   ============================================================================ */
#include <print>

int main() {
    std::print("Bienvenue dans la formation C++ moderne !\n");
    return 0;
}
