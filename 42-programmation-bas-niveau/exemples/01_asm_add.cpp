/* ============================================================================
   Section 42.1 : Inline Assembly en C++
   Description : Addition de deux entiers via asm etendu (extended asm)
   Fichier source : 01-inline-assembly.md
   ============================================================================ */

#include <cstdint>
#include <iostream>

int main() {
    int64_t a = 42;
    int64_t b = 18;
    int64_t result;

    asm(
        "movq %[src1], %[dst]\n\t"
        "addq %[src2], %[dst]"
        : [dst] "=r" (result)
        : [src1] "r" (a),
          [src2] "r" (b)
    );

    std::cout << "result = " << result << "\n";  // 60
}
