/* ============================================================================
   Section 14.2 : Contrôle du comportement interne
   Description : Buckets, facteur de charge, reserve
   Fichier source : 02-unordered-map.md
   ============================================================================ */
#include <unordered_map>
#include <string>
#include <print>

int main() {
    // === Buckets ===
    std::unordered_map<std::string, int> m {
        {"a", 1}, {"b", 2}, {"c", 3}, {"d", 4}, {"e", 5}
    };

    std::print("Nombre de buckets : {}\n", m.bucket_count());
    std::print("Nombre d'éléments : {}\n", m.size());

    // Dans quel bucket se trouve une clé ?
    std::print("'c' est dans le bucket {}\n", m.bucket("c"));

    // Combien d'éléments dans un bucket donné ?
    for (std::size_t i = 0; i < m.bucket_count(); ++i) {
        if (m.bucket_size(i) > 0) {
            std::print("Bucket {} : {} éléments\n", i, m.bucket_size(i));
        }
    }

    std::println("---");

    // === Facteur de charge ===
    std::print("Load factor actuel : {:.2f}\n", m.load_factor());
    std::print("Load factor max    : {:.2f}\n", m.max_load_factor());

    m.max_load_factor(0.5f);
    m.rehash(100);   // Demande au moins 100 buckets
    m.reserve(1000); // Pré-alloue pour accueillir 1000 éléments sans rehashing
    std::println("After rehash/reserve: buckets={}", m.bucket_count());

    std::println("---");

    // === reserve — bonne pratique pour insertions en masse ===
    std::unordered_map<int, std::string> lookup;
    lookup.reserve(10'000); // Un seul rehashing au lieu de plusieurs

    for (int i = 0; i < 10'000; ++i) {
        lookup.emplace(i, std::to_string(i));
    }
    std::println("lookup: size={}, buckets={}", lookup.size(), lookup.bucket_count());
}
