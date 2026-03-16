/* ============================================================================
   Section 29.4.3 : ThreadSanitizer (-fsanitize=thread)
   Description : Lock-order-inversion (deadlock potentiel) — deux threads
                 acquièrent deux mutex dans un ordre inversé
   Fichier source : 04.3-threadsanitizer.md
   Compilation : g++-15 -fsanitize=thread -g -O1 -fno-omit-frame-pointer -std=c++20 -o deadlock 14_deadlock.cpp
   ============================================================================ */

#include <mutex>
#include <thread>

std::mutex mtx_a;
std::mutex mtx_b;

void thread1() {
    std::lock_guard<std::mutex> lock_a(mtx_a);
    std::lock_guard<std::mutex> lock_b(mtx_b);    // Ordre : A → B
}

void thread2() {
    std::lock_guard<std::mutex> lock_b(mtx_b);
    std::lock_guard<std::mutex> lock_a(mtx_a);    // Ordre : B → A ← inversion !
}

int main() {
    std::thread t1(thread1);
    std::thread t2(thread2);
    t1.join();
    t2.join();
}
