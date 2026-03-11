/* ============================================================================
   Section 15.1 : Recherche : std::find, std::binary_search
   Description : Exemples complets couvrant find, find_if, find_if_not,
                 ranges::find avec projections, count, count_if,
                 all_of/any_of/none_of, binary_search, lower_bound,
                 upper_bound, equal_range, search, boyer_moore_searcher,
                 adjacent_find, min_element, max_element, minmax_element,
                 et les méthodes membres des conteneurs associatifs.
   Fichier source : 01-recherche.md
   ============================================================================ */

#include <algorithm>
#include <ranges>
#include <functional>
#include <vector>
#include <string>
#include <print>
#include <unordered_map>

struct Employee {
    std::string name;
    int department;
    double salary;
};

struct Event {
    std::string name;
    int timestamp;
};

int main() {
    // === std::find — Trouver une valeur exacte ===
    {
        std::vector<int> v = {10, 25, 42, 7, 33, 42, 18};

        auto it = std::find(v.begin(), v.end(), 42);

        if (it != v.end()) {
            std::print("Trouvé : {} à l'index {}\n", *it, std::distance(v.begin(), it));
            // Trouvé : 42 à l'index 2
        } else {
            std::print("Non trouvé\n");
        }
    }

    // === std::find_if — Trouver selon un prédicat ===
    {
        std::vector<int> v = {10, 25, 42, 7, 33, 18};

        auto it = std::find_if(v.begin(), v.end(), [](int x) {
            return x % 2 != 0;
        });

        if (it != v.end()) {
            std::print("Premier impair : {}\n", *it);
            // Premier impair : 25
        }
    }

    // === std::find_if avec Employee ===
    {
        std::vector<Employee> team = {
            {"Alice", 42, 75000.0},
            {"Bob", 17, 62000.0},
            {"Carol", 42, 88000.0},
            {"Dave", 23, 71000.0}
        };

        auto it = std::find_if(team.begin(), team.end(), [](const Employee& e) {
            return e.department == 42 && e.salary > 80000.0;
        });

        if (it != team.end()) {
            std::print("Trouvé : {}\n", it->name);
            // Trouvé : Carol
        }
    }

    // === std::find_if_not ===
    {
        std::vector<int> v = {2, 4, 6, 7, 8, 10};

        auto it = std::find_if_not(v.begin(), v.end(), [](int x) {
            return x % 2 == 0;
        });

        if (it != v.end()) {
            std::print("Premier non-pair : {}\n", *it);
            // Premier non-pair : 7
        }
    }

    // === Versions Ranges (C++20) ===
    {
        std::vector<int> v = {10, 25, 42, 7, 33, 18};

        auto it = std::ranges::find(v, 42);
        if (it != v.end()) {
            std::print("ranges::find(42) à l'index {}\n", std::distance(v.begin(), it));
        }

        auto it2 = std::ranges::find_if(v, [](int x) { return x > 30; });
        if (it2 != v.end()) {
            std::print("ranges::find_if(>30) : {}\n", *it2);
        }
    }

    // === Projections ===
    {
        std::vector<Employee> team = {
            {"Alice", 42, 75000.0},
            {"Bob", 17, 62000.0},
            {"Carol", 42, 88000.0}
        };

        auto it = std::ranges::find(team, "Carol", &Employee::name);

        if (it != team.end()) {
            std::print("Département de Carol : {}\n", it->department);
            // Département de Carol : 42
        }

        auto it2 = std::ranges::find(team, 17, &Employee::department);
        if (it2 != team.end()) {
            std::print("Employé dept 17 : {}\n", it2->name);
        }
    }

    // === std::count et std::count_if ===
    {
        std::vector<int> v = {1, 2, 3, 2, 5, 2, 7, 2};

        auto n = std::count(v.begin(), v.end(), 2);
        std::print("Nombre de 2 : {}\n", n);
        // Nombre de 2 : 4

        auto m = std::count_if(v.begin(), v.end(), [](int x) { return x > 3; });
        std::print("Éléments > 3 : {}\n", m);
        // Éléments > 3 : 2

        // Versions Ranges
        auto n2 = std::ranges::count(v, 2);
        auto m2 = std::ranges::count_if(v, [](int x) { return x > 3; });
        std::print("ranges: count(2)={}, count_if(>3)={}\n", n2, m2);
    }

    // === all_of, any_of, none_of ===
    {
        std::vector<int> v = {2, 4, 6, 8, 10};

        bool all_even = std::all_of(v.begin(), v.end(), [](int x) {
            return x % 2 == 0;
        });
        // true

        bool has_big = std::any_of(v.begin(), v.end(), [](int x) {
            return x > 9;
        });
        // true

        bool no_neg = std::none_of(v.begin(), v.end(), [](int x) {
            return x < 0;
        });
        // true

        std::print("all_even={}, has_big={}, no_neg={}\n", all_even, has_big, no_neg);

        // Comportement sur séquence vide
        std::vector<int> empty_vec;
        bool result = std::all_of(empty_vec.begin(), empty_vec.end(), [](int x) {
            return x > 1000;
        });
        std::print("all_of sur vide : {}\n", result);
        // true — vrai par vacuité
    }

    // === binary_search ===
    {
        std::vector<int> v = {1, 3, 5, 7, 9, 11, 13, 15};

        bool found = std::binary_search(v.begin(), v.end(), 7);
        // true
        bool not_found = std::binary_search(v.begin(), v.end(), 8);
        // false

        std::print("binary_search(7)={}, binary_search(8)={}\n", found, not_found);
    }

    // === lower_bound ===
    {
        std::vector<int> v = {1, 3, 5, 5, 5, 7, 9};

        auto it = std::lower_bound(v.begin(), v.end(), 5);
        std::print("lower_bound(5): *it={}, index {}\n", *it, std::distance(v.begin(), it));
        // *it == 5, index 2

        auto it2 = std::lower_bound(v.begin(), v.end(), 6);
        std::print("lower_bound(6): *it={}, index {}\n", *it2, std::distance(v.begin(), it2));
        // *it2 == 7, index 5
    }

    // === lower_bound pour vérifier l'existence ===
    {
        std::vector<int> v = {1, 3, 5, 7, 9, 11};

        auto it = std::lower_bound(v.begin(), v.end(), 7);
        if (it != v.end() && *it == 7) {
            std::print("Trouvé à l'index {}\n", std::distance(v.begin(), it));
        } else {
            std::print("Non trouvé\n");
        }
    }

    // === upper_bound et intervalle ===
    {
        std::vector<int> v = {1, 3, 5, 5, 5, 7, 9};

        auto it = std::upper_bound(v.begin(), v.end(), 5);
        std::print("upper_bound(5): *it={}, index {}\n", *it, std::distance(v.begin(), it));

        auto lo = std::lower_bound(v.begin(), v.end(), 5);
        auto hi = std::upper_bound(v.begin(), v.end(), 5);

        std::print("Nombre de 5 : {}\n", std::distance(lo, hi));
        // Nombre de 5 : 3

        for (auto iter = lo; iter != hi; ++iter) {
            std::print("{} ", *iter);
        }
        std::println("");
    }

    // === equal_range ===
    {
        std::vector<int> v = {1, 3, 5, 5, 5, 7, 9};

        auto [lo, hi] = std::equal_range(v.begin(), v.end(), 5);

        std::print("Occurrences de 5 : {}\n", std::distance(lo, hi));
        // Occurrences de 5 : 3

        if (lo == hi) {
            std::print("Valeur absente\n");
        }
    }

    // === Recherche dichotomique avec comparateur personnalisé ===
    {
        std::vector<int> v = {15, 13, 11, 9, 7, 5, 3, 1};

        bool found = std::binary_search(v.begin(), v.end(), 9, std::greater<int>{});
        std::print("binary_search(9, greater)={}\n", found);

        auto it = std::lower_bound(v.begin(), v.end(), 9, std::greater<int>{});
        std::print("lower_bound(9, greater): *it={}\n", *it);
    }

    // === lower_bound avec Event et comparateur ===
    {
        std::vector<Event> events = {
            {"boot", 100},
            {"init", 250},
            {"ready", 500},
            {"request", 750},
            {"shutdown", 1000}
        };

        auto it = std::lower_bound(events.begin(), events.end(), 500,
            [](const Event& e, int ts) { return e.timestamp < ts; }
        );

        if (it != events.end() && it->timestamp == 500) {
            std::print("Événement : {}\n", it->name);
            // Événement : ready
        }

        // Version Ranges avec projection
        auto it2 = std::ranges::lower_bound(events, 500, {}, &Event::timestamp);
        if (it2 != events.end() && it2->timestamp == 500) {
            std::print("ranges: Événement : {}\n", it2->name);
        }
    }

    // === std::search ===
    {
        std::vector<int> haystack = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        std::vector<int> needle   = {4, 5, 6};

        auto it = std::search(haystack.begin(), haystack.end(),
                              needle.begin(), needle.end());

        if (it != haystack.end()) {
            std::print("Sous-séquence trouvée à l'index {}\n",
                       std::distance(haystack.begin(), it));
            // Sous-séquence trouvée à l'index 3
        }
    }

    // === Boyer-Moore searcher ===
    {
        std::string text = "The quick brown fox jumps over the lazy dog";
        std::string pattern = "brown fox";

        auto it = std::search(text.begin(), text.end(),
            std::boyer_moore_searcher(pattern.begin(), pattern.end()));

        if (it != text.end()) {
            std::print("Trouvé à la position {}\n",
                       std::distance(text.begin(), it));
            // Trouvé à la position 10
        }
    }

    // === std::adjacent_find ===
    {
        std::vector<int> v = {1, 2, 3, 3, 4, 5, 5, 6};

        auto it = std::adjacent_find(v.begin(), v.end());
        if (it != v.end()) {
            std::print("Premier doublon consécutif : {} à l'index {}\n",
                       *it, std::distance(v.begin(), it));
            // Premier doublon consécutif : 3 à l'index 2
        }
    }

    // === adjacent_find avec greater ===
    {
        std::vector<int> v = {1, 2, 4, 3, 5, 8, 7};

        auto it = std::adjacent_find(v.begin(), v.end(), std::greater<int>{});

        if (it != v.end()) {
            std::print("Rupture d'ordre : {} > {} à l'index {}\n",
                       *it, *(it + 1), std::distance(v.begin(), it));
            // Rupture d'ordre : 4 > 3 à l'index 2
        }
    }

    // === min_element / max_element ===
    {
        std::vector<int> v = {42, 17, 93, 5, 68, 31};

        auto min_it = std::min_element(v.begin(), v.end());
        auto max_it = std::max_element(v.begin(), v.end());

        std::print("Min : {} (index {})\n", *min_it, std::distance(v.begin(), min_it));
        std::print("Max : {} (index {})\n", *max_it, std::distance(v.begin(), max_it));
        // Min : 5 (index 3)
        // Max : 93 (index 2)

        auto [min_it2, max_it2] = std::minmax_element(v.begin(), v.end());
        std::print("Min : {}, Max : {}\n", *min_it2, *max_it2);
    }

    // === minmax_element avec Employee ===
    {
        struct Emp {
            std::string name;
            double salary;
        };

        std::vector<Emp> team = {
            {"Alice", 75000.0}, {"Bob", 62000.0}, {"Carol", 88000.0}
        };

        auto [lowest, highest] = std::minmax_element(team.begin(), team.end(),
            [](const Emp& a, const Emp& b) {
                return a.salary < b.salary;
            }
        );

        std::print("Salaire le plus bas : {} ({})\n", lowest->name, lowest->salary);
        std::print("Salaire le plus haut : {} ({})\n", highest->name, highest->salary);

        // Version Ranges avec projection
        auto [lo2, hi2] = std::ranges::minmax_element(team, {}, &Emp::salary);
        std::print("ranges: {} (lo), {} (hi)\n", lo2->name, hi2->name);
    }

    // === Méthodes membres des conteneurs associatifs ===
    {
        std::unordered_map<std::string, int> ages = {
            {"Alice", 30}, {"Bob", 25}, {"Carol", 35}
        };

        if (ages.contains("Alice")) {
            std::print("Alice trouvée\n");
        }

        auto it = ages.find("Bob");
        if (it != ages.end()) {
            std::print("Bob a {} ans\n", it->second);
        }
    }
}
