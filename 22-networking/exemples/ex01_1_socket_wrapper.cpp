/* ============================================================================
   Section 22.1.1 : Création de sockets
   Description : Wrapper RAII Socket, AddrInfoPtr, resolve(), setsockopt,
                 déplacement (move semantics), getsockopt
   Fichier source : 01.1-creation-sockets.md
   ============================================================================ */
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <cerrno>
#include <system_error>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <memory>
#include <print>

class Socket {
public:
    Socket(int domain, int type, int protocol = 0)
        : fd_{socket(domain, type | SOCK_CLOEXEC, protocol)}
    {
        if (fd_ == -1) {
            throw std::system_error(errno, std::system_category(), "socket()");
        }
    }

    explicit Socket(int fd) noexcept : fd_{fd} {}

    ~Socket() {
        if (fd_ != -1) {
            close(fd_);
        }
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept
        : fd_{std::exchange(other.fd_, -1)} {}

    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            if (fd_ != -1) {
                close(fd_);
            }
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    [[nodiscard]] int fd() const noexcept { return fd_; }
    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] bool valid() const noexcept { return fd_ != -1; }
    int release() noexcept { return std::exchange(fd_, -1); }

    void set_option(int level, int optname, int value) {
        if (setsockopt(fd_, level, optname, &value, sizeof(value)) == -1) {
            throw std::system_error(errno, std::system_category(), "setsockopt()");
        }
    }

    void enable_reuse_addr() {
        set_option(SOL_SOCKET, SO_REUSEADDR, 1);
    }

    void enable_tcp_nodelay() {
        set_option(IPPROTO_TCP, TCP_NODELAY, 1);
    }

private:
    int fd_ = -1;
};

struct AddrInfoDeleter {
    void operator()(addrinfo* p) const noexcept {
        if (p) freeaddrinfo(p);
    }
};

using AddrInfoPtr = std::unique_ptr<addrinfo, AddrInfoDeleter>;

AddrInfoPtr resolve(const char* host, const char* service,
                    int family = AF_UNSPEC, int socktype = SOCK_STREAM)
{
    addrinfo hints{};
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result = nullptr;
    int status = getaddrinfo(host, service, &hints, &result);
    if (status != 0) {
        throw std::runtime_error(
            std::string("getaddrinfo: ") + gai_strerror(status)
        );
    }

    return AddrInfoPtr{result};
}

int main() {
    // Test 1 : Créer un socket TCP IPv4
    {
        Socket tcp4{AF_INET, SOCK_STREAM};
        std::println("TCP IPv4 créé : fd={}", tcp4.fd());
    }
    std::println("TCP IPv4 fermé automatiquement (RAII)");

    // Test 2 : Socket TCP IPv6
    {
        Socket tcp6{AF_INET6, SOCK_STREAM};
        tcp6.enable_reuse_addr();
        tcp6.enable_tcp_nodelay();
        std::println("TCP IPv6 créé avec options : fd={}", tcp6.fd());
    }

    // Test 3 : Socket UDP
    {
        Socket udp4{AF_INET, SOCK_DGRAM};
        std::println("UDP IPv4 créé : fd={}", udp4.fd());
    }

    // Test 4 : Socket non-bloquant
    {
        Socket async_sock{AF_INET6, SOCK_STREAM | SOCK_NONBLOCK};
        std::println("TCP IPv6 non-bloquant créé : fd={}", async_sock.fd());
    }

    // Test 5 : Déplacement
    {
        Socket s1{AF_INET, SOCK_STREAM};
        int fd1 = s1.fd();
        Socket s2 = std::move(s1);
        std::println("Déplacement : ancien fd={}, nouveau fd={}, ancien valid={}",
                     fd1, s2.fd(), s1.valid());
    }

    // Test 6 : resolve()
    {
        auto addr = resolve(nullptr, "8080", AF_INET6);
        Socket sock{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
        std::println("Socket résolu créé : fd={}, family={}", sock.fd(), addr->ai_family);
    }

    // Test 7 : getsockopt
    {
        Socket sock{AF_INET6, SOCK_STREAM};
        int opt;
        socklen_t opt_len = sizeof(opt);
        if (getsockopt(sock.fd(), SOL_SOCKET, SO_RCVBUF, &opt, &opt_len) == 0) {
            std::println("Buffer de réception : {} octets", opt);
        }
    }

    std::println("Tous les tests passés !");
}
