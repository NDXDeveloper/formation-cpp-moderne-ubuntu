/* ============================================================================
   Section 14.2.2 : hash_combine et utilitaire variadic
   Description : Technique Boost, version 64 bits, combined_hash variadic
   Fichier source : 02.2-custom-hash.md
   ============================================================================ */
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <print>
#include <cstddef>

// Utilitaire hash_combine réutilisable
namespace util {

inline void hash_combine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 12) + (seed >> 4);
}

template <typename T, typename... Rest>
void hash_combine(std::size_t& seed, const T& first, const Rest&... rest) {
    hash_combine(seed, std::hash<T>{}(first));
    (hash_combine(seed, std::hash<Rest>{}(rest)), ...); // Fold expression C++17
}

// Helper : retourne directement le hash combiné
template <typename... Args>
std::size_t combined_hash(const Args&... args) {
    std::size_t seed = 0;
    hash_combine(seed, args...);
    return seed;
}

} // namespace util

// Utilisation avec Coordinate et Coordinate3D
struct Coordinate {
    int x, y;
    bool operator==(const Coordinate&) const = default;
};

struct Coordinate3D {
    int x, y, z;
    bool operator==(const Coordinate3D&) const = default;
};

template <>
struct std::hash<Coordinate> {
    std::size_t operator()(const Coordinate& c) const noexcept {
        return util::combined_hash(c.x, c.y);
    }
};

template <>
struct std::hash<Coordinate3D> {
    std::size_t operator()(const Coordinate3D& c) const noexcept {
        return util::combined_hash(c.x, c.y, c.z);
    }
};

int main() {
    std::unordered_map<Coordinate, std::string> map2d;
    map2d[{1, 2}] = "A";
    map2d[{3, 4}] = "B";
    std::println("map2d size = {}", map2d.size());

    std::unordered_set<Coordinate3D> set3d;
    set3d.insert({1, 2, 3});
    set3d.insert({4, 5, 6});
    std::println("set3d size = {}", set3d.size());

    // Vérifier la non-commutativité
    auto h1 = util::combined_hash(3, 7);
    auto h2 = util::combined_hash(7, 3);
    auto h3 = util::combined_hash(5, 5);
    std::println("hash(3,7) != hash(7,3) : {}", h1 != h2);
    std::println("hash(5,5) != 0 : {}", h3 != 0);
}
