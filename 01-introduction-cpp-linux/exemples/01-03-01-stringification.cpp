/* ============================================================================
   Section 1.3.1 : Le préprocesseur — #include, #define, macros
   Description : Opérateurs # (stringification) et ## (concaténation de tokens)
   Fichier source : 03.1-preprocesseur.md
   Compilation : g++ -std=c++23 -o 01-03-01-stringification 01-03-01-stringification.cpp
   Sortie attendue :
     === Stringification (#) ===
     TO_STRING(42 + 3) = 42 + 3
     TO_STRING(hello) = hello
     === Concaténation (##) ===
     value1 = 10
     value2 = 20
   ============================================================================ */
#include <iostream>

// Stringification
#define TO_STRING(x) #x

// Concaténation de tokens
#define MAKE_VAR(prefix, num) prefix##num

int main() {
    std::cout << "=== Stringification (#) ===" << std::endl;
    std::cout << "TO_STRING(42 + 3) = " << TO_STRING(42 + 3) << std::endl;
    std::cout << "TO_STRING(hello) = " << TO_STRING(hello) << std::endl;

    std::cout << "=== Concaténation (##) ===" << std::endl;
    int MAKE_VAR(value, 1) = 10;   // → int value1 = 10;
    int MAKE_VAR(value, 2) = 20;   // → int value2 = 20;
    std::cout << "value1 = " << value1 << std::endl;
    std::cout << "value2 = " << value2 << std::endl;

    return 0;
}
