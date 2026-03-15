/* ============================================================================
   Section 22.4.1 : Standalone Asio — Networking sans Boost
   Description : Client TCP asynchrone avec coroutines — résolution DNS
                 async, connexion async, envoi/réception async
   Fichier source : 04.1-standalone-asio.md
   ============================================================================ */
#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_awaitable.hpp>
#include <print>
#include <thread>
#include <chrono>

using asio::ip::tcp;
using asio::awaitable;
using asio::use_awaitable;

// Simple echo server for the client to connect to
awaitable<void> echo_server(tcp::acceptor acceptor) {
    auto socket = co_await acceptor.async_accept(use_awaitable);
    std::array<char, 4096> buf{};
    auto n = co_await socket.async_read_some(asio::buffer(buf), use_awaitable);
    co_await asio::async_write(socket, asio::buffer(buf.data(), n), use_awaitable);
}

// The async client from the .md
awaitable<void> run_client(asio::io_context& ctx,
                            std::string host, std::string port) {
    // Résolution DNS asynchrone
    tcp::resolver resolver(ctx);
    auto endpoints = co_await resolver.async_resolve(host, port, use_awaitable);

    // Connexion asynchrone
    tcp::socket socket(ctx);
    co_await asio::async_connect(socket, endpoints, use_awaitable);

    std::println("Connecté à {}:{}", host, port);

    // Envoi
    std::string msg = "Hello from async client!\n";
    co_await asio::async_write(socket, asio::buffer(msg), use_awaitable);

    // Réception
    std::array<char, 4096> buffer{};
    auto [ec, n] = co_await socket.async_read_some(
        asio::buffer(buffer), asio::as_tuple(use_awaitable));

    if (!ec) {
        std::println("Réponse: {}", std::string_view(buffer.data(), n));
    }
}

int main() {
    try {
        asio::io_context ctx;

        // Start echo server
        tcp::acceptor acceptor(ctx);
        acceptor.open(tcp::v6());
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.set_option(asio::ip::v6_only(false));
        acceptor.bind(tcp::endpoint(tcp::v6(), 18090));
        acceptor.listen();

        co_spawn(ctx, echo_server(std::move(acceptor)), asio::detached);

        // Start async client (from the .md)
        co_spawn(ctx, run_client(ctx, "localhost", "18090"),
            [](std::exception_ptr ep) {
                if (ep) {
                    try { std::rethrow_exception(ep); }
                    catch (const std::exception& e) {
                        std::println(stderr, "Erreur: {}", e.what());
                    }
                }
            });

        ctx.run();
        std::println("Test async client OK !");
    } catch (const std::exception& e) {
        std::println(stderr, "Erreur fatale: {}", e.what());
        return 1;
    }
}
