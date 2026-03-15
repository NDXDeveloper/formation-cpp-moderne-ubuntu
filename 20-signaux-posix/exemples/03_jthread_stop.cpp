/* ============================================================================
   Section 20.3 : Signaux et threads
   Description : std::jthread + std::stop_source — arrêt coopératif C++20
   Fichier source : 03-signaux-threads.md
   ============================================================================ */
#include <signal.h>
#include <atomic>
#include <thread>
#include <print>
#include <vector>
#include <chrono>
#include <cstring>
#include <unistd.h>

// Source d'arrêt partagée — le thread de signaux la déclenche,
// les jthreads la consultent via leur stop_token
std::stop_source g_stop_source;

void signal_thread_func(sigset_t wait_set) {
    int sig;
    while (sigwait(&wait_set, &sig) == 0) {
        if (sig == SIGTERM || sig == SIGINT) {
            std::println("[signal] {} reçu — demande d'arrêt", strsignal(sig));
            g_stop_source.request_stop();
            return;
        }
    }
}

void worker_func(std::stop_token stop_tok, int id) {
    while (!stop_tok.stop_requested()) {
        std::println("[worker {}] traitement...", id);

        // Attente interruptible — se réveille si stop est demandé
        // Note : std::condition_variable_any supporte les stop_tokens
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::println("[worker {}] arrêt coopératif", id);
}

int main() {
    // Masquer les signaux avant la création des threads
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, nullptr);
    signal(SIGPIPE, SIG_IGN);

    std::println("Serveur démarré (PID = {})", getpid());

    // Thread dédié aux signaux
    std::thread sig_thread(signal_thread_func, mask);

    // Workers avec jthread — reliés au stop_source global
    // On capture le token de g_stop_source pour que les workers réagissent
    // immédiatement à g_stop_source.request_stop()
    std::vector<std::jthread> workers;
    for (int i = 0; i < 4; ++i) {
        auto token = g_stop_source.get_token();
        workers.emplace_back(
            [i, token](std::stop_token /* jthread_tok */) { worker_func(token, i); }
        );
    }

    // Pour la démonstration : envoyer SIGTERM après un court délai
    std::thread([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        kill(getpid(), SIGTERM);
    }).detach();

    // Attendre que le thread de signaux détecte un arrêt
    sig_thread.join();

    // g_stop_source.request_stop() a déjà été appelé par le signal thread
    // → les workers voient stop_requested() == true et terminent leur boucle
    // Les destructeurs des jthreads appellent join automatiquement
    workers.clear();

    std::println("Arrêt propre terminé");
    return 0;
}
