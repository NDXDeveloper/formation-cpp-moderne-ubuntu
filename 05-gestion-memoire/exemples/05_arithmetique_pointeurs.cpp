/* ============================================================================
   Section 5.3 : Arithmétique des pointeurs et accès bas niveau
   Description : Arithmétique de base (addition, soustraction), équivalence
                 pointeur-tableau, parcours idiomatique, pointeurs vers
                 structures, void*, nullptr, et les 4 formes de const
   Fichier source : 03-arithmetique-pointeurs.md
   ============================================================================ */
#include <iostream>
#include <cstddef>   // std::ptrdiff_t
#include <vector>

int main() {
    // --- Pointeur basique (lignes 10-14) ---
    std::cout << "--- Pointeur basique ---\n";
    int valeur = 42;
    int* ptr = &valeur;
    std::cout << "adresse : " << ptr << "\n";
    std::cout << "valeur  : " << *ptr << "\n";   // 42

    // --- Arithmétique de base (lignes 28-34) ---
    std::cout << "\n--- Arithmétique int ---\n";
    int tableau[] = {10, 20, 30, 40, 50};
    int* p = tableau;
    std::cout << *p << "\n";       // 10
    std::cout << *(p + 1) << "\n"; // 20
    std::cout << *(p + 2) << "\n"; // 30
    std::cout << *(p + 4) << "\n"; // 50

    // --- Double (lignes 50-56) ---
    std::cout << "\n--- Arithmétique double ---\n";
    double valeurs[] = {1.1, 2.2, 3.3};
    double* d = valeurs;
    std::cout << *d << "\n";       // 1.1
    std::cout << *(d + 1) << "\n"; // 2.2
    std::cout << *(d + 2) << "\n"; // 3.3

    // --- Soustraction (lignes 62-67) ---
    std::cout << "\n--- Soustraction ---\n";
    int tab2[] = {10, 20, 30, 40, 50};
    int* debut = &tab2[0];
    int* fin   = &tab2[4];
    std::ptrdiff_t distance = fin - debut;
    std::cout << "distance = " << distance << "\n";  // 4

    // --- Équivalence pointeur-tableau (lignes 79-85) ---
    std::cout << "\n--- Equivalence pointeur-tableau ---\n";
    int tab[] = {100, 200, 300};
    std::cout << "tab[2]      = " << tab[2] << "\n";      // 300
    std::cout << "*(tab + 2)  = " << *(tab + 2) << "\n";  // 300
    std::cout << "*(2 + tab)  = " << *(2 + tab) << "\n";  // 300
    std::cout << "2[tab]      = " << 2[tab] << "\n";      // 300

    // --- Parcours idiomatique (lignes 119-128) ---
    std::cout << "\n--- Parcours avec pointeur ---\n";
    int donnees[] = {5, 10, 15, 20, 25};
    int* pp = donnees;
    int* pfin = donnees + 5;   // past-the-end
    while (pp != pfin) {
        std::cout << *pp << " ";
        ++pp;
    }
    std::cout << "\n";  // 5 10 15 20 25

    // --- Structures et -> (lignes 151-171) ---
    std::cout << "\n--- Structures et -> ---\n";
    struct Point {
        double x;
        double y;
    };

    Point* pt = new Point{3.0, 4.0};
    std::cout << "(*pt).x = " << (*pt).x << "\n";   // 3
    std::cout << "pt->x   = " << pt->x << "\n";     // 3

    Point points[] = {{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}};
    Point* curseur = points;
    std::cout << "curseur->x       = " << curseur->x << "\n";        // 1
    std::cout << "(curseur+1)->y   = " << (curseur + 1)->y << "\n";  // 4
    std::cout << "(curseur+2)->x   = " << (curseur + 2)->x << "\n";  // 5

    delete pt;

    // --- void* (lignes 182-196) ---
    std::cout << "\n--- void* ---\n";
    int entier = 42;
    double flottant = 3.14;

    void* generique = &entier;
    int* pi = static_cast<int*>(generique);
    std::cout << "*pi = " << *pi << "\n";         // 42

    generique = &flottant;
    double* pd = static_cast<double*>(generique);
    std::cout << "*pd = " << *pd << "\n";         // 3.14

    // --- nullptr (lignes 209-222) ---
    std::cout << "\n--- nullptr ---\n";
    void traiter_int(int);
    void traiter_ptr(int*);
    // Démonstration conceptuelle — nullptr est de type std::nullptr_t
    int* p1 = nullptr;
    if (p1 == nullptr) {
        std::cout << "p1 est nullptr\n";
    }

    // --- const et pointeurs (lignes 248-270) ---
    std::cout << "\n--- const et pointeurs ---\n";
    int val = 10;
    int autre = 20;

    // 1. Pointeur modifiable vers donnée modifiable
    int* c1 = &val;
    *c1 = 30;
    c1 = &autre;

    // 2. Pointeur modifiable vers donnée constante
    const int* c2 = &val;
    // *c2 = 30;         // ❌ ne peut pas modifier la donnée
    c2 = &autre;         // ✅ peut réaffecter le pointeur

    // 3. Pointeur constant vers donnée modifiable
    int* const c3 = &val;
    *c3 = 30;            // ✅ peut modifier la donnée
    // c3 = &autre;      // ❌ ne peut pas réaffecter le pointeur

    // 4. Pointeur constant vers donnée constante
    const int* const c4 = &val;
    // *c4 = 30;         // ❌
    // c4 = &autre;      // ❌

    std::cout << "*c1=" << *c1 << " *c2=" << *c2
              << " *c3=" << *c3 << " *c4=" << *c4 << "\n";

    return 0;
}
