/* ============================================================================
   Section 41.1.1 : Cache L1, L2, L3
   Description : Detection de la taille de cache line via C++17
   Fichier source : 01.1-niveaux-cache.md
   ============================================================================ */

#include <new>
#include <cstddef>
#include <iostream>

int main() {
    #ifdef __cpp_lib_hardware_interference_size
        std::cout << "Cache line (destructive): "
                  << std::hardware_destructive_interference_size << " octets\n";
        std::cout << "Cache line (constructive): "
                  << std::hardware_constructive_interference_size << " octets\n";
    #else
        std::cout << "Cache line (fallback): 64 octets\n";
    #endif
}
