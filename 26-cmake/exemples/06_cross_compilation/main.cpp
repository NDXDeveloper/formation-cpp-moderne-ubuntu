/* ============================================================================
   Section 26.6 : Toolchains et cross-compilation
   Description : Programme cross-compilable — affiche l'architecture cible
   Fichier source : 06-toolchains-cross-compilation.md
   ============================================================================ */

#include <iostream>

int main() {
#if defined(__aarch64__)
    std::cout << "Architecture: ARM64 (AArch64)" << std::endl;
#elif defined(__riscv)
    std::cout << "Architecture: RISC-V" << std::endl;
#elif defined(__x86_64__)
    std::cout << "Architecture: x86_64" << std::endl;
#else
    std::cout << "Architecture: inconnue" << std::endl;
#endif
    std::cout << "Cross-compilation fonctionne !" << std::endl;
    return 0;
}
