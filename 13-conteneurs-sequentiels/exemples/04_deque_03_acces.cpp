/* ============================================================================
   Section 13.4 : Accès par index et insertion/suppression au milieu
   Description : operator[], at(), insert et erase sur un std::deque
   Fichier source : 04-deque.md
   ============================================================================ */
#include <deque>
#include <print>
#include <stdexcept>

int main() {
    // Accès par index
    {
        std::deque<int> dq{10, 20, 30, 40, 50};

        std::println("dq[2] = {}", dq[2]);       // 30
        std::println("dq.at(2) = {}", dq.at(2)); // 30

        try {
            [[maybe_unused]] auto val = dq.at(100);
        } catch (const std::out_of_range& e) {
            std::println("Exception : {}", e.what());
        }

        dq[0] = 99;
        std::println("dq[0] = {}", dq[0]);  // 99
    }

    // Insertion et suppression au milieu
    {
        std::deque<int> dq{10, 20, 30, 40, 50};
        auto it = dq.begin() + 2;
        dq.insert(it, 25);
        // {10, 20, 25, 30, 40, 50}

        dq.erase(dq.begin() + 3);
        // {10, 20, 25, 40, 50}

        for (auto val : dq) std::print("{} ", val);
        std::println("");
        // Sortie : 10 20 25 40 50
    }
}
