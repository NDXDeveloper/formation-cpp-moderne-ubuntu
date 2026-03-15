/* ============================================================================
   Section 19.2 : Wrapper RAII pour descripteurs de fichiers
   Description : Classe FileDescriptor avec Rule of Five
   Fichier source : 02-appels-posix.md
   ============================================================================ */
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cerrno>
#include <cstring>
#include <utility>

class FileDescriptor {
public:
    FileDescriptor() noexcept = default;

    explicit FileDescriptor(int fd) noexcept : fd_(fd) {}

    // Ouverture directe
    FileDescriptor(const char* path, int flags, mode_t mode = 0)
        : fd_(open(path, flags, mode)) {}

    ~FileDescriptor() { close_if_open(); }

    // Non copiable
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    // Déplaçable
    FileDescriptor(FileDescriptor&& other) noexcept
        : fd_(std::exchange(other.fd_, -1)) {}

    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) {
            close_if_open();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    [[nodiscard]] bool is_open() const noexcept { return fd_ >= 0; }
    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] explicit operator bool() const noexcept { return is_open(); }

    int release() noexcept { return std::exchange(fd_, -1); }

private:
    void close_if_open() noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    int fd_ = -1;
};

void process_file(const char* path) {
    FileDescriptor fd(path, O_RDONLY | O_CLOEXEC);
    if (!fd) {
        std::println("open: {}", strerror(errno));
        return;
    }

    char buf[4096];
    ssize_t n = read(fd.get(), buf, sizeof(buf));
    if (n == -1) {
        std::println("read: {}", strerror(errno));
        return;
    }

    std::println("Lu {} octets depuis {}", n, path);
}

int main() {
    process_file("/etc/hostname");
    process_file("/etc/passwd");

    // Test déplacement
    FileDescriptor fd1("/etc/hostname", O_RDONLY | O_CLOEXEC);
    std::println("fd1 ouvert : {}", fd1.is_open());

    FileDescriptor fd2 = std::move(fd1);
    std::println("Après move : fd1={}, fd2={}", fd1.is_open(), fd2.is_open());

    // Test fichier inexistant
    process_file("/tmp/inexistant_ex02.txt");
}
