/* ============================================================================
   Section 21.5 : std::packaged_task - Queue de tâches
   Description : TaskQueue avec submit() retournant un future, worker_loop
   Fichier source : 05-async-future.md
   ============================================================================ */

#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <print>

class TaskQueue {
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::packaged_task<void()>> tasks_;
    bool stopped_ = false;

public:
    template <typename F>
    std::future<std::invoke_result_t<F>> submit(F&& func) {
        using ReturnType = std::invoke_result_t<F>;

        std::packaged_task<ReturnType()> task(std::forward<F>(func));
        auto fut = task.get_future();

        {
            std::lock_guard lock(mtx_);
            // Encapsuler dans un packaged_task<void()> via un lambda
            tasks_.emplace([t = std::move(task)]() mutable { t(); });
        }
        cv_.notify_one();

        return fut;
    }

    void worker_loop() {
        while (true) {
            std::packaged_task<void()> task;
            {
                std::unique_lock lock(mtx_);
                cv_.wait(lock, [this] {
                    return !tasks_.empty() || stopped_;
                });
                if (stopped_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();  // Exécution — le résultat est envoyé au futur
        }
    }

    void stop() {
        {
            std::lock_guard lock(mtx_);
            stopped_ = true;
        }
        cv_.notify_all();
    }
};

int main() {
    TaskQueue queue;
    std::thread worker([&queue] { queue.worker_loop(); });

    auto f1 = queue.submit([] { return 42; });
    auto f2 = queue.submit([] { return 100; });

    std::println("f1 = {}", f1.get());   // 42
    std::println("f2 = {}", f2.get());   // 100

    queue.stop();
    worker.join();
}
