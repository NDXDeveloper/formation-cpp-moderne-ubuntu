/* ============================================================================
   Section 21.2.1 : Variantes de mutex - std::recursive_mutex
   Description : Mutex récursif permettant le verrouillage multiple par un thread
   Fichier source : 02.1-mutex.md
   ============================================================================ */

#include <mutex>
#include <print>

std::recursive_mutex rmtx;

void recursive_function(int depth) {
    rmtx.lock();
    std::println("Profondeur {}", depth);
    if (depth > 0) {
        recursive_function(depth - 1);  // Re-verrouille le même mutex — OK
    }
    rmtx.unlock();
}

int main() {
    recursive_function(3);
}
