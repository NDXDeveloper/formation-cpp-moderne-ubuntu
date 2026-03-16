/* ============================================================================
   Section 27.6 : CMake Presets
   Description : Test minimal — vérifie que le build fonctionne
   Fichier source : 06-cmake-presets.md
   ============================================================================ */

#include <cassert>
#include <print>

int main() {
    assert(1 + 1 == 2);
    std::println("Tous les tests passent !");
    return 0;
}
