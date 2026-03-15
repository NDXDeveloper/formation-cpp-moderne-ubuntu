/* ============================================================================
   Section 21.1 : std::thread est move-only
   Description : Factory de threads, déplacement et vecteur de threads
   Fichier source : 01-std-thread.md
   ============================================================================ */

#include <thread>
#include <vector>
#include <print>

std::thread create_thread() {
    return std::thread([] {
        std::println("Thread créé par une fonction factory");
    });
}

int main() {
    // Factory et déplacement
    std::thread t1 = create_thread();
    std::thread t2;
    t2 = std::move(t1);
    t2.join();

    // Vecteur de threads
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([i] {
            std::println("Worker {} actif", i);
        });
    }
    for (auto& t : workers) {
        t.join();
    }
}
