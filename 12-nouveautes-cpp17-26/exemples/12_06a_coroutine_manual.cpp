/* ============================================================================
   Section 12.6 : Coroutines (C++20) - Implementation manuelle
   Description : Generator<T> minimal avec promise_type, coroutine_handle,
                 co_yield - generateur range(start, end)
   Fichier source : 06-coroutines.md
   ============================================================================ */
#include <coroutine>
#include <optional>
#include <utility>
#include <print>

// === Generator<T> minimal (lignes 160-222) ===
template <typename T>
class Generator {
public:
    struct promise_type {
        std::optional<T> current_value;

        Generator get_return_object() {
            return Generator{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(T value) {
            current_value = std::move(value);
            return {};
        }

        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    explicit Generator(std::coroutine_handle<promise_type> h) : handle_(h) {}
    ~Generator() { if (handle_) handle_.destroy(); }

    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)) {}
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    bool next() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
            return !handle_.done();
        }
        return false;
    }

    const T& value() const {
        return *handle_.promise().current_value;
    }

private:
    std::coroutine_handle<promise_type> handle_;
};

// === range generator (lignes 230-233) ===
Generator<int> range(int start, int end) {
    for (int i = start; i < end; ++i) {
        co_yield i;
    }
}

// === main (lignes 236-242) ===
int main() {
    auto gen = range(1, 6);
    while (gen.next()) {
        std::print("{} ", gen.value());
    }
    std::print("\n");
    // Sortie : 1 2 3 4 5
}
