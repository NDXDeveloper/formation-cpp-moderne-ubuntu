/* ============================================================================
   Section 3.2 : Types primitifs, tailles et representation memoire
   Description : sizeof et alignof des types fondamentaux
   Fichier source : 02-types-primitifs.md
   ============================================================================ */
#include <print>

int main() {
    // --- sizeof des types fondamentaux (lignes 78-82) ---
    std::print("int    : {} octets\n", sizeof(int));      // 4
    std::print("double : {} octets\n", sizeof(double));    // 8
    std::print("char   : {} octet\n",  sizeof(char));      // 1

    // --- alignof (lignes 89-91) ---
    std::print("alignof(int)    : {}\n", alignof(int));    // 4
    std::print("alignof(double) : {}\n", alignof(double)); // 8
    std::print("alignof(char)   : {}\n", alignof(char));   // 1

    // Verifications
    static_assert(sizeof(char) == 1);
    static_assert(sizeof(char) <= sizeof(short));
    static_assert(sizeof(short) <= sizeof(int));
    static_assert(sizeof(int) <= sizeof(long));
    static_assert(sizeof(long) <= sizeof(long long));

    return 0;
}
