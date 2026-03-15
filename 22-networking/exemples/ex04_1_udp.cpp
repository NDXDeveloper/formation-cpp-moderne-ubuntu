/* ============================================================================
   Section 22.4.1 : Standalone Asio — Networking sans Boost
   Description : Serveur UDP echo avec coroutines, async_receive_from/
                 async_send_to, dual-stack IPv6
   Fichier source : 04.1-standalone-asio.md
   ============================================================================ */
#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/use_awaitable.hpp>
#include <print>
#include <thread>
#include <chrono>

using asio::ip::udp;
using asio::awaitable;
using asio::use_awaitable;

awaitable<void> udp_echo_server(asio::io_context& ctx, uint16_t port) {
    udp::socket socket(ctx);
    socket.open(udp::v6());
    socket.set_option(asio::ip::v6_only(false));
    socket.bind(udp::endpoint(udp::v6(), port));

    std::println("Serveur UDP echo sur le port {}", port);

    std::array<char, 65535> buffer{};

    while (true) {
        udp::endpoint sender;
        auto [ec, n] = co_await socket.async_receive_from(
            asio::buffer(buffer), sender,
            asio::as_tuple(use_awaitable));

        if (ec) {
            if (ec == asio::error::operation_aborted) break;
            std::println(stderr, "Erreur recv: {}", ec.message());
            continue;
        }

        co_await socket.async_send_to(
            asio::buffer(buffer.data(), n), sender, use_awaitable);
    }
}

void run_udp_client(uint16_t port) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    try {
        asio::io_context ctx;
        udp::socket socket(ctx, udp::endpoint(udp::v4(), 0));
        udp::endpoint server(asio::ip::make_address("127.0.0.1"), port);
        std::string msg = "Hello UDP!";
        socket.send_to(asio::buffer(msg), server);
        std::array<char, 4096> buf{};
        udp::endpoint sender;
        size_t n = socket.receive_from(asio::buffer(buf), sender);
        std::println("UDP réponse: {}", std::string_view(buf.data(), n));
    } catch (const std::exception& e) {
        std::println(stderr, "UDP client erreur: {}", e.what());
    }
}

int main() {
    try {
        asio::io_context ctx;

        co_spawn(ctx, udp_echo_server(ctx, 18091), asio::detached);

        std::jthread client([] { run_udp_client(18091); });
        std::jthread stopper([&ctx] {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            ctx.stop();
        });

        ctx.run();
        std::println("Test UDP server OK !");
    } catch (const std::exception& e) {
        std::println(stderr, "Erreur fatale: {}", e.what());
        return 1;
    }
}
