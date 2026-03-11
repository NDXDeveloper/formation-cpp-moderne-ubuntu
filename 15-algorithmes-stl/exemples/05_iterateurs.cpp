/* ============================================================================
   Section 15.5 : Itérateurs : input, output, forward, bidirectional, random_access
   Description : Exemples complets couvrant istream_iterator, ostream_iterator,
                 back_inserter, front_inserter, inserter, forward_list
                 multi-pass, bidirectional navigation, random access arithmetic,
                 reverse iterators, find_if avec reverse iterator, next/prev,
                 C++20 concepts check pour les catégories d'itérateurs.
   Fichier source : 05-iterateurs.md
   ============================================================================ */

#include <iterator>
#include <vector>
#include <list>
#include <forward_list>
#include <set>
#include <deque>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <print>
#include <concepts>

int main() {
    // === istream_iterator ===
    {
        std::istringstream input("10 20 30 40 50");
        std::vector<int> v{std::istream_iterator<int>(input), std::istream_iterator<int>()};
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 10 20 30 40 50
    }

    // === istream_iterator distance ===
    {
        std::istringstream input("10 20 30 40 50");
        int n = std::distance(std::istream_iterator<int>(input), std::istream_iterator<int>());
        std::print("n={}\n", n);
        // n=5
    }

    // === ostream_iterator ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << "\n";
        // 1 2 3 4 5
    }

    // === back_inserter / front_inserter ===
    {
        std::vector<int> src = {1, 2, 3};
        std::vector<int> dst;
        std::copy(src.begin(), src.end(), std::back_inserter(dst));
        for (int x : dst) std::print("{} ", x);
        std::println("");
        // 1 2 3

        std::list<int> lst;
        std::copy(src.begin(), src.end(), std::front_inserter(lst));
        for (int x : lst) std::print("{} ", x);
        std::println("");
        // 3 2 1
    }

    // === inserter with set ===
    {
        std::vector<int> src = {1, 2, 3};
        std::set<int> s;
        std::copy(src.begin(), src.end(), std::inserter(s, s.end()));
        for (int x : s) std::print("{} ", x);
        std::println("");
        // 1 2 3
    }

    // === forward_list multi-pass ===
    {
        std::forward_list<int> fl = {10, 20, 30, 40, 50};
        auto it = fl.begin();
        auto saved = it;
        ++it; ++it;
        std::print("*it={}, *saved={}\n", *it, *saved);
        // *it=30, *saved=10
    }

    // === bidirectional ===
    {
        std::list<int> lst = {10, 20, 30, 40, 50};
        auto it = lst.end();
        --it; // 50
        --it; // 40
        ++it; // 50
        std::print("*it={}\n", *it);
        // *it=50
    }

    // === random access ===
    {
        std::vector<int> v = {10, 20, 30, 40, 50};
        auto it = v.begin();
        it += 3;
        auto it2 = it - 2;
        int d = it - it2;
        std::print("*it={}, *it2={}, d={}\n", *it, *it2, d);
        // *it=40, *it2=20, d=2
    }

    // === reverse iterators ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        for (auto rit = v.rbegin(); rit != v.rend(); ++rit) {
            std::print("{} ", *rit);
        }
        std::println("");
        // 5 4 3 2 1
    }

    // === find_if with reverse iterator ===
    {
        std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};
        auto rit = std::find_if(v.rbegin(), v.rend(), [](int x) { return x > 5; });
        if (rit != v.rend()) {
            std::print("Dernier > 5 : {}\n", *rit);
            auto it = rit.base() - 1;
            std::print("Index : {}\n", std::distance(v.begin(), it));
            // Dernier > 5 : 6
            // Index : 7
        }
    }

    // === next / prev ===
    {
        std::list<int> lst = {10, 20, 30, 40, 50};
        auto it = lst.begin();
        auto third = std::next(it, 2);
        std::print("*third={}, *it={}\n", *third, *it);
        // *third=30, *it=10
        auto last = lst.end();
        auto before_last = std::prev(last);
        std::print("*before_last={}\n", *before_last);
        // *before_last=50
    }

    // === back_inserter append ===
    {
        std::vector<int> src = {1, 2, 3, 4, 5};
        std::vector<int> dst = {10, 20};
        std::copy(src.begin(), src.end(), std::back_inserter(dst));
        for (int x : dst) std::print("{} ", x);
        std::println("");
        // 10 20 1 2 3 4 5
    }

    // === front_inserter with deque ===
    {
        std::vector<int> src = {1, 2, 3};
        std::deque<int> dst = {10, 20};
        std::copy(src.begin(), src.end(), std::front_inserter(dst));
        for (int x : dst) std::print("{} ", x);
        std::println("");
        // 3 2 1 10 20
    }

    // === inserter at position ===
    {
        std::vector<int> src = {1, 2, 3};
        std::vector<int> dst = {10, 20, 30, 40};
        std::copy(src.begin(), src.end(), std::inserter(dst, dst.begin() + 2));
        for (int x : dst) std::print("{} ", x);
        std::println("");
        // 10 20 1 2 3 30 40
    }

    // === inserter with set ===
    {
        std::set<int> s = {10, 30, 50};
        std::vector<int> src = {20, 40, 60};
        std::copy(src.begin(), src.end(), std::inserter(s, s.end()));
        for (int x : s) std::print("{} ", x);
        std::println("");
        // 10 20 30 40 50 60
    }

    // === C++20 concept check ===
    {
        static_assert(std::random_access_iterator<std::vector<int>::iterator>);
        static_assert(std::bidirectional_iterator<std::list<int>::iterator>);
        static_assert(std::forward_iterator<std::forward_list<int>::iterator>);
        std::println("Concept checks passed");
    }
}
