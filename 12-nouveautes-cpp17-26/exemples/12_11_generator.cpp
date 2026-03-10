/* ============================================================================
   Section 12.11 : std::generator (C++23)
   Description : Generateurs paresseux - fibonacci, naturals pipeline,
                 powers, primes, parcours inorder d'arbre, perfect_squares,
                 collatz, generator<const string&>
   Fichier source : 11-generator.md
   ============================================================================ */
#include <generator>
#include <print>
#include <ranges>
#include <cmath>
#include <vector>
#include <string>

// === fibonacci (lignes 19-39) ===
std::generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}

// === naturals (lignes 48-70) ===
std::generator<int> naturals(int start = 1) {
    while (true) {
        co_yield start++;
    }
}

// === powers (lignes 78-103) ===
std::generator<double> powers(double base) {
    double value = 1.0;
    while (true) {
        co_yield value;
        value *= base;
    }
}

std::generator<int> primes() {
    co_yield 2;
    for (int n = 3; ; n += 2) {
        bool is_prime = true;
        for (int d = 3; d * d <= n; d += 2) {
            if (n % d == 0) { is_prime = false; break; }
        }
        if (is_prime) co_yield n;
    }
}

// === tree inorder (lignes 138-153) ===
struct TreeNode {
    int value;
    TreeNode* left = nullptr;
    TreeNode* right = nullptr;
};

std::generator<int> inorder(TreeNode* node) {
    if (!node) co_return;
    for (int v : inorder(node->left))  co_yield v;
    co_yield node->value;
    for (int v : inorder(node->right)) co_yield v;
}

// === perfect_squares (lignes 328-332) ===
std::generator<int> perfect_squares() {
    for (int n = 1; ; ++n) {
        co_yield n * n;
    }
}

// === collatz (lignes 346-352) ===
std::generator<int> collatz(int n) {
    while (n != 1) {
        co_yield n;
        n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
    }
    co_yield 1;
}

// === generator<const string&> (lignes 218-225) ===
std::generator<const std::string&> iterate_names(
    const std::vector<std::string>& names)
{
    for (const auto& name : names) {
        co_yield name;
    }
}

int main() {
    std::print("fibonacci(10): ");
    for (int n : fibonacci() | std::views::take(10)) {
        std::print("{} ", n);
    }
    std::print("\n");
    // 0 1 1 2 3 5 8 13 21 34

    std::print("naturals pipeline: ");
    auto result = naturals()
        | std::views::filter([](int n) { return n % 3 == 0; })
        | std::views::transform([](int n) { return n * n; })
        | std::views::take(5);
    for (int n : result) {
        std::print("{} ", n);
    }
    std::print("\n");
    // 9 36 81 144 225

    std::print("powers(2): ");
    for (double p : powers(2.0) | std::views::take(6)) {
        std::print("{} ", static_cast<int>(p));
    }
    std::print("\n");
    // 1 2 4 8 16 32

    std::print("primes: ");
    for (int p : primes() | std::views::take(7)) {
        std::print("{} ", p);
    }
    std::print("\n");
    // 2 3 5 7 11 13 17

    // Tree test
    TreeNode n1{1}, n3{3}, n5{5}, n7{7};
    TreeNode n2{2, &n1, &n3};
    TreeNode n6{6, &n5, &n7};
    TreeNode n4{4, &n2, &n6};

    std::print("inorder: ");
    for (int v : inorder(&n4)) {
        std::print("{} ", v);
    }
    std::print("\n");
    // 1 2 3 4 5 6 7

    std::print("perfect_squares: ");
    for (int n : perfect_squares() | std::views::take(6)) {
        std::print("{} ", n);
    }
    std::print("\n");
    // 1 4 9 16 25 36

    std::print("collatz(12): ");
    for (int n : collatz(12)) {
        std::print("{} ", n);
    }
    std::print("\n");
    // 12 6 3 10 5 16 8 4 2 1

    std::print("iterate_names: ");
    std::vector<std::string> names = {"Alice", "Bob", "Clara"};
    for (const auto& name : iterate_names(names)) {
        std::print("{} ", name);
    }
    std::print("\n");
}
