/* ============================================================================
   Section 22.4.2 : Boost.Asio — Écosystème complet
   Description : Serveur et client WebSocket avec Beast — handshake,
                 messages texte/binaire, fermeture propre (close_code)
   Fichier source : 04.2-boost-asio.md
   ============================================================================ */
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <print>
#include <thread>
#include <chrono>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace ws = beast::websocket;
using tcp = asio::ip::tcp;
using asio::awaitable;
using asio::use_awaitable;

// --- WebSocket server (from .md) ---
awaitable<void> handle_websocket(tcp::socket socket) {
    ws::stream<tcp::socket> wss(std::move(socket));
    co_await wss.async_accept(use_awaitable);

    std::println("WebSocket connecté");

    try {
        beast::flat_buffer buffer;

        while (true) {
            co_await wss.async_read(buffer, use_awaitable);
            wss.text(wss.got_text());
            co_await wss.async_write(buffer.data(), use_awaitable);
            buffer.consume(buffer.size());
        }
    } catch (const boost::system::system_error& e) {
        if (e.code() != ws::error::closed) {
            std::println(stderr, "WebSocket error: {}", e.what());
        }
    }

    std::println("WebSocket déconnecté");
}

// --- WebSocket client (from .md) ---
awaitable<void> websocket_client(asio::io_context& ctx,
                                  std::string host, std::string port) {
    tcp::resolver resolver(ctx);
    auto endpoints = co_await resolver.async_resolve(host, port, use_awaitable);

    ws::stream<tcp::socket> wss(ctx);
    co_await asio::async_connect(wss.next_layer(), endpoints, use_awaitable);

    co_await wss.async_handshake(host, "/ws", use_awaitable);

    co_await wss.async_write(
        asio::buffer(std::string("Hello WebSocket!")), use_awaitable);

    beast::flat_buffer buffer;
    co_await wss.async_read(buffer, use_awaitable);
    std::println("Reçu: {}", beast::buffers_to_string(buffer.data()));

    co_await wss.async_close(ws::close_code::normal, use_awaitable);
}

// --- Listener ---
awaitable<void> listener(tcp::acceptor acceptor) {
    auto [ec, socket] = co_await acceptor.async_accept(
        asio::as_tuple(use_awaitable));
    if (!ec) {
        co_await handle_websocket(std::move(socket));
    }
}

int main() {
    try {
        asio::io_context ctx;

        tcp::acceptor acceptor(ctx);
        acceptor.open(tcp::v6());
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.set_option(asio::ip::v6_only(false));
        acceptor.bind(tcp::endpoint(tcp::v6(), 18094));
        acceptor.listen();

        co_spawn(ctx, listener(std::move(acceptor)), asio::detached);

        std::jthread client_starter([&ctx]{
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            co_spawn(ctx, websocket_client(ctx, "localhost", "18094"),
                [](std::exception_ptr ep) {
                    if (ep) {
                        try { std::rethrow_exception(ep); }
                        catch (const std::exception& e) {
                            std::println(stderr, "Client error: {}", e.what());
                        }
                    }
                });
        });

        std::jthread stopper([&ctx]{
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ctx.stop();
        });

        ctx.run();
        std::println("Test WebSocket OK !");
    } catch (const std::exception& e) {
        std::println(stderr, "Fatal: {}", e.what());
        return 1;
    }
}
