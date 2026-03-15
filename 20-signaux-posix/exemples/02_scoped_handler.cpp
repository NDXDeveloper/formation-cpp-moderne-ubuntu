/* ============================================================================
   Section 20.2 : Installation de handlers (signal, sigaction)
   Description : ScopedSignalHandler RAII — sauvegarder et restaurer les dispositions
   Fichier source : 02-handlers.md
   ============================================================================ */
#include <signal.h>
#include <cstring>
#include <print>

class ScopedSignalHandler {
public:
    ScopedSignalHandler(int signum, void (*handler)(int))
        : signum_(signum)
    {
        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        sigaction(signum_, &sa, &old_action_);  // Sauvegarde l'ancienne disposition
    }

    ~ScopedSignalHandler() {
        sigaction(signum_, &old_action_, nullptr);  // Restaure
    }

    ScopedSignalHandler(const ScopedSignalHandler&) = delete;
    ScopedSignalHandler& operator=(const ScopedSignalHandler&) = delete;

private:
    int signum_;
    struct sigaction old_action_;
};

// Utilisation RAII
void critical_section() {
    ScopedSignalHandler guard(SIGINT, SIG_IGN);
    // SIGINT est ignoré pendant cette portée
    std::println("  SIGINT ignoré dans la section critique");
}   // L'ancienne disposition de SIGINT est restaurée ici

int main() {
    std::println("Avant section critique");
    critical_section();
    std::println("Après section critique — SIGINT restauré");
}
