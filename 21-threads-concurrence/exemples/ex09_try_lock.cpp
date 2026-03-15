/* ============================================================================
   Section 21.2.1 : std::mutex - try_lock()
   Description : Tentative non-bloquante d'acquisition d'un mutex
   Fichier source : 02.1-mutex.md
   ============================================================================ */

#include <mutex>
#include <print>

std::mutex mtx;
int shared_data = 0;

void opportunistic_update(int value) {
    if (mtx.try_lock()) {
        shared_data = value;
        mtx.unlock();
        std::println("Mise à jour effectuée");
    } else {
        std::println("Mutex occupé, on passe à autre chose");
    }
}

int main() {
    opportunistic_update(42);
    std::println("shared_data = {}", shared_data);
}
