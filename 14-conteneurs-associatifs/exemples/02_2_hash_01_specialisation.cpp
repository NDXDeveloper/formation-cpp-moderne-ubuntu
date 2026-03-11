/* ============================================================================
   Section 14.2.2 : Spécialiser std::hash
   Description : Hash personnalisé pour un type utilisateur via std::hash
   Fichier source : 02.2-custom-hash.md
   ============================================================================ */
#include <functional>
#include <unordered_map>
#include <string>
#include <print>

struct Coordinate {
    int x;
    int y;

    bool operator==(const Coordinate&) const = default; // C++20
};

// Spécialisation de std::hash pour Coordinate
template <>
struct std::hash<Coordinate> {
    std::size_t operator()(const Coordinate& c) const noexcept {
        std::size_t h1 = std::hash<int>{}(c.x);
        std::size_t h2 = std::hash<int>{}(c.y);
        // Combinaison des deux hash
        return h1 ^ (h2 << 1);
    }
};

int main() {
    // Fonctionne directement — std::hash<Coordinate> est trouvé automatiquement
    std::unordered_map<Coordinate, std::string> labels;
    labels[{10, 20}] = "Point A";
    labels[{30, 40}] = "Point B";

    for (const auto& [coord, name] : labels) {
        std::println("({}, {}) = {}", coord.x, coord.y, name);
    }
}
