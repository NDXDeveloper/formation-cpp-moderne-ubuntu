/* ============================================================================
   Section 5.5.2 : AddressSanitizer — Détection stack-buffer-overflow
   Description : Programme avec un accès hors limites sur un tableau local
                 (stack). Conçu pour démontrer la détection par ASan des
                 buffer overflows sur la stack, une capacité que Valgrind
                 ne possède pas.
   Fichier source : 05.2-addresssanitizer.md
   ============================================================================
   Compilation avec ASan :
     g++-15 -std=c++17 -g -O1 -fsanitize=address -o 08_stack_asan 08_stack_overflow_asan.cpp
     ./08_stack_asan
   ============================================================================ */

int main() {
    int tableau[5] = {1, 2, 3, 4, 5};
    return tableau[10];    // lecture hors limites sur la stack
}
