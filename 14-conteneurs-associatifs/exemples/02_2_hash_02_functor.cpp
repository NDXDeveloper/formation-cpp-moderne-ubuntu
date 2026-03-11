/* ============================================================================
   Section 14.2.2 : Hash functor externe
   Description : Struct callable et lambda comme hash functor
   Fichier source : 02.2-custom-hash.md
   ============================================================================ */
#include <functional>
#include <unordered_map>
#include <string>
#include <print>

struct Coordinate {
    int x;
    int y;
    bool operator==(const Coordinate&) const = default;
};

// Struct callable externe
struct CoordinateHash {
    std::size_t operator()(const Coordinate& c) const noexcept {
        std::size_t h1 = std::hash<int>{}(c.x);
        std::size_t h2 = std::hash<int>{}(c.y);
        return h1 ^ (h2 << 1);
    }
};

int main() {
    // Avec un struct callable
    std::unordered_map<Coordinate, std::string, CoordinateHash> labels;
    labels[{10, 20}] = "Point A";
    labels[{30, 40}] = "Point B";

    for (const auto& [coord, name] : labels) {
        std::println("({}, {}) = {}", coord.x, coord.y, name);
    }

    std::println("---");

    // Avec une lambda passée au constructeur
    auto hasher = [](const Coordinate& c) {
        return std::hash<int>{}(c.x) ^ (std::hash<int>{}(c.y) << 1);
    };

    std::unordered_map<Coordinate, std::string, decltype(hasher)> labels2(16, hasher);
    labels2[{5, 10}] = "Point C";
    std::println("labels2 size = {}", labels2.size());
}
