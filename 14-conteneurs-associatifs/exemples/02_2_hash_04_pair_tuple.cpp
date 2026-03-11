/* ============================================================================
   Section 14.2.2 : Hash pour std::pair et std::tuple
   Description : PairHash, TupleHash avec hash_combine, Employee avec hash
   Fichier source : 02.2-custom-hash.md
   ============================================================================ */
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <tuple>
#include <print>
#include <cstddef>

namespace util {

inline void hash_combine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 12) + (seed >> 4);
}

template <typename T, typename... Rest>
void hash_combine(std::size_t& seed, const T& first, const Rest&... rest) {
    hash_combine(seed, std::hash<T>{}(first));
    (hash_combine(seed, std::hash<Rest>{}(rest)), ...);
}

template <typename... Args>
std::size_t combined_hash(const Args&... args) {
    std::size_t seed = 0;
    hash_combine(seed, args...);
    return seed;
}

} // namespace util

// Hash pour std::pair
struct PairHash {
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const noexcept {
        return util::combined_hash(p.first, p.second);
    }
};

// Hash pour std::tuple
struct TupleHash {
    template <typename... Ts>
    std::size_t operator()(const std::tuple<Ts...>& t) const noexcept {
        return std::apply([](const auto&... args) {
            return util::combined_hash(args...);
        }, t);
    }
};

// Hash pour un type avec des champs complexes
struct Employee {
    std::string name;
    std::string department;
    int id;
    bool operator==(const Employee&) const = default;
};

template <>
struct std::hash<Employee> {
    std::size_t operator()(const Employee& e) const noexcept {
        return util::combined_hash(e.name, e.department, e.id);
    }
};

int main() {
    // Test PairHash
    std::unordered_map<std::pair<int, int>, std::string, PairHash> grid;
    grid[{0, 0}] = "origin";
    grid[{1, 2}] = "point A";
    std::println("grid size = {}", grid.size());

    // Test TupleHash
    using LocationKey = std::tuple<std::string, std::string, int>;
    std::unordered_map<LocationKey, double, TupleHash> data;
    data[{"75", "Paris", 75000}] = 2.161;
    std::println("data size = {}", data.size());

    // Test Employee
    std::unordered_set<Employee> team;
    team.insert({"Alice", "Engineering", 1});
    team.insert({"Bob", "Marketing", 2});
    std::println("team size = {}", team.size());
}
