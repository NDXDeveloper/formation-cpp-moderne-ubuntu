/* ============================================================================
   Section 1.3.1 : Le préprocesseur — #include, #define, macros
   Description : Macros constantes vs constexpr C++ moderne
                 Montre la différence entre le style hérité (#define) et
                 le style C++ moderne (constexpr, string_view)
   Fichier source : 03.1-preprocesseur.md
   Compilation : g++ -std=c++23 -o 01-03-01-macros-constantes 01-03-01-macros-constantes.cpp
   Sortie attendue :
     === Style hérité (#define) ===
     MAX_CONNECTIONS = 1024
     VERSION = 3.2.1
     PI = 3.14159
     === Style C++ moderne (constexpr) ===
     max_connections = 1024
     version = 3.2.1
     pi = 3.14159
   ============================================================================ */
#include <iostream>
#include <string_view>

// Style hérité (C / C++98)
#define MAX_CONNECTIONS 1024
#define VERSION "3.2.1"
#define PI 3.14159265358979323846

// Style C++ moderne
constexpr int max_connections = 1024;
constexpr std::string_view version = "3.2.1";
constexpr double pi = 3.14159265358979323846;

int main() {
    std::cout << "=== Style hérité (#define) ===" << std::endl;
    std::cout << "MAX_CONNECTIONS = " << MAX_CONNECTIONS << std::endl;
    std::cout << "VERSION = " << VERSION << std::endl;
    std::cout << "PI = " << PI << std::endl;

    std::cout << "=== Style C++ moderne (constexpr) ===" << std::endl;
    std::cout << "max_connections = " << max_connections << std::endl;
    std::cout << "version = " << version << std::endl;
    std::cout << "pi = " << pi << std::endl;

    return 0;
}
