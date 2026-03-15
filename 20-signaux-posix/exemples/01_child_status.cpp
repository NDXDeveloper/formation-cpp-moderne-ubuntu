/* ============================================================================
   Section 20.1 : Comprendre les signaux Unix
   Description : Détecter comment un processus enfant a terminé (waitpid)
   Fichier source : 01-signaux-unix.md
   ============================================================================ */
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <print>

void check_child_status() {
    int status;
    pid_t pid = wait(&status);

    if (pid == -1) return;

    if (WIFEXITED(status)) {
        std::println("Processus {} terminé normalement, code {}",
            pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        std::println("Processus {} tué par signal {} ({})",
            pid, WTERMSIG(status), strsignal(WTERMSIG(status)));

        if (WCOREDUMP(status)) {
            std::println("  Core dump généré");
        }
    } else if (WIFSTOPPED(status)) {
        std::println("Processus {} stoppé par signal {}",
            pid, WSTOPSIG(status));
    }
}

int main() {
    // Créer un processus enfant qui termine avec le code 42
    pid_t pid = fork();
    if (pid == 0) {
        // Enfant
        _exit(42);
    }
    // Parent — attendre et analyser le statut de l'enfant
    check_child_status();
}
