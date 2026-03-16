/* ============================================================================
   Section 29.5 : std::stacktrace — Traces d'exécution intégrées
   Description : Capture et affichage de la pile d'appels depuis le code
                 avec std::stacktrace::current() (C++23)
   Fichier source : 05-stacktrace-debug.md
   Compilation : g++-15 -std=c++23 -g -O0 -o stacktrace 09_stacktrace.cpp -lstdc++exp
   ============================================================================ */

#include <stacktrace>
#include <iostream>

void inner_function() {
    // Capture la pile d'appels à cet instant
    auto trace = std::stacktrace::current();
    std::cout << "=== Pile d'appels ===" << '\n';
    std::cout << trace << '\n';

    // Accès aux entrées individuelles
    std::cout << "=== Détails ===" << '\n';
    for (const auto& entry : trace) {
        std::cout << "  Fonction : " << entry.description() << '\n'
                  << "  Fichier  : " << entry.source_file() << '\n'
                  << "  Ligne    : " << entry.source_line() << '\n'
                  << '\n';
    }
}

void middle_function() {
    inner_function();
}

int main() {
    middle_function();
    return 0;
}
