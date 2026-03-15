/* ============================================================================
   Section 23.3 : Synchronisation inter-processus — pthread_mutex
   Description : pthread_mutex avec PTHREAD_PROCESS_SHARED et PTHREAD_MUTEX_ROBUST
   Fichier source : 03-shared-memory.md
   ============================================================================ */
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <print>

struct SharedState {
    pthread_mutex_t mutex;
    int counter;
    char data[256];
};

// Initialisation (un seul processus, typiquement le créateur du segment)
void init_shared_state(SharedState* state) {
    // Configurer le mutex pour l'usage inter-processus
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    // Robustesse : le mutex se déverrouille si le propriétaire meurt
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

    pthread_mutex_init(&state->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    state->counter = 0;
    std::memset(state->data, 0, sizeof(state->data));
}

// Utilisation (n'importe quel processus ayant mappé le segment)
void increment(SharedState* state) {
    int err = pthread_mutex_lock(&state->mutex);

    if (err == EOWNERDEAD) {
        // Le processus qui détenait le mutex est mort
        pthread_mutex_consistent(&state->mutex);
    }

    state->counter++;
    std::snprintf(state->data, sizeof(state->data),
                  "Counter: %d (PID %d)", state->counter, getpid());

    pthread_mutex_unlock(&state->mutex);
}

int main() {
    size_t size = sizeof(SharedState);
    void* shared = mmap(nullptr, size,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS,
                        -1, 0);
    if (shared == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "mmap()");
    }

    auto* state = static_cast<SharedState*>(shared);
    init_shared_state(state);

    std::fflush(nullptr);
    pid_t pid = fork();
    if (pid == 0) {
        for (int i = 0; i < 1000; ++i) {
            increment(state);
        }
        _exit(0);
    }

    for (int i = 0; i < 1000; ++i) {
        increment(state);
    }

    waitpid(pid, nullptr, 0);
    std::println("Final counter: {} (expected 2000)", state->counter);
    std::println("Last data: {}", state->data);

    pthread_mutex_destroy(&state->mutex);
    munmap(shared, size);
}
