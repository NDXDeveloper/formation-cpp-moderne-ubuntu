/* ============================================================================
   Section 23.3 : Mémoire partagée anonyme (parent ↔ enfant)
   Description : mmap MAP_SHARED|MAP_ANONYMOUS pour communication parent-enfant
   Fichier source : 03-shared-memory.md
   ============================================================================ */
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <print>
#include <cstring>
#include <cstdio>

int main() {
    // Créer une zone de mémoire partagée anonyme
    size_t size = 4096;
    void* shared = mmap(nullptr, size,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS,
                        -1, 0);

    if (shared == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "mmap()");
    }

    // Initialiser
    std::memset(shared, 0, size);

    std::fflush(nullptr);
    pid_t pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork()");
    }

    if (pid == 0) {
        // Enfant : écrire dans la mémoire partagée
        const char* msg = "Hello depuis l'enfant!";
        std::memcpy(shared, msg, std::strlen(msg) + 1);
        std::println("[Enfant] Écrit: {}", msg);
        std::fflush(stdout);
        _exit(0);
    }

    // Parent : attendre l'enfant puis lire
    waitpid(pid, nullptr, 0);

    auto* data = static_cast<const char*>(shared);
    std::println("[Parent] Lu: {}", data);

    // Libérer
    munmap(shared, size);
}
