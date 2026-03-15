/* ============================================================================
   Section 23.3 : Wrapper RAII — SharedMemory
   Description : Classe RAII pour shm_open/mmap avec test atomiques inter-processus
   Fichier source : 03-shared-memory.md
   ============================================================================ */
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <system_error>
#include <atomic>
#include <print>
#include <cstdio>
#include <new>

class SharedMemory {
public:
    static SharedMemory create(const std::string& name, size_t size) {
        int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
        if (fd == -1) {
            throw std::system_error(errno, std::system_category(), "shm_open(create)");
        }

        if (ftruncate(fd, static_cast<off_t>(size)) == -1) {
            close(fd);
            shm_unlink(name.c_str());
            throw std::system_error(errno, std::system_category(), "ftruncate()");
        }

        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);

        if (ptr == MAP_FAILED) {
            shm_unlink(name.c_str());
            throw std::system_error(errno, std::system_category(), "mmap()");
        }

        std::memset(ptr, 0, size);
        return SharedMemory(name, ptr, size, true);
    }

    static SharedMemory open(const std::string& name, size_t size, bool read_only = false) {
        int oflags = read_only ? O_RDONLY : O_RDWR;
        int fd = shm_open(name.c_str(), oflags, 0);
        if (fd == -1) {
            throw std::system_error(errno, std::system_category(), "shm_open(open)");
        }

        int prot = PROT_READ | (read_only ? 0 : PROT_WRITE);
        void* ptr = mmap(nullptr, size, prot, MAP_SHARED, fd, 0);
        close(fd);

        if (ptr == MAP_FAILED) {
            throw std::system_error(errno, std::system_category(), "mmap()");
        }

        return SharedMemory(name, ptr, size, false);
    }

    ~SharedMemory() {
        if (ptr_ && ptr_ != MAP_FAILED) {
            munmap(ptr_, size_);
        }
        if (owner_) {
            shm_unlink(name_.c_str());
        }
    }

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    SharedMemory(SharedMemory&& other) noexcept
        : name_{std::move(other.name_)},
          ptr_{other.ptr_}, size_{other.size_}, owner_{other.owner_} {
        other.ptr_ = nullptr;
        other.owner_ = false;
    }

    SharedMemory& operator=(SharedMemory&& other) noexcept {
        if (this != &other) {
            if (ptr_ && ptr_ != MAP_FAILED) munmap(ptr_, size_);
            if (owner_) shm_unlink(name_.c_str());

            name_ = std::move(other.name_);
            ptr_ = other.ptr_;
            size_ = other.size_;
            owner_ = other.owner_;
            other.ptr_ = nullptr;
            other.owner_ = false;
        }
        return *this;
    }

    template<typename T>
    T* as() noexcept { return static_cast<T*>(ptr_); }

    template<typename T>
    const T* as() const noexcept { return static_cast<const T*>(ptr_); }

    void* data() noexcept { return ptr_; }
    size_t size() const noexcept { return size_; }

private:
    SharedMemory(std::string name, void* ptr, size_t size, bool owner)
        : name_{std::move(name)}, ptr_{ptr}, size_{size}, owner_{owner} {}

    std::string name_;
    void* ptr_ = nullptr;
    size_t size_ = 0;
    bool owner_ = false;
};

// Structure de métriques partagées
struct Metrics {
    std::atomic<int64_t> request_count;
    std::atomic<int64_t> error_count;
    std::atomic<double> avg_latency_ms;
};

static_assert(std::atomic<int64_t>::is_always_lock_free,
              "int64_t atomics must be lock-free for shared memory IPC");

int main() {
    const char* name = "/app_metrics_ex18";

    // Nettoyer d'éventuels résidus
    shm_unlink(name);

    // Créer le segment
    auto shm = SharedMemory::create(name, sizeof(Metrics));
    auto* metrics = shm.as<Metrics>();
    new (metrics) Metrics{};  // Placement new pour initialiser les atomiques

    std::println("=== SharedMemory RAII Test ===");
    std::fflush(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        // Enfant : simuler un lecteur de monitoring
        sleep(1);  // Laisser le temps au parent d'écrire
        auto reader = SharedMemory::open(name, sizeof(Metrics), true);
        auto* m = reader.as<const Metrics>();

        std::println("[Monitoring]");
        std::println("  Requests: {}", m->request_count.load(std::memory_order_relaxed));
        std::println("  Errors:   {}", m->error_count.load(std::memory_order_relaxed));
        std::println("  Latency:  {:.1f}ms", m->avg_latency_ms.load(std::memory_order_relaxed));
        std::fflush(stdout);
        _exit(0);
    }

    // Parent : simuler un serveur qui écrit des métriques
    for (int i = 0; i < 100; ++i) {
        metrics->request_count.fetch_add(1, std::memory_order_relaxed);
        if (i % 10 == 0) {
            metrics->error_count.fetch_add(1, std::memory_order_relaxed);
        }
    }
    metrics->avg_latency_ms.store(12.5, std::memory_order_relaxed);

    std::println("[Serveur] 100 requêtes traitées");

    waitpid(pid, nullptr, 0);
    std::println("=== Test terminé ===");
}
