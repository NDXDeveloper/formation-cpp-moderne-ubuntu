/* ============================================================================
   Section 21.3 : Primitives C++20 - std::latch
   Description : Synchronisation ponctuelle avec latch (compteur à usage unique)
   Fichier source : 03-condition-variable.md
   ============================================================================ */

#include <latch>
#include <thread>
#include <print>
#include <vector>
#include <chrono>

int main() {
    const int num_workers = 4;
    std::latch startup_latch(num_workers);

    std::vector<std::thread> workers;
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back([&startup_latch, i] {
            // Simuler une initialisation
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(50ms * (i + 1));
            std::println("Worker {} initialisé", i);
            startup_latch.count_down();  // Signale que ce worker est prêt
        });
    }

    startup_latch.wait();  // Attend que TOUS les workers soient initialisés
    std::println("Tous les workers sont prêts — démarrage");

    for (auto& w : workers) w.join();
}
