/* ============================================================================
   Section 27.4 : Linkage statique (.a) vs dynamique (.so)
   Description : Programme utilisant greeter — le mode de linkage est
                 contrôlé par BUILD_SHARED_LIBS au moment de cmake -B
   Fichier source : 04-linkage-statique-dynamique.md
   ============================================================================ */

#include "greeter.h"
#include <print>

int main() {
    std::println("{}", greeter::greet("CMake"));
    return 0;
}
