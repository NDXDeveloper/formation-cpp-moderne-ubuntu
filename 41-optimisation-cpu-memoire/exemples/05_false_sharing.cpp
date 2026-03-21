/* ============================================================================
   Section 41.1.2 : Cache lines et false sharing
   Description : Benchmark false sharing — compteurs packed vs padded (alignas)
   Fichier source : 01.2-cache-lines.md
   ============================================================================ */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

constexpr int NUM_ITERATIONS = 100'000'000;

// CAS 1 : Compteurs contigus sur la meme cache line -> FALSE SHARING
struct CountersPacked {
    std::atomic<std::int64_t> counter_a{0};
    std::atomic<std::int64_t> counter_b{0};
};

// CAS 2 : Compteurs sur des cache lines separees -> PAS de false sharing
struct CountersPadded {
    alignas(64) std::atomic<std::int64_t> counter_a{0};
    alignas(64) std::atomic<std::int64_t> counter_b{0};
};

template <typename Counters>
void benchmark(const char* label) {
    Counters counters{};

    auto start = std::chrono::high_resolution_clock::now();

    std::thread t1([&]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i)
            counters.counter_a.fetch_add(1, std::memory_order_relaxed);
    });

    std::thread t2([&]() {
        for (int i = 0; i < NUM_ITERATIONS; ++i)
            counters.counter_b.fetch_add(1, std::memory_order_relaxed);
    });

    t1.join();
    t2.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << label << " : " << ms.count() << " ms\n";
}

int main() {
    benchmark<CountersPacked>("Packed (false sharing)  ");
    benchmark<CountersPadded>("Padded (pas de sharing) ");
}
