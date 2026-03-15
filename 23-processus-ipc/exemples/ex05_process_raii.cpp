/* ============================================================================
   Section 23.1 : Wrapper RAII — la classe Process
   Description : Classe RAII encapsulant posix_spawn avec wait, timeout, signaux
   Fichier source : 01-fork-exec.md
   ============================================================================ */
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>
#include <optional>
#include <system_error>
#include <print>
#include <chrono>
#include <thread>

extern char** environ;

class Process {
public:
    struct Result {
        int exit_code;
        bool signaled;
        int signal_num;
    };

    // Lancer un processus
    Process(const std::string& program, std::vector<std::string> args) {
        // Construire le tableau argv C
        std::vector<char*> c_args;
        c_args.push_back(const_cast<char*>(program.c_str()));
        for (auto& a : args) {
            c_args.push_back(const_cast<char*>(a.c_str()));
        }
        c_args.push_back(nullptr);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);

        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);

        int err = posix_spawnp(&pid_, program.c_str(),
                               &actions, &attr,
                               c_args.data(), environ);

        posix_spawnattr_destroy(&attr);
        posix_spawn_file_actions_destroy(&actions);

        if (err != 0) {
            throw std::system_error(err, std::system_category(),
                                    "posix_spawnp(" + program + ")");
        }
    }

    // Non copiable
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    // Déplaçable
    Process(Process&& other) noexcept : pid_{other.pid_}, waited_{other.waited_} {
        other.pid_ = -1;
        other.waited_ = true;
    }

    Process& operator=(Process&& other) noexcept {
        if (this != &other) {
            try_wait();  // Récolter l'ancien processus si nécessaire
            pid_ = other.pid_;
            waited_ = other.waited_;
            other.pid_ = -1;
            other.waited_ = true;
        }
        return *this;
    }

    // Destructeur — récolte l'enfant pour éviter les zombies
    ~Process() {
        try_wait();
    }

    // PID de l'enfant
    [[nodiscard]] pid_t pid() const noexcept { return pid_; }

    // Attendre la fin (bloquant)
    Result wait() {
        if (waited_) {
            throw std::logic_error("Process already waited");
        }

        int status;
        if (waitpid(pid_, &status, 0) == -1) {
            throw std::system_error(errno, std::system_category(), "waitpid()");
        }

        waited_ = true;
        return decode_status(status);
    }

    // Vérifier si le processus a terminé (non-bloquant)
    std::optional<Result> try_get_result() {
        if (waited_) return std::nullopt;

        int status;
        pid_t result = waitpid(pid_, &status, WNOHANG);

        if (result == 0) return std::nullopt;  // Encore en cours

        if (result == -1) {
            if (errno == ECHILD) {
                waited_ = true;
                return std::nullopt;
            }
            throw std::system_error(errno, std::system_category(), "waitpid()");
        }

        waited_ = true;
        return decode_status(status);
    }

    // Attendre avec timeout
    std::optional<Result> wait_for(std::chrono::milliseconds timeout) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        constexpr auto poll_interval = std::chrono::milliseconds(10);

        while (std::chrono::steady_clock::now() < deadline) {
            if (auto r = try_get_result()) {
                return r;
            }
            std::this_thread::sleep_for(poll_interval);
        }

        return std::nullopt;  // Timeout
    }

    // Envoyer un signal
    void send_signal(int sig) {
        if (!waited_ && pid_ > 0) {
            if (kill(pid_, sig) == -1 && errno != ESRCH) {
                throw std::system_error(errno, std::system_category(), "kill()");
            }
        }
    }

    // Demander un arrêt propre
    void terminate() { send_signal(SIGTERM); }

    // Forcer l'arrêt
    void force_kill() { send_signal(SIGKILL); }

private:
    void try_wait() {
        if (!waited_ && pid_ > 0) {
            int status;
            pid_t r = waitpid(pid_, &status, WNOHANG);
            if (r == 0) {
                kill(pid_, SIGTERM);
                waitpid(pid_, &status, 0);
            }
            waited_ = true;
        }
    }

    static Result decode_status(int status) {
        if (WIFEXITED(status)) {
            return {WEXITSTATUS(status), false, 0};
        }
        if (WIFSIGNALED(status)) {
            return {-1, true, WTERMSIG(status)};
        }
        return {-1, false, 0};
    }

    pid_t pid_ = -1;
    bool waited_ = false;
};

int main() {
    // Lancer une commande simple
    Process echo("echo", {"Hello", "from", "Process", "RAII"});
    std::println("Commande lancée (PID {})", echo.pid());

    auto result = echo.wait();
    if (result.exit_code == 0) {
        std::println("Exécution réussie");
    } else {
        std::println("Exécution échouée (code {})", result.exit_code);
    }

    // Lancer avec timeout
    Process slow("sleep", {"30"});
    std::println("\nSleep lancé (PID {}), attente max 2s...", slow.pid());
    auto r = slow.wait_for(std::chrono::seconds(2));

    if (!r) {
        std::println("Timeout — envoi de SIGTERM");
        slow.terminate();
        auto final_result = slow.wait();
        std::println("Processus terminé (signal={})", final_result.signaled);
    }
}
