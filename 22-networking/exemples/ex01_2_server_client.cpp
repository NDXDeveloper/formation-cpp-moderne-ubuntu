/* ============================================================================
   Section 22.1.2 : bind, listen, accept, connect
   Description : Serveur/client TCP avec accept4, log_client_info,
                 résolution DNS et connexion multi-adresses
   Fichier source : 01.2-operations-sockets.md
   ============================================================================ */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <utility>
#include <cerrno>
#include <cstring>
#include <system_error>
#include <memory>
#include <print>
#include <thread>
#include <chrono>

// === Socket wrapper (from 01.1) ===
class Socket {
public:
    Socket(int domain, int type, int protocol = 0)
        : fd_{socket(domain, type | SOCK_CLOEXEC, protocol)}
    {
        if (fd_ == -1)
            throw std::system_error(errno, std::system_category(), "socket()");
    }
    explicit Socket(int fd) noexcept : fd_{fd} {}
    ~Socket() { if (fd_ != -1) close(fd_); }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept : fd_{std::exchange(other.fd_, -1)} {}
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) { if (fd_ != -1) close(fd_); fd_ = std::exchange(other.fd_, -1); }
        return *this;
    }
    [[nodiscard]] int fd() const noexcept { return fd_; }
    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] bool valid() const noexcept { return fd_ != -1; }
    int release() noexcept { return std::exchange(fd_, -1); }
    void set_option(int level, int optname, int value) {
        if (setsockopt(fd_, level, optname, &value, sizeof(value)) == -1)
            throw std::system_error(errno, std::system_category(), "setsockopt()");
    }
    void enable_reuse_addr() { set_option(SOL_SOCKET, SO_REUSEADDR, 1); }
    void enable_tcp_nodelay() { set_option(IPPROTO_TCP, TCP_NODELAY, 1); }
private:
    int fd_ = -1;
};

// === AddrInfo RAII (from 01.1) ===
struct AddrInfoDeleter {
    void operator()(addrinfo* p) const noexcept { if (p) freeaddrinfo(p); }
};
using AddrInfoPtr = std::unique_ptr<addrinfo, AddrInfoDeleter>;

AddrInfoPtr resolve(const char* host, const char* service,
                    int family = AF_UNSPEC, int socktype = SOCK_STREAM) {
    addrinfo hints{};
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_flags = AI_PASSIVE;
    addrinfo* result = nullptr;
    int status = getaddrinfo(host, service, &hints, &result);
    if (status != 0)
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(status));
    return AddrInfoPtr{result};
}

// === log_client_info (from 01.2) ===
void log_client_info(const sockaddr_storage& addr) {
    char ip_str[INET6_ADDRSTRLEN];
    uint16_t port;

    if (addr.ss_family == AF_INET) {
        auto* v4 = reinterpret_cast<const sockaddr_in*>(&addr);
        inet_ntop(AF_INET, &v4->sin_addr, ip_str, sizeof(ip_str));
        port = ntohs(v4->sin_port);
    } else {
        auto* v6 = reinterpret_cast<const sockaddr_in6*>(&addr);
        inet_ntop(AF_INET6, &v6->sin6_addr, ip_str, sizeof(ip_str));
        port = ntohs(v6->sin6_port);
    }

    std::println("Connexion depuis {}:{}", ip_str, port);
}

// === Server (from 01.2) ===
void handle_client(Socket client) {
    char buffer[4096];
    ssize_t n = recv(client.fd(), buffer, sizeof(buffer), 0);
    if (n > 0) {
        send(client.fd(), buffer, n, 0);  // Echo
    }
}

void run_server() {
    auto addr = resolve(nullptr, "18080", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();

    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");

    std::println("Serveur en écoute sur le port 18080...");

    sockaddr_storage client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept4(server.fd(),
        reinterpret_cast<sockaddr*>(&client_addr), &addr_len, SOCK_CLOEXEC);
    if (client_fd == -1)
        throw std::system_error(errno, std::system_category(), "accept4()");

    log_client_info(client_addr);
    handle_client(Socket{client_fd});
    std::println("Serveur: client traité");
}

// === Client (from 01.2) ===
void run_client() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto addr = resolve("localhost", "18080", AF_UNSPEC, SOCK_STREAM);

    int sockfd = -1;
    for (auto* rp = addr.get(); rp != nullptr; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC, rp->ai_protocol);
        if (sockfd == -1) continue;
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sockfd);
        sockfd = -1;
    }

    if (sockfd == -1) {
        std::println(stderr, "Impossible de se connecter");
        return;
    }

    Socket connection{sockfd};

    const char* msg = "Hello, server!";
    send(connection.fd(), msg, strlen(msg), 0);

    char buffer[4096];
    ssize_t n = recv(connection.fd(), buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        std::println("Réponse du serveur : {}", buffer);
    }
}

int main() {
    std::jthread server_thread([] { run_server(); });
    run_client();
    std::println("Test serveur/client terminé !");
}
