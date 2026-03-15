/* ============================================================================
   Section 24.1.3 : Sérialisation d'objets C++
   Description : to_json/from_json, macros NLOHMANN_DEFINE_TYPE, champs
                 optionnels, enum, types imbriqués, héritage, round-trip
   Fichier source : 01.3-serialisation.md
   ============================================================================ */
#include <nlohmann/json.hpp>
#include <print>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cassert>

using json = nlohmann::json;

// === to_json / from_json manuelles (l.27-47) ===
struct Endpoint {
    std::string path;
    std::string method;
    int timeout_ms;
};

void to_json(json& j, const Endpoint& e) {
    j = json{{"path", e.path}, {"method", e.method}, {"timeout_ms", e.timeout_ms}};
}

void from_json(const json& j, Endpoint& e) {
    j.at("path").get_to(e.path);
    j.at("method").get_to(e.method);
    j.at("timeout_ms").get_to(e.timeout_ms);
}

// === Macro NON_INTRUSIVE (l.84-92) ===
struct DatabaseConfig {
    std::string host;
    int port;
    std::string database;
    bool ssl;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DatabaseConfig, host, port, database, ssl)

// === Macro NON_INTRUSIVE_WITH_DEFAULT (l.102-110) ===
struct DatabaseConfigDefault {
    std::string host = "localhost";
    int port = 5432;
    std::string database = "app";
    bool ssl = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DatabaseConfigDefault,
    host, port, database, ssl)

// === Macro INTRUSIVE (l.131-146) ===
class UserSession {
    std::string token_;
    std::string username_;
    int64_t expires_at_;
public:
    UserSession() = default;
    UserSession(std::string token, std::string user, int64_t exp)
        : token_(std::move(token)), username_(std::move(user)), expires_at_(exp) {}
    const std::string& token() const { return token_; }
    const std::string& username() const { return username_; }
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(UserSession, token_, username_, expires_at_)
};

// === Renommage de clés (l.173-191) ===
struct ServiceHealth {
    std::string service_name;
    bool is_healthy;
    double response_time_ms;
};

void to_json(json& j, const ServiceHealth& s) {
    j = json{{"serviceName", s.service_name}, {"healthy", s.is_healthy},
             {"responseTimeMs", s.response_time_ms}};
}
void from_json(const json& j, ServiceHealth& s) {
    j.at("serviceName").get_to(s.service_name);
    j.at("healthy").get_to(s.is_healthy);
    j.at("responseTimeMs").get_to(s.response_time_ms);
}

// === Champs optionnels (l.199-237) ===
struct DeploymentSpec {
    std::string image;
    int replicas;
    std::optional<std::string> namespace_override;
    std::optional<int> max_memory_mb;
};

template <typename T>
void optional_from_json(const json& j, const std::string& key,
                        std::optional<T>& target) {
    if (auto it = j.find(key); it != j.end() && !it->is_null()) {
        target = it->get<T>();
    } else {
        target = std::nullopt;
    }
}

void to_json(json& j, const DeploymentSpec& d) {
    j = json{{"image", d.image}, {"replicas", d.replicas}};
    if (d.namespace_override) j["namespace"] = *d.namespace_override;
    if (d.max_memory_mb) j["maxMemoryMb"] = *d.max_memory_mb;
}
void from_json(const json& j, DeploymentSpec& d) {
    j.at("image").get_to(d.image);
    j.at("replicas").get_to(d.replicas);
    optional_from_json(j, "namespace", d.namespace_override);
    optional_from_json(j, "maxMemoryMb", d.max_memory_mb);
}

// === Enum (l.289-305) ===
enum class LogLevel { trace, debug, info, warn, error, fatal };
NLOHMANN_JSON_SERIALIZE_ENUM(LogLevel, {
    {LogLevel::trace, "trace"}, {LogLevel::debug, "debug"},
    {LogLevel::info,  "info"},  {LogLevel::warn,  "warn"},
    {LogLevel::error, "error"}, {LogLevel::fatal, "fatal"},
})

struct LogConfig {
    std::string output;
    LogLevel level;
    bool colorize;
};
void to_json(json& j, const LogConfig& c) {
    j = json{{"output", c.output}, {"level", c.level}, {"colorize", c.colorize}};
}
void from_json(const json& j, LogConfig& c) {
    j.at("output").get_to(c.output);
    j.at("level").get_to(c.level);
    j.at("colorize").get_to(c.colorize);
}

// === Types imbriqués (l.345-377) ===
struct TlsConfig {
    std::string cert_path;
    std::string key_path;
    bool verify_client = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TlsConfig, cert_path, key_path, verify_client)

struct ServerConfig {
    std::string host;
    int port;
    std::optional<TlsConfig> tls;
    std::vector<Endpoint> endpoints;
};
void to_json(json& j, const ServerConfig& s) {
    j = json{{"host", s.host}, {"port", s.port}, {"endpoints", s.endpoints}};
    if (s.tls) j["tls"] = *s.tls;
}
void from_json(const json& j, ServerConfig& s) {
    j.at("host").get_to(s.host);
    j.at("port").get_to(s.port);
    j.at("endpoints").get_to(s.endpoints);
    optional_from_json(j, "tls", s.tls);
}

// === Héritage (l.412-447) ===
struct Resource {
    std::string id;
    std::string name;
    std::string created_at;
};
void to_json(json& j, const Resource& r) {
    j = json{{"id", r.id}, {"name", r.name}, {"created_at", r.created_at}};
}
void from_json(const json& j, Resource& r) {
    j.at("id").get_to(r.id); j.at("name").get_to(r.name);
    j.at("created_at").get_to(r.created_at);
}

struct Volume : Resource {
    int size_gb;
    std::string mount_path;
};
void to_json(json& j, const Volume& v) {
    to_json(j, static_cast<const Resource&>(v));
    j["size_gb"] = v.size_gb;
    j["mount_path"] = v.mount_path;
}
void from_json(const json& j, Volume& v) {
    from_json(j, static_cast<Resource&>(v));
    j.at("size_gb").get_to(v.size_gb);
    j.at("mount_path").get_to(v.mount_path);
}

int main() {
    // === to_json / from_json (l.52-67) ===
    std::print("=== Endpoint to_json/from_json ===\n");
    {
        Endpoint ep{"/api/users", "GET", 5000};
        json j = ep;
        std::print("{}\n", j.dump(2));

        auto ep2 = json::parse(R"({"path": "/health", "method": "GET", "timeout_ms": 1000})")
                        .get<Endpoint>();
        std::print("{} {} ({}ms)\n", ep2.method, ep2.path, ep2.timeout_ms);
    }

    // === Macro NON_INTRUSIVE ===
    std::print("\n=== DatabaseConfig NON_INTRUSIVE ===\n");
    {
        DatabaseConfig db{"db.prod", 5432, "mydb", true};
        json j = db;
        std::print("{}\n", j.dump());
        auto db2 = j.get<DatabaseConfig>();
        std::print("host={}\n", db2.host);
    }

    // === Macro WITH_DEFAULT (l.113-122) ===
    std::print("\n=== DatabaseConfig WITH_DEFAULT ===\n");
    {
        auto cfg = json::parse(R"({"host": "db.prod.internal", "ssl": true})")
                       .get<DatabaseConfigDefault>();
        std::print("host={}, port={}, db={}, ssl={}\n",
                   cfg.host, cfg.port, cfg.database, cfg.ssl);
    }

    // === Macro INTRUSIVE ===
    std::print("\n=== UserSession INTRUSIVE ===\n");
    {
        UserSession s("abc123", "alice", 1710000000);
        json j = s;
        std::print("{}\n", j.dump());
        auto s2 = j.get<UserSession>();
        std::print("user={}\n", s2.username());
    }

    // === Renommage de clés ===
    std::print("\n=== ServiceHealth renommage ===\n");
    {
        ServiceHealth h{"api", true, 12.5};
        json j = h;
        std::print("{}\n", j.dump());
    }

    // === Champs optionnels (l.241-257) ===
    std::print("\n=== DeploymentSpec optionnels ===\n");
    {
        auto spec = json::parse(R"({"image": "nginx:1.27", "replicas": 3})")
                        .get<DeploymentSpec>();
        std::print("ns={}, mem={}\n",
                   spec.namespace_override.has_value(),
                   spec.max_memory_mb.has_value());

        auto spec2 = json::parse(R"({
            "image": "api:2.1.0", "replicas": 5,
            "namespace": "staging", "maxMemoryMb": 512
        })").get<DeploymentSpec>();
        std::print("ns={}, mem={}\n",
                   *spec2.namespace_override, *spec2.max_memory_mb);
    }

    // === Enum (l.322-333) ===
    std::print("\n=== LogConfig enum ===\n");
    {
        auto cfg = json::parse(R"({"output": "stdout", "level": "warn", "colorize": true})")
                       .get<LogConfig>();
        std::print("level warn={}\n", cfg.level == LogLevel::warn);
        json j = cfg;
        std::print("{}\n", j.dump(2));
    }

    // === Types imbriqués (l.383-398) ===
    std::print("\n=== ServerConfig imbriqué ===\n");
    {
        auto server = json::parse(R"({
            "host": "0.0.0.0", "port": 443,
            "tls": {"cert_path": "/etc/ssl/server.crt", "key_path": "/etc/ssl/server.key", "verify_client": false},
            "endpoints": [
                {"path": "/health", "method": "GET", "timeout_ms": 1000},
                {"path": "/api/data", "method": "POST", "timeout_ms": 5000}
            ]
        })").get<ServerConfig>();
        std::print("{}:{}, tls={}, endpoints={}\n",
                   server.host, server.port, server.tls.has_value(),
                   server.endpoints.size());
    }

    // === Héritage ===
    std::print("\n=== Volume héritage ===\n");
    {
        Volume v;
        v.id = "vol-123"; v.name = "data"; v.created_at = "2026-03-15";
        v.size_gb = 100; v.mount_path = "/mnt/data";
        json j = v;
        std::print("{}\n", j.dump(2));
        auto v2 = j.get<Volume>();
        std::print("id={}, size={}\n", v2.id, v2.size_gb);
    }

    // === Round-trip (l.601-619) ===
    std::print("\n=== Round-trip ===\n");
    {
        ServerConfig original{
            .host = "0.0.0.0", .port = 443,
            .tls = TlsConfig{"/etc/ssl/cert.pem", "/etc/ssl/key.pem", true},
            .endpoints = {{"/health", "GET", 1000}, {"/api", "POST", 5000}}
        };
        json j = original;
        auto restored = j.get<ServerConfig>();
        assert(original.host == restored.host);
        assert(original.port == restored.port);
        assert(original.tls->cert_path == restored.tls->cert_path);
        assert(original.endpoints.size() == restored.endpoints.size());
        std::print("Round-trip OK\n");
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
