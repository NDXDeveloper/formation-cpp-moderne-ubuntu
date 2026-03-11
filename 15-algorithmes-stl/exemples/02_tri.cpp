/* ============================================================================
   Section 15.2 : Tri : std::sort, std::stable_sort
   Description : Exemples complets couvrant sort, sort avec comparateur,
                 sort de structs, sort de sous-range, ranges::sort avec
                 projection, stable_sort, partial_sort, partial_sort_copy,
                 nth_element, is_sorted, is_sorted_until, spaceship operator,
                 sort de tableaux C et std::array, list::sort.
   Fichier source : 02-tri.md
   ============================================================================ */

#include <algorithm>
#include <ranges>
#include <vector>
#include <string>
#include <array>
#include <list>
#include <print>
#include <compare>

struct Server {
    std::string hostname;
    int cpu_load;
    int memory_mb;
};

struct Task {
    std::string name;
    int priority;
    int creation_id;
};

struct Version {
    int major;
    int minor;
    int patch;
    auto operator<=>(const Version&) const = default;
};

int main() {
    // === sort basic ===
    {
        std::vector<int> v = {42, 17, 93, 5, 68, 31, 7};
        std::sort(v.begin(), v.end());
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 5 7 17 31 42 68 93
    }

    // === sort with comparator ===
    {
        std::vector<int> v = {42, 17, 93, 5, 68, 31, 7};
        std::sort(v.begin(), v.end(), std::greater<int>{});
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 93 68 42 31 17 7 5
    }

    // === sort Server ===
    {
        std::vector<Server> servers = {
            {"web-01", 72, 4096}, {"web-02", 45, 8192}, {"db-01", 91, 16384},
            {"cache-01", 23, 2048}, {"web-03", 45, 4096}
        };
        std::sort(servers.begin(), servers.end(), [](const Server& a, const Server& b) {
            return a.cpu_load < b.cpu_load;
        });
        for (const auto& s : servers) std::print("{}({}), ", s.hostname, s.cpu_load);
        std::println("");
        // cache-01(23), web-02(45), web-03(45), web-01(72), db-01(91),
    }

    // === sort sub-range ===
    {
        std::vector<int> v = {9, 7, 5, 3, 1, 8, 6, 4, 2, 0};
        std::sort(v.begin(), v.begin() + 5);
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 3 5 7 9 8 6 4 2 0
    }

    // === ranges::sort avec projection ===
    {
        std::vector<Server> servers = {
            {"web-01", 72, 4096}, {"web-02", 45, 8192}, {"db-01", 91, 16384},
            {"cache-01", 23, 2048}, {"web-03", 45, 4096}
        };
        std::ranges::sort(servers, {}, &Server::cpu_load);
        for (const auto& s : servers) std::print("{}({}), ", s.hostname, s.cpu_load);
        std::println("");
    }

    // === stable_sort ===
    {
        std::vector<Task> tasks = {
            {"backup", 2, 1}, {"deploy", 1, 2}, {"cleanup", 2, 3},
            {"monitor", 1, 4}, {"archive", 2, 5}
        };
        std::stable_sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
            return a.priority < b.priority;
        });
        for (const auto& t : tasks) std::print("{}({}, #{}), ", t.name, t.priority, t.creation_id);
        std::println("");
        // deploy(1, #2), monitor(1, #4), backup(2, #1), cleanup(2, #3), archive(2, #5),
    }

    // === partial_sort ===
    {
        std::vector<int> v = {42, 17, 93, 5, 68, 31, 7, 84, 12};
        std::partial_sort(v.begin(), v.begin() + 3, v.end());
        std::print("Top 3: {} {} {}\n", v[0], v[1], v[2]);
        // Top 3: 5 7 12
    }

    // === partial_sort_copy ===
    {
        std::vector<int> v = {42, 17, 93, 5, 68, 31, 7, 84, 12};
        std::vector<int> top3(3);
        std::partial_sort_copy(v.begin(), v.end(), top3.begin(), top3.end());
        std::print("top3: ");
        for (int x : top3) std::print("{} ", x);
        std::println("");
        // top3: 5 7 12
    }

    // === nth_element ===
    {
        std::vector<int> v = {42, 17, 93, 5, 68, 31, 7, 84, 12};
        std::nth_element(v.begin(), v.begin() + 4, v.end());
        std::print("Médiane (v[4]): {}\n", v[4]);
        // Médiane (v[4]): 31
    }

    // === is_sorted ===
    {
        std::vector<int> v1 = {1, 3, 5, 7, 9};
        std::vector<int> v2 = {1, 3, 8, 2, 9};
        std::print("v1 sorted: {}, v2 sorted: {}\n",
            std::is_sorted(v1.begin(), v1.end()),
            std::is_sorted(v2.begin(), v2.end()));

        std::vector<int> v = {1, 3, 5, 8, 2, 9, 11};
        auto it = std::is_sorted_until(v.begin(), v.end());
        std::print("Trié jusqu'à l'index {} (valeur {})\n",
                   std::distance(v.begin(), it), *it);
        // Trié jusqu'à l'index 4 (valeur 2)
    }

    // === Version with spaceship ===
    {
        std::vector<Version> versions = {
            {2, 1, 0}, {1, 9, 3}, {2, 0, 1}, {1, 9, 3}, {3, 0, 0}
        };
        std::sort(versions.begin(), versions.end());
        for (const auto& v : versions)
            std::print("{{{},{},{}}} ", v.major, v.minor, v.patch);
        std::println("");
        // {1,9,3} {1,9,3} {2,0,1} {2,1,0} {3,0,0}
    }

    // === sort C array ===
    {
        int arr[] = {42, 17, 93, 5, 68};
        std::sort(std::begin(arr), std::end(arr));
        for (int x : arr) std::print("{} ", x);
        std::println("");
        // 5 17 42 68 93
    }

    // === sort std::array ===
    {
        std::array<int, 5> a = {42, 17, 93, 5, 68};
        std::ranges::sort(a);
        for (int x : a) std::print("{} ", x);
        std::println("");
        // 5 17 42 68 93
    }

    // === list sort ===
    {
        std::list<int> lst = {42, 17, 93, 5, 68};
        lst.sort();
        for (int x : lst) std::print("{} ", x);
        std::println("");
        // 5 17 42 68 93
    }
}
