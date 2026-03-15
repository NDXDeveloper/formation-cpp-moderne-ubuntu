/* ============================================================================
   Section 21.6 : Thread-safety - Pattern RCU (Read-Copy-Update)
   Description : Table de routage avec lecteurs sans verrou et écritures par copie
   Fichier source : 06-thread-safety.md
   ============================================================================ */

#include <atomic>
#include <memory>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <print>

class RoutingTable {
    // Lecteurs accèdent sans verrou via le shared_ptr
    std::atomic<std::shared_ptr<const std::map<std::string, std::string>>> table_;

public:
    RoutingTable()
        : table_(std::make_shared<const std::map<std::string, std::string>>()) {}

    // Lecture — zéro contention
    std::string lookup(const std::string& key) const {
        auto snapshot = table_.load();  // Copie du shared_ptr (atomique)
        auto it = snapshot->find(key);
        return (it != snapshot->end()) ? it->second : "";
    }

    // Écriture — crée une nouvelle copie
    void update(const std::string& key, const std::string& value) {
        auto old = table_.load();
        auto updated = std::make_shared<std::map<std::string, std::string>>(*old);
        (*updated)[key] = value;
        table_.store(std::move(updated));
    }
};

int main() {
    RoutingTable routes;

    // Écrivain : met à jour les routes
    routes.update("/api/users", "service-users:8080");
    routes.update("/api/orders", "service-orders:8081");
    routes.update("/api/auth", "service-auth:8082");

    // Lecteurs concurrents
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&routes, i] {
            std::string result = routes.lookup("/api/users");
            std::println("Reader {} : /api/users → {}", i, result);
        });
    }

    for (auto& t : readers) t.join();

    // Mise à jour pendant que des lecteurs pourraient lire
    routes.update("/api/users", "service-users-v2:9090");
    std::println("Après mise à jour : /api/users → {}",
                 routes.lookup("/api/users"));
    std::println("Route inexistante : '{}'", routes.lookup("/api/unknown"));
}
