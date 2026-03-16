/* ============================================================================
   Section 28.1/28.2 : Makefile réaliste
   Description : Programme principal avec dépendance auto sur les headers
   Fichier source : 01-syntaxe-makefiles.md, 02-variables-regles.md
   ============================================================================ */

#include <iostream>
#include "utils.h"

int main() {
    std::cout << greet("Make réaliste") << std::endl;
    return 0;
}
