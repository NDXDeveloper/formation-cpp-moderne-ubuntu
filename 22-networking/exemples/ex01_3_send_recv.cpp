/* ============================================================================
   Section 22.1.3 : send, recv, sendto, recvfrom
   Description : send_all, recv_exact, framing length-prefixed, SendResult
                 robuste, safe_recv, retry_on_eintr
   Fichier source : 01.3-envoi-reception.md
   ============================================================================ */
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <utility>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <system_error>
#include <memory>
#include <vector>
#include <span>
#include <print>
#include <thread>
#include <chrono>
#include <string>

// === Socket wrapper (from 01.1) ===
class Socket {
public:
    Socket(int domain, int type, int protocol = 0)
        : fd_{socket(domain, type | SOCK_CLOEXEC, protocol)}
    { if (fd_ == -1) throw std::system_error(errno, std::system_category(), "socket()"); }
    explicit Socket(int fd) noexcept : fd_{fd} {}
    ~Socket() { if (fd_ != -1) close(fd_); }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& o) noexcept : fd_{std::exchange(o.fd_, -1)} {}
    Socket& operator=(Socket&& o) noexcept {
        if (this != &o) { if (fd_ != -1) close(fd_); fd_ = std::exchange(o.fd_, -1); }
        return *this;
    }
    [[nodiscard]] int fd() const noexcept { return fd_; }
    void set_option(int level, int optname, int value) {
        if (setsockopt(fd_, level, optname, &value, sizeof(value)) == -1)
            throw std::system_error(errno, std::system_category(), "setsockopt()");
    }
    void enable_reuse_addr() { set_option(SOL_SOCKET, SO_REUSEADDR, 1); }
private:
    int fd_ = -1;
};

struct AddrInfoDeleter {
    void operator()(addrinfo* p) const noexcept { if (p) freeaddrinfo(p); }
};
using AddrInfoPtr = std::unique_ptr<addrinfo, AddrInfoDeleter>;
AddrInfoPtr resolve(const char* host, const char* service,
                    int family = AF_UNSPEC, int socktype = SOCK_STREAM) {
    addrinfo hints{}; hints.ai_family = family; hints.ai_socktype = socktype; hints.ai_flags = AI_PASSIVE;
    addrinfo* result = nullptr;
    int status = getaddrinfo(host, service, &hints, &result);
    if (status != 0) throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(status));
    return AddrInfoPtr{result};
}

// === send_all (from 01.3 — void version) ===
void send_all(int sockfd, const void* buf, size_t len) {
    auto* ptr = static_cast<const std::byte*>(buf);
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t sent = send(sockfd, ptr, remaining, 0);
        if (sent == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "send()");
        }
        ptr += sent;
        remaining -= static_cast<size_t>(sent);
    }
}

// === send_all (from 01.3 — SendResult version) ===
enum class SendResult { ok, peer_closed, error };

SendResult send_all_robust(int sockfd, std::span<const std::byte> data) {
    auto* ptr = data.data();
    size_t remaining = data.size();
    while (remaining > 0) {
        ssize_t sent = send(sockfd, ptr, remaining, MSG_NOSIGNAL);
        if (sent == -1) {
            if (errno == EINTR) continue;
            if (errno == EPIPE) return SendResult::peer_closed;
            return SendResult::error;
        }
        ptr += sent;
        remaining -= static_cast<size_t>(sent);
    }
    return SendResult::ok;
}

// === recv_exact (from 01.3) ===
bool recv_exact(int sockfd, void* buf, size_t len) {
    auto* ptr = static_cast<std::byte*>(buf);
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t n = recv(sockfd, ptr, remaining, 0);
        if (n == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "recv()");
        }
        if (n == 0) return false;
        ptr += n;
        remaining -= static_cast<size_t>(n);
    }
    return true;
}

// === Framing (from 01.3) ===
void send_message(int sockfd, std::span<const std::byte> payload) {
    uint32_t len_net = htonl(static_cast<uint32_t>(payload.size()));
    send_all(sockfd, &len_net, sizeof(len_net));
    send_all(sockfd, payload.data(), payload.size());
}

std::vector<std::byte> recv_message(int sockfd) {
    uint32_t len_net = 0;
    if (!recv_exact(sockfd, &len_net, sizeof(len_net))) return {};
    uint32_t len = ntohl(len_net);
    constexpr uint32_t max_message_size = 16 * 1024 * 1024;
    if (len > max_message_size) throw std::runtime_error("Message trop grand : " + std::to_string(len));
    std::vector<std::byte> payload(len);
    if (!recv_exact(sockfd, payload.data(), len))
        throw std::runtime_error("Connexion fermée pendant la lecture du payload");
    return payload;
}

// === safe_recv (from 01.3) ===
ssize_t safe_recv(int sockfd, void* buf, size_t len, int flags) {
    ssize_t n;
    do {
        n = recv(sockfd, buf, len, flags);
    } while (n == -1 && errno == EINTR);
    return n;
}

// === retry_on_eintr (from 01.3) ===
template<typename Func, typename... Args>
auto retry_on_eintr(Func fn, Args&&... args) {
    decltype(fn(std::forward<Args>(args)...)) result;
    do {
        result = fn(std::forward<Args>(args)...);
    } while (result == -1 && errno == EINTR);
    return result;
}

// === Test program ===
void run_server() {
    auto addr = resolve(nullptr, "18081", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");

    sockaddr_storage caddr{}; socklen_t alen = sizeof(caddr);
    int cfd = accept(server.fd(), reinterpret_cast<sockaddr*>(&caddr), &alen);
    if (cfd == -1) throw std::system_error(errno, std::system_category(), "accept()");
    Socket client{cfd};

    // Test 1: recv_exact — read 14 bytes exactly
    char buf[14];
    if (recv_exact(client.fd(), buf, 14)) {
        std::println("Serveur reçu (exact): {}", std::string_view(buf, 14));
    }

    // Test 2: echo via send_all
    send_all(client.fd(), buf, 14);

    // Test 3: recv framed message
    auto msg = recv_message(client.fd());
    std::println("Serveur reçu (framé): {} octets, contenu: {}",
                 msg.size(), std::string_view(reinterpret_cast<char*>(msg.data()), msg.size()));

    // Test 4: send framed response
    std::string response = "Bonjour du serveur!";
    send_message(client.fd(), std::as_bytes(std::span{response}));
}

void run_client() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto addr = resolve("localhost", "18081", AF_UNSPEC, SOCK_STREAM);
    int sockfd = -1;
    for (auto* rp = addr.get(); rp; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC, rp->ai_protocol);
        if (sockfd == -1) continue;
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sockfd); sockfd = -1;
    }
    if (sockfd == -1) { std::println(stderr, "Connexion impossible"); return; }
    Socket conn{sockfd};

    // Test 1: send_all
    const char* msg = "Hello, server!";
    send_all(conn.fd(), msg, 14);

    // Test 2: recv echo
    char buf[14];
    if (recv_exact(conn.fd(), buf, 14)) {
        std::println("Client reçu (echo): {}", std::string_view(buf, 14));
    }

    // Test 3: send framed message
    std::string payload = "Message avec framing!";
    send_message(conn.fd(), std::as_bytes(std::span{payload}));

    // Test 4: recv framed response
    auto resp = recv_message(conn.fd());
    std::println("Client reçu (framé): {}", std::string_view(reinterpret_cast<char*>(resp.data()), resp.size()));

    // Test 5: SendResult robust version
    std::string data2 = "test robust";
    auto result = send_all_robust(conn.fd(), std::as_bytes(std::span{data2}));
    std::println("SendResult: {}", result == SendResult::ok ? "ok" : "error");

    // Test 6: retry_on_eintr
    char rbuf[64];
    ssize_t n = retry_on_eintr(recv, conn.fd(), rbuf, sizeof(rbuf), MSG_DONTWAIT);
    std::println("retry_on_eintr: n={} (EAGAIN expected)", n);
}

int main() {
    std::jthread server_thread([] { run_server(); });
    run_client();
    std::println("Tous les tests send/recv passés !");
}
