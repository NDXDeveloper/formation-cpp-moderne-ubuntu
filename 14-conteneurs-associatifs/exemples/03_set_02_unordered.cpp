/* ============================================================================
   Section 14.3 : std::unordered_set
   Description : Ensemble non ordonné, hash personnalisé, reserve
   Fichier source : 03-set-unordered-set.md
   ============================================================================ */
#include <unordered_set>
#include <string>
#include <functional>
#include <print>

struct Point {
    double x, y;
    bool operator==(const Point&) const = default;
};

struct PointHash {
    std::size_t operator()(const Point& p) const noexcept {
        std::size_t seed = 0;
        seed ^= std::hash<double>{}(p.x) + 0x9e3779b97f4a7c15ULL
                + (seed << 12) + (seed >> 4);
        seed ^= std::hash<double>{}(p.y) + 0x9e3779b97f4a7c15ULL
                + (seed << 12) + (seed >> 4);
        return seed;
    }
};

int main() {
    // Construction et opérations de base
    std::unordered_set<std::string> visited {"Paris", "Lyon", "Marseille"};

    visited.insert("Toulouse");
    visited.insert("Paris"); // Ignoré — déjà présent

    if (visited.contains("Lyon")) {
        std::print("Lyon déjà visitée\n");
    }

    std::println("visited size = {}", visited.size());

    std::println("---");

    // Type personnalisé avec hash
    std::unordered_set<Point, PointHash> cloud;
    cloud.insert({1.0, 2.5});
    cloud.insert({3.7, 4.2});
    std::println("cloud size = {}", cloud.size());

    std::println("---");

    // reserve
    std::unordered_set<int> ids;
    ids.reserve(100'000); // Évite les rehashings pendant le remplissage
    for (int i = 0; i < 100'000; ++i) {
        ids.insert(i);
    }
    std::println("ids size = {}", ids.size());
}
