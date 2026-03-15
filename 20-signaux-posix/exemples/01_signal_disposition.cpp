/* ============================================================================
   Section 20.1 : Comprendre les signaux Unix
   Description : Consulter la disposition actuelle d'un signal avec sigaction
   Fichier source : 01-signaux-unix.md
   ============================================================================ */
#include <csignal>
#include <print>

void show_signal_disposition() {
    // sigaction() avec un handler NULL retourne la disposition actuelle
    struct sigaction sa;
    sigaction(SIGTERM, nullptr, &sa);

    if (sa.sa_handler == SIG_DFL) {
        std::println("SIGTERM : action par défaut");
    } else if (sa.sa_handler == SIG_IGN) {
        std::println("SIGTERM : ignoré");
    } else {
        std::println("SIGTERM : handler personnalisé installé");
    }
}

int main() {
    // Disposition par défaut
    show_signal_disposition();

    // Ignorer SIGTERM puis vérifier
    std::signal(SIGTERM, SIG_IGN);
    show_signal_disposition();

    // Restaurer l'action par défaut
    std::signal(SIGTERM, SIG_DFL);
    show_signal_disposition();
}
