/* ============================================================================
   Section 13.3 : Opérations d'insertion
   Description : Insertion dans std::list (push_front/back, insert, emplace)
                 et std::forward_list (push_front, insert_after, before_begin)
   Fichier source : 03-list-forward-list.md
   ============================================================================ */
#include <list>
#include <forward_list>
#include <print>

int main() {
    // std::list
    {
        std::list<int> lst{10, 30, 40};

        lst.push_front(5);
        lst.push_back(50);

        auto it = std::next(lst.begin(), 2);  // pointe sur 30
        lst.insert(it, 20);
        lst.emplace(it, 25);

        lst.emplace_front(1);
        lst.emplace_back(100);

        for (auto val : lst) std::print("{} ", val);
        std::println("");
        // Sortie : 1 5 10 20 25 30 40 50 100
    }

    // std::forward_list
    {
        std::forward_list<int> fl{10, 30, 40};

        fl.push_front(5);

        auto it = fl.begin();
        std::advance(it, 1);
        fl.insert_after(it, 20);
        fl.emplace_after(it, 15);

        fl.insert_after(fl.before_begin(), 1);

        for (auto val : fl) std::print("{} ", val);
        std::println("");
        // Sortie : 1 5 10 15 20 30 40
    }
}
