/* ============================================================================
   Section 2.7 : Introduction a std::print (C++23)
   Description : Premier programme avec std::print et std::println
   Fichier source : 07-std-print.md
   Compilation : g++ -std=c++23 -Wall -Wextra hello_print.cpp -o hello_print
   ============================================================================ */
#include <print>
#include <string>

int main() {
    std::string nom = "Ubuntu";
    int version = 24;
    double pi = 3.14159265358979;

    std::println("Bienvenue sur {} {} !", nom, version);
    std::println("Pi vaut approximativement {:.4f}", pi);
    std::println("En hexadécimal : {:#x}", 255);

    return 0;
}
