/* ============================================================================
   Section 2.5.1 : Compilation étape par étape
   Description : Vérifie que les include guards empêchent la double inclusion
   Fichier source : 05.1-compilation-etapes.md
   Compilation : g++ -E test_double_include.cpp -o test_double.ii
   Vérification : grep 'aire_cercle' test_double.ii  (doit n'apparaître qu'une fois)
   ============================================================================ */
#include "mathutils.hpp"
#include "mathutils.hpp"

int main() { return 0; }
