/* ============================================================================
   Section 20.2 : Installation de handlers (signal, sigaction)
   Description : Pattern 2 — Self-pipe trick pour boucle d'événements
   Fichier source : 02-handlers.md
   ============================================================================ */
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <print>
#include <cstring>
#include <cerrno>
#include <thread>
#include <chrono>

// Pipe global — le handler écrit dedans, la boucle principale le surveille
int g_signal_pipe[2] = {-1, -1};

void pipe_handler(int sig) {
    // Écrire un octet identifiant le signal dans le pipe
    // write() sur un pipe est async-signal-safe
    char s = static_cast<char>(sig);
    write(g_signal_pipe[1], &s, 1);  // Ignorer le retour — best effort
}

int main() {
    // Créer le pipe en mode non-bloquant
    if (pipe(g_signal_pipe) == -1) {
        std::println("pipe: {}", strerror(errno));
        return 1;
    }
    fcntl(g_signal_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(g_signal_pipe[1], F_SETFL, O_NONBLOCK);
    fcntl(g_signal_pipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(g_signal_pipe[1], F_SETFD, FD_CLOEXEC);

    // Installer le handler
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = pipe_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    std::println("Serveur démarré (PID = {})", getpid());

    // Pour la démonstration : envoyer SIGTERM après un court délai
    std::thread([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        kill(getpid(), SIGTERM);
    }).detach();

    // Boucle d'événements avec poll
    // En production, on surveillerait aussi les sockets clients
    struct pollfd fds[1];
    fds[0].fd = g_signal_pipe[0];
    fds[0].events = POLLIN;

    bool running = true;
    while (running) {
        int ret = poll(fds, 1, 1000);  // Timeout 1 seconde

        if (ret > 0 && (fds[0].revents & POLLIN)) {
            // Signal reçu — lire l'octet du pipe
            char sig;
            while (read(g_signal_pipe[0], &sig, 1) > 0) {
                std::println("Signal {} reçu via self-pipe", static_cast<int>(sig));
                running = false;
            }
        }

        if (running) {
            // Travail normal...
            std::println("En attente de connexions...");
        }
    }

    // Nettoyage (code normal)
    std::println("Arrêt propre terminé");
    close(g_signal_pipe[0]);
    close(g_signal_pipe[1]);
    return 0;
}
