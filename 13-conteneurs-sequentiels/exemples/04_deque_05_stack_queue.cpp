/* ============================================================================
   Section 13.4 : std::stack et std::queue
   Description : Utilisation de std::deque comme conteneur sous-jacent par
                 défaut pour les adaptateurs stack et queue
   Fichier source : 04-deque.md
   ============================================================================ */
#include <stack>
#include <queue>
#include <print>

int main() {
    // std::stack utilise std::deque par défaut
    std::stack<int> pile;
    pile.push(10);
    pile.push(20);
    pile.push(30);
    std::println("top = {}", pile.top());  // 30

    // std::queue utilise std::deque par défaut
    std::queue<int> file;
    file.push(10);
    file.push(20);
    file.push(30);
    std::println("front = {}", file.front());  // 10
}
