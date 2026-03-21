/* ============================================================================
   Section 46.2 : Separation .h/.cpp et compilation incrementale
   Description : Test du Pimpl — le main n'inclut que le header public
   Fichier source : 02-separation-h-cpp.md
   ============================================================================ */

#include "01_pimpl.h"
#include <iostream>

int main() {
    monprojet::core::Engine e("default.yaml");
    std::cout << "Start: " << (e.start() ? "OK" : "FAIL") << "\n";
    std::cout << "Count: " << e.processed_count() << "\n";
    e.stop();
    std::cout << "Pimpl pattern works correctly\n";
}
