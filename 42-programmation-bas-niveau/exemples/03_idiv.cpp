/* ============================================================================
   Section 42.1 : Inline Assembly en C++
   Description : Division entiere via idiv (clobber cc, registres nommes)
   Fichier source : 01-inline-assembly.md
   ============================================================================ */

#include <cstdint>
#include <iostream>

int main() {
    int32_t a = 10, b = 3, quotient, remainder;
    asm(
        "cdq\n\t"
        "idivl %[divisor]"
        : "=a" (quotient),
          "=d" (remainder)
        : "a"  (a),
          [divisor] "r" (b)
        : "cc"
    );

    std::cout << a << " / " << b << " = " << quotient
              << " remainder " << remainder << "\n";
    // 10 / 3 = 3 remainder 1
}
