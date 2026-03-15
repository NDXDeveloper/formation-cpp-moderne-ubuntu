/* ============================================================================
   Section 20.2 : Installation de handlers (signal, sigaction)
   Description : Graceful shutdown — squelette complet de serveur avec arrêt propre
   Fichier source : 02-handlers.md
   ============================================================================ */
#include <signal.h>
#include <atomic>
#include <print>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <vector>

std::atomic<bool> g_shutdown{false};

void signal_handler(int /* sig */) {
    g_shutdown.store(true, std::memory_order_relaxed);
}

class Server {
public:
    void start() {
        install_signal_handlers();
        std::println("Serveur démarré (PID = {})", getpid());
        run();
        shutdown();
    }

private:
    void install_signal_handlers() {
        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = signal_handler;
        sigemptyset(&sa.sa_mask);
        // Masquer SIGINT pendant le traitement de SIGTERM et vice versa
        sigaddset(&sa.sa_mask, SIGTERM);
        sigaddset(&sa.sa_mask, SIGINT);
        sa.sa_flags = 0;  // Pas SA_RESTART — on veut interrompre les sleeps

        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGINT, &sa, nullptr);

        // Ignorer SIGPIPE globalement
        signal(SIGPIPE, SIG_IGN);
    }

    void run() {
        // Pour la démonstration : envoyer SIGTERM après un court délai
        std::thread([]{
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            kill(getpid(), SIGTERM);
        }).detach();

        while (!g_shutdown.load(std::memory_order_relaxed)) {
            // Simuler le traitement de requêtes
            process_pending_requests();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void process_pending_requests() {
        // ... traitement des requêtes en cours ...
    }

    void shutdown() {
        std::println("Arrêt en cours...");

        // 1. Arrêter d'accepter de nouvelles connexions
        std::println("  Refus des nouvelles connexions");

        // 2. Terminer les requêtes en cours (avec timeout)
        std::println("  Finalisation des requêtes en cours...");
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::seconds(10);

        while (has_pending_work()
               && std::chrono::steady_clock::now() < deadline)
        {
            process_pending_requests();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (has_pending_work()) {
            std::println("  Timeout — requêtes restantes abandonnées");
        }

        // 3. Fermer les ressources
        std::println("  Fermeture des connexions");
        std::println("  Flush des logs");
        std::println("  Suppression des fichiers temporaires");

        std::println("Arrêt terminé.");
    }

    bool has_pending_work() const { return false; /* stub */ }
};

int main() {
    Server server;
    server.start();
    return 0;
}
