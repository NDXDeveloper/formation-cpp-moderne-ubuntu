/* ============================================================================
   Section 1.3.3 : L'édition de liens — Résolution des symboles
   Description : Programme principal démontrant la compilation séparée et
                 l'édition de liens. Déclare ajouter() et multiplier() qui
                 sont définis dans 01-03-03-math-utils.cpp.
                 Démontre aussi le name mangling et l'inspection avec nm.
   Fichier source : 03.3-edition-liens.md
   Compilation :
     g++ -c 01-03-03-main-linkage.cpp -o main.o
     g++ -c 01-03-03-math-utils.cpp -o math_utils.o
     g++ main.o math_utils.o -o 01-03-03-programme
   Inspection des symboles :
     nm main.o            (U _Z7ajouterii, U _Z10multiplierii, T main)
     nm math_utils.o      (T _Z7ajouterii, T _Z10multiplierii)
     nm -C main.o         (noms démanglés)
     echo "_Z7ajouterii" | c++filt   → ajouter(int, int)
   Sortie attendue : Somme: 7, Produit: 12
   ============================================================================ */
#include <cstdio>

int ajouter(int a, int b);     // déclaration (pas de définition)
int multiplier(int a, int b);  // déclaration (pas de définition)

int main() {
    int somme = ajouter(3, 4);
    int produit = multiplier(3, 4);
    printf("Somme: %d, Produit: %d\n", somme, produit);
    return 0;
}
