/* ============================================================================
   Section 12.9.2 : Cas d'usage et limites - constexpr lookup table
   Description : Table de recherche constexpr avec std::array trie et
                 std::ranges::lower_bound, verifiee avec static_assert
   Fichier source : 09.2-cas-usage-limites.md
   ============================================================================ */
#include <algorithm>
#include <array>
#include <ranges>
#include <string_view>
#include <print>

// === Constexpr lookup table (lignes 134-163) ===
struct Entry {
    std::string_view key;
    int value;
};

constexpr std::array<Entry, 4> lookup_table = {{
    {"error",   3},
    {"info",    1},
    {"trace",   0},
    {"warning", 2},
}};

constexpr int log_level(std::string_view name) {
    auto it = std::ranges::lower_bound(
        lookup_table, name, {}, &Entry::key);
    if (it != lookup_table.end() && it->key == name) {
        return it->value;
    }
    return -1;
}

static_assert(log_level("info") == 1);
static_assert(log_level("unknown") == -1);

int main() {
    std::print("info={}\n", log_level("info"));       // 1
    std::print("error={}\n", log_level("error"));     // 3
    std::print("unknown={}\n", log_level("unknown")); // -1
    std::print("trace={}\n", log_level("trace"));     // 0
    std::print("warning={}\n", log_level("warning")); // 2
}
