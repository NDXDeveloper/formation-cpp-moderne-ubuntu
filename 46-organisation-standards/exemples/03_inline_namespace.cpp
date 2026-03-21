/* ============================================================================
   Section 46.3 : Namespaces et eviter la pollution globale
   Description : Inline namespaces pour le versioning d'API (C++11)
   Fichier source : 03-namespaces.md
   ============================================================================ */

#include <string>
#include <iostream>

namespace monprojet {

namespace v1 {
    struct Config {
        std::string host;
        int port;
    };
}

inline namespace v2 {
    struct Config {
        std::string host;
        int port;
        int max_connections = 100;
        bool tls_enabled = false;
    };
}

} // namespace monprojet

int main() {
    // Par defaut → v2 (grace au inline)
    monprojet::Config cfg{"localhost", 8080};
    std::cout << "Default (v2): " << cfg.host << ":" << cfg.port
              << " max_conn=" << cfg.max_connections << "\n";

    // Acces explicite a v1
    monprojet::v1::Config legacy{"old-server", 80};
    std::cout << "Legacy (v1): " << legacy.host << ":" << legacy.port << "\n";

    // Acces explicite a v2
    monprojet::v2::Config explicit_v2{"new-server", 443, 500, true};
    std::cout << "Explicit v2: " << explicit_v2.host
              << " tls=" << explicit_v2.tls_enabled << "\n";
}
