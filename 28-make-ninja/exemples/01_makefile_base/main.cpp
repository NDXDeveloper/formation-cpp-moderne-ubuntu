/* ============================================================================
   Section 28.1 : Syntaxe de base des Makefiles
   Description : Programme principal utilisant deux modules (utils, network)
   Fichier source : 01-syntaxe-makefiles.md
   ============================================================================ */

#include <iostream>
#include <string>

std::string get_utils_msg();
std::string get_network_msg();

int main() {
    std::cout << "Main: Hello" << std::endl;
    std::cout << get_utils_msg() << std::endl;
    std::cout << get_network_msg() << std::endl;
    return 0;
}
