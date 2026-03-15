/* ============================================================================
   Section 23.2 : Communication parent → enfant
   Description : Envoi de données du parent vers l'enfant via pipe anonyme
   Fichier source : 02-pipes.md
   ============================================================================ */
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <print>
#include <cstring>
#include <cstdio>

int main() {
    int pipefd[2];
    if (pipe2(pipefd, O_CLOEXEC) == -1) {
        throw std::system_error(errno, std::system_category(), "pipe2()");
    }

    std::fflush(nullptr);
    pid_t pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork()");
    }

    if (pid == 0) {
        // ── ENFANT : lecteur ──
        close(pipefd[1]);  // Fermer le côté écriture (inutile ici)

        char buffer[256];
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            std::println("[Enfant] Reçu: {}", buffer);
            std::fflush(stdout);
        }

        close(pipefd[0]);
        _exit(0);
    }

    // ── PARENT : écrivain ──
    close(pipefd[0]);  // Fermer le côté lecture (inutile ici)

    const char* msg = "Message du parent";
    write(pipefd[1], msg, strlen(msg));
    close(pipefd[1]);  // Fermer → l'enfant verra EOF

    waitpid(pid, nullptr, 0);
    std::println("[Parent] Enfant terminé");
}
