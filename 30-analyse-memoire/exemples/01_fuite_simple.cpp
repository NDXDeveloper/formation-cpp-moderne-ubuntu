/* ============================================================================
   Section 30.1 : Valgrind — Détection de fuites et erreurs mémoire
   Description : Fuite mémoire simple — allocation sans delete
   Fichier source : 01-valgrind.md
   Compilation : g++-15 -std=c++23 -g -O0 -o fuite_simple 01_fuite_simple.cpp
   Exécution  : valgrind --leak-check=full ./fuite_simple
   ============================================================================ */

#include <iostream>

int main() {
    int* tableau = new int[100];
    tableau[0] = 42;
    std::cout << tableau[0] << "\n";
    // Oubli volontaire : pas de delete[] tableau;
    return 0;
}
