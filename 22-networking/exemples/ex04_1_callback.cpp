/* ============================================================================
   Section 22.4.1 : Standalone Asio — Networking sans Boost
   Description : Serveur echo asynchrone avec callbacks et shared_from_this,
                 pattern Session/Server, dual-stack IPv6
   Fichier source : 04.1-standalone-asio.md
   ============================================================================ */
#include <asio.hpp>
#include <print>
#include <memory>
#include <thread>
#include <chrono>

using asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(tcp::socket socket) : socket_(std::move(socket)) {}
    void start() { do_read(); }
private:
    void do_read() {
        auto self = shared_from_this();
        socket_.async_read_some(
            asio::buffer(buffer_),
            [self](std::error_code ec, size_t length) {
                if (!ec) { self->do_write(length); }
                else { std::println("Client déconnecté: {}", ec.message()); }
            });
    }
    void do_write(size_t length) {
        auto self = shared_from_this();
        asio::async_write(socket_, asio::buffer(buffer_.data(), length),
            [self](std::error_code ec, size_t) {
                if (!ec) { self->do_read(); }
                else { std::println("Erreur écriture: {}", ec.message()); }
            });
    }
    tcp::socket socket_;
    std::array<char, 4096> buffer_{};
};

class Server {
public:
    Server(asio::io_context& ctx, uint16_t port)
        : acceptor_(ctx) {
        acceptor_.open(tcp::v6());
        acceptor_.set_option(asio::ip::v6_only(false));
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(tcp::endpoint(tcp::v6(), port));
        acceptor_.listen();
        std::println("Serveur echo (callbacks) sur le port {}", port);
        do_accept();
    }
private:
    void do_accept() {
        acceptor_.async_accept(
            [this](std::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::println("Nouveau client: {}:{}",
                        socket.remote_endpoint().address().to_string(),
                        socket.remote_endpoint().port());
                    std::make_shared<Session>(std::move(socket))->start();
                }
                do_accept();
            });
    }
    tcp::acceptor acceptor_;
};

void run_client(uint16_t port) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    try {
        asio::io_context ctx;
        tcp::resolver resolver(ctx);
        auto endpoints = resolver.resolve("localhost", std::to_string(port));
        tcp::socket socket(ctx);
        asio::connect(socket, endpoints);
        std::println("Connecté à {}:{}",
            socket.remote_endpoint().address().to_string(),
            socket.remote_endpoint().port());
        std::string message = "Hello from Asio!\n";
        asio::write(socket, asio::buffer(message));
        std::array<char, 4096> buf{};
        std::error_code ec;
        size_t n = socket.read_some(asio::buffer(buf), ec);
        if (!ec) {
            std::println("Réponse: {}", std::string_view(buf.data(), n));
        }
    } catch (const std::exception& e) {
        std::println(stderr, "Client erreur: {}", e.what());
    }
}

int main() {
    try {
        asio::io_context ctx;
        Server server(ctx, 18087);
        std::jthread client_thread([] { run_client(18087); });
        std::jthread stopper([&ctx] {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            ctx.stop();
        });
        ctx.run();
        std::println("Test callback server OK !");
    } catch (const std::exception& e) {
        std::println(stderr, "Erreur fatale: {}", e.what());
        return 1;
    }
}
