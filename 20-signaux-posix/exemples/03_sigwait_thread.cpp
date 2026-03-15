/* ============================================================================
   Section 20.3 : Signaux et threads
   Description : Thread dédié aux signaux avec sigwait() — pattern recommandé
   Fichier source : 03-signaux-threads.md
   ============================================================================ */
#include <signal.h>
#include <pthread.h>
#include <atomic>
#include <thread>
#include <print>
#include <vector>
#include <cstring>
#include <unistd.h>

std::atomic<bool> g_running{true};

void reload_configuration() { std::println("  Configuration rechargée"); }
void dump_stats() { std::println("  Statistiques affichées"); }

void signal_thread_func(sigset_t wait_set) {
    while (true) {
        int sig;
        // sigwait() attend de manière SYNCHRONE qu'un signal pending arrive
        int ret = sigwait(&wait_set, &sig);
        if (ret != 0) {
            continue;  // Erreur, réessayer
        }

        // === Contexte normal — toutes les fonctions sont autorisées ===
        switch (sig) {
            case SIGTERM:
                std::println("[signal_thread] SIGTERM reçu — arrêt demandé");
                g_running.store(false, std::memory_order_relaxed);
                return;

            case SIGINT:
                std::println("[signal_thread] SIGINT reçu — arrêt demandé");
                g_running.store(false, std::memory_order_relaxed);
                return;

            case SIGHUP:
                std::println("[signal_thread] SIGHUP reçu — rechargement config");
                reload_configuration();
                break;

            case SIGUSR1:
                std::println("[signal_thread] SIGUSR1 — dump des statistiques");
                dump_stats();
                break;

            default:
                std::println("[signal_thread] Signal {} ignoré", sig);
                break;
        }
    }
}

void worker_func(int id) {
    while (g_running.load(std::memory_order_relaxed)) {
        // Ce thread ne recevra JAMAIS de signal (ils sont tous masqués)
        // Pas de risque de deadlock, pas de EINTR, pas d'interruption
        std::println("[worker {}] Traitement...", id);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::println("[worker {}] Arrêt", id);
}

int main() {
    // 1. Masquer les signaux AVANT de créer les threads
    //    Le masque est hérité par tous les threads créés ensuite
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    // Ignorer SIGPIPE globalement (pas besoin de thread dédié pour ça)
    signal(SIGPIPE, SIG_IGN);

    std::println("Serveur démarré (PID = {})", getpid());

    // 2. Créer le thread dédié aux signaux
    std::thread signal_thread(signal_thread_func, mask);

    // 3. Créer les worker threads
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back(worker_func, i);
    }

    // Pour la démonstration : envoyer SIGTERM après un court délai
    std::thread([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        kill(getpid(), SIGTERM);
    }).detach();

    // 4. Attendre la terminaison
    signal_thread.join();

    // Le flag g_running est maintenant false — les workers vont terminer
    for (auto& w : workers) {
        w.join();
    }

    std::println("Tous les threads terminés — arrêt propre");
    return 0;
}
