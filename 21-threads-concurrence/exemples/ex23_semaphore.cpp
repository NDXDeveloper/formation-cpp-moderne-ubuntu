/* ============================================================================
   Section 21.3 : Primitives C++20 - std::counting_semaphore
   Description : Limitation de concurrence avec sémaphore (3 slots max)
   Fichier source : 03-condition-variable.md
   ============================================================================ */

#include <semaphore>
#include <thread>
#include <print>
#include <vector>
#include <chrono>

// Limiter à 3 connexions simultanées
std::counting_semaphore<3> connection_pool(3);

void worker(int id) {
    connection_pool.acquire();  // Bloque si les 3 slots sont pris
    std::println("Worker {} : connecté", id);

    // Simuler du travail
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(50ms);

    std::println("Worker {} : déconnecté", id);
    connection_pool.release();  // Libère un slot
}

int main() {
    std::vector<std::thread> threads;

    for (int i = 0; i < 8; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::println("Tous les workers terminés");
}
