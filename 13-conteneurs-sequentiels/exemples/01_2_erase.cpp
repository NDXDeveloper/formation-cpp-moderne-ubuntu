/* ============================================================================
   Section 13.1.2 : Suppression avec erase
   Description : Suppression d'éléments par itérateur (un seul ou une plage),
                 et pop_back pour le dernier élément
   Fichier source : 01.2-methodes-essentielles.md
   ============================================================================ */
#include <vector>
#include <print>

int main() {
    // pop_back
    {
        std::vector<int> v{10, 20, 30};
        v.pop_back();
        std::println("size={}, back={}", v.size(), v.back());
        // Sortie : size=2, back=20
    }

    // erase
    {
        std::vector<int> v{10, 20, 30, 40, 50, 60};

        // Suppression d'un seul élément (l'élément à l'index 2 → valeur 30)
        auto it = v.erase(v.begin() + 2);
        // v = {10, 20, 40, 50, 60}
        // it pointe sur l'élément qui suivait celui supprimé (40)
        std::println("Après erase, *it = {}", *it);

        // Suppression d'une plage [begin+1, begin+3)
        v.erase(v.begin() + 1, v.begin() + 3);
        // v = {10, 50, 60}

        for (auto val : v) std::print("{} ", val);
        // Sortie : 10 50 60
    }
}
