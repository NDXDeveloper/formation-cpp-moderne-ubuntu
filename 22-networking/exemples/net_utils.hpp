/* ============================================================================
   Section 22.1 : Sockets TCP/UDP — API POSIX
   Description : Header partagé RAII — Socket, AddrInfoPtr, resolve(),
                 send_all(), recv_exact(), format_address(), connect_to()
   Fichier source : 01.1-creation-sockets.md, 01.2-operations-sockets.md,
                    01.3-envoi-reception.md
   ============================================================================ */
#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#include <cerrno>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <print>
#include <utility>

// --- RAII Socket (section 22.1.1) ---
class Socket {
public:
    Socket(int domain, int type, int protocol = 0)
        : fd_{socket(domain, type | SOCK_CLOEXEC, protocol)} {
        if (fd_ == -1)
            throw std::system_error(errno, std::system_category(), "socket()");
    }
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
    void enable_tcp_nodelay() { set_option(IPPROTO_TCP, TCP_NODELAY, 1); }

private:
    int fd_ = -1;
};

// --- RAII addrinfo (section 22.1.1) ---
struct AddrInfoDeleter {
    void operator()(addrinfo* p) const noexcept { if (p) freeaddrinfo(p); }
};
using AddrInfoPtr = std::unique_ptr<addrinfo, AddrInfoDeleter>;

inline AddrInfoPtr resolve(const char* host, const char* service,
                           int family = AF_UNSPEC, int socktype = SOCK_STREAM) {
    addrinfo hints{};
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_flags = AI_PASSIVE;
    addrinfo* result = nullptr;
    if (int s = getaddrinfo(host, service, &hints, &result); s != 0)
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(s));
    return AddrInfoPtr{result};
}

// --- send_all (section 22.1.3) ---
inline void send_all(int sockfd, const void* buf, size_t len) {
    auto* ptr = static_cast<const std::byte*>(buf);
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t sent = send(sockfd, ptr, remaining, MSG_NOSIGNAL);
        if (sent == -1) {
            if (errno == EINTR) continue;
            throw std::system_error(errno, std::system_category(), "send()");
        }
        ptr += sent;
        remaining -= static_cast<size_t>(sent);
    }
}

// --- recv_exact (section 22.1.3) ---
inline bool recv_exact(int sockfd, void* buf, size_t len) {
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

// --- Affichage d'adresse (section 22.1.2) ---
inline std::string format_address(const sockaddr_storage& addr) {
    char ip[INET6_ADDRSTRLEN];
    uint16_t port;
    if (addr.ss_family == AF_INET) {
        auto* v4 = reinterpret_cast<const sockaddr_in*>(&addr);
        inet_ntop(AF_INET, &v4->sin_addr, ip, sizeof(ip));
        port = ntohs(v4->sin_port);
    } else {
        auto* v6 = reinterpret_cast<const sockaddr_in6*>(&addr);
        inet_ntop(AF_INET6, &v6->sin6_addr, ip, sizeof(ip));
        port = ntohs(v6->sin6_port);
    }
    return std::string(ip) + ":" + std::to_string(port);
}

// --- connect_to helper ---
inline Socket connect_to(const char* host, const char* port) {
    auto addr = resolve(host, port, AF_UNSPEC, SOCK_STREAM);
    for (auto* rp = addr.get(); rp != nullptr; rp = rp->ai_next) {
        int fd = socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC, rp->ai_protocol);
        if (fd == -1) continue;
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) return Socket{fd};
        close(fd);
    }
    throw std::runtime_error("Impossible de se connecter à " + std::string(host) + ":" + port);
}
