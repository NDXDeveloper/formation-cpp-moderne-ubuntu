/* ============================================================================
   Section 21.5 : std::shared_future - Partager un résultat
   Description : Plusieurs threads consomment le même résultat via shared_future
   Fichier source : 05-async-future.md
   ============================================================================ */

#include <future>
#include <thread>
#include <vector>
#include <print>

int main() {
    // Créer un shared_future à partir d'un future
    std::shared_future<int> shared =
        std::async(std::launch::async, [] { return 42; }).share();

    std::vector<std::thread> consumers;
    for (int i = 0; i < 5; ++i) {
        // shared_future est copiable — chaque thread a sa copie
        consumers.emplace_back([shared, i] {
            int value = shared.get();  // Chaque thread peut appeler get()
            std::println("Consumer {} : {}", i, value);
        });
    }

    for (auto& t : consumers) t.join();
}
