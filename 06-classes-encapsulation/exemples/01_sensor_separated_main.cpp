/* ============================================================================
   Section 6.1 : Définition de classes — Membres et méthodes
   Description : Séparation déclaration / définition — programme principal
   Fichier source : 01-definition-classes.md
   Compilation : g++ -std=c++17 -Wall -Wextra 01_sensor_separated.cpp
                     01_sensor_separated_main.cpp -o 01_sensor_separated
   ============================================================================ */
#include "01_sensor_separated.h"
#include <iostream>

int main() {
    Sensor s(1, "Temperature");
    s.read(23.5);
    std::cout << s.to_string() << "\n";

    return 0;
}
// Sortie attendue :
// Temperature (#1): 23.5
