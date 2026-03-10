/* ============================================================================
   Section 5.5 / 5.5.1 / 5.5.2 : Outils de détection (Valgrind, ASan)
   Description : Programme volontairement bugué pour tester Valgrind et
                 AddressSanitizer. Contient 4 bugs intentionnels :
                 fuite mémoire, buffer overflow, use-after-free, double free.
                 ⚠️ Ce programme contient du comportement indéfini — il est
                 conçu pour être analysé par les outils, pas exécuté tel quel.
   Fichier source : 05-outils-detection.md
   ============================================================================
   Utilisation avec Valgrind :
     g++-15 -std=c++17 -g -O0 -o 07_bugs_memoire 07_bugs_memoire.cpp
     valgrind --leak-check=full ./07_bugs_memoire

   Utilisation avec AddressSanitizer :
     g++-15 -std=c++17 -g -O0 -fsanitize=address -o 07_bugs_asan 07_bugs_memoire.cpp
     ./07_bugs_asan
   ============================================================================ */
#include <iostream>
#include <cstring>

int main() {
    // Bug 1 : fuite mémoire
    int* fuite = new int[100];
    // Jamais libéré

    // Bug 2 : buffer overflow (écriture hors limites)
    int* tableau = new int[5];
    tableau[5] = 999;          // index 5 → hors limites (0 à 4 valides)

    // Bug 3 : use-after-free
    int* ephemere = new int(42);
    delete ephemere;
    std::cout << *ephemere << "\n";   // lecture après libération

    // Bug 4 : double free
    int* victime = new int(7);
    delete victime;
    delete victime;

    delete[] tableau;
    return 0;
}
