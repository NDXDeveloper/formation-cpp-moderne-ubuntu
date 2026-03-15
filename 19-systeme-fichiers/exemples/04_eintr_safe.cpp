/* ============================================================================
   Section 19.4 : Gestion de EINTR avec template utilitaire
   Description : eintr_safe() pour relancer automatiquement les appels système
   Fichier source : 04-permissions-droits.md
   ============================================================================ */
#include <cerrno>
#include <utility>
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cstring>

// Relance automatiquement un appel système interrompu par un signal
template<typename Fn, typename... Args>
auto eintr_safe(Fn fn, Args&&... args) {
    decltype(fn(std::forward<Args>(args)...)) result;
    do {
        result = fn(std::forward<Args>(args)...);
    } while (result == -1 && errno == EINTR);
    return result;
}

int main() {
    // Ouvrir avec eintr_safe
    int fd = eintr_safe(open, "/etc/hostname", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return 1;
    }
    std::println("Ouvert avec eintr_safe (fd={})", fd);

    // Lire avec eintr_safe
    char buf[256] = {};
    ssize_t n = eintr_safe(read, fd, buf, sizeof(buf) - 1);
    if (n == -1) {
        std::println("read: {}", strerror(errno));
    } else {
        buf[n] = '\0';
        std::println("Lu {} octets : {}", n, buf);
    }

    // Fermer avec eintr_safe
    eintr_safe(close, fd);
    std::println("Fermé avec eintr_safe");

    // Écriture avec eintr_safe
    int fd2 = eintr_safe(open, "/tmp/eintr_ex04.txt",
        O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd2 >= 0) {
        const char* msg = "test eintr_safe\n";
        ssize_t w = eintr_safe(write, fd2, msg, strlen(msg));
        std::println("Écrit {} octets", w);
        eintr_safe(close, fd2);
        unlink("/tmp/eintr_ex04.txt");
    }
}
