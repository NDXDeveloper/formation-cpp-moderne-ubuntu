/* ============================================================================
   Section 20.1 : Comprendre les signaux Unix
   Description : Handler minimaliste pour SIGSEGV — diagnostic puis terminaison
   Fichier source : 01-signaux-unix.md
   ============================================================================ */
#include <csignal>
#include <unistd.h>
#include <cstring>
#include <print>

// Ce handler est minimaliste — c'est intentionnel (voir section 20.2)
void sigsegv_handler(int sig) {
    const char msg[] = "FATAL: Segmentation fault — arrêt du programme\n";
    // write() est async-signal-safe, std::println ne l'est PAS
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    _exit(128 + sig);  // _exit est async-signal-safe, exit() ne l'est PAS
}

int main() {
    // Installer le handler SIGSEGV
    std::signal(SIGSEGV, sigsegv_handler);
    std::println("Handler SIGSEGV installé");

    // Provoquer un SIGSEGV intentionnellement pour le test
    std::println("Déclenchement d'un SIGSEGV...");
    int* p = nullptr;
    // volatile pour empêcher l'optimisation
    *const_cast<volatile int*>(p) = 42;
}
