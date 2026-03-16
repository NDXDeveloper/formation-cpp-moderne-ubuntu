/* ============================================================================
   Section 29.4.2 : UndefinedBehaviorSanitizer (-fsanitize=undefined)
   Description : Shift invalide — décalage >= largeur du type, et shift négatif
   Fichier source : 04.2-ubsan.md
   Compilation : g++-15 -fsanitize=undefined -g -O1 -fno-omit-frame-pointer -std=c++20 -o shift_ub 10_shift_ub.cpp
   ============================================================================ */

#include <cstdio>
#include <cstdint>

int main() {
    int32_t a = 1;
    int32_t b = a << 32;    // UB : shift >= largeur du type (32 bits)
    std::printf("1 << 32 = %d\n", b);

    int32_t c = -1;
    int32_t d = c << 2;     // UB avant C++20 : shift gauche d'un négatif
    std::printf("-1 << 2 = %d\n", d);

    int shift_amount = -3;
    int32_t e = a << shift_amount;   // UB : shift par valeur négative
    std::printf("1 << -3 = %d\n", e);

    return 0;
}
