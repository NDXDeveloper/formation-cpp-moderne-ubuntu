/* ============================================================================
   Section 2.5 : Premier programme (multi-fichiers)
   Description : Implémentation des fonctions mathématiques
   Fichier source : 05-premier-programme.md / 05.1-compilation-etapes.md
   Compilation : g++ -c mathutils.cpp -o mathutils.o
   ============================================================================ */
#include "mathutils.hpp"

double aire_cercle(double rayon) {
    return PI * rayon * rayon;
}

double perimetre_cercle(double rayon) {
    return 2.0 * PI * rayon;
}
