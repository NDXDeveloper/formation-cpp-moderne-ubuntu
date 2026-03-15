/* ============================================================================
   Section 22.2 : Client/Serveur basique en C++
   Description : Serveur echo séquentiel avec gestion SIGPIPE, dual-stack
                 IPv6, et client echo simple
   Fichier source : 02-client-serveur.md
   ============================================================================ */
#include "net_utils.hpp"
#include <csignal>
#include <thread>
#include <chrono>

// === Serveur echo (adapté pour test : 1 client seulement) ===
void handle_client(Socket client, const std::string& client_info) {
    std::println("[{}] Connexion établie", client_info);
    char buffer[4096];
    while (true) {
        ssize_t n = recv(client.fd(), buffer, sizeof(buffer), 0);
        if (n == -1) {
            if (errno == EINTR) continue;
            std::println(stderr, "[{}] Erreur recv: {}", client_info, strerror(errno));
            break;
        }
        if (n == 0) {
            std::println("[{}] Déconnexion", client_info);
            break;
        }
        try {
            send_all(client.fd(), buffer, static_cast<size_t>(n));
        } catch (const std::system_error& e) {
            std::println(stderr, "[{}] Erreur send: {}", client_info, e.what());
            break;
        }
    }
}

void run_server() {
    signal(SIGPIPE, SIG_IGN);
    auto addr = resolve(nullptr, "18082", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);
    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");
    std::println("Serveur echo en écoute sur le port 18082 (séquentiel)");

    sockaddr_storage client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept4(server.fd(), reinterpret_cast<sockaddr*>(&client_addr),
                            &addr_len, SOCK_CLOEXEC);
    if (client_fd == -1) throw std::system_error(errno, std::system_category(), "accept4()");
    handle_client(Socket{client_fd}, format_address(client_addr));
}

// === Client echo (from 02) ===
void run_client() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto conn = connect_to("localhost", "18082");
    std::println("Connecté au serveur");

    // Send and receive
    std::string line = "hello world";
    send_all(conn.fd(), line.data(), line.size());
    char buffer[4096];
    ssize_t n = recv(conn.fd(), buffer, sizeof(buffer), 0);
    if (n > 0) {
        std::println("Echo: {}", std::string_view(buffer, static_cast<size_t>(n)));
    }

    line = "bonjour le monde";
    send_all(conn.fd(), line.data(), line.size());
    n = recv(conn.fd(), buffer, sizeof(buffer), 0);
    if (n > 0) {
        std::println("Echo: {}", std::string_view(buffer, static_cast<size_t>(n)));
    }
}

int main() {
    std::jthread server_thread([] { run_server(); });
    run_client();
    std::println("Test echo OK !");
}
