/* ============================================================================
   Section 6.5.5 : Opérateur d'affectation par déplacement
   Description : DynArray version finale — Règle des 5 complète
                 (programme de test)
   Fichier source : 05.5-move-assignment.md
   ============================================================================ */
#include "05_5_dynarray_final.h"
#include <iostream>
#include <utility>
#include <cassert>

int main() {
    // Construction par défaut
    DynArray empty;
    std::cout << "empty: " << empty << " size=" << empty.size()
              << " empty=" << empty.empty() << "\n";

    // Construction paramétrée
    DynArray a(5, 42);
    std::cout << "a: " << a << "\n";

    // Construction par copie
    DynArray b = a;
    b[0] = 99;
    std::cout << "Après copie et b[0]=99:\n";
    std::cout << "  a: " << a << "\n";
    std::cout << "  b: " << b << "\n";
    assert(a[0] == 42);

    // Construction par déplacement
    DynArray c = std::move(b);
    std::cout << "Après move b→c:\n";
    std::cout << "  b: " << b << " (vidé)\n";
    std::cout << "  c: " << c << "\n";
    assert(b.empty());

    // Affectation par copie
    DynArray d(3, 0);
    d = a;
    std::cout << "Après d=a: d=" << d << "\n";
    assert(d.size() == 5 && d[0] == 42);

    // Affectation par déplacement
    DynArray e(2, 7);
    e = std::move(d);
    std::cout << "Après e=move(d): e=" << e << " d=" << d << "\n";
    assert(e.size() == 5 && d.empty());

    // Auto-affectation par copie
    a = a;
    std::cout << "Après a=a: a=" << a << "\n";

    // Auto-affectation par déplacement
    e = std::move(e);
    std::cout << "Après e=move(e): e=" << e << "\n";

    // Chaînage
    DynArray f(1), g(1), h(3, 7);
    f = g = h;
    std::cout << "Après f=g=h: f=" << f << " g=" << g << " h=" << h << "\n";

    // Affectation depuis temporaire
    DynArray i;
    i = DynArray(4, 100);
    std::cout << "Après i=DynArray(4,100): i=" << i << "\n";

    // Swap
    DynArray x(2, 1), y(3, 9);
    swap(x, y);
    std::cout << "Après swap(x,y): x=" << x << " y=" << y << "\n";

    std::cout << "Tous les tests passés.\n";
    return 0;
}
// Sortie attendue :
// empty: [] size=0 empty=1
// a: [42, 42, 42, 42, 42]
// Après copie et b[0]=99:
//   a: [42, 42, 42, 42, 42]
//   b: [99, 42, 42, 42, 42]
// Après move b→c:
//   b: [] (vidé)
//   c: [99, 42, 42, 42, 42]
// Après d=a: d=[42, 42, 42, 42, 42]
// Après e=move(d): e=[42, 42, 42, 42, 42] d=[]
// Après a=a: a=[42, 42, 42, 42, 42]
// Après e=move(e): e=[42, 42, 42, 42, 42]
// Après f=g=h: f=[7, 7, 7] g=[7, 7, 7] h=[7, 7, 7]
// Après i=DynArray(4,100): i=[100, 100, 100, 100]
// Après swap(x,y): x=[9, 9, 9] y=[1, 1]
// Tous les tests passés.
