/* ============================================================================
   Section 1.3.3 : L'édition de liens — Résolution des symboles
   Description : Démonstration de l'erreur "multiple definition" et de la
                 solution avec inline (C++17). Montre aussi extern "C"
                 pour désactiver le name mangling.
   Fichier source : 03.3-edition-liens.md
   Compilation : g++ -std=c++17 -o 01-03-03-inline-header 01-03-03-inline-header.cpp
   Sortie attendue :
     compteur = 0
     fonction_c appelée avec x = 42
   ============================================================================ */
#include <iostream>

// C++17 : inline permet des définitions multiples sans erreur de linker
inline int compteur = 0;

// extern "C" désactive le name mangling
extern "C" {
    void fonction_c(int x) {
        std::cout << "fonction_c appelée avec x = " << x << std::endl;
    }
}

int main() {
    std::cout << "compteur = " << compteur << std::endl;
    fonction_c(42);
    return 0;
}
