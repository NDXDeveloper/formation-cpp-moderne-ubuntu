/* ============================================================================
   Section 22.4.2 : Boost.Asio — Écosystème complet
   Description : API REST JSON avec Beast + Boost.JSON — sérialisation/
                 parsing JSON, GET liste, POST création, gestion erreurs
   Fichier source : 04.2-boost-asio.md
   ============================================================================ */
#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <print>
#include <thread>
#include <chrono>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
using tcp = asio::ip::tcp;
using asio::awaitable;
using asio::use_awaitable;

// --- API handler from .md ---
http::response<http::string_body>
handle_api(const http::request<http::string_body>& req) {
    http::response<http::string_body> res;
    res.version(req.version());
    res.set(http::field::content_type, "application/json");

    if (req.method() == http::verb::get && req.target() == "/api/users") {
        json::object users_response;
        json::array users;
        users.push_back({{"id", 1}, {"name", "Alice"}});
        users.push_back({{"id", 2}, {"name", "Bob"}});
        users_response["users"] = std::move(users);
        users_response["total"] = 2;

        res.result(http::status::ok);
        res.body() = json::serialize(users_response);
    }
    else if (req.method() == http::verb::post && req.target() == "/api/users") {
        try {
            auto body = json::parse(req.body());
            auto& obj = body.as_object();
            auto name = obj.at("name").as_string();

            json::object created;
            created["id"] = 3;
            created["name"] = name;
            created["created"] = true;

            res.result(http::status::created);
            res.body() = json::serialize(created);
        } catch (const std::exception& e) {
            res.result(http::status::bad_request);
            json::object err;
            err["error"] = e.what();
            res.body() = json::serialize(err);
        }
    }
    else {
        res.result(http::status::not_found);
        res.body() = R"({"error":"not found"})";
    }

    res.prepare_payload();
    return res;
}

awaitable<void> handle_session(tcp::socket socket) {
    beast::flat_buffer buffer;
    try {
        while (true) {
            http::request<http::string_body> req;
            co_await http::async_read(socket, buffer, req, use_awaitable);
            bool keep_alive = req.keep_alive();
            auto res = handle_api(req);
            res.keep_alive(keep_alive);
            co_await http::async_write(socket, res, use_awaitable);
            if (!keep_alive) break;
        }
    } catch (...) {}
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

awaitable<void> listener(tcp::acceptor acceptor) {
    while (true) {
        auto [ec, socket] = co_await acceptor.async_accept(
            asio::as_tuple(use_awaitable));
        if (ec) {
            if (ec == asio::error::operation_aborted) break;
            continue;
        }
        co_spawn(acceptor.get_executor(),
                 handle_session(std::move(socket)), asio::detached);
    }
}

void test_get(uint16_t port) {
    asio::io_context ctx;
    tcp::resolver resolver(ctx);
    auto ep = resolver.resolve("localhost", std::to_string(port));
    tcp::socket socket(ctx);
    asio::connect(socket, ep);

    http::request<http::empty_body> req{http::verb::get, "/api/users", 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::connection, "close");
    http::write(socket, req);

    beast::flat_buffer buf;
    http::response<http::string_body> res;
    http::read(socket, buf, res);
    std::println("GET /api/users → {}", res.body());
}

void test_post(uint16_t port) {
    asio::io_context ctx;
    tcp::resolver resolver(ctx);
    auto ep = resolver.resolve("localhost", std::to_string(port));
    tcp::socket socket(ctx);
    asio::connect(socket, ep);

    http::request<http::string_body> req{http::verb::post, "/api/users", 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    req.set(http::field::connection, "close");
    req.body() = R"({"name":"Charlie"})";
    req.prepare_payload();
    http::write(socket, req);

    beast::flat_buffer buf;
    http::response<http::string_body> res;
    http::read(socket, buf, res);
    std::println("POST /api/users → {}", res.body());
}

int main() {
    try {
        asio::io_context ctx;

        tcp::acceptor acceptor(ctx);
        acceptor.open(tcp::v6());
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.set_option(asio::ip::v6_only(false));
        acceptor.bind(tcp::endpoint(tcp::v6(), 18095));
        acceptor.listen();

        co_spawn(ctx, listener(std::move(acceptor)), asio::detached);

        std::jthread client([]{
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            test_get(18095);
            test_post(18095);
        });
        std::jthread stopper([&ctx]{
            std::this_thread::sleep_for(std::chrono::seconds(3));
            ctx.stop();
        });

        ctx.run();
        std::println("Test Boost.JSON API OK !");
    } catch (const std::exception& e) {
        std::println(stderr, "Fatal: {}", e.what());
        return 1;
    }
}
