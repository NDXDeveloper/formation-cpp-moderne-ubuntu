/* ============================================================================
   Section 23.3 : Mémoire partagée POSIX (shm_open + mmap)
   Description : Writer/reader combiné via fork pour test autonome
   Fichier source : 03-shared-memory.md
   ============================================================================ */
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cstring>
#include <cstdio>
#include <atomic>

struct SharedData {
    int counter;
    char message[256];
    std::atomic<bool> ready;
};

int main() {
    const char* name = "/demo_shm_ex15";
    const size_t size = sizeof(SharedData);

    // Nettoyer d'éventuels résidus
    shm_unlink(name);

    // Créer le segment
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category(), "shm_open()");
    }

    if (ftruncate(fd, static_cast<off_t>(size)) == -1) {
        throw std::system_error(errno, std::system_category(), "ftruncate()");
    }

    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "mmap()");
    }
    close(fd);

    auto* data = static_cast<SharedData*>(ptr);
    data->ready.store(false, std::memory_order_relaxed);

    std::fflush(nullptr);
    pid_t pid = fork();

    if (pid == 0) {
        // ── LECTEUR (enfant) ──
        // Attendre que les données soient prêtes
        while (!data->ready.load(std::memory_order_acquire)) {
            usleep(1000);
        }

        std::println("[Lecteur]");
        std::println("  counter = {}", data->counter);
        std::println("  message = {}", data->message);
        std::fflush(stdout);

        munmap(ptr, size);
        _exit(0);
    }

    // ── ÉCRIVAIN (parent) ──
    data->counter = 42;
    std::strcpy(data->message, "Hello depuis la mémoire partagée!");
    data->ready.store(true, std::memory_order_release);

    std::println("[Écrivain] Données écrites");
    std::println("  counter = {}", data->counter);
    std::println("  message = {}", data->message);

    waitpid(pid, nullptr, 0);

    // Nettoyer
    munmap(ptr, size);
    shm_unlink(name);
    std::println("Segment supprimé");
}
