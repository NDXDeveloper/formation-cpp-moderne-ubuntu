/* ============================================================================
   Section 23.1 : fork() — Cloner le processus
   Description : Exemple basique de fork() avec identification parent/enfant
   Fichier source : 01-fork-exec.md
   ============================================================================ */
#include <unistd.h>
#include <sys/wait.h>
#include <print>
#include <cstdio>

int main() {
    std::println("Avant fork — PID {}", getpid());
    std::fflush(nullptr);  // Flush tous les buffers AVANT fork

    pid_t pid = fork();

    if (pid == -1) {
        // Erreur — fork a échoué
        throw std::system_error(errno, std::system_category(), "fork()");
    }

    if (pid == 0) {
        // ── Code exécuté par l'ENFANT ──
        std::println("Enfant — PID {}, parent PID {}", getpid(), getppid());
        std::fflush(stdout);  // Flush avant _exit (qui ne flush pas les buffers)
        _exit(0);  // Terminer l'enfant
    }

    // ── Code exécuté par le PARENT ──
    std::println("Parent — PID {}, enfant PID {}", getpid(), pid);

    // Attendre que l'enfant se termine
    int status;
    waitpid(pid, &status, 0);
    std::println("Enfant terminé avec le code {}", WEXITSTATUS(status));
}
