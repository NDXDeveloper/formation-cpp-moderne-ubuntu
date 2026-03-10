/* ============================================================================
   Section 2.6.4 / 2.7.3 : Standard / Support compilateur
   Description : Vérification des feature test macros (print, format, concepts)
   Fichier source : 06.4-standard.md / 07.3-support-compilateur.md
   Compilation : g++ -std=c++23 -Wall -Wextra feature_test.cpp -o feature_test
   ============================================================================ */
#include <version>
#include <iostream>

int main() {
    std::cout << "__cplusplus = " << __cplusplus << std::endl;
    std::cout << std::endl;

#ifdef __cpp_concepts
    std::cout << "__cpp_concepts = " << __cpp_concepts << std::endl;
#else
    std::cout << "__cpp_concepts : non disponible" << std::endl;
#endif

#ifdef __cpp_lib_expected
    std::cout << "__cpp_lib_expected = " << __cpp_lib_expected << std::endl;
#else
    std::cout << "__cpp_lib_expected : non disponible" << std::endl;
#endif

#ifdef __cpp_lib_format
    std::cout << "__cpp_lib_format = " << __cpp_lib_format << std::endl;
#else
    std::cout << "__cpp_lib_format : non disponible" << std::endl;
#endif

#ifdef __cpp_lib_print
    std::cout << "__cpp_lib_print = " << __cpp_lib_print << std::endl;
#else
    std::cout << "__cpp_lib_print : non disponible" << std::endl;
#endif

    return 0;
}
