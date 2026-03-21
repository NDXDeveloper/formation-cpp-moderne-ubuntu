/* ============================================================================
   Section 44.1.2 : Factory et Abstract Factory
   Description : Factory avec registry (auto-enregistrement dynamique)
   Fichier source : 01.2-factory.md
   ============================================================================ */

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <print>
#include <stdexcept>

class Compressor {
public:
    virtual ~Compressor() = default;
    virtual std::string name() const = 0;
};

class CompressorFactory {
public:
    using Creator = std::function<std::unique_ptr<Compressor>()>;

    static bool register_type(const std::string& name, Creator creator) {
        return registry().emplace(name, std::move(creator)).second;
    }

    static std::unique_ptr<Compressor> create(const std::string& name) {
        auto& reg = registry();
        auto it = reg.find(name);
        if (it == reg.end())
            throw std::invalid_argument("Unknown compressor: " + name);
        return it->second();
    }

    static std::vector<std::string> available() {
        std::vector<std::string> names;
        for (const auto& [name, _] : registry())
            names.push_back(name);
        return names;
    }

private:
    static std::unordered_map<std::string, Creator>& registry() {
        static std::unordered_map<std::string, Creator> reg;
        return reg;
    }
};

#define REGISTER_COMPRESSOR(Type, Name)                                  \
    static bool _reg_##Type = CompressorFactory::register_type(          \
        Name, []{ return std::make_unique<Type>(); })

class GzipCompressor : public Compressor {
public: std::string name() const override { return "gzip"; }
};
REGISTER_COMPRESSOR(GzipCompressor, "gzip");

class Lz4Compressor : public Compressor {
public: std::string name() const override { return "lz4"; }
};
REGISTER_COMPRESSOR(Lz4Compressor, "lz4");

class ZstdCompressor : public Compressor {
public: std::string name() const override { return "zstd"; }
};
REGISTER_COMPRESSOR(ZstdCompressor, "zstd");

int main() {
    for (const auto& algo : {"gzip", "lz4", "zstd"}) {
        auto c = CompressorFactory::create(algo);
        std::print("Created: {}\n", c->name());
    }

    try {
        CompressorFactory::create("brotli");
    } catch (const std::exception& e) {
        std::print("Error: {}\n", e.what());
    }
}
