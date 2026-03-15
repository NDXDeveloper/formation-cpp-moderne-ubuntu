/* ============================================================================
   Section 21.2.1 : Associer un mutex à ses données - Pattern Synchronized
   Description : Template Synchronized<T> forçant l'accès via with_lock()
   Fichier source : 02.1-mutex.md
   ============================================================================ */

#include <mutex>
#include <vector>
#include <print>

template <typename T>
class Synchronized {
    mutable std::mutex mtx_;
    T data_;

public:
    template <typename F>
    auto with_lock(F&& func) -> decltype(func(data_)) {
        std::lock_guard lock(mtx_);
        return func(data_);
    }

    template <typename F>
    auto with_lock(F&& func) const -> decltype(func(data_)) {
        std::lock_guard lock(mtx_);
        return func(data_);
    }
};

int main() {
    Synchronized<std::vector<int>> safe_vec;

    safe_vec.with_lock([](std::vector<int>& v) {
        v.push_back(42);
    });

    int size = safe_vec.with_lock([](const std::vector<int>& v) {
        return static_cast<int>(v.size());
    });

    std::println("Taille du vecteur : {}", size);

    safe_vec.with_lock([](std::vector<int>& v) {
        v.push_back(100);
        v.push_back(200);
    });

    safe_vec.with_lock([](const std::vector<int>& v) {
        std::println("Contenu :");
        for (int val : v) {
            std::println("  {}", val);
        }
    });
}
