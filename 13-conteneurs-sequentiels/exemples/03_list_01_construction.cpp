/* ============================================================================
   Section 13.3 : Déclaration et construction
   Description : Construction de std::list et std::forward_list (vide, taille,
                 liste d'initialisation, copie, itérateurs, CTAD)
   Fichier source : 03-list-forward-list.md
   ============================================================================ */
#include <list>
#include <forward_list>
#include <print>

int main() {
    // std::list
    {
        std::list<int> l1;
        std::list<int> l2(5, 42);
        std::list<int> l3{10, 20, 30, 40};
        std::list<int> l4 = l3;
        std::list<int> l5(l3.begin(), l3.end());
        std::list l6{1.0, 2.0, 3.0};

        std::println("l3 size : {}", l3.size());  // 4
    }

    // std::forward_list
    {
        std::forward_list<int> fl1;
        std::forward_list<int> fl2(5, 42);
        std::forward_list<int> fl3{10, 20, 30, 40};
        std::forward_list fl4{1.0, 2.0, 3.0};

        auto n = std::distance(fl3.begin(), fl3.end());
        std::println("fl3 éléments : {}", n);  // 4
    }
}
