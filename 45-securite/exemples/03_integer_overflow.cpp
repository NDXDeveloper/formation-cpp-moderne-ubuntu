/* ============================================================================
   Section 45.2 : Integer overflows et underflows
   Description : Wrap-around unsigned (defini) vs signed overflow (UB)
   Fichier source : 02-integer-overflows.md
   ============================================================================ */

#include <cstdint>
#include <limits>
#include <print>

void unsigned_wrap() {
    uint32_t a = std::numeric_limits<uint32_t>::max();  // 4 294 967 295
    uint32_t b = a + 1;

    std::print("a     = {}\n", a);    // 4294967295
    std::print("a + 1 = {}\n", b);    // 0 (wrap-around)

    uint32_t c = 0;
    uint32_t d = c - 1;

    std::print("0 - 1 = {}\n", d);    // 4294967295
}

int main() {
    unsigned_wrap();
    std::print("\nNote: le signed overflow est un UB detectable par UBSan:\n");
    std::print("  g++-15 -fsanitize=undefined -o test main.cpp\n");
}
