/* ============================================================================
   Section 19.2 : Copie efficace avec buffer adaptatif
   Description : copy_file_posix avec taille de buffer basée sur fstat()
   Fichier source : 02-appels-posix.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <print>
#include <cerrno>
#include <cstring>
#include <memory>
#include <cstdint>
#include <fstream>

auto copy_file_posix(const char* src, const char* dst) -> bool {
    int fd_in = open(src, O_RDONLY | O_CLOEXEC);
    if (fd_in == -1) return false;

    int fd_out = open(dst, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd_out == -1) {
        close(fd_in);
        return false;
    }

    // Utiliser la taille de bloc optimale du filesystem
    struct stat st;
    size_t buf_size = 65536;  // 64 Ko par défaut
    if (fstat(fd_in, &st) == 0 && st.st_blksize > 0) {
        buf_size = static_cast<size_t>(st.st_blksize);
        if (buf_size < 4096) buf_size = 4096;
        if (buf_size > 1048576) buf_size = 1048576;
    }

    auto buf = std::make_unique<char[]>(buf_size);
    uint64_t total = 0;

    while (true) {
        ssize_t n = read(fd_in, buf.get(), buf_size);
        if (n == -1) {
            if (errno == EINTR) continue;
            std::println("read: {}", strerror(errno));
            close(fd_in);
            close(fd_out);
            return false;
        }
        if (n == 0) break;  // EOF

        const char* ptr = buf.get();
        size_t remaining = static_cast<size_t>(n);
        while (remaining > 0) {
            ssize_t w = write(fd_out, ptr, remaining);
            if (w == -1) {
                if (errno == EINTR) continue;
                std::println("write: {}", strerror(errno));
                close(fd_in);
                close(fd_out);
                return false;
            }
            ptr += w;
            remaining -= static_cast<size_t>(w);
        }

        total += static_cast<uint64_t>(n);
    }

    close(fd_in);
    close(fd_out);
    std::println("Copié {} octets (buffer {} octets)", total, buf_size);
    return true;
}

int main() {
    // Créer un fichier source de test
    {
        std::ofstream out("/tmp/src_ex02cp.txt");
        for (int i = 0; i < 1000; ++i) {
            out << "Ligne " << i << " — données de test pour la copie POSIX\n";
        }
    }

    // Copier
    bool ok = copy_file_posix("/tmp/src_ex02cp.txt", "/tmp/dst_ex02cp.txt");
    std::println("Copie : {}", ok ? "succès" : "échec");

    // Vérifier
    struct stat st1, st2;
    stat("/tmp/src_ex02cp.txt", &st1);
    stat("/tmp/dst_ex02cp.txt", &st2);
    std::println("Source : {} octets", st1.st_size);
    std::println("Dest   : {} octets", st2.st_size);
    std::println("Tailles identiques : {}", st1.st_size == st2.st_size);

    // Nettoyage
    unlink("/tmp/src_ex02cp.txt");
    unlink("/tmp/dst_ex02cp.txt");
}
