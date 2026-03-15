/* ============================================================================
   Section 21.4 : std::atomic - Spin lock minimaliste (TTAS)
   Description : SpinLock avec Test-and-Test-and-Set et atomic_flag
   Fichier source : 04-atomiques.md
   ============================================================================ */

#include <atomic>
#include <thread>
#include <print>
#include <vector>

class SpinLock {
    std::atomic<bool> locked_{false};

public:
    void lock() {
        while (locked_.exchange(true, std::memory_order_acquire)) {
            // Boucle interne de lecture seule (évite le bus locking)
            while (locked_.load(std::memory_order_relaxed)) {
            }
        }
    }

    void unlock() {
        locked_.store(false, std::memory_order_release);
    }
};

SpinLock spinlock;
int shared_counter = 0;

void increment(int n) {
    for (int i = 0; i < n; ++i) {
        spinlock.lock();
        ++shared_counter;
        spinlock.unlock();
    }
}

int main() {
    const int per_thread = 500'000;
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(increment, per_thread);
    }
    for (auto& t : threads) {
        t.join();
    }

    std::println("shared_counter = {} (attendu : {})",
                 shared_counter, 4 * per_thread);
}
