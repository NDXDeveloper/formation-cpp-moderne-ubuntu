/* ============================================================================
   Section 23.2 : Pipes anonymes — Création et utilisation de base
   Description : Écriture et lecture dans un pipe anonyme (même processus)
   Fichier source : 02-pipes.md
   ============================================================================ */
#include <unistd.h>
#include <fcntl.h>
#include <print>
#include <cstring>

int main() {
    int pipefd[2];
    if (pipe2(pipefd, O_CLOEXEC) == -1) {
        throw std::system_error(errno, std::system_category(), "pipe2()");
    }

    // Écrire dans le pipe
    const char* message = "Hello depuis le pipe!";
    write(pipefd[1], message, strlen(message));

    // Lire depuis le pipe
    char buffer[256];
    ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
    buffer[n] = '\0';

    std::println("Lu: {}", buffer);

    close(pipefd[0]);
    close(pipefd[1]);
}
