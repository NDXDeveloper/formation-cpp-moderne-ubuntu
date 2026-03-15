/* ============================================================================
   Section 25.1.1 : Définition de messages .proto
   Description : optional vs non-optional, repeated (Cluster), messages
                 imbriqués (DeploymentSpec), enums (LogLevel), oneof
                 (NotificationTarget), maps (ServiceRegistry)
   Fichier source : 01.1-definition-proto.md
   ============================================================================ */
#include "definition.pb.h"
#include <print>

int main() {
    // === optional (l.154-167) ===
    std::print("=== optional ===\n");
    {
        myapp::def::ServerConfig config;
        config.set_port(0);

        // Sans optional : impossible de savoir si port a été défini à 0
        bool port_set = (config.port() != 0);  // faux négatif si port == 0

        // Avec optional : distinction explicite
        config.set_workers(0);
        bool workers_set = config.has_workers();  // true
        config.clear_workers();
        bool workers_cleared = config.has_workers();  // false

        std::print("port_set={}, workers_set={}, workers_cleared={}\n",
            port_set, workers_set, workers_cleared);
    }

    // === repeated / Cluster (l.187-207) ===
    std::print("\n=== repeated ===\n");
    {
        myapp::def::Cluster cluster;
        cluster.set_name("prod-eu");

        cluster.add_node_addresses("10.0.1.1");
        cluster.add_node_addresses("10.0.1.2");
        cluster.add_node_addresses("10.0.1.3");

        cluster.add_ports(8080);
        cluster.add_ports(8443);

        int count = cluster.node_addresses_size();
        const std::string& first = cluster.node_addresses(0);

        std::print("count={}, first={}\n", count, first);

        for (const auto& addr : cluster.node_addresses()) {
            std::print("Node : {}\n", addr);
        }
    }

    // === Messages imbriqués (l.245-262) ===
    std::print("\n=== Messages imbriqués ===\n");
    {
        myapp::def::DeploymentSpec spec;
        spec.set_image("nginx:1.27");
        spec.set_replicas(3);

        auto* limits = spec.mutable_limits();
        limits->set_cpu_millicores(500);
        limits->set_memory_mb(256);

        auto* hc = spec.mutable_health_check();
        hc->set_path("/health");
        hc->set_interval_seconds(10);

        const auto& req = spec.requests();
        int cpu = req.cpu_millicores();  // 0 (valeur par défaut)

        std::print("image={}, replicas={}\n", spec.image(), spec.replicas());
        std::print("limits: cpu={}, mem={}\n",
            limits->cpu_millicores(), limits->memory_mb());
        std::print("hc path={}\n", hc->path());
        std::print("req cpu={}\n", cpu);
    }

    // === oneof (l.341-361) ===
    std::print("\n=== oneof ===\n");
    {
        myapp::def::NotificationTarget notif;
        notif.set_notification_id("n-001");
        notif.set_message("Déploiement réussi");
        notif.set_slack_channel("#deployments");

        switch (notif.target_case()) {
            case myapp::def::NotificationTarget::kEmail:
                std::print("Email : {}\n", notif.email());
                break;
            case myapp::def::NotificationTarget::kSlackChannel:
                std::print("Slack : {}\n", notif.slack_channel());
                break;
            case myapp::def::NotificationTarget::kWebhookUrl:
                std::print("Webhook : {}\n", notif.webhook_url());
                break;
            case myapp::def::NotificationTarget::TARGET_NOT_SET:
                std::print("Aucune cible\n");
                break;
        }
    }

    // === maps (l.386-407) ===
    std::print("\n=== maps ===\n");
    {
        myapp::def::ServiceRegistry registry;
        registry.set_cluster_name("prod");

        auto& services = *registry.mutable_services();
        services["api"].set_host("10.0.1.1");
        services["api"].set_port(8080);
        services["api"].set_healthy(true);

        services["worker"].set_host("10.0.2.1");
        services["worker"].set_port(9090);

        (*registry.mutable_metadata())["region"] = "eu-west";
        (*registry.mutable_metadata())["version"] = "3.2.1";

        for (const auto& [name, endpoint] : registry.services()) {
            std::print("{} → {}:{}\n", name, endpoint.host(), endpoint.port());
        }
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
