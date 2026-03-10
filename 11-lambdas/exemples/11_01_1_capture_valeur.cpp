/* ============================================================================
   Section 11.1.1 : Capture par valeur [=]
   Description : Exemples complets de capture par valeur — copie, moment de
                 la capture, boucles, coût, pointeurs bruts, shared_ptr,
                 types move-only, capture de this
   Fichier source : 01.1-capture-valeur.md
   ============================================================================ */
#include <print>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <algorithm>
#include <utility>

int main() {
    // --- Principe (lignes 10-14) ---
    {
        int x = 10;
        auto snapshot = [x]() { return x; };
        x = 999;
        std::print("{}\n", snapshot());  // 10
    }

    // --- Capture explicite (lignes 28-33) ---
    {
        int width = 800;
        int height = 600;
        auto area = [width, height]() { return width * height; };
        std::print("area = {}\n", area());  // 480000
    }

    // --- Capture par défaut [=] (lignes 54-60) ---
    {
        int width = 800;
        int height = 600;
        int depth = 32;
        auto volume = [=]() { return width * height * depth; };
        std::print("volume = {}\n", volume());  // 15360000
    }

    // --- Moment de la copie — boucle (lignes 100-110) ---
    {
        std::vector<std::function<int()>> functions;
        for (int i = 0; i < 5; ++i) {
            functions.push_back([i]() { return i; });
        }
        for (auto& fn : functions) {
            std::print("{} ", fn());
        }
        std::print("\n");  // 0 1 2 3 4
    }

    // --- Moment de la copie — string (lignes 118-124) ---
    {
        std::string name = "Alice";
        auto greet = [name]() { std::print("Hello, {}!\n", name); };
        name = "Bob";
        name.clear();
        greet();  // Hello, Alice!
    }

    // --- Coût de la copie (lignes 136-141) ---
    {
        std::vector<int> large_data(1'000'000, 42);
        auto process = [large_data]() { return large_data.size(); };
        std::print("size = {}\n", process());  // 1000000
    }

    // --- Capture par ref const via init capture (lignes 152-155) ---
    {
        std::vector<int> large_data(1'000'000, 42);
        auto process = [&data = std::as_const(large_data)]() {
            return data.size();
        };
        std::print("const ref size = {}\n", process());  // 1000000
    }

    // --- Init capture avec std::move (lignes 161-167) ---
    {
        std::vector<int> large_data(1'000'000, 42);
        auto process = [data = std::move(large_data)]() {
            return data.size();
        };
        std::print("moved size = {}, original empty = {}\n",
                   process(), large_data.empty());  // 1000000, true
    }

    // --- Capture par référence simple (lignes 174-176) ---
    {
        std::vector<int> large_data(100, 42);
        auto count = std::count_if(large_data.begin(), large_data.end(),
            [](int v) { return v > 0; });
        std::print("count = {}\n", count);  // 100
    }

    // --- Capture de pointeurs bruts (lignes 186-193) ---
    {
        int* ptr = new int(42);
        auto reader = [ptr]() { return *ptr; };
        *ptr = 99;
        std::print("{}\n", reader());  // 99
        delete ptr;
    }

    // --- Solution shared_ptr (lignes 205-212) ---
    {
        auto ptr = std::make_shared<int>(42);
        auto reader = [ptr]() { return *ptr; };
        ptr.reset();
        std::print("{}\n", reader());  // 42
    }

    // --- Types non-copiables avec move (lignes 234-236) ---
    {
        auto ptr = std::make_unique<int>(42);
        auto reader = [p = std::move(ptr)]() { return *p; };
        std::print("{}\n", reader());  // 42
        std::print("ptr is null: {}\n", ptr == nullptr);  // true
    }

    // --- Capture de this (lignes 246-253) ---
    {
        struct Timer {
            int interval_ = 100;
            auto create_callback() {
                return [this]() { return interval_; };
            }
        };
        Timer t;
        auto cb = t.create_callback();
        std::print("interval = {}\n", cb());  // 100
    }

    return 0;
}
