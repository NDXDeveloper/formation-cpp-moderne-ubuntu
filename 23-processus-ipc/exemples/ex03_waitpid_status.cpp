/* ============================================================================
   Section 23.1 : wait() et waitpid() — Récupérer le statut de l'enfant
   Description : Analyse complète du status retourné par waitpid()
   Fichier source : 01-fork-exec.md
   ============================================================================ */
#include <unistd.h>
#include <sys/wait.h>
#include <print>
#include <cstdio>
#include <cstring>

int main() {
    // Test 1 : enfant qui termine normalement
    std::println("=== Test 1 : terminaison normale ===");
    std::fflush(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        execlp("echo", "echo", "hello from child", nullptr);
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        std::println("Enfant terminé avec le code {}", code);
        if (code == 0) {
            std::println("Exécution réussie");
        } else {
            std::println("Exécution échouée");
        }
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        std::println("Enfant tué par le signal {} ({})",
                     sig, strsignal(sig));
    }

    // Test 2 : enfant tué par un signal
    std::println("\n=== Test 2 : enfant tué par signal ===");
    std::fflush(nullptr);

    pid = fork();
    if (pid == 0) {
        // L'enfant boucle indéfiniment
        while (true) { sleep(1); }
        _exit(0);
    }

    // Parent envoie SIGKILL
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        std::println("Enfant terminé avec le code {}", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        std::println("Enfant tué par le signal {} ({})",
                     sig, strsignal(sig));
    }

    // Test 3 : vérification non-bloquante (WNOHANG)
    std::println("\n=== Test 3 : polling non-bloquant ===");
    std::fflush(nullptr);

    pid = fork();
    if (pid == 0) {
        _exit(42);
    }

    usleep(100000);  // Laisser le temps au child de finir

    pid_t result = waitpid(pid, &status, WNOHANG);
    if (result == 0) {
        std::println("L'enfant tourne encore");
    } else if (result == pid) {
        std::println("L'enfant a terminé (code {})", WEXITSTATUS(status));
    } else {
        std::println("Erreur waitpid");
    }
}
