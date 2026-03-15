/* ============================================================================
   Section 22.4.1 : Standalone Asio — Networking sans Boost
   Description : Serveur echo avec coroutines C++20 (co_await, co_spawn),
                 use_awaitable, détection EOF/connection_reset
   Fichier source : 04.1-standalone-asio.md
   ============================================================================ */
#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/use_awaitable.hpp>
#include <print>
#include <thread>
#include <chrono>

using asio::ip::tcp;
using asio::awaitable;
using asio::use_awaitable;

awaitable<void> handle_client(tcp::socket socket) {
    auto endpoint = socket.remote_endpoint();
    std::println("Client connecté: {}:{}",
                 endpoint.address().to_string(), endpoint.port());
    try {
        std::array<char, 4096> buffer{};
        while (true) {
            size_t n = co_await socket.async_read_some(
                asio::buffer(buffer), use_awaitable);
            co_await asio::async_write(
                socket, asio::buffer(buffer.data(), n), use_awaitable);
        }
    } catch (const std::system_error& e) {
        if (e.code() == asio::error::eof ||
            e.code() == asio::error::connection_reset) {
            std::println("Client {}:{} déconnecté",
                         endpoint.address().to_string(), endpoint.port());
        } else {
            std::println(stderr, "Erreur: {}", e.what());
        }
    }
}

awaitable<void> listener(tcp::acceptor acceptor) {
    std::println("Serveur echo (coroutines) sur le port {}",
                 acceptor.local_endpoint().port());
    while (true) {
        tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
        co_spawn(acceptor.get_executor(), handle_client(std::move(socket)),
                 asio::detached);
    }
}

void run_client(uint16_t port) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    try {
        asio::io_context ctx;
        tcp::resolver resolver(ctx);
        auto endpoints = resolver.resolve("localhost", std::to_string(port));
        tcp::socket socket(ctx);
        asio::connect(socket, endpoints);
        std::string msg = "Hello from coroutine test!\n";
        asio::write(socket, asio::buffer(msg));
        std::array<char, 4096> buf{};
        std::error_code ec;
        size_t n = socket.read_some(asio::buffer(buf), ec);
        if (!ec) std::println("Réponse: {}", std::string_view(buf.data(), n));
    } catch (const std::exception& e) {
        std::println(stderr, "Client erreur: {}", e.what());
    }
}

int main() {
    try {
        asio::io_context ctx;

        tcp::acceptor acceptor(ctx);
        acceptor.open(tcp::v6());
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.set_option(asio::ip::v6_only(false));
        acceptor.bind(tcp::endpoint(tcp::v6(), 18088));
        acceptor.listen();

        co_spawn(ctx, listener(std::move(acceptor)), asio::detached);

        std::jthread client_thread([] { run_client(18088); });
        std::jthread stopper([&ctx] {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            ctx.stop();
        });

        ctx.run();
        std::println("Test coroutine server OK !");
    } catch (const std::exception& e) {
        std::println(stderr, "Erreur fatale: {}", e.what());
        return 1;
    }
}
