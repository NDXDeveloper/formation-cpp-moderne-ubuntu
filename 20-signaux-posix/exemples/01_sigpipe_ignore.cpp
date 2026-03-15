/* ============================================================================
   Section 20.1 : Comprendre les signaux Unix
   Description : Ignorer SIGPIPE — bonne pratique pour les programmes réseau
   Fichier source : 01-signaux-unix.md
   ============================================================================ */
#include <csignal>
#include <print>

int main() {
    // Ignorer SIGPIPE globalement — bonne pratique pour les programmes réseau
    std::signal(SIGPIPE, SIG_IGN);

    // Désormais, write() sur un pipe cassé retourne -1 avec errno == EPIPE
    // au lieu de tuer le processus
    std::println("SIGPIPE ignoré avec succès");
}
