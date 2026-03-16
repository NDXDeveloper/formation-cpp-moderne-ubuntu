/* ============================================================================
   Section 27.4 : Linkage statique (.a) vs dynamique (.so)
   Description : Implémentation de la bibliothèque greeter
   Fichier source : 04-linkage-statique-dynamique.md
   ============================================================================ */

#include "greeter.h"

namespace greeter {

std::string greet(const std::string& name) {
    return "Hello, " + name + "!";
}

std::string version() {
    return "1.0.0";
}

} // namespace greeter
