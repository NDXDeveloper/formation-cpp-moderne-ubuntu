/* ============================================================================
   Section 26.5 / 26.6 : CMake Presets
   Description : Programme utilisant cmake --preset pour la configuration
   Fichier source : 05-generation-ninja.md, 06-toolchains-cross-compilation.md
   ============================================================================ */

#include <print>

int main() {
#ifdef NDEBUG
    std::println("Preset: release (NDEBUG défini)");
#else
    std::println("Preset: default/debug (NDEBUG non défini)");
#endif
    std::println("CMake Presets fonctionnent !");
    return 0;
}
