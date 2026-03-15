/* ============================================================================
   Section 21.2.1 : Variantes de mutex - std::timed_mutex
   Description : Tentative d'acquisition avec timeout (try_lock_for/try_lock_until)
   Fichier source : 02.1-mutex.md
   ============================================================================ */

#include <mutex>
#include <chrono>
#include <print>

std::timed_mutex tmtx;

void cautious_operation() {
    using namespace std::chrono_literals;

    // try_lock_for : attend au maximum la durée spécifiée
    if (tmtx.try_lock_for(100ms)) {
        std::println("Verrou acquis");
        // ... section critique ...
        tmtx.unlock();
    } else {
        std::println("Timeout : le verrou n'a pas été obtenu en 100ms");
    }

    // try_lock_until : attend jusqu'à un instant absolu
    auto deadline = std::chrono::steady_clock::now() + 500ms;
    if (tmtx.try_lock_until(deadline)) {
        std::println("Verrou acquis (try_lock_until)");
        tmtx.unlock();
    }
}

int main() {
    cautious_operation();
}
