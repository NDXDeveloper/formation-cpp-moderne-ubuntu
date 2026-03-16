/* ============================================================================
   Section 26.2.4 : PUBLIC, PRIVATE, INTERFACE
   Description : Démonstration des cibles INTERFACE pour warnings,
                 sanitizers et configuration globale
   Fichier source : 02.4-public-private-interface.md
   ============================================================================ */

#include <print>

int main() {
#ifdef MY_PROJECT_DEBUG
    std::println("Mode: Debug");
#elif defined(MY_PROJECT_RELEASE)
    std::println("Mode: Release");
#else
    std::println("Mode: Non spécifié");
#endif
    std::println("Les cibles INTERFACE fonctionnent !");
    return 0;
}
