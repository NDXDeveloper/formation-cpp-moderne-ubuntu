/* ============================================================================
   Section 21.2.1 : Variantes de mutex - std::shared_mutex (C++17)
   Description : Cache thread-safe avec lecteurs multiples / écrivain unique
   Fichier source : 02.1-mutex.md
   ============================================================================ */

#include <shared_mutex>
#include <mutex>
#include <string>
#include <map>
#include <print>

class ThreadSafeCache {
    mutable std::shared_mutex mtx_;
    std::map<std::string, std::string> cache_;

public:
    std::string get(const std::string& key) const {
        // Verrou partagé : plusieurs lecteurs simultanés
        std::shared_lock lock(mtx_);
        auto it = cache_.find(key);
        return (it != cache_.end()) ? it->second : "";
    }

    void set(const std::string& key, const std::string& value) {
        // Verrou exclusif : un seul écrivain, aucun lecteur
        std::unique_lock lock(mtx_);
        cache_[key] = value;
    }
};

int main() {
    ThreadSafeCache cache;
    cache.set("hello", "world");
    cache.set("foo", "bar");
    std::println("hello = {}", cache.get("hello"));
    std::println("foo = {}", cache.get("foo"));
    std::println("missing = '{}'", cache.get("missing"));
}
