/* ============================================================================
   Section 13.3 : Opérations de suppression
   Description : Suppression dans std::list (pop_front/back, erase, remove,
                 remove_if) et std::forward_list (pop_front, erase_after)
   Fichier source : 03-list-forward-list.md
   ============================================================================ */
#include <list>
#include <forward_list>
#include <print>

int main() {
    // std::list
    {
        std::list<int> lst{10, 20, 30, 40, 50, 60};

        lst.pop_front();   // {20, 30, 40, 50, 60}
        lst.pop_back();    // {20, 30, 40, 50}

        auto it = std::next(lst.begin(), 1);  // pointe sur 30
        auto suivant = lst.erase(it);
        std::println("Après erase : *suivant = {}", *suivant);  // 40

        lst.push_back(40);
        lst.remove(40);

        lst.push_back(15);
        lst.push_back(25);
        lst.remove_if([](int n) { return n > 20; });

        for (auto val : lst) std::print("{} ", val);
        std::println("");
        // Sortie : 20 15
    }

    // std::forward_list
    {
        std::forward_list<int> fl{10, 20, 30, 40, 50};

        fl.pop_front();

        auto it = fl.begin();
        fl.erase_after(it);

        fl.erase_after(fl.begin(), fl.end());

        fl = {5, 10, 15, 20, 25};
        fl.remove_if([](int n) { return n % 10 != 0; });

        for (auto val : fl) std::print("{} ", val);
        std::println("");
        // Sortie : 10 20
    }
}
