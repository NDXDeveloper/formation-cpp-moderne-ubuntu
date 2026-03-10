/* ============================================================================
   Section 6.1 : Définition de classes — Fil conducteur
   Description : Test de la première version de DynArray
   Fichier source : 01-definition-classes.md
   Compilation : g++ -std=c++17 -Wall -Wextra 01_dynarray_v1.cpp
                     01_dynarray_v1_main.cpp -o 01_dynarray_v1
   ============================================================================ */
#include "01_dynarray_v1.h"
#include <iostream>

int main() {
    DynArray arr(5);
    std::cout << "Size: " << arr.size() << "\n";
    std::cout << "Empty: " << arr.empty() << "\n";

    arr[0] = 42;
    arr[4] = 99;
    std::cout << "arr[0] = " << arr[0] << "\n";
    std::cout << "arr[4] = " << arr[4] << "\n";

    try {
        arr[5] = 100;   // out of range
    } catch (const std::out_of_range& e) {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return 0;
}
// Sortie attendue :
// Size: 5
// Empty: 0
// arr[0] = 42
// arr[4] = 99
// Exception: DynArray: index out of range
