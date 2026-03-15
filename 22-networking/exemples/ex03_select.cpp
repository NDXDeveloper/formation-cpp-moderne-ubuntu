/* ============================================================================
   Section 22.3 : Multiplexage I/O — select, poll, epoll
   Description : Serveur echo multiplexé avec select(), gestion de
                 connexions multiples sans threads
   Fichier source : 03-multiplexage-io.md
   ============================================================================ */
#include "net_utils.hpp"
#include <csignal>
#include <sys/select.h>
#include <algorithm>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

std::atomic<bool> server_ready{false};

void run_select_server() {
    signal(SIGPIPE, SIG_IGN);
    auto addr = resolve(nullptr, "18084", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);
    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");
    std::println("Serveur echo (select) sur le port 18084");
    server_ready = true;

    std::vector<int> client_fds;
    int iterations = 0;

    while (iterations < 10) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server.fd(), &read_fds);
        int max_fd = server.fd();
        for (int fd : client_fds) {
            FD_SET(fd, &read_fds);
            max_fd = std::max(max_fd, fd);
        }
        timeval timeout{}; timeout.tv_sec = 2;
        int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (ready == -1) { if (errno == EINTR) continue; break; }
        if (ready == 0) break;

        if (FD_ISSET(server.fd(), &read_fds)) {
            int client_fd = accept4(server.fd(), nullptr, nullptr, SOCK_CLOEXEC);
            if (client_fd != -1) {
                std::println("Select: nouveau client fd={}", client_fd);
                client_fds.push_back(client_fd);
            }
        }

        for (auto it = client_fds.begin(); it != client_fds.end(); ) {
            int fd = *it;
            if (!FD_ISSET(fd, &read_fds)) { ++it; continue; }
            char buffer[4096];
            ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                std::println("Select: client fd={} déconnecté", fd);
                close(fd); it = client_fds.erase(it);
            } else {
                send(fd, buffer, static_cast<size_t>(n), MSG_NOSIGNAL);
                ++it;
            }
        }
        iterations++;
    }
    for (int fd : client_fds) close(fd);
}

void run_client() {
    while (!server_ready) std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto conn = connect_to("localhost", "18084");
    const char* msg = "Hello select!";
    send_all(conn.fd(), msg, strlen(msg));
    char buf[4096];
    ssize_t n = recv(conn.fd(), buf, sizeof(buf), 0);
    if (n > 0) std::println("Client: echo={}", std::string_view(buf, n));
}

int main() {
    std::jthread server(run_select_server);
    run_client();
    std::println("Test select OK !");
}
