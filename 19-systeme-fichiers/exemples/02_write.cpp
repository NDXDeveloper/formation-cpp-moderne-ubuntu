/* ============================================================================
   Section 19.2 : write() — écrire des données
   Description : Écriture de base et write_full() pour écritures complètes
   Fichier source : 02-appels-posix.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <memory>

// Écriture complète : écrit exactement count octets ou échoue
auto write_full(int fd, const void* buf, size_t count) -> ssize_t {
    const auto* ptr = static_cast<const uint8_t*>(buf);
    size_t total = 0;

    while (total < count) {
        ssize_t n = write(fd, ptr + total, count - total);

        if (n == -1) {
            if (errno == EINTR) {
                continue;  // Signal, réessayer
            }
            return -1;  // Vraie erreur
        }

        total += static_cast<size_t>(n);
    }

    return static_cast<ssize_t>(total);
}

int main() {
    // Écriture de base
    int fd = open("/tmp/output_ex02w.txt", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return 1;
    }

    const char* message = "Hello from POSIX write!\n";
    ssize_t n = write(fd, message, strlen(message));

    if (n == -1) {
        std::println("write: {}", strerror(errno));
    } else {
        std::println("{} octets écrits", n);
    }

    close(fd);

    // Écriture complète avec write_full
    int fd2 = open("/tmp/data_ex02w.bin", O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd2 == -1) {
        std::println("open: {}", strerror(errno));
        return 1;
    }

    constexpr size_t size = 1024 * 1024;  // 1 Mo
    auto data = std::make_unique<char[]>(size);
    std::memset(data.get(), 'A', size);

    ssize_t n2 = write_full(fd2, data.get(), size);
    if (n2 == -1) {
        std::println("write_full: {}", strerror(errno));
    } else {
        std::println("{} octets écrits", n2);
    }

    close(fd2);

    // Nettoyage
    unlink("/tmp/output_ex02w.txt");
    unlink("/tmp/data_ex02w.bin");
}
