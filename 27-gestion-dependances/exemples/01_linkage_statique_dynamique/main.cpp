/* ============================================================================
   Section 27.4 : Linkage statique (.a) vs dynamique (.so)
   Description : Programme principal qui utilise la bibliothèque greeter —
                 compilable en linkage statique ou dynamique
   Fichier source : 04-linkage-statique-dynamique.md
   ============================================================================ */

#include "greeter.h"
#include <iostream>

int main() {
    std::cout << greeter::greet("world") << std::endl;
    std::cout << "Version: " << greeter::version() << std::endl;
    return 0;
}
