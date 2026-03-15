/* ============================================================================
   Section 20.2 : Installation de handlers (signal, sigaction)
   Description : Installation d'un handler simple avec sigaction()
   Fichier source : 02-handlers.md
   ============================================================================ */
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <print>

void term_handler(int /* sig */) {
    const char msg[] = "Signal SIGTERM reçu — arrêt propre\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    _exit(0);
}

int main() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));   // Initialiser à zéro — indispensable

    sa.sa_handler = term_handler;
    sigemptyset(&sa.sa_mask);          // Aucun signal supplémentaire masqué
    sa.sa_flags = 0;

    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        std::println("sigaction: {}", strerror(errno));
        return 1;
    }

    std::println("En attente de SIGTERM (PID = {})...", getpid());

    // Pour la démonstration : s'envoyer SIGTERM
    raise(SIGTERM);
}
