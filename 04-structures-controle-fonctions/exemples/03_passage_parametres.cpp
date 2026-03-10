/* ============================================================================
   Section 4.3 : Passage de paramètres
   Description : Démonstration des quatre modes de passage (valeur, référence,
                 référence constante, pointeur) et comparaison de leur effet
                 sur la variable originale
   Fichier source : 03-passage-parametres.md
   ============================================================================ */
#include <iostream>

void par_valeur(int x)        { x = 99; }
void par_reference(int& x)    { x = 99; }
void par_ref_const(const int& x) {
    // x = 99;  // ❌ Erreur de compilation — const
    std::cout << "  ref_const lit : " << x << "\n";
}
void par_pointeur(int* x)     { *x = 99; }

int main() {
    int a = 0, b = 0, c = 0, d = 0;

    par_valeur(a);
    par_reference(b);
    par_ref_const(c);
    par_pointeur(&d);

    std::cout << "a=" << a << "\n";  // a=0  (copie modifiée, original intact)
    std::cout << "b=" << b << "\n";  // b=99 (original modifié via référence)
    std::cout << "c=" << c << "\n";  // c=0  (lecture seule, original intact)
    std::cout << "d=" << d << "\n";  // d=99 (original modifié via pointeur)

    return 0;
}
