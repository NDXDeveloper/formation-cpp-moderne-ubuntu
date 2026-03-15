/* ============================================================================
   Section 22.2 : Client/Serveur basique en C++
   Description : Serveur echo avec thread pool (ConnectionQueue), 4 workers,
                 gestion concurrente de connexions multiples
   Fichier source : 02-client-serveur.md
   ============================================================================ */
#include "net_utils.hpp"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <csignal>
#include <chrono>

// --- ConnectionQueue (from 02) ---
class ConnectionQueue {
public:
    void push(Socket sock, std::string info) {
        { std::lock_guard lock{mutex_}; queue_.emplace(std::move(sock), std::move(info)); }
        cv_.notify_one();
    }
    bool pop(Socket& sock, std::string& info) {
        std::unique_lock lock{mutex_};
        cv_.wait(lock, [this] { return !queue_.empty() || shutdown_; });
        if (shutdown_ && queue_.empty()) return false;
        auto& [s, i] = queue_.front();
        sock = std::move(s); info = std::move(i);
        queue_.pop();
        return true;
    }
    void shutdown() {
        { std::lock_guard lock{mutex_}; shutdown_ = true; }
        cv_.notify_all();
    }
private:
    struct Entry {
        Socket socket; std::string info;
        Entry(Socket s, std::string i) : socket{std::move(s)}, info{std::move(i)} {}
    };
    std::queue<Entry> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool shutdown_ = false;
};

void handle_client(Socket client, const std::string& client_info) {
    std::println("[{}] Traitement (thread {})", client_info, std::this_thread::get_id());
    char buffer[4096];
    while (true) {
        ssize_t n = recv(client.fd(), buffer, sizeof(buffer), 0);
        if (n == -1) { if (errno == EINTR) continue; break; }
        if (n == 0) break;
        try { send_all(client.fd(), buffer, static_cast<size_t>(n)); }
        catch (const std::system_error&) { break; }
    }
    std::println("[{}] Déconnexion", client_info);
}

void worker(ConnectionQueue& queue) {
    while (true) {
        Socket sock{-1}; std::string info;
        if (!queue.pop(sock, info)) break;
        handle_client(std::move(sock), info);
    }
}

void run_server(ConnectionQueue& queue) {
    signal(SIGPIPE, SIG_IGN);
    auto addr = resolve(nullptr, "18083", AF_INET6, SOCK_STREAM);
    Socket server{addr->ai_family, addr->ai_socktype, addr->ai_protocol};
    server.enable_reuse_addr();
    server.set_option(IPPROTO_IPV6, IPV6_V6ONLY, 0);
    if (bind(server.fd(), addr->ai_addr, addr->ai_addrlen) == -1)
        throw std::system_error(errno, std::system_category(), "bind()");
    if (listen(server.fd(), SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category(), "listen()");
    std::println("Serveur echo en écoute sur le port 18083 (4 workers)");

    for (int i = 0; i < 2; ++i) {
        sockaddr_storage ca{}; socklen_t al = sizeof(ca);
        int cfd = accept4(server.fd(), reinterpret_cast<sockaddr*>(&ca), &al, SOCK_CLOEXEC);
        if (cfd == -1) throw std::system_error(errno, std::system_category(), "accept4()");
        queue.push(Socket{cfd}, format_address(ca));
    }
}

void run_client(int id) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200 + id * 100));
    auto conn = connect_to("localhost", "18083");
    std::string msg = "Hello from client " + std::to_string(id);
    send_all(conn.fd(), msg.data(), msg.size());
    char buffer[4096];
    ssize_t n = recv(conn.fd(), buffer, sizeof(buffer), 0);
    if (n > 0) std::println("Client {} echo: {}", id, std::string_view(buffer, static_cast<size_t>(n)));
}

int main() {
    ConnectionQueue queue;
    constexpr int num_workers = 4;
    std::vector<std::jthread> workers_vec;
    for (int i = 0; i < num_workers; ++i)
        workers_vec.emplace_back(worker, std::ref(queue));

    std::jthread server_thread([&queue] { run_server(queue); });
    std::jthread c1([] { run_client(1); });
    std::jthread c2([] { run_client(2); });

    c1.join(); c2.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    queue.shutdown();
    std::println("Test pool OK !");
}
