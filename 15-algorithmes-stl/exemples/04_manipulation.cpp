/* ============================================================================
   Section 15.4 : Manipulation de séquences
   Description : Exemples complets couvrant copy, copy_if, copy_n,
                 copy_backward, move, remove-erase idiom, C++20 std::erase,
                 unique, reverse, rotate, shuffle, partition, stable_partition,
                 partition_copy, fill, fill_n, swap_ranges, replace,
                 replace_if, set operations (union, intersection, difference,
                 symmetric_difference), merge, inplace_merge, includes.
   Fichier source : 04-manipulation.md
   ============================================================================ */

#include <algorithm>
#include <ranges>
#include <vector>
#include <string>
#include <list>
#include <set>
#include <iterator>
#include <deque>
#include <random>
#include <print>

int main() {
    // === copy ===
    {
        std::vector<int> src = {1, 2, 3, 4, 5};
        std::vector<int> dst(src.size());
        std::copy(src.begin(), src.end(), dst.begin());
        for (int x : dst) std::print("{} ", x);
        std::println("");
        // 1 2 3 4 5
    }

    // === copy sub-range ===
    {
        std::vector<int> src = {10, 20, 30, 40, 50, 60, 70};
        std::vector<int> dst;
        std::copy(src.begin() + 2, src.begin() + 5, std::back_inserter(dst));
        for (int x : dst) std::print("{} ", x);
        std::println("");
        // 30 40 50
    }

    // === copy_if ===
    {
        std::vector<int> src = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::vector<int> evens;
        std::copy_if(src.begin(), src.end(), std::back_inserter(evens),
            [](int x) { return x % 2 == 0; });
        for (int x : evens) std::print("{} ", x);
        std::println("");
        // 2 4 6 8 10
    }

    // === copy_n ===
    {
        std::vector<int> src = {10, 20, 30, 40, 50};
        std::vector<int> dst;
        std::copy_n(src.begin(), 3, std::back_inserter(dst));
        for (int x : dst) std::print("{} ", x);
        std::println("");
        // 10 20 30
    }

    // === copy_backward ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 0, 0};
        std::copy_backward(v.begin(), v.begin() + 5, v.begin() + 6);
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 1 2 3 4 5 0
        v[0] = 99;
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 99 1 2 3 4 5 0
    }

    // === move algorithm ===
    {
        std::vector<std::string> src = {"alpha", "bravo", "charlie", "delta"};
        std::vector<std::string> dst(src.size());
        std::move(src.begin(), src.end(), dst.begin());
        for (const auto& s : dst) std::print("{} ", s);
        std::println("");
        // alpha bravo charlie delta
    }

    // === remove-erase idiom ===
    {
        std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};
        v.erase(std::remove(v.begin(), v.end(), 2), v.end());
        for (int x : v) std::print("{} ", x);
        std::print("(size={})\n", v.size());
        // 1 3 5 7 (size=4)
    }

    // === remove_if + erase ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        v.erase(std::remove_if(v.begin(), v.end(), [](int x) { return x % 3 == 0; }), v.end());
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 2 4 5 7 8 10
    }

    // === C++20 std::erase ===
    {
        std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};
        auto removed = std::erase(v, 2);
        for (int x : v) std::print("{} ", x);
        std::print("(removed={})\n", removed);
        // 1 3 5 7 (removed=3)

        std::vector<int> v2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        auto removed2 = std::erase_if(v2, [](int x) { return x % 3 == 0; });
        for (int x : v2) std::print("{} ", x);
        std::print("(removed={})\n", removed2);
        // 1 2 4 5 7 8 10 (removed=3)
    }

    // === unique ===
    {
        std::vector<int> v = {1, 1, 2, 2, 2, 3, 3, 4, 5, 5};
        auto new_end = std::unique(v.begin(), v.end());
        v.erase(new_end, v.end());
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 2 3 4 5
    }

    // === sort + unique + erase ===
    {
        std::vector<int> v = {3, 1, 2, 1, 3, 2, 4};
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 2 3 4
    }

    // === reverse ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        std::reverse(v.begin(), v.end());
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 5 4 3 2 1
    }

    // === rotate ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5};
        std::rotate(v.begin(), v.begin() + 2, v.end());
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 3 4 5 1 2
    }

    // === rotate move element ===
    {
        std::vector<std::string> menu = {"Home", "About", "Blog", "Contact", "Shop"};
        std::rotate(menu.begin() + 1, menu.begin() + 4, menu.end());
        for (const auto& m : menu) std::print("{} ", m);
        std::println("");
        // Home Contact Shop About Blog
    }

    // === shuffle ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::mt19937 rng(42);
        std::shuffle(v.begin(), v.end(), rng);
        std::print("shuffled: ");
        for (int x : v) std::print("{} ", x);
        std::println("");
    }

    // === partition ===
    {
        std::vector<int> v = {8, 3, 5, 1, 9, 2, 7, 4, 6};
        auto partition_point = std::partition(v.begin(), v.end(),
            [](int x) { return x % 2 == 0; });
        std::print("Nombre de pairs : {}\n", std::distance(v.begin(), partition_point));
        // Nombre de pairs : 4
    }

    // === stable_partition ===
    {
        std::vector<int> v = {8, 3, 5, 1, 9, 2, 7, 4, 6};
        auto pp = std::stable_partition(v.begin(), v.end(),
            [](int x) { return x % 2 == 0; });
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 8 2 4 6 3 5 1 9 7
        (void)pp;
    }

    // === partition_copy ===
    {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::vector<int> evens, odds;
        std::partition_copy(v.begin(), v.end(),
            std::back_inserter(evens), std::back_inserter(odds),
            [](int x) { return x % 2 == 0; });
        std::print("evens: "); for (int x : evens) std::print("{} ", x); std::println("");
        std::print("odds: "); for (int x : odds) std::print("{} ", x); std::println("");
    }

    // === fill ===
    {
        std::vector<int> v(10);
        std::fill(v.begin(), v.end(), 42);
        std::fill(v.begin(), v.begin() + 5, 0);
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 0 0 0 0 0 42 42 42 42 42
    }

    // === fill_n ===
    {
        std::vector<int> v(10, 0);
        std::fill_n(v.begin() + 3, 4, 99);
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 0 0 0 99 99 99 99 0 0 0
    }

    // === swap_ranges ===
    {
        std::vector<int> a = {1, 2, 3, 4, 5};
        std::vector<int> b = {10, 20, 30, 40, 50};
        std::swap_ranges(a.begin(), a.end(), b.begin());
        std::print("a: "); for (int x : a) std::print("{} ", x); std::println("");
        std::print("b: "); for (int x : b) std::print("{} ", x); std::println("");
    }

    // === replace ===
    {
        std::vector<int> v = {1, 2, 3, 2, 5, 2, 7};
        std::replace(v.begin(), v.end(), 2, 99);
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 99 3 99 5 99 7
    }

    // === replace_if ===
    {
        std::vector<int> v = {1, -2, 3, -4, 5, -6, 7};
        std::replace_if(v.begin(), v.end(), [](int x) { return x < 0; }, 0);
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 0 3 0 5 0 7
    }

    // === set operations ===
    {
        std::vector<int> a = {1, 2, 3, 4, 5};
        std::vector<int> b = {3, 4, 5, 6, 7};
        std::vector<int> result;

        std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(result));
        std::print("Union: "); for (int x : result) std::print("{} ", x); std::println("");
        // Union: 1 2 3 4 5 6 7

        result.clear();
        std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(result));
        std::print("Intersection: "); for (int x : result) std::print("{} ", x); std::println("");
        // Intersection: 3 4 5

        result.clear();
        std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(result));
        std::print("Difference: "); for (int x : result) std::print("{} ", x); std::println("");
        // Difference: 1 2

        result.clear();
        std::set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(result));
        std::print("Sym diff: "); for (int x : result) std::print("{} ", x); std::println("");
        // Sym diff: 1 2 6 7
    }

    // === merge ===
    {
        std::vector<int> a = {1, 3, 5, 7};
        std::vector<int> b = {2, 4, 6, 8};
        std::vector<int> merged;
        std::merge(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(merged));
        for (int x : merged) std::print("{} ", x);
        std::println("");
        // 1 2 3 4 5 6 7 8
    }

    // === inplace_merge ===
    {
        std::vector<int> v = {1, 3, 5, 7, 2, 4, 6, 8};
        std::inplace_merge(v.begin(), v.begin() + 4, v.end());
        for (int x : v) std::print("{} ", x);
        std::println("");
        // 1 2 3 4 5 6 7 8
    }

    // === includes ===
    {
        std::vector<int> all = {1, 2, 3, 4, 5, 6, 7, 8};
        std::vector<int> sub = {2, 4, 6};
        bool contains = std::includes(all.begin(), all.end(), sub.begin(), sub.end());
        std::print("includes: {}\n", contains);
        // includes: true
    }
}
