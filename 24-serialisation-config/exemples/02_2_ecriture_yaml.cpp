/* ============================================================================
   Section 24.2.2 : Écriture YAML
   Description : Construction via YAML::Node, Dump, écriture fichier,
                 YAML::Emitter (base, séquences), styles flow/bloc,
                 indentation, quotation, multi-documents, helper RAII,
                 combinaison Node+Emitter
   Fichier source : 02.2-ecriture-yaml.md
   ============================================================================ */
#include <yaml-cpp/yaml.h>
#include <print>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// === YamlWriter helper RAII (l.443-497) ===
class YamlWriter {
    YAML::Emitter out_;
public:
    YamlWriter() { out_.SetIndent(2); }

    class MapScope {
        YAML::Emitter& out_;
    public:
        explicit MapScope(YAML::Emitter& out) : out_(out) {
            out_ << YAML::BeginMap;
        }
        ~MapScope() { out_ << YAML::EndMap; }
    };

    class SeqScope {
        YAML::Emitter& out_;
    public:
        explicit SeqScope(YAML::Emitter& out) : out_(out) {
            out_ << YAML::BeginSeq;
        }
        ~SeqScope() { out_ << YAML::EndSeq; }
    };

    [[nodiscard]] MapScope begin_map() { return MapScope(out_); }
    [[nodiscard]] SeqScope begin_seq() { return SeqScope(out_); }

    template <typename T>
    void field(const std::string& key, const T& value) {
        out_ << YAML::Key << key << YAML::Value << value;
    }

    void field_flow_seq(const std::string& key,
                        const std::vector<std::string>& values) {
        out_ << YAML::Key << key << YAML::Value;
        out_ << YAML::Flow << YAML::BeginSeq;
        for (const auto& v : values) {
            out_ << v;
        }
        out_ << YAML::EndSeq;
    }

    std::string str() const { return out_.c_str(); }
    bool good() const { return out_.good(); }
};

int main() {
    // === Construction via YAML::Node (l.22-48) ===
    std::print("=== Construction Node ===\n");
    {
        YAML::Node config;
        config["server"]["host"] = "0.0.0.0";
        config["server"]["port"] = 8080;
        config["server"]["workers"] = 4;
        config["server"]["allowed_origins"].push_back("https://app.example.com");
        config["server"]["allowed_origins"].push_back("https://admin.example.com");
        config["database"]["host"] = "db.internal";
        config["database"]["port"] = 5432;
        config["database"]["name"] = "production";
        config["database"]["ssl"] = true;

        std::print("{}\n", YAML::Dump(config));
    }

    // === Écriture vers fichier (l.70-87) ===
    std::print("\n=== Écriture fichier ===\n");
    {
        YAML::Node config;
        config["name"] = "test-app";
        config["version"] = "1.0";

        std::ofstream file("test_output.yaml");
        file << config;
        file.close();

        // Relecture pour vérification
        YAML::Node reloaded = YAML::LoadFile("test_output.yaml");
        std::print("Relu : name={}, version={}\n",
                   reloaded["name"].as<std::string>(),
                   reloaded["version"].as<std::string>());
        std::remove("test_output.yaml");
    }

    // === Emitter de base (l.121-153) ===
    std::print("\n=== Emitter base ===\n");
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "server" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "host" << YAML::Value << "0.0.0.0";
        out << YAML::Key << "port" << YAML::Value << 8080;
        out << YAML::Key << "workers" << YAML::Value << 4;
        out << YAML::EndMap;
        out << YAML::Key << "database" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "host" << YAML::Value << "db.internal";
        out << YAML::Key << "port" << YAML::Value << 5432;
        out << YAML::EndMap;
        out << YAML::EndMap;

        std::print("{}\n", out.c_str());
        if (!out.good()) {
            std::print(stderr, "Erreur d'émission : {}\n", out.GetLastError());
        }
    }

    // === Séquences (l.159-174) ===
    std::print("\n=== Séquences ===\n");
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "ports" << YAML::Value;
        out << YAML::BeginSeq;
        out << 8080 << 8443 << 9090;
        out << YAML::EndSeq;
        out << YAML::Key << "tags" << YAML::Value;
        out << YAML::BeginSeq;
        out << "production" << "eu-west" << "critical";
        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::print("{}\n", out.c_str());
    }

    // === Style flow vs bloc (l.213-248) ===
    std::print("\n=== Style flow vs bloc ===\n");
    {
        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "ports" << YAML::Value;
        out << YAML::Flow << YAML::BeginSeq;
        out << 8080 << 8443 << 9090;
        out << YAML::EndSeq;

        out << YAML::Key << "metadata" << YAML::Value;
        out << YAML::Flow << YAML::BeginMap;
        out << YAML::Key << "env" << YAML::Value << "prod";
        out << YAML::Key << "region" << YAML::Value << "eu-west";
        out << YAML::EndMap;

        out << YAML::Key << "features" << YAML::Value;
        out << YAML::Block << YAML::BeginSeq;
        out << "logging" << "metrics" << "tracing";
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::print("{}\n", out.c_str());
    }

    // === Indentation (l.254-276) ===
    std::print("\n=== Indentation 4 espaces ===\n");
    {
        YAML::Emitter out;
        out.SetIndent(4);
        out << YAML::BeginMap;
        out << YAML::Key << "server" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "host" << YAML::Value << "localhost";
        out << YAML::Key << "port" << YAML::Value << 8080;
        out << YAML::EndMap;
        out << YAML::EndMap;

        std::print("{}\n", out.c_str());
    }

    // === Quotation (l.280-311) ===
    std::print("\n=== Quotation ===\n");
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value;
        out << YAML::DoubleQuoted << "my-service";
        out << YAML::Key << "version" << YAML::Value;
        out << YAML::SingleQuoted << "1.0";
        out << YAML::Key << "description" << YAML::Value;
        out << YAML::Literal << "Première ligne\nDeuxième ligne\nTroisième ligne";
        out << YAML::EndMap;

        std::print("{}\n", out.c_str());
    }

    // === Quotation globale (l.316-337) ===
    std::print("\n=== Quotation globale ===\n");
    {
        YAML::Emitter out;
        out.SetStringFormat(YAML::DoubleQuoted);
        out << YAML::BeginMap;
        out << YAML::Key << "country" << YAML::Value << "NO";
        out << YAML::Key << "enabled" << YAML::Value << "yes";
        out << YAML::Key << "version" << YAML::Value << "1.0";
        out << YAML::EndMap;

        std::print("{}\n", out.c_str());
    }

    // === Multi-documents (l.380-413) ===
    std::print("\n=== Multi-documents ===\n");
    {
        YAML::Emitter out;

        out << YAML::BeginDoc;
        out << YAML::BeginMap;
        out << YAML::Key << "apiVersion" << YAML::Value << "apps/v1";
        out << YAML::Key << "kind" << YAML::Value << "Deployment";
        out << YAML::Key << "metadata" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << "api";
        out << YAML::EndMap;
        out << YAML::Key << "spec" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "replicas" << YAML::Value << 3;
        out << YAML::EndMap;
        out << YAML::EndMap;
        out << YAML::EndDoc;

        out << YAML::BeginDoc;
        out << YAML::BeginMap;
        out << YAML::Key << "apiVersion" << YAML::Value << "v1";
        out << YAML::Key << "kind" << YAML::Value << "Service";
        out << YAML::Key << "metadata" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << "api-svc";
        out << YAML::EndMap;
        out << YAML::EndMap;
        out << YAML::EndDoc;

        std::print("{}\n", out.c_str());
    }

    // === YamlWriter helper RAII (l.500-517) ===
    std::print("\n=== YamlWriter helper ===\n");
    {
        YamlWriter w;
        {
            auto root = w.begin_map();
            w.field("name", "my-service");
            w.field("version", "1.0.0");
            w.field("replicas", 3);
            w.field_flow_seq("tags", {"production", "eu-west"});
        }
        std::print("{}\n", w.str());
    }

    // === Combiner Node et Emitter (l.522-532) ===
    std::print("\n=== Node + Emitter ===\n");
    {
        YAML::Node deployment;
        deployment["apiVersion"] = "apps/v1";
        deployment["kind"] = "Deployment";
        deployment["metadata"]["name"] = "api";
        deployment["spec"]["replicas"] = 3;

        YAML::Emitter out;
        out.SetIndent(2);
        out << deployment;

        std::print("{}\n", out.c_str());
    }

    // === Vérification d'erreur d'émission (l.540-549) ===
    std::print("\n=== Vérification émission ===\n");
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "key" << YAML::Value << "value";
        out << YAML::EndMap;
        std::print("Émission valide : {}\n", out.good());
    }

    std::print("\nTous les tests passent !\n");
    return 0;
}
