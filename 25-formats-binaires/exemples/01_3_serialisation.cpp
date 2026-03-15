/* ============================================================================
   Section 25.1.3 : Sérialisation / Désérialisation
   Description : Sérialisation vers chaîne/fichier/buffer, sérialisation
                 partielle, désérialisation depuis chaîne/fichier/buffer,
                 parsing sécurisé (CodedInputStream), streaming delimited,
                 framing manuel, Text Format, JSON interop
   Fichier source : 01.3-serialisation.md
   ============================================================================ */
#include "config.pb.h"
#include "user.pb.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/util/delimited_message_util.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include <print>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include <cstring>

// === Framing manuel (l.283-321) ===
bool write_framed(std::ostream& out, const google::protobuf::MessageLite& msg) {
    std::string data;
    if (!msg.SerializeToString(&data)) return false;

    uint32_t size = static_cast<uint32_t>(data.size());
    uint8_t header[4] = {
        static_cast<uint8_t>((size >> 24) & 0xFF),
        static_cast<uint8_t>((size >> 16) & 0xFF),
        static_cast<uint8_t>((size >> 8) & 0xFF),
        static_cast<uint8_t>(size & 0xFF)
    };
    out.write(reinterpret_cast<char*>(header), 4);
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    return out.good();
}

bool read_framed(std::istream& in, google::protobuf::MessageLite& msg) {
    uint8_t header[4];
    in.read(reinterpret_cast<char*>(header), 4);
    if (in.gcount() != 4) return false;

    uint32_t size = (static_cast<uint32_t>(header[0]) << 24)
                  | (static_cast<uint32_t>(header[1]) << 16)
                  | (static_cast<uint32_t>(header[2]) << 8)
                  | static_cast<uint32_t>(header[3]);

    if (size > 64 * 1024 * 1024) return false;

    std::string data(size, '\0');
    in.read(data.data(), size);
    if (static_cast<uint32_t>(in.gcount()) != size) return false;

    return msg.ParseFromString(data);
}

// === Pattern robuste pour données non fiables (l.175-203) ===
bool parse_untrusted(const std::string& data, myapp::ServerConfig& config,
                     int max_size_bytes = 1024 * 1024,
                     int max_recursion_depth = 32) {
    google::protobuf::io::ArrayInputStream raw_input(
        data.data(), static_cast<int>(data.size()));
    google::protobuf::io::CodedInputStream input(&raw_input);

    input.SetTotalBytesLimit(max_size_bytes);
    input.SetRecursionLimit(max_recursion_depth);

    if (!config.ParseFromCodedStream(&input)) {
        return false;
    }
    if (!input.ConsumedEntireMessage()) {
        config.Clear();
        return false;
    }
    return true;
}

int main() {
    // === Sérialisation vers chaîne (l.21-37) ===
    std::print("=== Sérialisation vers chaîne ===\n");
    std::string binary;
    {
        myapp::ServerConfig config;
        config.set_host("0.0.0.0");
        config.set_port(8080);
        config.set_workers(4);
        config.add_allowed_origins("https://app.example.com");

        if (!config.SerializeToString(&binary)) {
            std::print(stderr, "Échec de la sérialisation\n");
        }
        std::print("Sérialisé : {} octets\n", binary.size());
    }

    // === Sérialisation vers fichier (l.46-58) ===
    std::print("\n=== Sérialisation vers fichier ===\n");
    {
        myapp::ServerConfig config;
        config.set_host("0.0.0.0");
        config.set_port(8080);

        std::ofstream file("config_test.pb", std::ios::binary);
        if (!config.SerializeToOstream(&file)) {
            std::print(stderr, "Échec de l'écriture\n");
        }
        file.close();
        std::print("Fichier écrit\n");
    }

    // === Sérialisation vers buffer (l.67-80) ===
    std::print("\n=== Sérialisation vers buffer ===\n");
    {
        myapp::ServerConfig config;
        config.set_host("0.0.0.0");
        config.set_port(8080);

        size_t size = config.ByteSizeLong();
        std::vector<uint8_t> buffer(size);
        if (!config.SerializeToArray(buffer.data(),
                                     static_cast<int>(buffer.size()))) {
            std::print(stderr, "Échec sérialisation buffer\n");
        }
        std::print("Buffer : {} octets\n", buffer.size());
    }

    // === Sérialisation partielle (l.89-91) ===
    std::print("\n=== Sérialisation partielle ===\n");
    {
        myapp::ServerConfig config;
        config.set_host("test");
        std::string data;
        config.SerializePartialToString(&data);
        std::print("Partial : {} octets\n", data.size());
    }

    // === Désérialisation depuis chaîne (l.102-111) ===
    std::print("\n=== Désérialisation depuis chaîne ===\n");
    {
        myapp::ServerConfig restored;
        if (!restored.ParseFromString(binary)) {
            std::print(stderr, "Données binaires invalides\n");
            return 1;
        }
        std::print("Host : {}\n", restored.host());
        std::print("Port : {}\n", restored.port());
    }

    // === Désérialisation depuis fichier (l.117-128) ===
    std::print("\n=== Désérialisation depuis fichier ===\n");
    {
        myapp::ServerConfig config;
        std::ifstream file("config_test.pb", std::ios::binary);
        if (!config.ParseFromIstream(&file)) {
            std::print(stderr, "Fichier Protobuf invalide\n");
            return 1;
        }
        std::print("Host : {}\n", config.host());
    }

    // === Désérialisation depuis buffer (l.133-139) ===
    std::print("\n=== Désérialisation depuis buffer ===\n");
    {
        myapp::ServerConfig config;
        if (!config.ParseFromArray(binary.data(),
                                   static_cast<int>(binary.size()))) {
            std::print(stderr, "Buffer invalide\n");
            return 1;
        }
        std::print("Host : {}\n", config.host());
    }

    // === Parsing sécurisé (l.175-203) ===
    std::print("\n=== Parsing sécurisé ===\n");
    {
        myapp::ServerConfig config;
        if (parse_untrusted(binary, config)) {
            std::print("OK : {}\n", config.host());
        }
    }

    // === Streaming delimited (l.233-274) ===
    std::print("\n=== Streaming delimited ===\n");
    {
        const std::string path = "users_test.pb";

        // Écriture
        {
            std::ofstream file(path, std::ios::binary);
            google::protobuf::io::OstreamOutputStream raw_output(&file);

            for (int i = 0; i < 3; ++i) {
                myapp::User user;
                user.set_id(i);
                user.set_name("User" + std::to_string(i));
                google::protobuf::util::SerializeDelimitedToZeroCopyStream(
                    user, &raw_output);
            }
        }

        // Lecture
        {
            std::ifstream file(path, std::ios::binary);
            google::protobuf::io::IstreamInputStream raw_input(&file);

            bool clean_eof = false;
            int count = 0;
            while (true) {
                myapp::User user;
                if (!google::protobuf::util::ParseDelimitedFromZeroCopyStream(
                        &user, &raw_input, &clean_eof)) {
                    break;
                }
                std::print("  id={}, name={}\n", user.id(), user.name());
                ++count;
            }
            std::print("Lu {} messages, clean_eof={}\n", count, clean_eof);
        }
        std::remove(path.c_str());
    }

    // === Framing manuel (l.283-321) ===
    std::print("\n=== Framing manuel ===\n");
    {
        std::stringstream stream(std::ios::binary | std::ios::in
                                 | std::ios::out);

        for (int i = 0; i < 3; ++i) {
            myapp::User user;
            user.set_id(i + 1);
            user.set_name("User" + std::to_string(i + 1));
            write_framed(stream, user);
        }

        stream.seekg(0);
        int count = 0;
        while (true) {
            myapp::User user;
            if (!read_framed(stream, user)) break;
            std::print("  id={}, name={}\n", user.id(), user.name());
            ++count;
        }
        std::print("Lu {} messages\n", count);
    }

    // === Text Format (l.519-534) ===
    std::print("\n=== Text Format ===\n");
    {
        myapp::ServerConfig config;
        config.set_host("0.0.0.0");
        config.set_port(8080);
        config.set_workers(4);

        std::string text;
        google::protobuf::TextFormat::PrintToString(config, &text);
        std::print("Text format :\n{}", text);

        myapp::ServerConfig parsed;
        if (!google::protobuf::TextFormat::ParseFromString(text, &parsed)) {
            std::print(stderr, "Text format invalide\n");
        }
        std::print("Parsed : {}:{}\n", parsed.host(), parsed.port());
    }

    // === JSON interop (l.494-512) ===
    std::print("\n=== JSON interop ===\n");
    {
        myapp::ServerConfig config;
        config.set_host("0.0.0.0");
        config.set_port(8080);

        std::string json_response;
        google::protobuf::util::MessageToJsonString(config, &json_response);
        std::print("JSON : {}\n", json_response);
    }

    // Nettoyage
    std::remove("config_test.pb");

    std::print("\nTous les tests passent !\n");
    return 0;
}
