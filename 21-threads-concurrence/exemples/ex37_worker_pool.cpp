/* ============================================================================
   Section 21.7 : std::jthread - Pool de workers stoppable
   Description : WorkerPool avec jthread, stop_token et condition_variable_any
   Fichier source : 07-jthread.md
   ============================================================================ */

#include <thread>
#include <stop_token>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>
#include <print>
#include <chrono>

class WorkerPool {
    std::mutex mtx_;
    std::condition_variable_any cv_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::jthread> workers_;

public:
    explicit WorkerPool(int num_threads) {
        for (int i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this](std::stop_token stoken) {
                worker_loop(stoken);
            });
        }
        std::println("Pool démarré avec {} workers", num_threads);
    }

    ~WorkerPool() {
        cv_.notify_all();  // Réveiller les workers endormis
        // Les ~jthread font le reste
        std::println("Pool arrêté");
    }

    void submit(std::function<void()> task) {
        {
            std::lock_guard lock(mtx_);
            tasks_.push(std::move(task));
        }
        cv_.notify_one();
    }

private:
    void worker_loop(std::stop_token stoken) {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lock(mtx_);

                // Attente interruptible par stop_token
                bool stopped = !cv_.wait(lock, stoken, [this] {
                    return !tasks_.empty();
                });

                if (stopped) return;  // Arrêt demandé

                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }
};

int main() {
    {
        WorkerPool pool(4);

        for (int i = 0; i < 20; ++i) {
            pool.submit([i] {
                std::println("Tâche {} exécutée par thread {}",
                             i, std::this_thread::get_id());
            });
        }

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(500ms);
    }
    // ~WorkerPool → request_stop sur chaque worker → join → propre
}
