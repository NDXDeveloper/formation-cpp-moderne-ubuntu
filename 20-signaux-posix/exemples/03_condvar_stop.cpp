/* ============================================================================
   Section 20.3 : Signaux et threads
   Description : condition_variable_any avec stop_token — attente interruptible
   Fichier source : 03-signaux-threads.md
   ============================================================================ */
#include <mutex>
#include <condition_variable>
#include <queue>
#include <print>
#include <thread>
#include <chrono>
#include <vector>

std::mutex g_queue_mutex;
std::condition_variable_any g_queue_cv;
std::queue<int> g_work_queue;

void consumer(std::stop_token stop_tok, int id) {
    while (true) {
        std::unique_lock lock(g_queue_mutex);

        // Attente interruptible : se réveille si
        // (a) un élément est disponible, OU
        // (b) l'arrêt est demandé via le stop_token
        bool got_work = g_queue_cv.wait(lock, stop_tok, [] {
            return !g_work_queue.empty();
        });

        if (!got_work) {
            // Le stop_token a été déclenché
            std::println("[consumer {}] arrêt demandé", id);
            return;
        }

        int item = g_work_queue.front();
        g_work_queue.pop();
        lock.unlock();

        // Traiter l'item...
        std::println("[consumer {}] traitement item {}", id, item);
    }
}

int main() {
    // Créer des consumers avec jthread
    std::vector<std::jthread> consumers;
    for (int i = 0; i < 3; ++i) {
        consumers.emplace_back(consumer, i);
    }

    // Produire quelques items
    {
        std::lock_guard lock(g_queue_mutex);
        for (int i = 1; i <= 6; ++i) {
            g_work_queue.push(i);
        }
    }
    g_queue_cv.notify_all();

    // Laisser le temps de traiter
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Les destructeurs des jthreads appellent request_stop() puis join()
    // → les consumers en attente sont réveillés par le stop_token
    consumers.clear();

    std::println("Tous les consumers terminés");
}
