/* ============================================================================
   Section 25.3 : MessagePack : JSON binaire compact
   Description : Conversion MessagePack ↔ JSON — convertisseur récursif
                 msgpack::object → nlohmann::json, tous types MessagePack
   Fichier source : 03-messagepack.md
   ============================================================================ */
#include <msgpack.hpp>
#include <nlohmann/json.hpp>
#include <print>
#include <map>
#include <string>

using json = nlohmann::json;

// Conversion récursive : msgpack::object → nlohmann::json (l.438-471)
json msgpack_to_json(const msgpack::object& obj) {
    switch (obj.type) {
        case msgpack::type::NIL:
            return nullptr;
        case msgpack::type::BOOLEAN:
            return obj.as<bool>();
        case msgpack::type::POSITIVE_INTEGER:
            return obj.as<uint64_t>();
        case msgpack::type::NEGATIVE_INTEGER:
            return obj.as<int64_t>();
        case msgpack::type::FLOAT32:
        case msgpack::type::FLOAT64:
            return obj.as<double>();
        case msgpack::type::STR:
            return obj.as<std::string>();
        case msgpack::type::ARRAY: {
            json arr = json::array();
            for (uint32_t i = 0; i < obj.via.array.size; ++i) {
                arr.push_back(msgpack_to_json(obj.via.array.ptr[i]));
            }
            return arr;
        }
        case msgpack::type::MAP: {
            json map = json::object();
            for (uint32_t i = 0; i < obj.via.map.size; ++i) {
                auto& kv = obj.via.map.ptr[i];
                map[kv.key.as<std::string>()] = msgpack_to_json(kv.val);
            }
            return map;
        }
        default:
            return nullptr;
    }
}

int main() {
    std::print("=== MessagePack → JSON ===\n");

    // Créer des données MessagePack (map avec types variés)
    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> packer(buffer);
    packer.pack_map(4);
    packer.pack(std::string("name"));
    packer.pack(std::string("alice"));
    packer.pack(std::string("age"));
    packer.pack(30);
    packer.pack(std::string("active"));
    packer.pack(true);
    packer.pack(std::string("scores"));
    packer.pack(std::vector<int>{95, 87, 72});

    auto handle = msgpack::unpack(buffer.data(), buffer.size());
    json j = msgpack_to_json(handle.get());
    std::print("{}\n", j.dump(2));

    // Vérifications
    assert(j["name"] == "alice");
    assert(j["age"] == 30);
    assert(j["active"] == true);
    assert(j["scores"].size() == 3);

    std::print("\nTous les tests passent !\n");
    return 0;
}
