/* ============================================================================
   Section 21.2.1 : std::mutex - Résoudre le problème du compteur
   Description : Compteur protégé par un mutex vs data race
   Fichier source : 02.1-mutex.md
   ============================================================================ */

#include <thread>
#include <mutex>
#include <print>

std::mutex mtx;
int counter = 0;

void increment() {
    for (int i = 0; i < 1'000'000; ++i) {
        mtx.lock();
        ++counter;       // Section critique : un seul thread à la fois
        mtx.unlock();
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    t1.join();
    t2.join();
    std::println("counter = {}", counter);  // Toujours 2'000'000
}
