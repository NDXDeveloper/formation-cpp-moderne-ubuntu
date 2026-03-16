/* ============================================================================
   Section 32.2 : cppcheck — Détection d'erreurs
   Description : Programme avec défauts détectables par cppcheck :
                 pointeur vers variable locale, fuite mémoire sur chemin
                 d'erreur, variable non initialisée
   Fichier source : 02-cppcheck.md
   Exécution : cppcheck --enable=all 02_cppcheck_defauts.cpp
   ============================================================================ */

#include <cstring>

void copier_donnees(char* dest, const char* src, int taille) {
    for (int i = 0; i <= taille; ++i) {  // Off-by-one : <= au lieu de <
        dest[i] = src[i];
    }
}

int* creer_tableau() {
    int tab[10];
    tab[0] = 42;
    return tab;  // Retourne un pointeur vers une variable locale
}

void traiter() {
    int* p = new int[100];
    if (p[0] > 0) {
        return;  // Fuite mémoire sur ce chemin
    }
    delete[] p;
}

void calcul() {
    int x;
    int y = x + 1;  // Utilisation d'une variable non initialisée
}
