/* ============================================================================
   Section 20.2 : Installation de handlers (signal, sigaction)
   Description : Handler étendu SA_SIGINFO avec informations siginfo_t
   Fichier source : 02-handlers.md
   ============================================================================ */
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <print>

void detailed_handler(int sig, siginfo_t* info, void* /* ucontext */) {
    // Toutes les fonctions utilisées ici doivent être async-signal-safe
    char buf[128];
    int len = snprintf(buf, sizeof(buf),
        "Signal %d reçu de PID %d (UID %d), code=%d\n",
        sig, info->si_pid, info->si_uid, info->si_code);
    if (len > 0) {
        write(STDERR_FILENO, buf, static_cast<size_t>(len));
    }
}

int main() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = detailed_handler;  // Utiliser sa_sigaction, PAS sa_handler
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;            // Activer le mode étendu

    sigaction(SIGUSR1, &sa, nullptr);

    std::println("Handler SA_SIGINFO installé pour SIGUSR1");

    // Pour la démonstration : s'envoyer SIGUSR1
    raise(SIGUSR1);
}
