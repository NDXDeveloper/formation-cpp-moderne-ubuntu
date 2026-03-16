/* ============================================================================
   Section 26.5 : Génération pour Ninja
   Description : Programme simple illustrant Ninja Multi-Config —
                 affiche la configuration de build détectée
   Fichier source : 05-generation-ninja.md
   ============================================================================ */

#include <print>

int main() {
#ifdef NDEBUG
    std::println("Configuration: Release (NDEBUG défini)");
#else
    std::println("Configuration: Debug (NDEBUG non défini)");
#endif
    std::println("Ninja Multi-Config fonctionne !");
    return 0;
}
