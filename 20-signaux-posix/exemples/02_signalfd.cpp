/* ============================================================================
   Section 20.2 : Installation de handlers (signal, sigaction)
   Description : Pattern 3 — signalfd (Linux) pour lecture synchrone des signaux
   Fichier source : 02-handlers.md
   ============================================================================ */
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <print>
#include <cstring>
#include <cerrno>
#include <thread>
#include <chrono>

int main() {
    // 1. Masquer les signaux qu'on veut lire via signalfd
    //    (un signal masqué n'invoque pas de handler, il reste pending
    //    et sera lu par signalfd)
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    // 2. Créer le signalfd
    int sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (sfd == -1) {
        std::println("signalfd: {}", strerror(errno));
        return 1;
    }

    std::println("Serveur démarré (PID = {})", getpid());

    // Pour la démonstration : envoyer SIGTERM après un court délai
    std::thread([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        kill(getpid(), SIGTERM);
    }).detach();

    // 3. Intégrer dans la boucle d'événements
    struct pollfd fds[1];
    fds[0].fd = sfd;
    fds[0].events = POLLIN;

    bool running = true;
    while (running) {
        int ret = poll(fds, 1, 1000);

        if (ret > 0 && (fds[0].revents & POLLIN)) {
            struct signalfd_siginfo info;
            ssize_t n = read(sfd, &info, sizeof(info));
            if (n == sizeof(info)) {
                std::println("Signal {} reçu de PID {} (UID {})",
                    info.ssi_signo, info.ssi_pid, info.ssi_uid);
                running = false;
            }
        }

        if (running) {
            std::println("Traitement...");
        }
    }

    std::println("Arrêt propre terminé");
    close(sfd);
    return 0;
}
