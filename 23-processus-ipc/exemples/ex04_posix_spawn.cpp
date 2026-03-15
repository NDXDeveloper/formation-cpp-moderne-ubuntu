/* ============================================================================
   Section 23.1 : posix_spawn() — L'alternative moderne
   Description : Lancement de processus avec posix_spawn et gestion du status
   Fichier source : 01-fork-exec.md
   ============================================================================ */
#include <spawn.h>
#include <sys/wait.h>
#include <print>
#include <cstring>

extern char** environ;  // Variables d'environnement héritées

void run_command(const char* program, char* const argv[]) {
    pid_t pid;

    // Attributs du nouveau processus
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);

    // Actions sur les file descriptors (optionnel)
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    // Lancer le processus
    int err = posix_spawnp(&pid, program, &actions, &attr, argv, environ);

    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&actions);

    if (err != 0) {
        std::println(stderr, "posix_spawnp failed: {}", strerror(err));
        return;
    }

    std::println("Lancé {} avec PID {}", program, pid);

    // Attendre la fin
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        std::println("{} terminé avec le code {}", program, WEXITSTATUS(status));
    }
}

int main() {
    char* args[] = {
        const_cast<char*>("ls"),
        const_cast<char*>("-la"),
        const_cast<char*>("/tmp"),
        nullptr
    };
    run_command("ls", args);
}
