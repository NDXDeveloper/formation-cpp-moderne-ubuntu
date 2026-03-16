/* ============================================================================
   Section 29.4.3 : ThreadSanitizer (-fsanitize=thread)
   Description : Data race — deux threads incrémentent un compteur global
                 sans synchronisation
   Fichier source : 04.3-threadsanitizer.md
   Compilation : g++-15 -fsanitize=thread -g -O1 -fno-omit-frame-pointer -std=c++20 -o data_race 08_data_race.cpp
   ============================================================================ */

#include <thread>
#include <cstdio>

int counter = 0;

void increment(int n) {
    for (int i = 0; i < n; ++i) {
        ++counter;    // Data race : lecture-modification-écriture non atomique
    }
}

int main() {
    std::thread t1(increment, 100000);
    std::thread t2(increment, 100000);

    t1.join();
    t2.join();

    std::printf("Counter: %d\n", counter);
    return 0;
}
