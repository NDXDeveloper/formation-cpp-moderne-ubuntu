/* ============================================================================
   Section 21.5 : std::promise - Le côté producteur
   Description : Transmission d'un résultat entre threads via promise/future
   Fichier source : 05-async-future.md
   ============================================================================ */

#include <future>
#include <thread>
#include <functional>
#include <print>

void submit_to_pool(std::function<int()> task, std::promise<int> prom) {
    std::thread([task = std::move(task), prom = std::move(prom)]() mutable {
        try {
            prom.set_value(task());
        } catch (...) {
            prom.set_exception(std::current_exception());
        }
    }).detach();
}

int main() {
    std::promise<int> prom;
    auto fut = prom.get_future();

    submit_to_pool([] { return 42; }, std::move(prom));

    std::println("Résultat : {}", fut.get());
}
