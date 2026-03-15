/* ============================================================================
   Section 21.3 : Variables de condition - Pattern producteur/consommateur
   Description : BlockingQueue<T> avec close(), producteur et 3 consommateurs
   Fichier source : 03-condition-variable.md
   ============================================================================ */

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <print>
#include <vector>

template <typename T>
class BlockingQueue {
    std::mutex mtx_;
    std::condition_variable cv_not_empty_;
    std::queue<T> queue_;
    bool closed_ = false;

public:
    void push(const T& item) {
        {
            std::lock_guard lock(mtx_);
            if (closed_) {
                throw std::runtime_error("push sur une queue fermée");
            }
            queue_.push(item);
        }
        cv_not_empty_.notify_one();
    }

    // Retourne false si la queue est fermée et vide (plus rien à consommer)
    bool pop(T& item) {
        std::unique_lock lock(mtx_);
        cv_not_empty_.wait(lock, [this] {
            return !queue_.empty() || closed_;
        });

        if (queue_.empty()) {
            return false;  // Queue fermée et vide → arrêt du consommateur
        }

        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void close() {
        {
            std::lock_guard lock(mtx_);
            closed_ = true;
        }
        cv_not_empty_.notify_all();  // Réveiller TOUS les consommateurs
    }
};

int main() {
    BlockingQueue<int> queue;

    // Producteur
    std::thread producer([&queue] {
        for (int i = 0; i < 20; ++i) {
            queue.push(i);
            std::println("[P] Produit : {}", i);
        }
        queue.close();
    });

    // Consommateurs
    std::vector<std::thread> consumers;
    for (int id = 0; id < 3; ++id) {
        consumers.emplace_back([&queue, id] {
            int item;
            while (queue.pop(item)) {
                std::println("[C{}] Traité : {}", id, item);
            }
            std::println("[C{}] Terminé", id);
        });
    }

    producer.join();
    for (auto& c : consumers) {
        c.join();
    }
}
