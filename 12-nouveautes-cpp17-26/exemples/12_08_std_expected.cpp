/* ============================================================================
   Section 12.8 : std::expected (C++23)
   Description : Gestion d'erreurs sans exceptions - expected basique,
                 API (has_value, value, error, value_or), and_then pipeline,
                 transform, or_else, transform_error, expected<void, E>
   Fichier source : 08-std-expected.md
   ============================================================================ */
#include <expected>
#include <string>
#include <print>
#include <charconv>
#include <format>

using namespace std::string_literals;

// === Erreur enum (lignes 88-92 et 197) ===
enum class ParseError { InvalidFormat, OutOfRange, FileNotFound };

// === read_file simulee (lignes 94-105) ===
std::expected<std::string, ParseError> read_file(const std::string& path) {
    if (path == "/etc/port.conf") {
        return "8080"s;
    }
    return std::unexpected(ParseError::FileNotFound);
}

// === parse_int (lignes 203-210) ===
std::expected<int, ParseError> parse_int(const std::string& s) {
    int value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc{}) {
        return std::unexpected(ParseError::InvalidFormat);
    }
    return value;
}

// === validate_port (lignes 212-217) ===
std::expected<int, ParseError> validate_port(int port) {
    if (port < 1 || port > 65535) {
        return std::unexpected(ParseError::OutOfRange);
    }
    return port;
}

// === expected<void, E> (lignes 367-383) ===
enum class FileError { NotFound, PermissionDenied, IoError };

std::expected<void, FileError> write_file(const std::string& path,
                                           const std::string& content) {
    if (path.empty()) {
        return std::unexpected(FileError::PermissionDenied);
    }
    // Simulation d'ecriture reussie
    (void)content;
    return {};
}

int main() {
    // --- expected basique (lignes 127-134) ---
    std::print("=== expected basic ===\n");
    auto result = read_file("/etc/port.conf");
    if (result) {
        std::print("Contenu : {}\n", *result);
    } else {
        std::print("Erreur : {}\n", static_cast<int>(result.error()));
    }

    auto result2 = read_file("/missing");
    if (result2) {
        std::print("Contenu : {}\n", *result2);
    } else {
        std::print("Erreur : {}\n", static_cast<int>(result2.error()));
    }

    // --- API (lignes 140-145) ---
    std::print("\n=== expected API ===\n");
    std::print("has_value: {}\n", result.has_value());
    std::print("value: {}\n", result.value());
    std::print("value_or: {}\n", result2.value_or("default"));

    // --- and_then pipeline (lignes 220-222) ---
    std::print("\n=== and_then pipeline ===\n");
    auto port = read_file("/etc/port.conf")
        .and_then(parse_int)
        .and_then(validate_port);
    if (port) {
        std::print("port = {}\n", *port);
    }

    // Pipeline avec fichier manquant
    auto port2 = read_file("/missing")
        .and_then(parse_int)
        .and_then(validate_port);
    if (!port2) {
        std::print("pipeline erreur: {}\n", static_cast<int>(port2.error()));
    }

    // --- transform (lignes 248-253) ---
    std::print("\n=== transform ===\n");
    auto port_str = read_file("/etc/port.conf")
        .and_then(parse_int)
        .and_then(validate_port)
        .transform([](int p) {
            return std::format(":{}", p);
        });
    if (port_str) {
        std::print("port_str = {}\n", *port_str);
    }

    // --- or_else (lignes 265-272) ---
    std::print("\n=== or_else ===\n");
    auto port_default = read_file("/missing")
        .and_then(parse_int)
        .or_else([](ParseError e) -> std::expected<int, ParseError> {
            if (e == ParseError::FileNotFound) {
                return 8080;
            }
            return std::unexpected(e);
        });
    std::print("port with fallback = {}\n", *port_default);

    // --- transform_error (lignes 280-287) ---
    std::print("\n=== transform_error ===\n");
    enum class AppError { ConfigError, NetworkError };
    auto app_result = read_file("/missing")
        .transform_error([](ParseError) -> AppError {
            return AppError::ConfigError;
        });
    if (!app_result) {
        std::print("app error: {}\n", static_cast<int>(app_result.error()));
    }

    // --- expected<void, E> (lignes 380-383) ---
    std::print("\n=== expected<void, E> ===\n");
    if (auto wr = write_file("/tmp/output.txt", "data"); !wr) {
        std::print("Erreur d'ecriture : {}\n", static_cast<int>(wr.error()));
    } else {
        std::print("Ecriture reussie\n");
    }
    if (auto wr = write_file("", "data"); !wr) {
        std::print("Erreur d'ecriture : {}\n", static_cast<int>(wr.error()));
    }
}
