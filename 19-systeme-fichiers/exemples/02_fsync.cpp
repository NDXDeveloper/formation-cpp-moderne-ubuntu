/* ============================================================================
   Section 19.2 : fsync() et écriture atomique POSIX
   Description : fsync(), fdatasync(), pattern write-then-rename durable
   Fichier source : 02-appels-posix.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <print>
#include <cerrno>
#include <cstring>
#include <string>

namespace fs = std::filesystem;

bool atomic_write_durable(const fs::path& target, const std::string& content) {
    fs::path tmp = target;
    tmp += ".tmp";

    // 1. Ouvrir le fichier temporaire
    int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) {
        std::println("open: {}", strerror(errno));
        return false;
    }

    // 2. Écriture complète
    const char* ptr = content.data();
    size_t remaining = content.size();
    while (remaining > 0) {
        ssize_t n = write(fd, ptr, remaining);
        if (n == -1) {
            if (errno == EINTR) continue;
            std::println("write: {}", strerror(errno));
            close(fd);
            fs::remove(tmp);
            return false;
        }
        ptr += n;
        remaining -= static_cast<size_t>(n);
    }

    // 3. Forcer sur le disque AVANT le rename
    if (fsync(fd) == -1) {
        std::println("fsync: {}", strerror(errno));
        close(fd);
        fs::remove(tmp);
        return false;
    }

    // 4. Fermer
    if (close(fd) == -1) {
        std::println("close: {}", strerror(errno));
    }

    // 5. Renommage atomique
    fs::rename(tmp, target);

    // 6. fsync du répertoire parent
    int dir_fd = open(target.parent_path().c_str(), O_RDONLY | O_CLOEXEC);
    if (dir_fd != -1) {
        fsync(dir_fd);
        close(dir_fd);
    }

    return true;
}

int main() {
    fs::path dir = "/tmp/durable_ex02";
    fs::create_directories(dir);

    fs::path target = dir / "config.yaml";
    bool ok = atomic_write_durable(target, "database:\n  host: localhost\n  port: 5432\n");
    std::println("Écriture durable : {}", ok ? "succès" : "échec");

    // Vérifier
    std::println("Taille : {} octets", fs::file_size(target));

    // Nettoyage
    fs::remove_all(dir);
}
