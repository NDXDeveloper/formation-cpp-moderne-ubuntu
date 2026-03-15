/* ============================================================================
   Section 23.2 : Alimenter stdin d'un processus enfant
   Description : Envoi de données sur stdin via Pipe RAII + dup2
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

    ssize_t write_data(const void* buf, size_t count) {
        ssize_t n;
        do { n = write(fds_[1], buf, count); } while (n == -1 && errno == EINTR);
        return n;
    }

private:
    int fds_[2] = {-1, -1};
};

int main() {
    Pipe stdin_pipe;

    std::fflush(nullptr);
    pid_t pid = fork();
    if (pid == 0) {
        // Enfant : remplacer stdin par le côté lecture du pipe
        stdin_pipe.close_write();
        dup2(stdin_pipe.read_fd(), STDIN_FILENO);
        stdin_pipe.close_read();

        // wc lira depuis le pipe (comme s'il lisait depuis le terminal)
        execlp("wc", "wc", "-l", nullptr);
        _exit(127);
    }

    // Parent : écrire dans le stdin de l'enfant
    stdin_pipe.close_read();
    std::string data = "ligne 1\nligne 2\nligne 3\nligne 4\n";
    stdin_pipe.write_data(data.data(), data.size());
    stdin_pipe.close_write();  // EOF → wc peut terminer son comptage

    int status;
    waitpid(pid, &status, 0);
    // wc affiche "4" sur le terminal (son stdout n'est pas redirigé)
    std::println("wc terminé avec le code {}", WEXITSTATUS(status));
}
