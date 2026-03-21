/* ============================================================================
   Section 47.6 : Semantic Versioning et changelogs
   Description : Utilisation de version.h genere — affiche la version du projet
   Fichier source : 06-semantic-versioning.md
   ============================================================================ */

#include "version.h"
#include <print>
#include <string_view>

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string_view(argv[1]) == "--version") {
        std::println("myproject {}", myproject::VERSION_STRING);
        return 0;
    }

    std::println("myproject {} — run with --version for version info",
                 myproject::VERSION_STRING);
    std::println("  Major: {}", myproject::VERSION_MAJOR);
    std::println("  Minor: {}", myproject::VERSION_MINOR);
    std::println("  Patch: {}", myproject::VERSION_PATCH);
    return 0;
}
