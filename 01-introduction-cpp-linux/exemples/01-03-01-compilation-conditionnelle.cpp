/* ============================================================================
   Section 1.3.1 : Le préprocesseur — #include, #define, macros
   Description : Compilation conditionnelle avec __cplusplus et macros
                 prédéfinies. Montre l'adaptation au standard C++ utilisé.
   Fichier source : 03.1-preprocesseur.md
   Compilation : g++ -std=c++23 -o 01-03-01-compilation-conditionnelle 01-03-01-compilation-conditionnelle.cpp
   Sortie attendue (avec -std=c++23) :
     === Macros prédéfinies ===
     __cplusplus = 202302
     __FILE__ = 01-03-01-compilation-conditionnelle.cpp
     Compilateur : GCC <version>
     === Compilation conditionnelle ===
     C++23 ou plus récent
   ============================================================================ */
#include <iostream>

int main() {
    std::cout << "=== Macros prédéfinies ===" << std::endl;
    std::cout << "__cplusplus = " << __cplusplus << std::endl;
    std::cout << "__FILE__ = " << __FILE__ << std::endl;

#ifdef __GNUC__
    std::cout << "Compilateur : GCC " << __GNUC__ << "." << __GNUC_MINOR__ << std::endl;
#elif defined(__clang__)
    std::cout << "Compilateur : Clang " << __clang_major__ << "." << __clang_minor__ << std::endl;
#else
    std::cout << "Compilateur : inconnu" << std::endl;
#endif

    std::cout << "=== Compilation conditionnelle ===" << std::endl;

#if __cplusplus >= 202302L
    std::cout << "C++23 ou plus récent" << std::endl;
#elif __cplusplus >= 202002L
    std::cout << "C++20" << std::endl;
#elif __cplusplus >= 201703L
    std::cout << "C++17" << std::endl;
#else
    std::cout << "Standard antérieur à C++17" << std::endl;
#endif

    return 0;
}
