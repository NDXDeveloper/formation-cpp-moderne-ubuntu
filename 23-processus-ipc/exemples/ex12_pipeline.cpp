/* ============================================================================
   Section 23.2 : Chaîne de pipes (pipeline)
   Description : Reproduction de cat /etc/passwd | grep root | wc -l en C++
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

private:
    int fds_[2] = {-1, -1};
};

void exec_in_pipeline(Pipe* input, Pipe* output,
                       const char* program, char* const argv[]) {
    pid_t pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork()");
    }

    if (pid == 0) {
        // Rediriger stdin si un pipe d'entrée est fourni
        if (input) {
            input->close_write();
            dup2(input->read_fd(), STDIN_FILENO);
            input->close_read();
        }

        // Rediriger stdout si un pipe de sortie est fourni
        if (output) {
            output->close_read();
            dup2(output->write_fd(), STDOUT_FILENO);
            output->close_write();
        }

        execvp(program, argv);
        _exit(127);
    }
}

int main() {
    std::println("Pipeline: cat /etc/passwd | grep root | wc -l");
    std::fflush(nullptr);

    // cat /etc/passwd | grep root | wc -l
    Pipe pipe1;  // cat → grep
    Pipe pipe2;  // grep → wc

    // Lancer cat (pas de stdin redirigé, stdout → pipe1)
    char* cat_args[] = {
        const_cast<char*>("cat"),
        const_cast<char*>("/etc/passwd"),
        nullptr
    };
    exec_in_pipeline(nullptr, &pipe1, "cat", cat_args);

    // Lancer grep (stdin ← pipe1, stdout → pipe2)
    char* grep_args[] = {
        const_cast<char*>("grep"),
        const_cast<char*>("root"),
        nullptr
    };
    exec_in_pipeline(&pipe1, &pipe2, "grep", grep_args);

    // Lancer wc (stdin ← pipe2, pas de stdout redirigé)
    char* wc_args[] = {
        const_cast<char*>("wc"),
        const_cast<char*>("-l"),
        nullptr
    };
    exec_in_pipeline(&pipe2, nullptr, "wc", wc_args);

    // CRUCIAL : fermer tous les bouts de pipe dans le parent
    pipe1.close_read();
    pipe1.close_write();
    pipe2.close_read();
    pipe2.close_write();

    // Attendre tous les enfants
    while (wait(nullptr) > 0) {}

    std::println("Pipeline terminé");
}
