/* ============================================================================
   Section 20.1 : Comprendre les signaux Unix
   Description : Envoi de signaux avec kill() et vérification d'existence
   Fichier source : 01-signaux-unix.md
   ============================================================================ */
#include <csignal>
#include <sys/types.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>

int main() {
    // Envoyer un signal à un processus spécifique
    pid_t target_pid = 1234;
    if (kill(target_pid, SIGTERM) == -1) {
        std::println("kill: {}", strerror(errno));
        // ESRCH : processus inexistant
        // EPERM : permission refusée
    }

    // Vérifier l'existence d'un processus sans envoyer de signal
    if (kill(target_pid, 0) == -1) {
        if (errno == ESRCH) {
            std::println("Le processus {} n'existe pas", target_pid);
        }
    } else {
        std::println("Le processus {} existe", target_pid);
    }

    // Envoyer un signal à tout son groupe de processus
    // kill(0, SIGTERM);  // 0 signifie "mon groupe de processus"

    // Envoyer un signal à soi-même (attention : termine le processus !)
    // raise(SIGTERM);  // Équivalent à kill(getpid(), SIGTERM)
}
