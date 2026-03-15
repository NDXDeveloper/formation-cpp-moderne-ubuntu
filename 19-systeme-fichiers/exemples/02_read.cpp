/* ============================================================================
   Section 19.2 : read() — lire des données
   Description : Lecture de base et read_full() pour lectures complètes
   Fichier source : 02-appels-posix.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <array>
#include <cerrno>
#include <cstring>
#include <cstdint>

// Lecture complète : lit exactement count octets ou jusqu'à EOF
auto read_full(int fd, void* buf, size_t count) -> ssize_t {
    auto* ptr = static_cast<uint8_t*>(buf);
    size_t total = 0;

    while (total < count) {
        ssize_t n = read(fd, ptr + total, count - total);

        if (n == -1) {
            if (errno == EINTR) {
                continue;  // Interrompu par un signal, réessayer
            }
            return -1;  // Vraie erreur
        }
        if (n == 0) {
            break;  // EOF atteint
        }

        total += static_cast<size_t>(n);
    }

    return static_cast<ssize_t>(total);
}

int main() {
    // Lecture de base
    int fd = open("/etc/hostname", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return 1;
    }

    std::array<char, 256> buf{};
    ssize_t n = read(fd, buf.data(), buf.size() - 1);

    if (n == -1) {
        std::println("read: {}", strerror(errno));
    } else {
        buf[static_cast<size_t>(n)] = '\0';
        std::println("Contenu ({} octets) : {}", n, buf.data());
    }

    close(fd);

    // Lecture complète avec read_full
    int fd2 = open("/etc/passwd", O_RDONLY | O_CLOEXEC);
    if (fd2 == -1) {
        std::println("open: {}", strerror(errno));
        return 1;
    }

    char buf2[4096];
    ssize_t n2 = read_full(fd2, buf2, sizeof(buf2) - 1);
    if (n2 >= 0) {
        buf2[n2] = '\0';
        std::println("Lu {} octets depuis /etc/passwd", n2);
    }

    close(fd2);
}
