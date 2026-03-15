/* ============================================================================
   Section 22.2 : Client/Serveur basique en C++
   Description : Serveur protocole avec framing length-prefixed, commandes
                 JSON (echo, time, stats), statistiques atomiques
   Fichier source : 02-client-serveur.md
   ============================================================================ */
#include "net_utils.hpp"
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>
#include <optional>
#include <cstdint>

// === protocol.hpp (from 02) ===
constexpr uint32_t MAX_MSG_SIZE = 1024 * 1024;

void send_message(int sockfd, std::string_view payload) {
    uint32_t len_net = htonl(static_cast<uint32_t>(payload.size()));
    send_all(sockfd, &len_net, sizeof(len_net));
    send_all(sockfd, payload.data(), payload.size());
}

std::optional<std::string> recv_message(int sockfd) {
    uint32_t len_net = 0;
    if (!recv_exact(sockfd, &len_net, sizeof(len_net))) return std::nullopt;
    uint32_t len = ntohl(len_net);
    if (len > MAX_MSG_SIZE) throw std::runtime_error("Message trop grand: " + std::to_string(len));
    std::string payload(len, '\0');
    if (!recv_exact(sockfd, payload.data(), len))
        throw std::runtime_error("Connexion fermée pendant lecture du payload");
    return payload;
}

// === Server (from 02) ===
struct ServerStats {
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> active_connections{0};
};

ServerStats stats;

std::string process_request(std::string_view request) {
    stats.total_requests++;
    if (request.find("\"echo\"") != std::string_view::npos) {
        auto pos = request.find("\"data\"");
        if (pos != std::string_view::npos) {
            auto start = request.find('"', pos + 6);
            auto end = request.find('"', start + 1);
            if (start != std::string_view::npos && end != std::string_view::npos) {
                auto data = request.substr(start + 1, end - start - 1);
                return "{\"status\":\"ok\",\"data\":\"" + std::string(data) + "\"}";
            }
        }
        return R"({"status":"error","message":"missing data field"})";
    }
    if (request.find("\"time\"") != std::string_view::npos) {
        auto now = std::chrono::system_clock::now();
        auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        return "{\"status\":\"ok\",\"data\":\"" + std::to_string(epoch) + "\"}";
    }
    if (request.find("\"stats\"") != std::string_view::npos) {
        return "{\"status\":\"ok\","
               "\"requests\":" + std::to_string(stats.total_requests.load()) + ","
               "\"connections\":" + std::to_string(stats.active_connections.load()) + "}";
    }
    return R"({"status":"error","message":"unknown command"})";
}

void handle_protocol_client(Socket client, std::string client_info) {
    stats.active_connections++;
    std::println("[{}] Connecté", client_info);
    try {
        while (true) {
            auto msg = recv_message(client.fd());
            if (!msg) break;
            std::println("[{}] Requête: {}", client_info, *msg);
            auto response = process_request(*msg);
            send_message(client.fd(), response);
        }
    } catch (const std::exception& e) {
        std::println(stderr, "[{}] Erreur: {}", client_info, e.what());
    }
    stats.active_connections--;
    std::println("[{}] Déconnecté", client_info);
}

void run_protocol_server() {
    signal(SIGPIPE, SIG_IGN);
    auto addr = resolve(nullptr, "19000", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);
    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");
    std::println("Serveur protocole en écoute sur le port 19000");

    sockaddr_storage client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept4(server.fd(), reinterpret_cast<sockaddr*>(&client_addr),
                            &addr_len, SOCK_CLOEXEC);
    if (client_fd == -1) throw std::system_error(errno, std::system_category(), "accept4()");
    handle_protocol_client(Socket{client_fd}, format_address(client_addr));
}

// === Client (from 02) ===
void run_protocol_client() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto conn = connect_to("localhost", "19000");
    std::println("Connecté au serveur");

    send_message(conn.fd(), R"({"cmd":"echo","data":"Hello, protocol!"})");
    if (auto resp = recv_message(conn.fd())) {
        std::println("Echo: {}", *resp);
    }

    send_message(conn.fd(), R"({"cmd":"time"})");
    if (auto resp = recv_message(conn.fd())) {
        std::println("Time: {}", *resp);
    }

    send_message(conn.fd(), R"({"cmd":"stats"})");
    if (auto resp = recv_message(conn.fd())) {
        std::println("Stats: {}", *resp);
    }

    send_message(conn.fd(), R"({"cmd":"unknown"})");
    if (auto resp = recv_message(conn.fd())) {
        std::println("Unknown: {}", *resp);
    }
}

int main() {
    std::jthread server_thread([] { run_protocol_server(); });
    run_protocol_client();
    std::println("Test protocole OK !");
}
