/* ============================================================================
   Section 38.5 : Artifacts et gestion des releases
   Description : Programme avec --version alimenté par configure_file
   Fichier source : 05-artifacts-releases.md
   ============================================================================ */
#include "version.hpp"
#include <print>
#include <cstring>

int main(int argc, char* argv[]) {
    if (argc > 1 && std::strcmp(argv[1], "--version") == 0) {
        std::println("mon-application v{}", PROJECT_VERSION);
        return 0;
    }
    std::println("mon-application v{} — prêt", PROJECT_VERSION);
    return 0;
}
