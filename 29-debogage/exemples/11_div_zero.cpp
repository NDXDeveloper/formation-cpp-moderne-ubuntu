/* ============================================================================
   Section 29.4.2 : UndefinedBehaviorSanitizer (-fsanitize=undefined)
   Description : Division par zéro (entière) — UB détecté par UBSan
   Fichier source : 04.2-ubsan.md
   Compilation : g++-15 -fsanitize=undefined -g -O1 -fno-omit-frame-pointer -o div_zero 11_div_zero.cpp
   ============================================================================ */

#include <cstdio>

int average(int total, int count) {
    return total / count;    // UB si count == 0
}

int main() {
    std::printf("Moyenne : %d\n", average(100, 0));
    return 0;
}
