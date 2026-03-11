/* ============================================================================
   Section 13.1.2 : insert et emplace
   Description : Insertion d'éléments à une position arbitraire dans un
                 std::vector (valeur unique, copies multiples, plage, init list)
   Fichier source : 01.2-methodes-essentielles.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30, 40};

    // Insertion d'un seul élément à la position 2 (avant 30)
    auto it = v.insert(v.begin() + 2, 25);
    // v = {10, 20, 25, 30, 40}
    // it pointe sur l'élément inséré (25)
    std::println("Élément inséré : {}", *it);

    // Insertion de 3 copies de la même valeur
    v.insert(v.begin(), 3, 0);
    // v = {0, 0, 0, 10, 20, 25, 30, 40}

    // Insertion d'une plage depuis un autre conteneur
    std::vector<int> extras{99, 98, 97};
    v.insert(v.end(), extras.begin(), extras.end());
    // v = {0, 0, 0, 10, 20, 25, 30, 40, 99, 98, 97}

    // Insertion par liste d'initialisation (C++11)
    v.insert(v.begin() + 1, {7, 8, 9});
    // v = {0, 7, 8, 9, 0, 0, 10, 20, 25, 30, 40, 99, 98, 97}

    for (auto val : v) std::print("{} ", val);
    // Sortie : 0 7 8 9 0 0 10 20 25 30 40 99 98 97
}
