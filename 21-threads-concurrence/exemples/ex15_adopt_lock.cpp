/* ============================================================================
   Section 21.2.2 : std::lock_guard - Constructeur adopt_lock
   Description : Verrouillage multi-mutex avec std::lock + adopt_lock
   Fichier source : 02.2-lock-guard.md
   ============================================================================ */

#include <mutex>
#include <print>

std::mutex mtx1;
std::mutex mtx2;

void transfer() {
    // Verrouillage simultané de deux mutex (évite les deadlocks)
    std::lock(mtx1, mtx2);

    // Adopter les verrous déjà acquis pour gérer le unlock via RAII
    std::lock_guard lock1(mtx1, std::adopt_lock);
    std::lock_guard lock2(mtx2, std::adopt_lock);

    // ... opération nécessitant les deux mutex ...
    std::println("Les deux mutex sont verrouillés en toute sécurité");
}
// Destruction de lock2 → mtx2.unlock()
// Destruction de lock1 → mtx1.unlock()

int main() {
    transfer();
    std::println("Transfer terminé sans deadlock");
}
