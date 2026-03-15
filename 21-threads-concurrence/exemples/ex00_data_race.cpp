/* ============================================================================
   Section 21 : Introduction - Les dangers de la programmation concurrente
   Description : Illustration d'une data race sur un compteur partagé
   Fichier source : README.md
   ============================================================================ */

#include <thread>
#include <print>

// ⚠️ DATA RACE — comportement indéfini
int counter = 0;

void increment() {
    for (int i = 0; i < 1'000'000; ++i) {
        ++counter;  // lecture-modification-écriture non atomique
    }
}
// Si deux threads exécutent increment(), le résultat final
// n'est PAS garanti d'être 2'000'000.

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    t1.join();
    t2.join();
    std::println("counter = {} (attendu 2'000'000, probablement différent)", counter);
}
