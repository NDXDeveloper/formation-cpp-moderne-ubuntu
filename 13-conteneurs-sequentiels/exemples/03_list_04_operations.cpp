/* ============================================================================
   Section 13.3 : Opérations spécifiques aux listes
   Description : splice (transfert de nœuds O(1)), sort, merge (fusion de
                 listes triées), unique, reverse
   Fichier source : 03-list-forward-list.md
   ============================================================================ */
#include <list>
#include <print>
#include <functional>

auto afficher = [](const std::string& nom, const std::list<int>& l) {
    std::print("{}: ", nom);
    for (auto val : l) std::print("{} ", val);
    std::println("");
};

int main() {
    // splice
    {
        std::list<int> a{1, 2, 3, 4, 5};
        std::list<int> b{100, 200, 300};

        auto pos = std::next(a.begin(), 2);
        a.splice(pos, b);
        afficher("a", a);  // a: 1 2 100 200 300 3 4 5
        afficher("b", b);  // b:

        std::list<int> c{999};
        a.splice(a.begin(), c, c.begin());
        afficher("a", a);  // a: 999 1 2 100 200 300 3 4 5

        std::list<int> d{7, 8, 9};
        a.splice(a.end(), d, d.begin(), d.end());
        afficher("a", a);  // a: 999 1 2 100 200 300 3 4 5 7 8 9
    }

    // sort
    {
        std::list<int> lst{50, 20, 40, 10, 30};
        lst.sort();
        for (auto val : lst) std::print("{} ", val);
        std::println("");  // 10 20 30 40 50

        lst.sort(std::greater<int>{});
        for (auto val : lst) std::print("{} ", val);
        std::println("");  // 50 40 30 20 10
    }

    // merge
    {
        std::list<int> a{1, 3, 5, 7};
        std::list<int> b{2, 4, 6, 8};
        a.merge(b);
        for (auto val : a) std::print("{} ", val);
        std::println("");
        // Sortie : 1 2 3 4 5 6 7 8
    }

    // unique
    {
        std::list<int> lst{1, 1, 2, 3, 3, 3, 4, 4, 5};
        lst.unique();
        for (auto val : lst) std::print("{} ", val);
        std::println("");
        // Sortie : 1 2 3 4 5
    }

    // reverse
    {
        std::list<int> lst{1, 2, 3, 4, 5};
        lst.reverse();
        for (auto val : lst) std::print("{} ", val);
        std::println("");
        // Sortie : 5 4 3 2 1
    }
}
