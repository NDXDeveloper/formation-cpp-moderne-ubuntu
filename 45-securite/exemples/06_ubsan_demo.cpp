/* ============================================================================
   Section 45.2 : Integer overflows et underflows
   Description : Signed overflow volontaire — a compiler avec -fsanitize=undefined
   Fichier source : 02-integer-overflows.md
   ============================================================================ */

// ATTENTION : ce programme contient un UB VOLONTAIRE (signed integer overflow)
// Il est concu pour etre detecte par UndefinedBehaviorSanitizer :
//   g++-15 -std=c++23 -O0 -fsanitize=undefined -o test 06_ubsan_demo.cpp
//   ./test   → UBSan signale "signed integer overflow"

#include <cstdint>
#include <limits>
#include <iostream>

int main() {
    int32_t x = std::numeric_limits<int32_t>::max();  // 2147483647
    int32_t y = x + 1;  // UB: signed integer overflow
    std::cout << "x = " << x << ", x+1 = " << y << "\n";
}
