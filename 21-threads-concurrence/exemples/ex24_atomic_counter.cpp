/* ============================================================================
   Section 21.4 : std::atomic - Compteur atomique
   Description : Compteur incrémenté par 2 threads sans mutex
   Fichier source : 04-atomiques.md
   ============================================================================ */

#include <atomic>
#include <thread>
#include <print>

std::atomic<int> counter{0};

void increment() {
    for (int i = 0; i < 1'000'000; ++i) {
        ++counter;  // Opération atomique — pas de data race
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    t1.join();
    t2.join();
    std::println("counter = {}", counter.load());  // Toujours 2'000'000
}
