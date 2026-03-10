/* ============================================================================
   Section 2.6.1 : Warnings
   Description : Erreur de type intentionnelle — comparaison diagnostics GCC/Clang
   Fichier source : 06.1-warnings.md
   Compilation : g++ -Wall -Wextra type_error.cpp -o type_error   (ERREUR attendue)
                 clang++ -Wall -Wextra type_error.cpp -o type_error (ERREUR attendue)
   Comportement : Ne compile PAS — montre la qualité des diagnostics
   ============================================================================ */
#include <string>

int main() {
    std::string s = 42;  // Erreur : pas de conversion int → string
    return 0;
}
