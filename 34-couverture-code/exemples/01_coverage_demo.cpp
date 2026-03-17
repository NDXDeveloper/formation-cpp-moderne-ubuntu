/* ============================================================================
   Section 34.1 : gcov et lcov — Mesure de la couverture
   Description : Programme de démonstration pour le workflow de couverture :
                 compilation --coverage, exécution, gcov, lcov, genhtml
   Fichier source : README.md, 01-gcov-lcov.md
   Compilation : g++-15 --coverage -O0 -g -std=c++20 -o coverage_demo 01_coverage_demo.cpp
   Exécution  : ./coverage_demo && gcov-15 01_coverage_demo.cpp
   ============================================================================ */

#include <string>
#include <cstdio>

std::string classify(int value) {
    if (value > 0)
        return "positive";
    else if (value == 0)
        return "zero";
    else
        return "negative";
}

bool validate_port(int port) {
    if (port < 1 || port > 65535)
        return false;
    return true;
}

int main() {
    // Test classify — toutes les branches
    std::printf("classify(42)  = %s\n", classify(42).c_str());
    std::printf("classify(0)   = %s\n", classify(0).c_str());
    std::printf("classify(-5)  = %s\n", classify(-5).c_str());

    // Test validate_port — certaines branches seulement
    std::printf("port(8080)    = %s\n", validate_port(8080) ? "valid" : "invalid");
    std::printf("port(0)       = %s\n", validate_port(0) ? "valid" : "invalid");
    // Note : port(70000) n'est PAS testé — branche non couverte visible dans gcov

    return 0;
}
