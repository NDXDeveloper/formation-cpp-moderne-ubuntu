/* ============================================================================
   Section 29.4.1 : AddressSanitizer (-fsanitize=address)
   Description : Double-free — libérer deux fois la même allocation
   Fichier source : 04.1-addresssanitizer.md
   Compilation : g++-15 -fsanitize=address -g -O1 -fno-omit-frame-pointer -o double_free 05_double_free.cpp
   ============================================================================ */

#include <cstdio>

void cleanup(int* data) {
    delete[] data;
}

int main() {
    int* values = new int[5]{1, 2, 3, 4, 5};

    cleanup(values);

    // ... du code intermédiaire ...

    delete[] values;    // Bug : déjà libéré par cleanup()

    return 0;
}
