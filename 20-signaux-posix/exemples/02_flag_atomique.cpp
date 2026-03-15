/* ============================================================================
   Section 20.2 : Installation de handlers (signal, sigaction)
   Description : Pattern 1 — Flag atomique pour arrêt propre
   Fichier source : 02-handlers.md
   ============================================================================ */
#include <signal.h>
#include <atomic>
#include <print>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <chrono>

// Variable globale — le seul moyen de communiquer avec un handler
// volatile sig_atomic_t est le type garanti par le standard C
// std::atomic<bool> est aussi sûr en pratique sur Linux
volatile sig_atomic_t g_shutdown_requested = 0;

void shutdown_handler(int /* sig */) {
    g_shutdown_requested = 1;
    // C'est TOUT ce que fait le handler. Rien d'autre.
}

int main() {
    // Installer le handler pour SIGTERM et SIGINT
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = shutdown_handler;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);   // Pendant SIGTERM, masquer SIGINT
    sigaddset(&sa.sa_mask, SIGTERM);  // Pendant SIGINT, masquer SIGTERM
    sa.sa_flags = 0;  // Pas de SA_RESTART — on veut interrompre les appels bloquants

    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    std::println("Service démarré (PID = {}). Ctrl+C ou kill -TERM pour arrêter.",
        getpid());

    // Pour la démonstration : envoyer SIGTERM après un court délai
    std::thread([]{
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        kill(getpid(), SIGTERM);
    }).detach();

    // Boucle principale — vérifie le flag régulièrement
    while (!g_shutdown_requested) {
        // Simuler du travail
        std::println("Traitement en cours...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // === Zone de nettoyage — code normal, toutes les fonctions sont autorisées ===
    std::println("Arrêt propre en cours...");

    // Sauvegarder l'état
    // Fermer les connexions réseau
    // Supprimer les fichiers temporaires
    // Flush des logs

    std::println("Arrêt terminé.");
    return 0;
}
