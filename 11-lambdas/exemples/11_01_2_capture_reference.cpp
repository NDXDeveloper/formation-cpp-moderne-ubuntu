/* ============================================================================
   Section 11.1.2 : Capture par référence [&]
   Description : Exemples complets de capture par référence — mutation
                 bidirectionnelle, algorithmes STL, boucles, const, as_const,
                 init captures, performance, concurrence (atomic, async)
   Fichier source : 01.2-capture-reference.md
   ============================================================================ */
#include <print>
#include <string>
#include <functional>
#include <vector>
#include <algorithm>
#include <map>
#include <utility>
#include <thread>
#include <atomic>
#include <future>

int main() {
    // --- Principe (lignes 10-14) ---
    {
        int x = 10;
        auto modifier = [&x]() { x += 5; };
        modifier();
        std::print("{}\n", x);  // 15
    }

    // --- Capture explicite (lignes 41-51) ---
    {
        int total = 0;
        int count = 0;
        auto accumulate = [&total, &count](int value) {
            total += value;
            ++count;
        };
        accumulate(10);
        accumulate(20);
        std::print("Total: {}, Count: {}\n", total, count);  // Total: 30, Count: 2
    }

    // --- Capture par défaut [&] (lignes 61-73) ---
    {
        int total = 0;
        int count = 0;
        double average = 0.0;
        auto accumulate = [&](int value) {
            total += value;
            ++count;
            average = static_cast<double>(total) / count;
        };
        accumulate(10);
        accumulate(20);
        std::print("Average: {}\n", average);  // Average: 15
    }

    // --- Mutation bidirectionnelle (lignes 85-96) ---
    {
        int shared_state = 0;
        auto increment = [&shared_state]() { ++shared_state; };
        auto read      = [&shared_state]() { return shared_state; };

        increment();
        increment();
        std::print("{}\n", read());           // 2
        std::print("{}\n", shared_state);     // 2

        shared_state = 100;
        std::print("{}\n", read());           // 100
    }

    // --- Algorithmes STL — analyze (lignes 107-118) ---
    {
        auto analyze = [](const std::vector<int>& data, int threshold) {
            int above = 0;
            int below = 0;
            std::for_each(data.begin(), data.end(), [&above, &below, threshold](int v) {
                if (v > threshold) ++above;
                else ++below;
            });
            std::print("Above: {}, Below: {}\n", above, below);
        };
        analyze({1, 5, 10, 15, 20, 25}, 12);  // Above: 3, Below: 3
    }

    // --- Prefix sums (lignes 126-136) ---
    {
        std::vector<int> values = {3, 1, 4, 1, 5, 9};
        int running_sum = 0;
        std::vector<int> prefix_sums;
        prefix_sums.reserve(values.size());

        std::for_each(values.begin(), values.end(), [&](int v) {
            running_sum += v;
            prefix_sums.push_back(running_sum);
        });
        // prefix_sums = {3, 4, 8, 9, 14, 23}
        for (int v : prefix_sums) std::print("{} ", v);
        std::print("\n");
    }

    // --- make_adder corrigé (lignes 162-164) ---
    {
        auto make_adder = [](int base) {
            return [base](int x) { return base + x; };
        };
        auto add_ten = make_adder(10);
        std::print("{}\n", add_ten(5));  // 15
    }

    // --- Boucle — capture par valeur (lignes 278-287) ---
    {
        std::vector<std::function<int()>> getters;
        for (int i = 0; i < 5; ++i) {
            getters.push_back([i]() { return i; });
        }
        for (auto& fn : getters) {
            std::print("{} ", fn());
        }
        std::print("\n");  // 0 1 2 3 4
    }

    // --- Capture de const (lignes 234-238) ---
    {
        const int max_retries = 3;
        auto check = [&max_retries](int attempt) {
            return attempt < max_retries;
        };
        std::print("check(2) = {}, check(5) = {}\n", check(2), check(5));
    }

    // --- Capture ref const via as_const (lignes 244-250) ---
    {
        std::vector<int> data = {1, 2, 3, 4, 5};
        auto reader = [&cdata = std::as_const(data)]() {
            return cdata.size();
        };
        std::print("size = {}\n", reader());  // 5
    }

    // --- Init capture par valeur (lignes 361-362) ---
    {
        auto ok = [x = 42]() { return x; };
        auto ok2 = [s = std::string("hello")]() { return s; };
        std::print("{}, {}\n", ok(), ok2());  // 42, hello
    }

    // --- Performance (lignes 374-400) ---
    {
        std::map<std::string, std::vector<double>> large_dataset;
        large_dataset["key1"] = {1.0, 2.0, 3.0};
        auto slow = [large_dataset]() { return large_dataset.size(); };
        auto fast = [&large_dataset]() { return large_dataset.size(); };
        std::print("slow: {}, fast: {}\n", slow(), fast());

        int limit = 1000;
        auto ref_version = [&limit]() {
            int sum = 0;
            for (int i = 0; i < limit; ++i) sum += i;
            return sum;
        };
        auto val_version = [limit]() {
            int sum = 0;
            for (int i = 0; i < limit; ++i) sum += i;
            return sum;
        };
        std::print("ref: {}, val: {}\n", ref_version(), val_version());
    }

    // --- Concurrence — atomic (lignes 317-324) ---
    {
        std::atomic<int> counter{0};
        auto task = [&counter]() {
            for (int i = 0; i < 100'000; ++i) {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        };
        std::thread t1(task);
        std::thread t2(task);
        t1.join();
        t2.join();
        std::print("counter = {}\n", counter.load());  // 200000
    }

    // --- Concurrence — async (lignes 329-339) ---
    {
        auto task = [](int iterations) {
            int local = 0;
            for (int i = 0; i < iterations; ++i) {
                ++local;
            }
            return local;
        };
        auto f1 = std::async(std::launch::async, task, 100'000);
        auto f2 = std::async(std::launch::async, task, 100'000);
        int total = f1.get() + f2.get();
        std::print("total = {}\n", total);  // 200000
    }

    return 0;
}
