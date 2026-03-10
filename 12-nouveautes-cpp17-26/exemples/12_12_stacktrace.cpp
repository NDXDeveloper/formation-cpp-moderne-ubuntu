/* ============================================================================
   Section 12.12 : std::stacktrace (C++23)
   Description : Capture de pile d'appels - basic capture, format integration,
                 anatomy, entry details, skip parameter, traced_error exception,
                 assert avec trace et source_location
   Fichier source : 12-stacktrace.md
   Compilation : g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -lstdc++exp
   ============================================================================ */
#include <stacktrace>
#include <print>
#include <format>
#include <string>
#include <source_location>
#include <cstdlib>

// === basic capture (lignes 19-35) ===
void inner_function() {
    auto trace = std::stacktrace::current();
    std::print("Pile d'appels :\n{}\n", trace);
}

void middle_function() {
    inner_function();
}

// === format integration (lignes 52-69) ===
void test_format_integration() {
    std::print("=== format integration ===\n");
    auto trace = std::stacktrace::current();
    std::string trace_str = std::format("{}", trace);
    std::print("trace_str length: {}\n", trace_str.size());

    for (const auto& entry : trace) {
        std::print("  -> {}\n", entry);
    }
}

// === anatomy (lignes 79-99) ===
void test_anatomy() {
    std::print("=== anatomy ===\n");
    auto trace = std::stacktrace::current();
    std::print("size={}, empty={}\n", trace.size(), trace.empty());

    for (const auto& frame : trace) {
        std::print("{}\n", frame);
    }
}

// === entry details (lignes 107-128) ===
void test_entry_details() {
    std::print("=== entry details ===\n");
    auto trace = std::stacktrace::current();

    for (const auto& entry : trace) {
        std::string desc = entry.description();
        std::string file = entry.source_file();
        uint_least32_t line = entry.source_line();
        std::print("  {} ({}:{})\n", desc, file, line);
    }
}

// === skip parameter (lignes 145-156) ===
void log_error(const std::string& message) {
    auto trace = std::stacktrace::current(1);
    std::print(stderr, "ERREUR: {}\nStack trace:\n{}\n", message, trace);
}

// === traced_error (lignes 175-211) ===
class traced_error : public std::runtime_error {
    std::stacktrace trace_;

public:
    explicit traced_error(const std::string& message)
        : std::runtime_error(message)
        , trace_(std::stacktrace::current(1))
    {}

    const std::stacktrace& trace() const noexcept { return trace_; }

    std::string full_report() const {
        return std::format("Error: {}\nStack trace:\n{}", what(), trace_);
    }
};

void validate(int value) {
    if (value < 0) {
        throw traced_error(std::format("Valeur negative: {}", value));
    }
}

void test_traced_error() {
    std::print("=== traced_error ===\n");
    try {
        validate(-1);
    } catch (const traced_error& e) {
        std::print("{}\n", e.full_report());
    }
}

// === assert with trace (lignes 287-306) ===
void assert_that(bool condition,
                 const char* expr,
                 std::source_location loc = std::source_location::current()) {
    if (!condition) {
        auto trace = std::stacktrace::current(1);
        std::print(stderr,
            "Assertion failed: {}\n"
            "  at {}:{}:{}\n"
            "Stack trace:\n{}\n",
            expr, loc.file_name(), loc.line(), loc.column(), trace);
    }
}

#define ASSERT(expr) assert_that(static_cast<bool>(expr), #expr)

void test_assert() {
    std::print("=== assert with trace ===\n");
    int* ptr = nullptr;
    ASSERT(ptr != nullptr);
}

int main() {
    std::print("=== basic capture ===\n");
    middle_function();

    test_format_integration();
    test_anatomy();
    test_entry_details();

    std::print("=== skip parameter ===\n");
    log_error("quelque chose a mal tourne");

    test_traced_error();
    test_assert();
}
