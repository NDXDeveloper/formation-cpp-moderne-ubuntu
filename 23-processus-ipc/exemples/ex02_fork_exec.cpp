/* ============================================================================
   Section 23.1 : La famille exec() — Remplacer l'image du processus
   Description : Utilisation de execvp et execlp après fork()
   Fichier source : 01-fork-exec.md
   ============================================================================ */
#include <unistd.h>
#include <sys/wait.h>
#include <print>
#include <cstdio>

int main() {
    std::println("=== Test execvp ===");
    std::fflush(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        // Enfant — se transformer en "ls -la /tmp"
        char* args[] = {
            const_cast<char*>("ls"),
            const_cast<char*>("-la"),
            const_cast<char*>("/tmp"),
            nullptr  // Terminateur obligatoire
        };

        execvp("ls", args);

        // Si on arrive ici, exec a échoué
        perror("execvp");
        _exit(127);  // Convention : 127 = commande non trouvée
    }

    int status;
    waitpid(pid, &status, 0);
    std::println("execvp terminé avec le code {}", WEXITSTATUS(status));

    std::println("\n=== Test execlp ===");
    std::fflush(nullptr);

    pid = fork();
    if (pid == 0) {
        execlp("ls", "ls", "-la", "/tmp", nullptr);
        perror("execlp");
        _exit(127);
    }

    waitpid(pid, &status, 0);
    std::println("execlp terminé avec le code {}", WEXITSTATUS(status));
}
