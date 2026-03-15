/* ============================================================================
   Section 21.6 : Thread-safety - Pattern Moniteur
   Description : Template Monitor<T> avec apply() et wait_and_apply()
   Fichier source : 06-thread-safety.md
   ============================================================================ */

#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <print>

template <typename T>
class Monitor {
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    T data_;

public:
    template <typename F>
    auto apply(F&& func) -> decltype(func(data_)) {
        std::unique_lock lock(mtx_);
        return func(data_);
    }

    template <typename Pred, typename F>
    auto wait_and_apply(Pred pred, F&& func) -> decltype(func(data_)) {
        std::unique_lock lock(mtx_);
        cv_.wait(lock, [&] { return pred(data_); });
        auto result = func(data_);
        lock.unlock();
        cv_.notify_all();
        return result;
    }
};

int main() {
    Monitor<std::queue<int>> monitored_queue;

    // Producteur
    std::thread producer([&] {
        for (int i = 0; i < 10; ++i) {
            monitored_queue.apply([i](std::queue<int>& q) {
                q.push(i);
                std::println("[P] Produit : {}", i);
            });
        }
        // Sentinelle de fin
        monitored_queue.apply([](std::queue<int>& q) {
            q.push(-1);
        });
    });

    // Consommateur
    std::thread consumer([&] {
        while (true) {
            int val = monitored_queue.wait_and_apply(
                [](const std::queue<int>& q) { return !q.empty(); },
                [](std::queue<int>& q) {
                    int v = q.front();
                    q.pop();
                    return v;
                }
            );
            if (val == -1) break;
            std::println("[C] Traité : {}", val);
        }
    });

    producer.join();
    consumer.join();
    std::println("Terminé");
}
