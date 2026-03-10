/* ============================================================================
   Section 12.3 : std::span (C++20)
   Description : Vue legere non-owning sur donnees contigues - process avec
                 span, interface (size, front, back, subspan), span mutable,
                 span statique vs dynamique
   Fichier source : 03-std-span.md
   ============================================================================ */
#include <span>
#include <vector>
#include <array>
#include <print>

// === process avec span (lignes 60-65) ===
void process(std::span<const int> data) {
    for (int value : data) {
        std::print("{} ", value);
    }
    std::print("\n");
}

// === span mutable (lignes 170-174) ===
void double_values(std::span<int> data) {
    for (int& v : data) {
        v *= 2;
    }
}

int main() {
    // --- process avec differents conteneurs (lignes 67-76) ---
    std::print("=== process avec span ===\n");
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::array<int, 4> arr = {10, 20, 30, 40};
    int c_arr[] = {100, 200, 300};

    process(vec);
    process(arr);
    process(c_arr);
    process({vec.data() + 1, 3});

    // --- Interface de conteneur (lignes 133-151) ---
    std::print("\n=== interface span ===\n");
    std::span<const int> s = vec;

    std::print("size={}, empty={}\n", s.size(), s.empty());
    std::print("front={}, back={}, s[2]={}\n", s.front(), s.back(), s[2]);

    for (int v : s) {
        std::print("{} ", v);
    }
    std::print("\n");

    auto first_three = s.first(3);
    std::print("first(3): ");
    for (int v : first_three) std::print("{} ", v);
    std::print("\n");

    auto last_two = s.last(2);
    std::print("last(2): ");
    for (int v : last_two) std::print("{} ", v);
    std::print("\n");

    auto middle = s.subspan(1, 3);
    std::print("subspan(1,3): ");
    for (int v : middle) std::print("{} ", v);
    std::print("\n");

    // --- span mutable (lignes 176-178) ---
    std::print("\n=== span mutable ===\n");
    std::vector<int> mvec = {1, 2, 3};
    double_values(mvec);
    std::print("after double: ");
    for (int v : mvec) std::print("{} ", v);
    std::print("\n");
    // mvec == {2, 4, 6}

    // --- span statique vs dynamique (lignes 186-188) ---
    std::print("\n=== static vs dynamic span ===\n");
    int data[5] = {10, 20, 30, 40, 50};
    std::span<int> dynamic_span(data, 5);
    std::span<int, 5> static_span(data);
    std::print("dynamic size={}, static size={}\n",
        dynamic_span.size(), static_span.size());
}
