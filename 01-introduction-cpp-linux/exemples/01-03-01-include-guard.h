/* ============================================================================
   Section 1.3.1 : Le préprocesseur — #include, #define, macros
   Description : En-tête avec garde d'inclusion (#ifndef / #define / #endif)
                 Utilisé par 01-03-01-include-guard.cpp
   Fichier source : 03.1-preprocesseur.md
   ============================================================================ */
#ifndef TYPES_H
#define TYPES_H

struct Point {
    double x;
    double y;
};

#endif // TYPES_H
