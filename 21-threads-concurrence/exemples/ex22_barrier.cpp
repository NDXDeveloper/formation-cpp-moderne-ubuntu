/* ============================================================================
   Section 21.3 : Primitives C++20 - std::barrier
   Description : Synchronisation cyclique entre phases avec barrier réutilisable
   Fichier source : 03-condition-variable.md
   ============================================================================ */

#include <barrier>
#include <thread>
#include <print>
#include <vector>
#include <chrono>

int main() {
    const int num_threads = 4;

    // Callback exécuté quand tous les threads ont atteint la barrière
    auto on_completion = [] noexcept {
        std::println("--- Phase terminée ---");
    };

    std::barrier sync_point(num_threads, on_completion);

    std::vector<std::thread> threads;
    for (int id = 0; id < num_threads; ++id) {
        threads.emplace_back([&sync_point, id] {
            for (int phase = 0; phase < 3; ++phase) {
                // Simuler un calcul de phase
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(10ms * (id + 1));
                std::println("Thread {} : phase {} terminée", id, phase);
                sync_point.arrive_and_wait();  // Synchronisation inter-phase
            }
        });
    }

    for (auto& t : threads) t.join();
}
