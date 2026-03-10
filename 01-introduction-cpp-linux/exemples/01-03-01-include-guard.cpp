/* ============================================================================
   Section 1.3.1 : Le préprocesseur — #include, #define, macros
   Description : Démonstration du garde d'inclusion — le fichier types.h est
                 inclus deux fois mais le garde empêche la redéfinition
   Fichier source : 03.1-preprocesseur.md
   Compilation : g++ -std=c++23 -o 01-03-01-include-guard 01-03-01-include-guard.cpp
   Sortie attendue :
     Point: (3.5, 7.2)
   ============================================================================ */
#include <iostream>
#include "01-03-01-include-guard.h"
#include "01-03-01-include-guard.h" // inclusion volontairement dupliquée

int main() {
    Point p{3.5, 7.2};
    std::cout << "Point: (" << p.x << ", " << p.y << ")" << std::endl;
    return 0;
}
