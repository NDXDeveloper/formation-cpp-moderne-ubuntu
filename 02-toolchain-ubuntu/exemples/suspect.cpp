/* ============================================================================
   Section 2.6 / 2.6.1 : Options de compilation / Warnings
   Description : Code volontairement bogué pour illustrer les warnings
   Fichier source : 06-options-compilation.md / 06.1-warnings.md
   Compilation :
     g++ suspect.cpp -o suspect                        (aucun warning)
     g++ -Wall suspect.cpp -o suspect                  (3 warnings)
     g++ -Wall -O1 suspect.cpp -o suspect              (+Wmaybe-uninitialized)
     g++ -Wall -Wextra -Wconversion suspect.cpp -o suspect  (encore plus)
   ============================================================================ */
#include <iostream>

int calculer(int x) {
    int resultat;        // Non initialisé
    if (x > 0)
        resultat = x * 2;
    // Chemin manquant : que vaut resultat si x <= 0 ?
    return resultat;
}

int main() {
    unsigned int a = -1;            // Conversion signée → non signée
    int b = 3.14;                   // Troncature float → int

    if (a = 42) {                   // Affectation au lieu de comparaison
        std::cout << calculer(0) << std::endl;
    }

    int tableau[5];
    for (int i = 0; i <= 5; ++i) {  // Débordement : i va jusqu'à 5
        tableau[i] = i;
    }

    return 0;
}
