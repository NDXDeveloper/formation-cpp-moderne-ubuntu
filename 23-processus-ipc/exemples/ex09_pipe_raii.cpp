/* ============================================================================
   Section 23.2 : Wrapper RAII pour les pipes
   Description : Classe Pipe RAII avec utilisation fork parent→enfant
   Fichier source : 02-pipes.md
   ============================================================================ */
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <utility>
#include <system_error>
#include <string>
#include <sys/wait.h>
#include <print>
#include <cstdio>

class Pipe {
public:
    Pipe() {
        if (pipe2(fds_, O_CLOEXEC) == -1) {
            throw std::system_error(errno, std::system_category(), "pipe2()");
        }
    }

    ~Pipe() {
        close_read();
        close_write();
    }

    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    Pipe(Pipe&& other) noexcept {
        fds_[0] = std::exchange(other.fds_[0], -1);
        fds_[1] = std::exchange(other.fds_[1], -1);
    }

    Pipe& operator=(Pipe&& other) noexcept {
        if (this != &other) {
            close_read();
            close_write();
            fds_[0] = std::exchange(other.fds_[0], -1);
            fds_[1] = std::exchange(other.fds_[1], -1);
        }
        return *this;
    }

    [[nodiscard]] int read_fd() const noexcept { return fds_[0]; }
    [[nodiscard]] int write_fd() const noexcept { return fds_[1]; }

    void close_read() {
        if (fds_[0] != -1) { close(fds_[0]); fds_[0] = -1; }
    }

    void close_write() {
        if (fds_[1] != -1) { close(fds_[1]); fds_[1] = -1; }
    }

    ssize_t read_data(void* buf, size_t count) {
        ssize_t n;
        do {
            n = read(fds_[0], buf, count);
        } while (n == -1 && errno == EINTR);
        return n;
    }

    ssize_t write_data(const void* buf, size_t count) {
        ssize_t n;
        do {
            n = write(fds_[1], buf, count);
        } while (n == -1 && errno == EINTR);
        return n;
    }

    std::string read_all() {
        std::string result;
        char buffer[4096];
        ssize_t n;
        while ((n = read_data(buffer, sizeof(buffer))) > 0) {
            result.append(buffer, static_cast<size_t>(n));
        }
        if (n == -1) {
            throw std::system_error(errno, std::system_category(), "read()");
        }
        return result;
    }

private:
    int fds_[2] = {-1, -1};
};

int main() {
    Pipe p;

    std::fflush(nullptr);
    pid_t pid = fork();
    if (pid == 0) {
        // Enfant : lecteur
        p.close_write();
        std::string data = p.read_all();
        std::println("[Enfant] Reçu {} octets: {}", data.size(), data);
        std::fflush(stdout);
        _exit(0);
    }

    // Parent : écrivain
    p.close_read();
    std::string msg = "Données envoyées via RAII Pipe";
    p.write_data(msg.data(), msg.size());
    p.close_write();  // Signal EOF à l'enfant

    waitpid(pid, nullptr, 0);
    std::println("[Parent] Terminé");
}
