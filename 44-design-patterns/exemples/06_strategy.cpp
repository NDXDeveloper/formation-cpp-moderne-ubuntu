/* ============================================================================
   Section 44.2 : Observer, Strategy, Command
   Description : Strategy — strategies de retry via std::function
   Fichier source : 02-behavioral-patterns.md
   ============================================================================ */

#include <functional>
#include <chrono>
#include <print>
#include <algorithm>

using RetryStrategy = std::function<std::chrono::milliseconds(int attempt)>;

namespace retry {
    inline RetryStrategy constant(std::chrono::milliseconds delay) {
        return [delay](int) { return delay; };
    }
    inline RetryStrategy linear(std::chrono::milliseconds base) {
        return [base](int attempt) { return base * attempt; };
    }
    inline RetryStrategy exponential(
        std::chrono::milliseconds base,
        std::chrono::milliseconds max_delay = std::chrono::seconds(60))
    {
        return [base, max_delay](int attempt) {
            auto delay = base * (1 << std::min(attempt, 20));
            return std::min(delay, max_delay);
        };
    }
}

int main() {
    auto strats = std::vector<std::pair<std::string, RetryStrategy>>{
        {"Constant(500ms)", retry::constant(std::chrono::milliseconds(500))},
        {"Linear(200ms)",   retry::linear(std::chrono::milliseconds(200))},
        {"Exponential(100ms)", retry::exponential(std::chrono::milliseconds(100))},
    };

    for (const auto& [name, strat] : strats) {
        std::print("{}:\n", name);
        for (int i = 0; i < 5; ++i)
            std::print("  attempt {}: {}ms\n", i, strat(i).count());
    }
}
