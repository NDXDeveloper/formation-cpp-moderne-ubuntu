/* ============================================================================
   Section 27.5 : Installation et distribution de librairies sur Linux
   Description : Programme consommateur qui utilise mylib via find_package()
                 après installation dans un préfixe local
   Fichier source : 05-distribution-linux.md
   ============================================================================ */

#include <mylib/greet.h>
#include <print>

int main() {
    std::println("{}", mylib::greet("find_package"));
    return 0;
}
