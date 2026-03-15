/* ============================================================================
   Section 25.3 : MessagePack : JSON binaire compact
   Description : Sérialisation/désérialisation de base (primitifs, vector,
                 maps), types utilisateur (MSGPACK_DEFINE, MSGPACK_DEFINE_MAP),
                 inspection dynamique, streaming (unpacker), gestion erreurs,
                 limites de sécurité, réutilisation sbuffer
   Fichier source : 03-messagepack.md
   ============================================================================ */
#include <msgpack.hpp>
#include <print>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstring>

// === Types utilisateur avec MSGPACK_DEFINE (l.221-245) ===
struct Endpoint {
    std::string path;
    std::string method;
    int timeout_ms;
    MSGPACK_DEFINE(path, method, timeout_ms);
};

struct ServerConfig {
    std::string host;
    int port;
    int workers;
    std::vector<Endpoint> endpoints;
    std::optional<std::string> description;
    MSGPACK_DEFINE(host, port, workers, endpoints, description);
};

// === MSGPACK_DEFINE_MAP (l.284-292) ===
struct ServerConfigMap {
    std::string host;
    int port;
    int workers;
    MSGPACK_DEFINE_MAP(host, port, workers);
};

// === Enum non intrusif (l.307-309) ===
enum class LogLevel { trace, debug, info, warn, error, fatal };
MSGPACK_ADD_ENUM(LogLevel);

int main() {
    // === Types primitifs et STL (l.118-144) ===
    std::print("=== Types primitifs ===\n");
    {
        msgpack::sbuffer buffer;
        msgpack::packer<msgpack::sbuffer> packer(buffer);

        packer.pack(42);
        packer.pack(std::string("hello"));
        packer.pack(3.14);
        packer.pack(true);

        std::print("Sérialisé : {} octets\n", buffer.size());

        msgpack::object_handle oh =
            msgpack::unpack(buffer.data(), buffer.size());
        msgpack::object obj = oh.get();
        std::print("Type  : {}\n", static_cast<int>(obj.type));
        std::print("Value : {}\n", obj.as<int>());
    }

    // === Vector (l.149-163) ===
    std::print("\n=== Vector ===\n");
    {
        std::vector<int> scores = {95, 87, 72, 100};

        msgpack::sbuffer buffer;
        msgpack::pack(buffer, scores);

        auto handle = msgpack::unpack(buffer.data(), buffer.size());
        auto restored = handle.get().as<std::vector<int>>();

        for (int s : restored) {
            std::print("{} ", s);
        }
        std::print("\n");
    }

    // === Maps (l.168-186) ===
    std::print("\n=== Maps ===\n");
    {
        std::map<std::string, std::string> metadata = {
            {"region", "eu-west"},
            {"env", "production"},
            {"version", "3.2.1"}
        };

        msgpack::sbuffer buffer;
        msgpack::pack(buffer, metadata);

        auto handle = msgpack::unpack(buffer.data(), buffer.size());
        auto restored =
            handle.get().as<std::map<std::string, std::string>>();

        for (const auto& [key, value] : restored) {
            std::print("{} = {}\n", key, value);
        }
    }

    // === Types utilisateur MSGPACK_DEFINE (l.248-276) ===
    std::print("\n=== ServerConfig (MSGPACK_DEFINE) ===\n");
    {
        ServerConfig config{
            .host = "0.0.0.0",
            .port = 8080,
            .workers = 4,
            .endpoints = {
                {"/health", "GET", 1000},
                {"/api/data", "POST", 5000}
            },
            .description = "Production API server"
        };

        msgpack::sbuffer buffer;
        msgpack::pack(buffer, config);
        std::print("Taille : {} octets\n", buffer.size());

        auto handle = msgpack::unpack(buffer.data(), buffer.size());
        auto restored = handle.get().as<ServerConfig>();

        std::print("{}:{} ({} workers)\n",
            restored.host, restored.port, restored.workers);
        for (const auto& ep : restored.endpoints) {
            std::print("  {} {} ({}ms)\n", ep.method, ep.path, ep.timeout_ms);
        }
    }

    // === MSGPACK_DEFINE_MAP (l.284-292) ===
    std::print("\n=== ServerConfigMap (MSGPACK_DEFINE_MAP) ===\n");
    {
        ServerConfigMap config{
            .host = "localhost", .port = 9090, .workers = 2};

        msgpack::sbuffer buffer;
        msgpack::pack(buffer, config);

        auto handle = msgpack::unpack(buffer.data(), buffer.size());
        auto restored = handle.get().as<ServerConfigMap>();
        std::print("{}:{}\n", restored.host, restored.port);
    }

    // === Inspection dynamique (l.318-366) ===
    std::print("\n=== Inspection dynamique ===\n");
    {
        msgpack::sbuffer buffer;
        msgpack::pack(buffer, std::string("hello world"));

        auto handle = msgpack::unpack(buffer.data(), buffer.size());
        msgpack::object obj = handle.get();

        switch (obj.type) {
            case msgpack::type::POSITIVE_INTEGER:
                std::print("Entier : {}\n", obj.as<int64_t>());
                break;
            case msgpack::type::STR:
                std::print("Chaîne : {}\n", obj.as<std::string>());
                break;
            case msgpack::type::ARRAY: {
                std::print("Tableau ({} éléments)\n", obj.via.array.size);
                break;
            }
            case msgpack::type::MAP: {
                std::print("Map ({} entrées)\n", obj.via.map.size);
                break;
            }
            case msgpack::type::BOOLEAN:
                std::print("Booléen : {}\n", obj.as<bool>());
                break;
            case msgpack::type::NIL:
                std::print("Null\n");
                break;
            case msgpack::type::FLOAT32:
            case msgpack::type::FLOAT64:
                std::print("Flottant : {}\n", obj.as<double>());
                break;
            default:
                std::print("Type inconnu : {}\n",
                    static_cast<int>(obj.type));
        }
    }

    // === Streaming (l.379-420) ===
    std::print("\n=== Streaming ===\n");
    {
        msgpack::sbuffer buffer;
        msgpack::packer<msgpack::sbuffer> packer(buffer);

        packer.pack(std::string("event_a"));
        packer.pack(42);
        packer.pack(std::map<std::string, int>{{"x", 1}, {"y", 2}});

        // Lecture séquentielle avec unpacker
        msgpack::unpacker unpacker;
        unpacker.reserve_buffer(buffer.size());
        std::memcpy(unpacker.buffer(), buffer.data(), buffer.size());
        unpacker.buffer_consumed(buffer.size());

        msgpack::object_handle handle;
        while (unpacker.next(handle)) {
            msgpack::object obj = handle.get();
            std::print("Message type={} : ", static_cast<int>(obj.type));

            switch (obj.type) {
                case msgpack::type::STR:
                    std::print("{}\n", obj.as<std::string>());
                    break;
                case msgpack::type::POSITIVE_INTEGER:
                    std::print("{}\n", obj.as<int>());
                    break;
                case msgpack::type::MAP:
                    std::print("(map, {} entrées)\n", obj.via.map.size);
                    break;
                default:
                    std::print("(autre)\n");
            }
        }
    }

    // === Gestion des erreurs (l.489-507) ===
    std::print("\n=== Gestion erreurs ===\n");
    {
        // type_error
        msgpack::sbuffer buffer;
        msgpack::pack(buffer, std::string("hello"));
        try {
            auto handle = msgpack::unpack(buffer.data(), buffer.size());
            int value = handle.get().as<int>();
            (void)value;
        } catch (const msgpack::type_error& e) {
            std::print("type_error : {}\n", e.what());
        }

        // unpack_error
        std::string corrupted = "bad";
        try {
            auto handle = msgpack::unpack(corrupted.data(),
                                           corrupted.size());
        } catch (const msgpack::unpack_error& e) {
            std::print("unpack_error : {}\n", e.what());
        }
    }

    // === Limites de sécurité (l.516-528) ===
    std::print("\n=== Limites de sécurité ===\n");
    {
        msgpack::sbuffer buffer;
        msgpack::pack(buffer, std::vector<int>{1, 2, 3});

        msgpack::unpack_limit limits(
            1000,       // taille max des tableaux
            1000,       // taille max des maps
            1024*1024,  // taille max des chaînes (1 Mo)
            1024*1024,  // taille max des binaires (1 Mo)
            1024*1024,  // taille max des extensions (1 Mo)
            64          // profondeur max de récursion
        );

        auto handle = msgpack::unpack(
            buffer.data(), buffer.size(),
            nullptr,    // unpack_reference_func
            nullptr,    // user_data
            limits      // limites de décodage
        );

        auto vec = handle.get().as<std::vector<int>>();
        std::print("Avec limites : {} éléments\n", vec.size());
    }

    // === Optimisation : réutiliser sbuffer (l.548-557) ===
    std::print("\n=== Réutilisation sbuffer ===\n");
    {
        msgpack::sbuffer buffer;
        for (int i = 0; i < 3; ++i) {
            buffer.clear();
            msgpack::pack(buffer, i * 10);
            auto handle = msgpack::unpack(buffer.data(), buffer.size());
            std::print("  {}\n", handle.get().as<int>());
        }
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
