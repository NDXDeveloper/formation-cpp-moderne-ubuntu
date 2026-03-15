/* ============================================================================
   Section 20.2 : Installation de handlers (signal, sigaction)
   Description : Masquage temporaire de signaux avec sigprocmask
   Fichier source : 02-handlers.md
   ============================================================================ */
#include <signal.h>
#include <print>
#include <cstring>

void critical_update() {
    sigset_t block_set, old_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGTERM);
    sigaddset(&block_set, SIGINT);

    // Bloquer SIGTERM et SIGINT
    sigprocmask(SIG_BLOCK, &block_set, &old_set);

    // === Section critique ===
    // Les signaux sont en pending, pas perdus
    std::println("Mise à jour atomique en cours...");
    // ... opérations qui ne doivent pas être interrompues ...
    std::println("Mise à jour terminée");

    // Restaurer le masque — les signaux pending sont délivrés maintenant
    sigprocmask(SIG_SETMASK, &old_set, nullptr);
}

int main() {
    critical_update();
    std::println("sigprocmask OK");
}
