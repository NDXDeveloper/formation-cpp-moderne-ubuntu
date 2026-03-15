/* ============================================================================
   Section 23.2 : Capturer stdout d'un processus enfant avec dup2()
   Description : Redirection stdout via Pipe RAII + dup2 + fork/exec
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

    ~Pipe() { close_read(); close_write(); }

    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    [[nodiscard]] int read_fd() const noexcept { return fds_[0]; }
    [[nodiscard]] int write_fd() const noexcept { return fds_[1]; }

    void close_read() { if (fds_[0] != -1) { close(fds_[0]); fds_[0] = -1; } }
    void close_write() { if (fds_[1] != -1) { close(fds_[1]); fds_[1] = -1; } }

    std::string read_all() {
        std::string result;
        char buffer[4096];
        ssize_t n;
        while ((n = read(fds_[0], buffer, sizeof(buffer))) > 0) {
            result.append(buffer, static_cast<size_t>(n));
        }
        return result;
    }

private:
    int fds_[2] = {-1, -1};
};

int main() {
    Pipe stdout_pipe;

    std::fflush(nullptr);
    pid_t pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork()");
    }

    if (pid == 0) {
        // Enfant : remplacer stdout par le côté écriture du pipe
        stdout_pipe.close_read();
        dup2(stdout_pipe.write_fd(), STDOUT_FILENO);
        stdout_pipe.close_write();

        execlp("echo", "echo", "Sortie capturée via dup2!", nullptr);
        _exit(127);
    }

    // Parent : lire la sortie de l'enfant
    stdout_pipe.close_write();
    std::string output = stdout_pipe.read_all();

    int status;
    waitpid(pid, &status, 0);

    std::println("Sortie capturée ({} octets): {}", output.size(), output);
}
