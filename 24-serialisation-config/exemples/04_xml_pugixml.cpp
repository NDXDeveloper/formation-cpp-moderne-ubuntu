/* ============================================================================
   Section 24.4 : XML : Parsing avec pugixml (legacy systems)
   Description : Parsing fichier/chaîne, navigation DOM, attributs, contenu
                 textuel, itération enfants/attributs, XPath, conversion types
                 métier, écriture XML, modification document, namespaces,
                 gestion des erreurs
   Fichier source : 04-xml-pugixml.md
   ============================================================================ */
#include <pugixml.hpp>
#include <print>
#include <string>
#include <vector>
#include <optional>
#include <sstream>

// === Types métier (l.433-453) ===
struct Endpoint {
    std::string path;
    std::string method;
    int timeout_ms;
};

struct DatabaseConfig {
    std::string host;
    int port;
    std::string username;
    std::string password;
};

struct ServerConfig {
    std::string name;
    std::string host;
    int port;
    int workers;
    DatabaseConfig database;
    std::vector<Endpoint> endpoints;
};

Endpoint parse_endpoint(pugi::xml_node node) {
    return Endpoint{
        .path = node.attribute("path").as_string(),
        .method = node.attribute("method").as_string(),
        .timeout_ms = node.attribute("timeout").as_int(5000)
    };
}

DatabaseConfig parse_database(pugi::xml_node node) {
    return DatabaseConfig{
        .host = node.child("host").text().as_string(),
        .port = node.child("port").text().as_int(5432),
        .username = node.child("credentials").attribute("username").as_string(),
        .password = node.child("credentials").attribute("password").as_string()
    };
}

std::optional<ServerConfig> parse_server_config(pugi::xml_document& doc) {
    pugi::xml_node server = doc.child("server");
    if (!server) {
        std::print(stderr, "Élément <server> manquant\n");
        return std::nullopt;
    }

    ServerConfig cfg;
    cfg.name    = server.attribute("name").as_string("unnamed");
    cfg.host    = server.child("host").text().as_string("localhost");
    cfg.port    = server.child("port").text().as_int(8080);
    cfg.workers = server.child("workers").text().as_int(4);

    if (pugi::xml_node db = server.child("database")) {
        cfg.database = parse_database(db);
    }

    for (auto ep : server.child("endpoints").children("endpoint")) {
        cfg.endpoints.push_back(parse_endpoint(ep));
    }

    return cfg;
}

// === Émission depuis types métier (l.597-620) ===
void emit_endpoint(pugi::xml_node parent, const Endpoint& ep) {
    pugi::xml_node node = parent.append_child("endpoint");
    node.append_attribute("path") = ep.path.c_str();
    node.append_attribute("method") = ep.method.c_str();
    node.append_attribute("timeout") = ep.timeout_ms;
}

void emit_server_config(pugi::xml_document& doc, const ServerConfig& cfg) {
    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    pugi::xml_node server = doc.append_child("server");
    server.append_attribute("name") = cfg.name.c_str();

    server.append_child("host").text().set(cfg.host.c_str());
    server.append_child("port").text().set(cfg.port);
    server.append_child("workers").text().set(cfg.workers);

    pugi::xml_node endpoints = server.append_child("endpoints");
    for (const auto& ep : cfg.endpoints) {
        emit_endpoint(endpoints, ep);
    }
}

int main() {
    // === Parsing fichier (l.169-187) ===
    std::print("=== Parsing fichier ===\n");
    {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file("config.xml");
        if (!result) {
            std::print(stderr, "Erreur XML : {}\n", result.description());
            return 1;
        }

        pugi::xml_node server = doc.child("server");
        std::print("Serveur : {}\n", server.attribute("name").as_string());
        std::print("Port    : {}\n", server.child("port").text().as_int());
    }

    // === Parsing chaîne (l.193-203) ===
    std::print("\n=== Parsing chaîne ===\n");
    {
        const char* xml_content = R"(
<config>
    <name>test</name>
    <value>42</value>
</config>
)";
        pugi::xml_document doc;
        doc.load_string(xml_content);
        std::print("name={}, value={}\n",
                   doc.child("config").child("name").text().as_string(),
                   doc.child("config").child("value").text().as_int());
    }

    // === Navigation DOM (l.254-268) ===
    std::print("\n=== Navigation DOM ===\n");
    {
        pugi::xml_document doc;
        doc.load_file("config.xml");

        pugi::xml_node server = doc.child("server");
        pugi::xml_node host = server.child("host");
        pugi::xml_node db = server.child("database");

        if (!server.child("nonexistent")) {
            std::print("Nœud absent\n");
        }
        std::print("host={}\n", host.text().as_string());
    }

    // === Attributs (l.274-289) ===
    std::print("\n=== Attributs ===\n");
    {
        pugi::xml_document doc;
        doc.load_file("config.xml");

        pugi::xml_node server = doc.child("server");
        std::string name = server.attribute("name").as_string();
        std::string version = server.attribute("version").as_string();
        int timeout = server.attribute("timeout").as_int(30000);
        std::print("name={}, version={}, timeout={}\n", name, version, timeout);
    }

    // === Contenu textuel (l.294-304) ===
    std::print("\n=== Contenu textuel ===\n");
    {
        pugi::xml_document doc;
        doc.load_file("config.xml");

        pugi::xml_node server = doc.child("server");
        std::string host = server.child("host").text().as_string();
        int port = server.child("port").text().as_int();
        int workers = server.child("workers").text().as_int(4);
        std::print("host={}, port={}, workers={}\n", host, port, workers);
    }

    // === Itération enfants (l.310-329) ===
    std::print("\n=== Itération enfants ===\n");
    {
        pugi::xml_document doc;
        doc.load_file("config.xml");

        pugi::xml_node endpoints = doc.child("server").child("endpoints");
        for (pugi::xml_node ep : endpoints.children("endpoint")) {
            std::string path = ep.attribute("path").as_string();
            std::string method = ep.attribute("method").as_string();
            int timeout = ep.attribute("timeout").as_int(5000);
            std::print("{} {} (timeout: {}ms)\n", method, path, timeout);
        }

        // Tous les enfants
        std::print("Enfants de <server> :\n");
        for (pugi::xml_node child : doc.child("server").children()) {
            std::print("  Élément : {}\n", child.name());
        }
    }

    // === Itération attributs (l.334-339) ===
    std::print("\n=== Itération attributs ===\n");
    {
        pugi::xml_document doc;
        doc.load_file("config.xml");

        pugi::xml_node server = doc.child("server");
        for (pugi::xml_attribute attr : server.attributes()) {
            std::print("  {} = {}\n", attr.name(), attr.as_string());
        }
    }

    // === XPath (l.360-386) ===
    std::print("\n=== XPath ===\n");
    {
        pugi::xml_document doc;
        doc.load_file("config.xml");

        // Tous les endpoints GET
        pugi::xpath_node_set get_endpoints =
            doc.select_nodes("//endpoint[@method='GET']");
        for (const auto& xnode : get_endpoints) {
            std::print("GET {}\n",
                xnode.node().attribute("path").as_string());
        }

        // Valeur unique
        pugi::xpath_node result =
            doc.select_node("/server/database/host");
        if (result) {
            std::print("DB host : {}\n", result.node().text().as_string());
        }

        // count()
        pugi::xpath_query count_query("count(//endpoint)");
        double endpoint_count = count_query.evaluate_number(doc);
        std::print("Nombre d'endpoints : {}\n", static_cast<int>(endpoint_count));
    }

    // === Conversion types métier (l.455-512) ===
    std::print("\n=== ServerConfig ===\n");
    {
        pugi::xml_document doc;
        auto result = doc.load_file("config.xml");
        if (!result) {
            std::print(stderr, "XML invalide : {}\n", result.description());
            return 1;
        }

        auto config = parse_server_config(doc);
        if (!config) return 1;

        std::print("Serveur '{}' sur {}:{}\n",
            config->name, config->host, config->port);
        std::print("DB : {}:{}\n",
            config->database.host, config->database.port);
        std::print("Endpoints : {}\n", config->endpoints.size());
    }

    // === Écriture XML (l.524-558) ===
    std::print("\n=== Écriture XML ===\n");
    {
        pugi::xml_document doc;

        pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
        decl.append_attribute("version") = "1.0";
        decl.append_attribute("encoding") = "UTF-8";

        pugi::xml_node server = doc.append_child("server");
        server.append_attribute("name") = "api-prod";
        server.append_child("host").text().set("0.0.0.0");
        server.append_child("port").text().set(8080);
        server.append_child("workers").text().set(4);

        pugi::xml_node db = server.append_child("database");
        db.append_child("host").text().set("db.internal");
        db.append_child("port").text().set(5432);

        pugi::xml_node endpoints = server.append_child("endpoints");
        pugi::xml_node ep1 = endpoints.append_child("endpoint");
        ep1.append_attribute("path") = "/health";
        ep1.append_attribute("method") = "GET";
        ep1.append_attribute("timeout") = 1000;

        std::ostringstream stream;
        doc.save(stream, "  ", pugi::format_indent);
        std::print("{}\n", stream.str());
    }

    // === Émission depuis types métier (l.597-620) ===
    std::print("\n=== Émission ServerConfig ===\n");
    {
        ServerConfig cfg{
            .name = "test-server",
            .host = "localhost",
            .port = 9090,
            .workers = 2,
            .database = {},
            .endpoints = {{"/health", "GET", 1000}, {"/api", "POST", 5000}}
        };

        pugi::xml_document doc;
        emit_server_config(doc, cfg);

        std::ostringstream stream;
        doc.save(stream, "  ", pugi::format_indent);
        std::print("{}\n", stream.str());
    }

    // === Modification document (l.631-654) ===
    std::print("\n=== Modification document ===\n");
    {
        pugi::xml_document doc;
        doc.load_file("config.xml");

        pugi::xml_node server = doc.child("server");
        server.child("workers").text().set(16);

        pugi::xml_node ep = server.child("endpoints").append_child("endpoint");
        ep.append_attribute("path") = "/metrics";
        ep.append_attribute("method") = "GET";
        ep.append_attribute("timeout") = 2000;

        server.attribute("version").set_value("3.3");

        // Vérifier les modifications
        std::print("workers={}\n", server.child("workers").text().as_int());
        std::print("version={}\n", server.attribute("version").as_string());

        int ep_count = 0;
        for (auto e : server.child("endpoints").children("endpoint")) {
            ++ep_count;
        }
        std::print("endpoints={}\n", ep_count);
    }

    // === Namespaces (l.665-697) ===
    std::print("\n=== Namespaces ===\n");
    {
        const char* xml = R"(
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/">
    <s:Body>
        <GetUserResponse xmlns="http://example.com/api">
            <User>
                <Name>Alice</Name>
                <Email>alice@example.com</Email>
            </User>
        </GetUserResponse>
    </s:Body>
</s:Envelope>
)";
        pugi::xml_document doc;
        doc.load_string(xml);

        pugi::xml_node body = doc.child("s:Envelope").child("s:Body");
        pugi::xml_node response = body.child("GetUserResponse");
        pugi::xml_node user = response.child("User");

        std::string name = user.child("Name").text().as_string();
        std::string email = user.child("Email").text().as_string();
        std::print("name={}, email={}\n", name, email);

        // XPath avec local-name()
        auto xpath_user = doc.select_node("//*[local-name()='User']");
        if (xpath_user) {
            std::print("XPath User : {}\n",
                xpath_user.node().child("Name").text().as_string());
        }
    }

    // === Gestion des erreurs (l.705-734) ===
    std::print("\n=== Gestion des erreurs ===\n");
    {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_string("<broken>xml");
        if (!result) {
            std::print(stderr, "Erreur XML : {}\n", result.description());
            std::print(stderr, "  Offset  : {}\n", result.offset);
        }

        // Nœud manquant → valeur par défaut
        pugi::xml_document doc2;
        doc2.load_string("<root/>");
        pugi::xml_node server = doc2.child("server");
        if (!server) {
            std::print("Élément <server> absent\n");
        }
    }

    // === XPath erreur (l.403-424) ===
    std::print("\n=== XPath erreur ===\n");
    {
        pugi::xml_document doc;
        doc.load_string("<root/>");
        try {
            auto nodes = doc.select_nodes("///invalid[xpath");
        } catch (const pugi::xpath_exception& e) {
            std::print(stderr, "XPath invalide : {}\n", e.what());
        }
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
