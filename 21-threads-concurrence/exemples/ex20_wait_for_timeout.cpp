/* ============================================================================
   Section 21.3 : Variables de condition - wait_for() avec timeout
   Description : Attente bornée avec wait_for() et prédicat
   Fichier source : 03-condition-variable.md
   ============================================================================ */

#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>
#include <print>

std::mutex mtx;
std::condition_variable cv;
bool event_occurred = false;

void timed_waiter() {
    using namespace std::chrono_literals;

    std::unique_lock lock(mtx);

    // Forme avec prédicat et timeout (recommandée)
    bool success = cv.wait_for(lock, 500ms, [] {
        return event_occurred;
    });

    if (success) {
        std::println("Événement reçu à temps");
    } else {
        std::println("Timeout après 500ms");
    }
}

int main() {
    // Test 1 : timeout (personne ne signale)
    std::println("--- Test 1 : timeout ---");
    std::thread t1(timed_waiter);
    t1.join();

    // Reset
    event_occurred = false;

    // Test 2 : signal reçu avant le timeout
    std::println("--- Test 2 : signal reçu ---");
    std::thread t2(timed_waiter);

    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
        {
            std::lock_guard lock(mtx);
            event_occurred = true;
        }
        cv.notify_one();
    }

    t2.join();
}
