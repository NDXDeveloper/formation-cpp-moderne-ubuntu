/* ============================================================================
   Section 29.4.1 : AddressSanitizer (-fsanitize=address)
   Description : Heap buffer overflow — accès au-delà d'un tableau alloué
                 sur le heap (boucle <= au lieu de <)
   Fichier source : 04.1-addresssanitizer.md
   Compilation : g++-15 -fsanitize=address -g -O1 -fno-omit-frame-pointer -o heap_overflow 02_heap_overflow.cpp
   ============================================================================ */

#include <cstdio>

int main() {
    int* array = new int[10];

    for (int i = 0; i <= 10; ++i) {   // Bug : <= au lieu de <
        array[i] = i * 100;
    }

    std::printf("Dernier : %d\n", array[9]);
    delete[] array;
    return 0;
}
