/* ============================================================================
   Section 15.6 : Ranges (C++20) : Simplification des algorithmes
   Description : Exemples complets couvrant ranges::sort, projections (sort,
                 find, count_if, minmax_element), valeur de retour enrichie
                 (ranges::copy, ranges::remove), subrange, sentinelles
                 (NullTerminator, unreachable_sentinel), views filter+transform.
   Fichier source : 06-ranges.md
   ============================================================================ */

#include <ranges>
#include <algorithm>
#include <numeric>
#include <vector>
#include <string>
#include <print>

struct Server {
    std::string hostname;
    int cpu_load;
    int memory_mb;
};

std::vector<int> get_vector() { return {1, 2, 3, 42, 5}; }

int main() {
    // === Algorithme classique vs Ranges ===
    {
        std::vector<int> v = {5, 3, 8, 1, 9, 2, 7};
        std::ranges::sort(v);
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 2 3 5 7 8 9
    }

    // === Projection : sort par cpu_load ===
    {
        std::vector<Server> servers = {
            {"web-01", 72, 4096}, {"db-01", 91, 16384},
            {"cache-01", 23, 2048}, {"web-02", 45, 8192}
        };
        std::ranges::sort(servers, {}, &Server::cpu_load);
        for (const auto& s : servers) std::print("{}({}), ", s.hostname, s.cpu_load);
        std::println("");
        // cache-01(23), web-02(45), web-01(72), db-01(91),
    }

    // === Projection : sort décroissant par hostname ===
    {
        std::vector<Server> servers = {
            {"web-01", 72, 4096}, {"db-01", 91, 16384},
            {"cache-01", 23, 2048}, {"web-02", 45, 8192}
        };
        std::ranges::sort(servers, std::ranges::greater{}, &Server::hostname);
        for (const auto& s : servers) std::print("{}({}), ", s.hostname, s.cpu_load);
        std::println("");
        // web-02(45), web-01(72), db-01(91), cache-01(23),
    }

    // === Projection : sort par score composite ===
    {
        std::vector<Server> servers = {
            {"web-01", 72, 4096}, {"db-01", 91, 16384},
            {"cache-01", 23, 2048}, {"web-02", 45, 8192}
        };
        std::ranges::sort(servers, {}, [](const Server& s) {
            return s.cpu_load * 100 + s.memory_mb;
        });
        for (const auto& s : servers) std::print("{}({}/{}), ", s.hostname, s.cpu_load, s.memory_mb);
        std::println("");
    }

    // === Projection : find par hostname ===
    {
        std::vector<Server> servers = {
            {"web-01", 72, 4096}, {"db-01", 91, 16384},
            {"cache-01", 23, 2048}, {"web-02", 45, 8192}
        };
        auto it = std::ranges::find(servers, "db-01", &Server::hostname);
        if (it != servers.end()) {
            std::print("Trouvé: {} (cpu={})\n", it->hostname, it->cpu_load);
        }
    }

    // === Projection : count_if ===
    {
        std::vector<Server> servers = {
            {"web-01", 72, 4096}, {"db-01", 91, 16384},
            {"cache-01", 23, 2048}, {"web-02", 45, 8192}
        };
        auto n = std::ranges::count_if(servers,
            [](int load) { return load > 50; },
            &Server::cpu_load
        );
        std::print("Serveurs > 50%: {}\n", n);
        // Serveurs > 50%: 2
    }

    // === Projection : minmax_element ===
    {
        std::vector<Server> servers = {
            {"web-01", 72, 4096}, {"db-01", 91, 16384},
            {"cache-01", 23, 2048}, {"web-02", 45, 8192}
        };
        auto [min_it, max_it] = std::ranges::minmax_element(servers, {}, &Server::memory_mb);
        std::print("Moins de RAM : {} ({}MB)\n", min_it->hostname, min_it->memory_mb);
        std::print("Plus de RAM : {} ({}MB)\n", max_it->hostname, max_it->memory_mb);
    }

    // === Projection : find_if ===
    {
        std::vector<Server> servers = {
            {"web-01", 72, 4096}, {"db-01", 91, 16384},
            {"cache-01", 23, 2048}, {"web-02", 45, 8192}
        };
        auto it = std::ranges::find_if(servers,
            [](int mem) { return mem > 10000; },
            &Server::memory_mb
        );
        if (it != servers.end()) {
            std::print("Serveur > 10000MB: {} ({}MB)\n", it->hostname, it->memory_mb);
        }
    }

    // === Valeur de retour enrichie : ranges::copy ===
    {
        std::vector<int> src = {1, 2, 3, 4, 5};
        std::vector<int> dst(5);
        auto [in_it, out_it] = std::ranges::copy(src, dst.begin());
        std::print("in_it==end: {}, out_it==end: {}\n",
            in_it == src.end(), out_it == dst.end());
        // true, true
    }

    // === ranges::remove ===
    {
        std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};
        v.erase(std::ranges::remove(v, 2).begin(), v.end());
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 3 5 7
    }

    // === Subrange sort ===
    {
        std::vector<int> v = {9, 7, 5, 3, 1, 8, 6, 4, 2, 0};
        std::ranges::sort(std::ranges::subrange(v.begin(), v.begin() + 5));
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 3 5 7 9 8 6 4 2 0

        // Équivalent avec paire itérateur-sentinelle
        std::vector<int> v2 = {9, 7, 5, 3, 1, 8, 6, 4, 2, 0};
        std::ranges::sort(v2.begin(), v2.begin() + 5);
        for (int x : v2) std::print("{} ", x);
        std::println("");
    }

    // === Sentinelle NullTerminator ===
    {
        struct NullTerminator {
            bool operator==(const char* p) const { return *p == '\0'; }
        };

        const char* msg = "Hello, Ranges!";
        auto n = std::ranges::count_if(
            msg, NullTerminator{},
            [](char c) { return c == 'l'; }
        );
        std::print("Nombre de 'l' : {}\n", n);
        // Nombre de 'l' : 2
    }

    // === View filter + transform (aperçu) ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        auto view = v | std::views::filter([](int x) { return x % 2 == 0; })
                      | std::views::transform([](int x) { return x * x; });
        for (int x : view) {
            std::print("{} ", x);
        }
        std::println("");
        // 4 16 36 64 100
    }

    // === Dangling protection (avec variable) ===
    {
        std::vector<int> v = get_vector();
        auto it = std::ranges::find(v, 42);
        if (it != v.end()) {
            std::print("Trouvé 42 à l'index {}\n", std::distance(v.begin(), it));
            // Trouvé 42 à l'index 3
        }
    }

    // === Composition : filter + transform + accumulate ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // Filtrer les pairs, mettre au carré, puis sommer
        auto view = v | std::views::filter([](int x) { return x % 2 == 0; })
                      | std::views::transform([](int x) { return x * x; });

        auto common = std::views::common(view);
        int total = std::accumulate(common.begin(), common.end(), 0);
        std::print("total={}\n", total);
        // total=220  (4 + 16 + 36 + 64 + 100)
    }
}
