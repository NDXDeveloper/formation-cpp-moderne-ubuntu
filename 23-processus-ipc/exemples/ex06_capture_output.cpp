/* ============================================================================
   Section 23.1 : Capturer la sortie d'un processus
   Description : Pipe + posix_spawn pour capturer stdout dans une string
   Fichier source : 01-fork-exec.md
   ============================================================================ */
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#include <fcntl.h>
#include <array>
#include <string>
#include <vector>
#include <print>
#include <algorithm>

extern char** environ;

struct CommandResult {
    std::string output;
    int exit_code;
};

CommandResult run_and_capture(const std::string& program,
                               std::vector<std::string> args) {
    // Créer un pipe : pipe_fds[0] = lecture, pipe_fds[1] = écriture
    int pipe_fds[2];
    if (pipe2(pipe_fds, O_CLOEXEC) == -1) {
        throw std::system_error(errno, std::system_category(), "pipe2()");
    }

    // Configurer posix_spawn pour rediriger stdout vers le pipe
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipe_fds[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, pipe_fds[0]);
    posix_spawn_file_actions_addclose(&actions, pipe_fds[1]);

    // Construire argv
    std::vector<char*> c_args;
    c_args.push_back(const_cast<char*>(program.c_str()));
    for (auto& a : args) c_args.push_back(const_cast<char*>(a.c_str()));
    c_args.push_back(nullptr);

    pid_t pid;
    int err = posix_spawnp(&pid, program.c_str(),
                           &actions, nullptr, c_args.data(), environ);
    posix_spawn_file_actions_destroy(&actions);

    // Fermer le côté écriture du pipe dans le parent
    close(pipe_fds[1]);

    if (err != 0) {
        close(pipe_fds[0]);
        throw std::system_error(err, std::system_category(), "posix_spawnp");
    }

    // Lire toute la sortie de l'enfant
    std::string output;
    std::array<char, 4096> buffer;
    ssize_t n;
    while ((n = read(pipe_fds[0], buffer.data(), buffer.size())) > 0) {
        output.append(buffer.data(), static_cast<size_t>(n));
    }
    close(pipe_fds[0]);

    // Attendre la fin du processus
    int status;
    waitpid(pid, &status, 0);

    int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return {std::move(output), code};
}

int main() {
    auto [output, code] = run_and_capture("uname", {"-a"});

    if (code == 0) {
        std::println("Système : {}", output);
    }

    auto [files, rc] = run_and_capture("ls", {"-1", "/tmp"});
    std::println("Fichiers dans /tmp ({} lignes):",
                 std::count(files.begin(), files.end(), '\n'));
    std::print("{}", files);
}
