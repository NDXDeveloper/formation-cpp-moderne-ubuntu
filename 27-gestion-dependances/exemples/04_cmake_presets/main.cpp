/* ============================================================================
   Section 27.6 : CMake Presets
   Description : Programme principal — affiche la configuration de build
   Fichier source : 06-cmake-presets.md
   ============================================================================ */

#include <print>

int main() {
#ifdef NDEBUG
    std::println("Configuration: Release");
#else
    std::println("Configuration: Debug");
#endif
    std::println("CMake Presets fonctionnent !");
    return 0;
}
