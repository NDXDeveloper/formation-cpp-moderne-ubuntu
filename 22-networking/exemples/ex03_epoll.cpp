/* ============================================================================
   Section 22.3 : Multiplexage I/O — select, poll, epoll
   Description : Serveur echo avec epoll (RAII Epoll wrapper), edge-triggered,
                 intégration timerfd, sockets non-bloquants
   Fichier source : 03-multiplexage-io.md
   ============================================================================ */
#include "net_utils.hpp"
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>
#include <map>

// set_nonblocking (from 03)
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) throw std::system_error(errno, std::system_category(), "fcntl(F_GETFL)");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::system_error(errno, std::system_category(), "fcntl(F_SETFL)");
}

// Epoll wrapper (from 03)
class Epoll {
public:
    Epoll() : fd_{epoll_create1(EPOLL_CLOEXEC)} {
        if (fd_ == -1) throw std::system_error(errno, std::system_category(), "epoll_create1()");
    }
    ~Epoll() { if (fd_ != -1) close(fd_); }
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;

    void add(int fd, uint32_t events) {
        epoll_event ev{}; ev.events = events; ev.data.fd = fd;
        if (epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev) == -1)
            throw std::system_error(errno, std::system_category(), "epoll_ctl(ADD)");
    }
    void modify(int fd, uint32_t events) {
        epoll_event ev{}; ev.events = events; ev.data.fd = fd;
        if (epoll_ctl(fd_, EPOLL_CTL_MOD, fd, &ev) == -1)
            throw std::system_error(errno, std::system_category(), "epoll_ctl(MOD)");
    }
    void remove(int fd) { epoll_ctl(fd_, EPOLL_CTL_DEL, fd, nullptr); }
    int wait(epoll_event* events, int max_events, int timeout_ms) {
        return epoll_wait(fd_, events, max_events, timeout_ms);
    }
    [[nodiscard]] int fd() const noexcept { return fd_; }
private:
    int fd_;
};

std::atomic<bool> server_ready{false};

void run_epoll_server() {
    signal(SIGPIPE, SIG_IGN);
    auto addr = resolve(nullptr, "18086", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype | SOCK_NONBLOCK, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);
    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");

    Epoll epoll;
    epoll.add(server.fd(), EPOLLIN);

    // Test timerfd integration (from 03)
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    itimerspec spec{};
    spec.it_interval.tv_sec = 1;
    spec.it_value.tv_sec = 1;
    timerfd_settime(tfd, 0, &spec, nullptr);
    epoll.add(tfd, EPOLLIN);

    std::println("Serveur echo (epoll) sur le port 18086");
    server_ready = true;

    constexpr int MAX_EVENTS = 256;
    epoll_event events[MAX_EVENTS];
    int num_clients = 0;
    int timer_count = 0;

    while (timer_count < 3) {
        int n = epoll.wait(events, MAX_EVENTS, 2000);
        if (n == -1) { if (errno == EINTR) continue; break; }
        if (n == 0) break;

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == tfd && (ev & EPOLLIN)) {
                uint64_t expirations;
                [[maybe_unused]] auto r = read(tfd, &expirations, sizeof(expirations));
                timer_count++;
                std::println("Timer #{} expiré", timer_count);
                continue;
            }

            if (fd == server.fd()) {
                while (true) {
                    sockaddr_storage client_addr{};
                    socklen_t addr_len = sizeof(client_addr);
                    int client_fd = accept4(server.fd(),
                        reinterpret_cast<sockaddr*>(&client_addr), &addr_len,
                        SOCK_NONBLOCK | SOCK_CLOEXEC);
                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        break;
                    }
                    epoll.add(client_fd, EPOLLIN | EPOLLRDHUP);
                    num_clients++;
                    auto info = format_address(client_addr);
                    std::println("[{}] Connecté (fd={}, total={})", info, client_fd, num_clients);
                }
                continue;
            }

            if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                epoll.remove(fd); close(fd); num_clients--;
                std::println("fd={} déconnecté (total={})", fd, num_clients);
                continue;
            }

            if (ev & EPOLLIN) {
                char buffer[4096];
                ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
                if (bytes <= 0) {
                    if (bytes == 0 || (bytes == -1 && errno != EAGAIN)) {
                        epoll.remove(fd); close(fd); num_clients--;
                        std::println("fd={} déconnecté (total={})", fd, num_clients);
                    }
                    continue;
                }
                ssize_t sent = send(fd, buffer, static_cast<size_t>(bytes), MSG_NOSIGNAL);
                if (sent == -1 && errno != EAGAIN) {
                    epoll.remove(fd); close(fd); num_clients--;
                }
            }
        }
    }
    close(tfd);
    std::println("Arrêt — {} clients encore connectés", num_clients);
}

void run_client() {
    while (!server_ready) std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto conn = connect_to("localhost", "18086");
    const char* msg = "Hello epoll!";
    send_all(conn.fd(), msg, strlen(msg));
    char buf[4096];
    ssize_t n = recv(conn.fd(), buf, sizeof(buf), 0);
    if (n > 0) std::println("Client: echo={}", std::string_view(buf, n));
}

int main() {
    std::jthread server(run_epoll_server);
    run_client();
    std::println("Test epoll OK !");
}
