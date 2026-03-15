/* ============================================================================
   Section 21.5 : std::async et std::future - Usage de base
   Description : Lancer un calcul asynchrone et récupérer le résultat
   Fichier source : 05-async-future.md
   ============================================================================ */

#include <future>
#include <print>
#include <chrono>

int heavy_computation(int x) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);  // Simuler un calcul coûteux
    return x * x;
}

int main() {
    // Lancer le calcul de façon asynchrone
    std::future<int> result = std::async(std::launch::async, heavy_computation, 42);

    // Faire autre chose pendant que le calcul tourne...
    std::println("Calcul en cours...");

    // Récupérer le résultat (bloque si pas encore prêt)
    int value = result.get();
    std::println("Résultat : {}", value);  // 1764
}
