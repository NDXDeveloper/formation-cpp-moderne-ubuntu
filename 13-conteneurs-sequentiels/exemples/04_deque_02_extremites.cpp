/* ============================================================================
   Section 13.4 : Opérations aux deux extrémités
   Description : push_back/front, emplace_back/front, pop_back/front —
                 toutes en O(1) amorti
   Fichier source : 04-deque.md
   ============================================================================ */
#include <deque>
#include <print>

int main() {
    std::deque<int> dq;

    dq.push_back(30);
    dq.push_back(40);
    dq.emplace_back(50);
    // {30, 40, 50}

    dq.push_front(20);
    dq.push_front(10);
    dq.emplace_front(5);
    // {5, 10, 20, 30, 40, 50}

    dq.pop_front();    // {10, 20, 30, 40, 50}
    dq.pop_back();     // {10, 20, 30, 40}

    std::println("front={}, back={}", dq.front(), dq.back());
    // Sortie : front=10, back=40

    for (auto val : dq) std::print("{} ", val);
    std::println("");
    // Sortie : 10 20 30 40
}
