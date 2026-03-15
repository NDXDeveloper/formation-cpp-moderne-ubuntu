/* ============================================================================
   Section 22.4.1 : Standalone Asio — Networking sans Boost
   Description : Timers Asio — style callback (async_wait) et style
                 coroutine (periodic_timer avec co_await)
   Fichier source : 04.1-standalone-asio.md
   ============================================================================ */
#include <asio.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/steady_timer.hpp>
#include <asio/use_awaitable.hpp>
#include <print>
#include <chrono>

using asio::awaitable;
using asio::use_awaitable;
using namespace std::chrono_literals;

// --- Style callbacks ---
void start_timer_callback(asio::io_context& ctx) {
    auto timer = std::make_shared<asio::steady_timer>(ctx, 500ms);

    timer->async_wait([timer](std::error_code ec) {
        if (!ec) {
            std::println("Timer expiré (callback)");
        }
    });
}

// --- Style coroutine ---
awaitable<void> periodic_timer() {
    auto executor = co_await asio::this_coro::executor;
    asio::steady_timer timer(executor);

    for (int i = 0; i < 3; ++i) {
        timer.expires_after(200ms);
        co_await timer.async_wait(use_awaitable);
        std::println("Tick {} à t+{}ms", i + 1, (i + 1) * 200);
    }

    std::println("Timer terminé");
}

int main() {
    try {
        asio::io_context ctx;

        start_timer_callback(ctx);
        co_spawn(ctx, periodic_timer(), asio::detached);

        ctx.run();
        std::println("Test timers OK !");
    } catch (const std::exception& e) {
        std::println(stderr, "Erreur fatale: {}", e.what());
        return 1;
    }
}
