/* ============================================================================
   Section 42.3 : Memory Ordering et Barrieres Memoire
   Description : Spin-lock minimaliste avec acquire/release
   Fichier source : 03-memory-ordering.md
   ============================================================================ */

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

class SpinLock {
    std::atomic<bool> locked_{false};

public:
    void lock() {
        while (locked_.exchange(true, std::memory_order_acquire)) {
            // Spin — en attente active
        }
    }

    void unlock() {
        locked_.store(false, std::memory_order_release);
    }
};

int main() {
    SpinLock sl;
    int counter = 0;
    constexpr int N = 100'000;
    constexpr int NUM_THREADS = 4;

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < N; ++i) {
                sl.lock();
                ++counter;
                sl.unlock();
            }
        });
    }
    for (auto& t : threads) t.join();

    std::cout << "Counter: " << counter
              << " (expected: " << N * NUM_THREADS << ")\n";
    std::cout << (counter == N * NUM_THREADS ? "PASS" : "FAIL") << "\n";
}
