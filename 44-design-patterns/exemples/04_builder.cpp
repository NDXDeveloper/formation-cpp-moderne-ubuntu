/* ============================================================================
   Section 44.1.3 : Builder
   Description : Builder fluent avec validation dans build()
   Fichier source : 01.3-builder.md
   ============================================================================ */

#include <chrono>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>

class HttpServer {
public:
    class Builder;

    void start() { std::print("Server started on {}:{}\n", config_.host, config_.port); }

    std::string_view host() const { return config_.host; }
    uint16_t port() const { return config_.port; }
    bool tls_enabled() const { return config_.enable_tls; }

private:
    struct Config {
        std::string host            = "0.0.0.0";
        uint16_t    port            = 8080;
        int         thread_count    = 0;
        bool        enable_tls      = false;
        std::string cert_path;
        std::string key_path;
        bool        enable_compression = false;
        std::size_t max_body_size   = 65536;
        int         timeout_seconds = 30;
    };

    explicit HttpServer(Config config) : config_(std::move(config)) {}
    Config config_;
    friend class Builder;
};

class HttpServer::Builder {
public:
    Builder& host(std::string h)   { config_.host = std::move(h); return *this; }
    Builder& port(uint16_t p)      { config_.port = p; return *this; }
    Builder& threads(int n)        { config_.thread_count = n; return *this; }
    Builder& tls(std::string cert, std::string key) {
        config_.enable_tls = true;
        config_.cert_path = std::move(cert);
        config_.key_path = std::move(key);
        return *this;
    }
    Builder& compression(bool e = true) { config_.enable_compression = e; return *this; }
    Builder& max_body_size(std::size_t s) { config_.max_body_size = s; return *this; }
    Builder& timeout(std::chrono::seconds d) {
        config_.timeout_seconds = static_cast<int>(d.count());
        return *this;
    }

    HttpServer build() {
        if (config_.port == 0)
            throw std::invalid_argument("Port cannot be 0");
        if (config_.enable_tls && (config_.cert_path.empty() || config_.key_path.empty()))
            throw std::invalid_argument("TLS requires cert and key paths");
        return HttpServer(std::move(config_));
    }

private:
    HttpServer::Config config_;
};

int main() {
    auto server = HttpServer::Builder()
        .port(9090)
        .threads(8)
        .tls("/etc/certs/server.crt", "/etc/certs/server.key")
        .compression()
        .timeout(std::chrono::seconds(60))
        .build();

    server.start();
    std::print("TLS: {}\n", server.tls_enabled());

    try {
        HttpServer::Builder().port(0).build();
    } catch (const std::exception& e) {
        std::print("Validation error: {}\n", e.what());
    }
}
