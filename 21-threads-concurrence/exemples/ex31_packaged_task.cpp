/* ============================================================================
   Section 21.5 : std::packaged_task - Encapsuler un callable
   Description : packaged_task exécuté dans un thread avec future associé
   Fichier source : 05-async-future.md
   ============================================================================ */

#include <future>
#include <thread>
#include <print>

int main() {
    // Encapsuler un lambda dans un packaged_task
    std::packaged_task<int(int, int)> task([](int a, int b) {
        return a + b;
    });

    // Obtenir le futur AVANT d'exécuter la tâche
    std::future<int> fut = task.get_future();

    // Exécuter la tâche dans un autre thread
    std::thread t(std::move(task), 10, 20);

    std::println("Résultat : {}", fut.get());  // 30
    t.join();
}
