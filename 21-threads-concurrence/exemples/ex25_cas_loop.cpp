/* ============================================================================
   Section 21.4 : std::atomic - Compare-And-Swap (boucle CAS)
   Description : Multiplication atomique via compare_exchange_weak en boucle
   Fichier source : 04-atomiques.md
   ============================================================================ */

#include <atomic>
#include <thread>
#include <print>
#include <vector>

std::atomic<int> value{1};

void atomic_multiply_by_2() {
    int expected = value.load();
    while (!value.compare_exchange_weak(expected, expected * 2)) {
        // expected a été mis à jour avec la valeur courante.
        // On recalcule la valeur désirée et on réessaie.
    }
}

int main() {
    // 10 threads multiplient chacun par 2 → résultat = 2^10 = 1024
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(atomic_multiply_by_2);
    }
    for (auto& t : threads) {
        t.join();
    }
    std::println("value = {} (attendu : 1024)", value.load());
}
