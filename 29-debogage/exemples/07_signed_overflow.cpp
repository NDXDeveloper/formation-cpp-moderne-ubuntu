/* ============================================================================
   Section 29.4.2 : UndefinedBehaviorSanitizer (-fsanitize=undefined)
   Description : Signed integer overflow — INT_MAX + 1 est un UB
   Fichier source : 04.2-ubsan.md
   Compilation : g++-15 -fsanitize=undefined -g -O1 -fno-omit-frame-pointer -o signed_overflow 07_signed_overflow.cpp
   ============================================================================ */

#include <cstdio>
#include <climits>

int main() {
    int x = INT_MAX;
    int y = x + 1;    // UB : overflow d'entier signé

    std::printf("INT_MAX + 1 = %d\n", y);

    return 0;
}
