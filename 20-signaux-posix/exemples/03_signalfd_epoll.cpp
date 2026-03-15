/* ============================================================================
   Section 20.3 : Signaux et threads
   Description : signalfd + epoll — boucle d'événements sans handler
   Fichier source : 03-signaux-threads.md
   ============================================================================ */
#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>
#include <atomic>
#include <print>
#include <cstring>
#include <cerrno>
#include <thread>
#include <vector>

std::atomic<bool> g_running{true};

class EventLoop {
public:
    bool init() {
        // Masquer les signaux
        sigemptyset(&sig_mask_);
        sigaddset(&sig_mask_, SIGTERM);
        sigaddset(&sig_mask_, SIGINT);
        sigaddset(&sig_mask_, SIGHUP);
        sigprocmask(SIG_BLOCK, &sig_mask_, nullptr);

        // Créer le signalfd
        signal_fd_ = signalfd(-1, &sig_mask_, SFD_NONBLOCK | SFD_CLOEXEC);
        if (signal_fd_ == -1) return false;

        // Créer l'instance epoll
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd_ == -1) return false;

        // Enregistrer le signalfd dans epoll
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = signal_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, signal_fd_, &ev) == -1) {
            return false;
        }

        // Ici, on enregistrerait aussi les sockets serveur dans epoll
        return true;
    }

    void run() {
        constexpr int MAX_EVENTS = 64;
        struct epoll_event events[MAX_EVENTS];

        while (g_running.load(std::memory_order_relaxed)) {
            int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);

            for (int i = 0; i < n; ++i) {
                if (events[i].data.fd == signal_fd_) {
                    handle_signal();
                } else {
                    // Traiter les événements réseau...
                }
            }
        }
    }

    ~EventLoop() {
        if (signal_fd_ >= 0) close(signal_fd_);
        if (epoll_fd_ >= 0) close(epoll_fd_);
    }

private:
    void handle_signal() {
        struct signalfd_siginfo info;
        while (read(signal_fd_, &info, sizeof(info)) == sizeof(info)) {
            switch (info.ssi_signo) {
                case SIGTERM:
                case SIGINT:
                    std::println("Signal {} — arrêt demandé", info.ssi_signo);
                    g_running.store(false, std::memory_order_relaxed);
                    break;
                case SIGHUP:
                    std::println("SIGHUP — rechargement");
                    break;
            }
        }
    }

    sigset_t sig_mask_{};
    int signal_fd_ = -1;
    int epoll_fd_ = -1;
};

int main() {
    signal(SIGPIPE, SIG_IGN);

    EventLoop loop;
    if (!loop.init()) {
        std::println("Erreur d'initialisation : {}", strerror(errno));
        return 1;
    }

    std::println("Serveur epoll démarré (PID = {})", getpid());

    // Les worker threads sont créés APRÈS le masquage (fait dans init())
    // Ils héritent du masque → aucun signal ne les atteint
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([i] {
            while (g_running.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::println("[worker {}] terminé", i);
        });
    }

    // Pour la démonstration : envoyer SIGTERM après un court délai
    std::thread([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        kill(getpid(), SIGTERM);
    }).detach();

    // La boucle d'événements tourne dans le thread principal
    loop.run();

    for (auto& w : workers) w.join();
    std::println("Arrêt propre terminé");
    return 0;
}
