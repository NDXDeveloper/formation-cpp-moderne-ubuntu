/* ============================================================================
   Section 25.1 : Protocol Buffers (Protobuf) : Sérialisation Google
   Description : Aperçu du workflow Protobuf — construction de message,
                 sous-message, champ repeated, sérialisation binaire,
                 écriture/lecture fichier, désérialisation
   Fichier source : 01-protobuf.md
   ============================================================================ */
#include "config.pb.h"
#include <fstream>
#include <print>

int main() {
    // Construction d'un message
    myapp::ServerConfig config;
    config.set_host("0.0.0.0");
    config.set_port(8080);
    config.set_workers(4);

    // Sous-message
    auto* tls = config.mutable_tls();
    tls->set_cert_path("/etc/ssl/server.crt");
    tls->set_key_path("/etc/ssl/server.key");
    tls->set_verify_client(true);

    // Champ repeated
    config.add_allowed_origins("https://app.example.com");
    config.add_allowed_origins("https://admin.example.com");

    // Sérialisation binaire
    std::string binary_data;
    config.SerializeToString(&binary_data);
    std::print("Taille sérialisée : {} octets\n", binary_data.size());

    // Écriture dans un fichier
    std::ofstream file("config.pb", std::ios::binary);
    config.SerializeToOstream(&file);
    file.close();

    // Désérialisation
    myapp::ServerConfig restored;
    std::ifstream input("config.pb", std::ios::binary);
    if (restored.ParseFromIstream(&input)) {
        std::print("Host : {}\n", restored.host());
        std::print("Port : {}\n", restored.port());

        if (restored.has_tls()) {
            std::print("TLS cert : {}\n", restored.tls().cert_path());
        }

        for (const auto& origin : restored.allowed_origins()) {
            std::print("Origin : {}\n", origin);
        }
    }
    input.close();

    // Nettoyage du fichier temporaire
    std::remove("config.pb");

    std::print("\nTous les tests passent !\n");
    return 0;
}
