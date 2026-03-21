/* ============================================================================
   Section 42.4 : Lock-free Programming
   Description : Compteur lock-free via CAS-loop (compare_exchange_weak)
   Fichier source : 04-lock-free.md
   ============================================================================ */

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

std::atomic<int> value{0};

void lock_free_increment() {
    int expected = value.load(std::memory_order_relaxed);
    while (!value.compare_exchange_weak(
                expected,
                expected + 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed)) {
        // expected mis a jour automatiquement, on reboucle
    }
}

int main() {
    constexpr int N = 100'000;
    constexpr int NUM_THREADS = 8;

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t)
        threads.emplace_back([&]() {
            for (int i = 0; i < N; ++i)
                lock_free_increment();
        });
    for (auto& t : threads) t.join();

    int result = value.load();
    std::cout << "CAS counter: " << result
              << " (expected: " << N * NUM_THREADS << ")\n";
    std::cout << (result == N * NUM_THREADS ? "PASS" : "FAIL") << "\n";
}
