/* ============================================================================
   Section 29.4.3 : ThreadSanitizer (-fsanitize=thread)
   Description : Data race sur un std::vector — push_back concurrent
                 sans synchronisation
   Fichier source : 04.3-threadsanitizer.md
   Compilation : g++-15 -fsanitize=thread -g -O1 -fno-omit-frame-pointer -std=c++20 -o vector_race 13_vector_race.cpp
   ============================================================================ */

#include <vector>
#include <thread>

std::vector<int> results;

void producer(int id) {
    results.push_back(id * 100);    // Data race sur le vector
}

int main() {
    std::thread t1(producer, 1);
    std::thread t2(producer, 2);
    t1.join();
    t2.join();
}
