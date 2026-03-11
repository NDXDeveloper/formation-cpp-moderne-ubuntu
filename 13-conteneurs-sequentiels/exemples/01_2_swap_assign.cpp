/* ============================================================================
   Section 13.1.2 : swap et assign
   Description : Échange O(1) du contenu de deux vecteurs avec swap, et
                 remplacement complet du contenu avec assign
   Fichier source : 01.2-methodes-essentielles.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    // swap
    {
        std::vector<int> a{1, 2, 3};
        std::vector<int> b{100, 200};

        a.swap(b);

        std::print("a : ");
        for (auto v : a) std::print("{} ", v);
        std::println("");  // a : 100 200

        std::print("b : ");
        for (auto v : b) std::print("{} ", v);
        std::println("");  // b : 1 2 3
    }

    // assign
    {
        std::vector<int> v;

        v.assign(5, 42);
        // v = {42, 42, 42, 42, 42}

        std::vector<int> source{10, 20, 30};
        v.assign(source.begin(), source.end());
        // v = {10, 20, 30}

        v.assign({7, 8, 9, 10});
        // v = {7, 8, 9, 10}

        for (auto val : v) std::print("{} ", val);
        std::println("");
        // Sortie : 7 8 9 10
    }
}
