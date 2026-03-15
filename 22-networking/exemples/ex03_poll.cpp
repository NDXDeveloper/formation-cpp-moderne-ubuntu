/* ============================================================================
   Section 22.3 : Multiplexage I/O — select, poll, epoll
   Description : Serveur echo multiplexé avec poll(), gestion dynamique
                 du tableau pollfd, détection POLLHUP/POLLERR
   Fichier source : 03-multiplexage-io.md
   ============================================================================ */
#include "net_utils.hpp"
#include <csignal>
#include <poll.h>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

std::atomic<bool> server_ready{false};

void run_poll_server() {
    signal(SIGPIPE, SIG_IGN);
    auto addr = resolve(nullptr, "18085", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);
    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");
    std::println("Serveur echo (poll) sur le port 18085");
    server_ready = true;

    std::vector<pollfd> poll_fds;
    poll_fds.push_back({server.fd(), POLLIN, 0});

    int iterations = 0;
    while (iterations < 10) {
        int ready = poll(poll_fds.data(), static_cast<nfds_t>(poll_fds.size()), 2000);
        if (ready == -1) { if (errno == EINTR) continue; break; }
        if (ready == 0) break;

        if (poll_fds[0].revents & POLLIN) {
            int client_fd = accept4(server.fd(), nullptr, nullptr, SOCK_CLOEXEC);
            if (client_fd != -1) {
                std::println("Poll: nouveau client fd={}", client_fd);
                poll_fds.push_back({client_fd, POLLIN, 0});
            }
        }

        for (size_t i = 1; i < poll_fds.size(); ) {
            if (poll_fds[i].revents == 0) { ++i; continue; }
            int fd = poll_fds[i].fd;
            if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                std::println("Poll: client fd={} erreur/déconnexion", fd);
                close(fd);
                poll_fds.erase(poll_fds.begin() + static_cast<ptrdiff_t>(i));
                continue;
            }
            if (poll_fds[i].revents & POLLIN) {
                char buffer[4096];
                ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
                if (n <= 0) {
                    std::println("Poll: client fd={} déconnecté", fd);
                    close(fd);
                    poll_fds.erase(poll_fds.begin() + static_cast<ptrdiff_t>(i));
                    continue;
                }
                send(fd, buffer, static_cast<size_t>(n), MSG_NOSIGNAL);
            }
            ++i;
        }
        iterations++;
    }
}

void run_client() {
    while (!server_ready) std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto conn = connect_to("localhost", "18085");
    const char* msg = "Hello poll!";
    send_all(conn.fd(), msg, strlen(msg));
    char buf[4096];
    ssize_t n = recv(conn.fd(), buf, sizeof(buf), 0);
    if (n > 0) std::println("Client: echo={}", std::string_view(buf, n));
}

int main() {
    std::jthread server(run_poll_server);
    run_client();
    std::println("Test poll OK !");
}
