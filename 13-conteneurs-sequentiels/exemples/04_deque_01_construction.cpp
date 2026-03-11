/* ============================================================================
   Section 13.4 : Déclaration et construction
   Description : Construction de std::deque (vide, taille, liste
                 d'initialisation, itérateurs, copie, move, CTAD)
   Fichier source : 04-deque.md
   ============================================================================ */
#include <deque>
#include <print>

int main() {
    std::deque<int> d1;
    std::deque<int> d2(5, 42);
    std::deque<int> d3{10, 20, 30, 40};
    std::deque<int> d4(d3.begin(), d3.end());
    std::deque<int> d5 = d3;
    std::deque<int> d6 = std::move(d5);
    std::deque d7{1.0, 2.0, 3.0};

    std::println("d3 size={}, d6 size={}", d3.size(), d6.size());
    // Sortie : d3 size=4, d6 size=4
}
