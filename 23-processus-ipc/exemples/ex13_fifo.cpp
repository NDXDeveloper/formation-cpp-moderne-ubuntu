/* ============================================================================
   Section 23.2 : Pipes nommés (FIFOs)
   Description : Producteur/consommateur via FIFO (fork pour test autonome)
   Fichier source : 02-pipes.md
   ============================================================================ */
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <print>
#include <string>
#include <cstdio>

int main() {
    const char* fifo_path = "/tmp/test_fifo_ex13";

    // Nettoyer d'éventuels résidus
    unlink(fifo_path);

    // Créer le FIFO
    if (mkfifo(fifo_path, 0666) == -1) {
        throw std::system_error(errno, std::system_category(), "mkfifo()");
    }

    std::println("FIFO créé: {}", fifo_path);
    std::fflush(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        // ── CONSOMMATEUR (enfant) ──
        int fd = open(fifo_path, O_RDONLY);
        if (fd == -1) {
            perror("open(consumer)");
            _exit(1);
        }

        std::println("[Consommateur] Connecté");
        std::fflush(stdout);

        char buffer[256];
        ssize_t n;
        while ((n = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            std::print("[Consommateur] Reçu: {}", buffer);
            std::fflush(stdout);
        }

        close(fd);
        std::println("[Consommateur] Terminé");
        std::fflush(stdout);
        _exit(0);
    }

    // ── PRODUCTEUR (parent) ──
    int fd = open(fifo_path, O_WRONLY);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category(), "open(producer)");
    }

    std::println("[Producteur] Connecté");

    for (int i = 0; i < 5; ++i) {
        std::string msg = "Message " + std::to_string(i) + "\n";
        write(fd, msg.data(), msg.size());
        std::println("[Producteur] Envoyé: Message {}", i);
        usleep(200000);  // 200ms
    }

    close(fd);
    std::println("[Producteur] Terminé");

    waitpid(pid, nullptr, 0);

    // Nettoyer le FIFO
    unlink(fifo_path);
    std::println("FIFO nettoyé");
}
