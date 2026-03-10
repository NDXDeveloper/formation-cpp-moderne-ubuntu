/* ============================================================================
   Section 2.0 : README du chapitre
   Description : Premier exemple du chapitre — Hello avec std::print (C++23)
   Fichier source : README.md
   Compilation : g++ -std=c++23 -Wall -Wextra readme_hello.cpp -o readme_hello
   ============================================================================ */
#include <print>

int main() {
    std::print("Hello, {}!\n", "Ubuntu");
}
