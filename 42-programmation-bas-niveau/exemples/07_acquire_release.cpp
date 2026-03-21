/* ============================================================================
   Section 42.3 : Memory Ordering et Barrieres Memoire
   Description : Pattern acquire/release pour la publication de donnees
   Fichier source : 03-memory-ordering.md
   ============================================================================ */

#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

std::atomic<bool> ready{false};
int data = 0;

void producer() {
    data = 42;                                       // Ecriture non-atomique
    ready.store(true, std::memory_order_release);    // Release : publie data
}

void consumer() {
    while (!ready.load(std::memory_order_acquire))   // Acquire : attend ready
        ;
    assert(data == 42);                              // Toujours vrai
}

int main() {
    for (int i = 0; i < 1000; ++i) {
        ready.store(false);
        data = 0;

        std::thread t1(producer);
        std::thread t2(consumer);
        t1.join();
        t2.join();
    }
    std::cout << "1000 iterations: acquire/release pattern correct\n";
}
