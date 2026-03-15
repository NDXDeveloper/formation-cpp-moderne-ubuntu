/* ============================================================================
   Section 22.4.2 : Boost.Asio — Écosystème complet
   Description : Client HTTP asynchrone avec Beast — résolution DNS async,
                 requête GET, affichage headers et body, fermeture propre
   Fichier source : 04.2-boost-asio.md
   ============================================================================ */
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast.hpp>
#include <print>
#include <thread>
#include <chrono>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;
using asio::awaitable;
using asio::use_awaitable;

// Simple echo HTTP server for testing
awaitable<void> handle_session(tcp::socket socket) {
    beast::flat_buffer buffer;
    try {
        http::request<http::string_body> req;
        co_await http::async_read(socket, buffer, req, use_awaitable);

        http::response<http::string_body> res;
        res.version(req.version());
        res.result(http::status::ok);
        res.set(http::field::server, "TestServer/1.0");
        res.set(http::field::content_type, "text/plain");
        res.body() = "Response from test server";
        res.prepare_payload();
        res.keep_alive(false);

        co_await http::async_write(socket, res, use_awaitable);
    } catch (...) {}
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

awaitable<void> server(tcp::acceptor acceptor) {
    auto [ec, socket] = co_await acceptor.async_accept(
        asio::as_tuple(use_awaitable));
    if (!ec) {
        co_await handle_session(std::move(socket));
    }
}

// The client from the .md (adapted for test port)
awaitable<void> fetch(asio::io_context& ctx,
                       std::string host, std::string port,
                       std::string target) {
    // Résolution + connexion
    tcp::resolver resolver(ctx);
    auto endpoints = co_await resolver.async_resolve(host, port, use_awaitable);

    tcp::socket socket(ctx);
    co_await asio::async_connect(socket, endpoints, use_awaitable);

    // Construire la requête
    http::request<http::empty_body> req{http::verb::get, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "Beast-Client/1.0");

    // Envoyer
    co_await http::async_write(socket, req, use_awaitable);

    // Lire la réponse
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    co_await http::async_read(socket, buffer, res, use_awaitable);

    // Afficher (conversion en std::string_view car Beast
    // utilise boost::core::string_view, non formatable directement)
    std::println("HTTP {} {}", static_cast<int>(res.result()),
                 std::string_view(res.reason()));
    for (auto const& field : res) {
        std::println("  {}: {}", std::string_view(field.name_string()),
                     std::string_view(field.value()));
    }
    std::println("\n{}", res.body());

    // Fermeture propre
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);
}

int main() {
    try {
        asio::io_context ctx;

        tcp::acceptor acceptor(ctx);
        acceptor.open(tcp::v6());
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.set_option(asio::ip::v6_only(false));
        acceptor.bind(tcp::endpoint(tcp::v6(), 18093));
        acceptor.listen();

        co_spawn(ctx, server(std::move(acceptor)), asio::detached);

        // Delay client start slightly
        std::jthread client_starter([&ctx]{
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            co_spawn(ctx, fetch(ctx, "localhost", "18093", "/"),
                [](std::exception_ptr ep) {
                    if (ep) {
                        try { std::rethrow_exception(ep); }
                        catch (const std::exception& e) {
                            std::println(stderr, "Error: {}", e.what());
                        }
                    }
                });
        });

        std::jthread stopper([&ctx]{
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ctx.stop();
        });

        ctx.run();
        std::println("Test HTTP client OK !");
    } catch (const std::exception& e) {
        std::println(stderr, "Fatal: {}", e.what());
        return 1;
    }
}
