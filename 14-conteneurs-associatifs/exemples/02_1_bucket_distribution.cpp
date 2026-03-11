/* ============================================================================
   Section 14.2.1 : Visualiser la distribution des buckets
   Description : Diagnostic de la distribution des éléments dans les buckets
   Fichier source : 02.1-hash-tables.md
   ============================================================================ */
#include <unordered_map>
#include <string>
#include <print>

template <typename Map>
void print_bucket_distribution(const Map& m) {
    std::size_t empty = 0, single = 0, multi = 0, max_len = 0;

    for (std::size_t i = 0; i < m.bucket_count(); ++i) {
        auto len = m.bucket_size(i);
        if (len == 0) ++empty;
        else if (len == 1) ++single;
        else ++multi;
        if (len > max_len) max_len = len;
    }

    std::print("Buckets: {} total, {} vides, {} simples, {} multiples\n",
               m.bucket_count(), empty, single, multi);
    std::print("Plus long bucket: {} éléments\n", max_len);
    std::print("Load factor: {:.3f}\n", m.load_factor());
}

int main() {
    std::unordered_map<std::string, int> m;
    for (int i = 0; i < 100; ++i) {
        m.emplace("key_" + std::to_string(i), i);
    }
    print_bucket_distribution(m);
}
