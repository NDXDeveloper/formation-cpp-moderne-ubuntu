/* ============================================================================
   Section 28.4 : Meson — Build system montant
   Description : Programme principal construit avec Meson
   Fichier source : 04-meson.md, 04.1-syntaxe-meson.md
   ============================================================================ */

#include "greeter.h"
#include <iostream>

int main() {
    std::cout << greeter::greet("world") << std::endl;
    return 0;
}
