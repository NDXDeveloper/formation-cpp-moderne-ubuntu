/* ============================================================================
   Section 22.4.1 : Standalone Asio — Networking sans Boost
   Description : Serveur echo production-ready avec as_tuple, TCP_NODELAY,
                 compteur de connexions atomique, gestion d'erreurs robuste
   Fichier source : 04.1-standalone-asio.md
   ============================================================================ */
#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/signal_set.hpp>
#include <asio/use_awaitable.hpp>
#include <print>
#include <atomic>
#include <thread>
#include <chrono>

using asio::ip::tcp;
using asio::awaitable;
using asio::use_awaitable;

std::atomic<int> active_connections{0};

awaitable<void> handle_client(tcp::socket socket) {
    auto ep = socket.remote_endpoint();
    auto client = ep.address().to_string() + ":" + std::to_string(ep.port());
    active_connections++;
    std::println("[+] {} (actives: {})", client, active_connections.load());

    try {
        socket.set_option(tcp::no_delay(true));
        std::array<char, 8192> buffer{};

        while (true) {
            auto [ec, n] = co_await socket.async_read_some(
                asio::buffer(buffer),
                asio::as_tuple(use_awaitable));

            if (ec) break;

            co_await asio::async_write(
                socket, asio::buffer(buffer.data(), n), use_awaitable);
        }
    } catch (const std::exception& e) {
        std::println("[!] {} erreur: {}", client, e.what());
    }

    active_connections--;
    std::println("[-] {} (actives: {})", client, active_connections.load());
}

awaitable<void> listener(tcp::acceptor acceptor) {
    while (true) {
        auto [ec, socket] = co_await acceptor.async_accept(
            asio::as_tuple(use_awaitable));

        if (ec) {
            if (ec == asio::error::operation_aborted) {
                break;
            }
            std::println(stderr, "Accept error: {}", ec.message());
            continue;
        }

        co_spawn(acceptor.get_executor(),
                 handle_client(std::move(socket)),
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
        std::string msg = "Hello from production test!\n";
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
        acceptor.bind(tcp::endpoint(tcp::v6(), 18089));
        acceptor.listen();

        std::println("Serveur echo (production) sur le port 18089");

        co_spawn(ctx, listener(std::move(acceptor)), asio::detached);

        std::jthread client_thread([] { run_client(18089); });
        std::jthread stopper([&ctx] {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            ctx.stop();
        });

        ctx.run();

        std::println("Serveur arrêté. {} connexions encore actives.",
                     active_connections.load());
        std::println("Test production server OK !");
    } catch (const std::exception& e) {
        std::println(stderr, "Erreur fatale: {}", e.what());
        return 1;
    }
}
