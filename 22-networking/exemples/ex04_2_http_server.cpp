/* ============================================================================
   Section 22.4.2 : Boost.Asio — Écosystème complet
   Description : Serveur HTTP avec Boost.Beast — routage simple,
                 keep-alive, coroutines, réponses JSON
   Fichier source : 04.2-boost-asio.md
   ============================================================================ */
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast.hpp>
#include <print>
#include <string>
#include <thread>
#include <chrono>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;
using asio::awaitable;
using asio::use_awaitable;

// --- Routage simple ---
http::response<http::string_body>
handle_request(const http::request<http::string_body>& req) {
    http::response<http::string_body> res;
    res.version(req.version());
    res.set(http::field::server, "Beast-Server/1.0");

    if (req.method() != http::verb::get) {
        res.result(http::status::method_not_allowed);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Method not allowed";
        res.prepare_payload();
        return res;
    }

    auto target = req.target();

    if (target == "/") {
        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Hello from Boost.Beast!";
    }
    else if (target == "/api/status") {
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"status":"ok","connections":42})";
    }
    else if (target == "/api/health") {
        res.result(http::status::ok);
        res.set(http::field::content_type, "application/json");
        res.body() = R"({"healthy":true})";
    }
    else {
        res.result(http::status::not_found);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Not found: " + std::string(target);
    }

    res.prepare_payload();
    return res;
}

// --- Session HTTP (une par connexion) ---
awaitable<void> handle_session(tcp::socket socket) {
    beast::flat_buffer buffer;

    try {
        while (true) {
            http::request<http::string_body> req;
            co_await http::async_read(socket, buffer, req, use_awaitable);

            bool keep_alive = req.keep_alive();
            auto res = handle_request(req);
            res.keep_alive(keep_alive);

            co_await http::async_write(socket, res, use_awaitable);

            if (!keep_alive) {
                break;
            }
        }
    } catch (const boost::system::system_error& e) {
        if (e.code() != beast::errc::not_connected &&
            e.code() != asio::error::eof) {
            std::println(stderr, "Session error: {}", e.what());
        }
    }

    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

// --- Listener ---
awaitable<void> listener(tcp::acceptor acceptor) {
    while (true) {
        auto [ec, socket] = co_await acceptor.async_accept(
            asio::as_tuple(use_awaitable));

        if (ec) {
            if (ec == asio::error::operation_aborted) break;
            std::println(stderr, "Accept error: {}", ec.message());
            continue;
        }

        co_spawn(acceptor.get_executor(),
                 handle_session(std::move(socket)),
                 asio::detached);
    }
}

// --- Test client using Beast ---
void run_http_client(uint16_t port, const std::string& target) {
    try {
        asio::io_context ctx;
        tcp::resolver resolver(ctx);
        auto endpoints = resolver.resolve("localhost", std::to_string(port));
        tcp::socket socket(ctx);
        asio::connect(socket, endpoints);

        http::request<http::empty_body> req{http::verb::get, target, 11};
        req.set(http::field::host, "localhost");
        req.set(http::field::connection, "close");
        http::write(socket, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(socket, buffer, res);

        std::println("GET {} → {} {}: {}",
                     target, static_cast<int>(res.result()),
                     std::string_view(res.reason()), res.body());
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
        acceptor.bind(tcp::endpoint(tcp::v6(), 18092));
        acceptor.listen();

        std::println("Serveur HTTP sur http://localhost:18092/");

        co_spawn(ctx, listener(std::move(acceptor)), asio::detached);

        std::jthread client([]{
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            run_http_client(18092, "/");
            run_http_client(18092, "/api/status");
            run_http_client(18092, "/api/health");
            run_http_client(18092, "/nonexistent");
        });
        std::jthread stopper([&ctx]{
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ctx.stop();
        });

        ctx.run();
        std::println("Test HTTP server OK !");
    } catch (const std::exception& e) {
        std::println(stderr, "Fatal: {}", e.what());
        return 1;
    }
}
