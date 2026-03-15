/* ============================================================================
   Section 21.2.2 : std::lock_guard - Protection d'une structure partagée
   Description : File d'attente thread-safe avec lock_guard et mutable mutex
   Fichier source : 02.2-lock-guard.md
   ============================================================================ */

#include <mutex>
#include <vector>
#include <thread>
#include <print>

class ThreadSafeQueue {
    mutable std::mutex mtx_;
    std::vector<int> queue_;

public:
    void push(int value) {
        std::lock_guard lock(mtx_);
        queue_.push_back(value);
    }

    std::size_t size() const {
        std::lock_guard lock(mtx_);
        return queue_.size();
    }

    void dump() const {
        std::lock_guard lock(mtx_);
        for (int v : queue_) {
            std::print("{} ", v);
        }
        std::println();
    }
};

int main() {
    ThreadSafeQueue q;

    std::thread producer([&q] {
        for (int i = 0; i < 100; ++i) {
            q.push(i);
        }
    });

    std::thread consumer([&q] {
        for (int i = 100; i < 200; ++i) {
            q.push(i);
        }
    });

    producer.join();
    consumer.join();
    std::println("Taille : {}", q.size());  // Toujours 200
}
