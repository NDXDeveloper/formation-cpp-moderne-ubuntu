/* ============================================================================
   Section 20.2 : Installation de handlers (signal, sigaction)
   Description : ScopedSignalBlock RAII — masquage temporaire de signaux
   Fichier source : 02-handlers.md
   ============================================================================ */
#include <signal.h>
#include <initializer_list>
#include <print>

class ScopedSignalBlock {
public:
    explicit ScopedSignalBlock(std::initializer_list<int> signals) {
        sigset_t block_set;
        sigemptyset(&block_set);
        for (int sig : signals) {
            sigaddset(&block_set, sig);
        }
        sigprocmask(SIG_BLOCK, &block_set, &old_set_);
    }

    ~ScopedSignalBlock() {
        sigprocmask(SIG_SETMASK, &old_set_, nullptr);
    }

    ScopedSignalBlock(const ScopedSignalBlock&) = delete;
    ScopedSignalBlock& operator=(const ScopedSignalBlock&) = delete;

private:
    sigset_t old_set_;
};

// Utilisation
void safe_update() {
    ScopedSignalBlock guard({SIGTERM, SIGINT});
    // Aucun signal ne sera délivré dans cette portée
    std::println("  Dans la zone protégée par ScopedSignalBlock");
}   // Signaux débloqués et délivrés ici

int main() {
    safe_update();
    std::println("ScopedSignalBlock OK");
}
