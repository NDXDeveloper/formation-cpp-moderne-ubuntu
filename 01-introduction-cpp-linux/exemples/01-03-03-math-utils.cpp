/* ============================================================================
   Section 1.3.3 : L'édition de liens — Résolution des symboles
   Description : Fichier de bibliothèque contenant deux fonctions (ajouter,
                 multiplier) pour démontrer la compilation séparée et le
                 linkage. Voir aussi 01-03-03-main-linkage.cpp
   Fichier source : 03.3-edition-liens.md
   Compilation : g++ -c 01-03-03-math-utils.cpp -o math_utils.o
   ============================================================================ */
int ajouter(int a, int b) {
    return a + b;
}

int multiplier(int a, int b) {
    return a * b;
}
