/* ============================================================================
   Section 13.3 : Stabilité des itérateurs
   Description : Démonstration que les itérateurs de std::list restent
                 valides après insertions et suppressions
   Fichier source : 03-list-forward-list.md
   ============================================================================ */
#include <list>
#include <print>

int main() {
    std::list<int> lst{10, 20, 30, 40, 50};

    auto it30 = std::next(lst.begin(), 2);  // pointe sur 30
    std::println("Avant : *it30 = {}", *it30);

    // Insertions massives
    lst.push_front(1);
    lst.push_back(99);
    lst.insert(it30, 25);  // insère 25 AVANT 30

    // it30 pointe toujours sur 30
    std::println("Après : *it30 = {}", *it30);  // 30

    // Suppression d'éléments voisins
    lst.pop_front();
    lst.pop_back();

    // it30 pointe toujours sur 30
    std::println("Encore : *it30 = {}", *it30);  // 30
}
