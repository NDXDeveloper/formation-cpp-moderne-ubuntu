/* ============================================================================
   Section 19.3 : Scénario 3 — Écriture critique avec fsync
   Description : write_critical() avec durabilité garantie (POSIX + filesystem)
   Fichier source : 03-comparaison-api.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <print>
#include <cerrno>
#include <cstring>

namespace fs = std::filesystem;

bool write_critical(const fs::path& target, const std::string& data) {
    fs::path tmp = target;
    tmp += ".tmp";

    int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd == -1) return false;

    // Écriture complète (boucle write)
    const char* ptr = data.data();
    size_t remaining = data.size();
    while (remaining > 0) {
        ssize_t n = write(fd, ptr, remaining);
        if (n == -1) {
            if (errno == EINTR) continue;
            close(fd);
            return false;
        }
        ptr += n;
        remaining -= static_cast<size_t>(n);
    }

    // Durabilité garantie
    fsync(fd);
    close(fd);

    // Renommage atomique
    fs::rename(tmp, target);

    // fsync du répertoire parent
    int dir_fd = open(target.parent_path().c_str(), O_RDONLY | O_CLOEXEC);
    if (dir_fd >= 0) { fsync(dir_fd); close(dir_fd); }

    return true;
}

int main() {
    fs::path dir = "/tmp/critical_ex03";
    fs::create_directories(dir);

    fs::path target = dir / "database.conf";
    bool ok = write_critical(target, "max_connections=100\nshared_buffers=256MB\n");
    std::println("Écriture critique : {}", ok ? "succès" : "échec");
    std::println("Taille : {} octets", fs::file_size(target));

    // Nettoyage
    fs::remove_all(dir);
}
