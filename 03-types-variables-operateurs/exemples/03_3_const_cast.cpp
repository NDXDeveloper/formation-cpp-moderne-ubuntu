/* ============================================================================
   Section 3.3.3 : const_cast - Manipulation de const
   Description : Retrait de const sur objet mutable, TextBuffer avec
                 delegation const/non-const, Cache avec mutable
   Fichier source : 03.3-const-cast.md
   ============================================================================ */
#include <print>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <stdexcept>

// --- TextBuffer : delegation const/non-const (lignes 133-152) ---
class TextBuffer {
    std::vector<char> data_;

public:
    TextBuffer(std::initializer_list<char> init) : data_(init) {}

    const char& char_at(std::size_t index) const {
        if (index >= data_.size()) {
            throw std::out_of_range("Index hors limites");
        }
        return data_[index];
    }

    char& char_at(std::size_t index) {
        return const_cast<char&>(
            std::as_const(*this).char_at(index)
        );
    }
};

// --- Cache avec mutable (lignes 166-179) ---
class Cache {
    mutable std::unordered_map<int, int> cache_;

public:
    int compute(int key) const {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        int result = key * key;
        cache_[key] = result;
        return result;
    }
};

int main() {
    // --- const_cast sur objet non-const (lignes 80-84) ---
    int mutable_value = 42;
    const int* cp = &mutable_value;
    int* p = const_cast<int*>(cp);
    *p = 99;
    std::print("mutable_value apres const_cast = {}\n", mutable_value);

    // --- TextBuffer (lignes 133-152) ---
    TextBuffer buf = {'H', 'e', 'l', 'l', 'o'};
    std::print("buf[0] = {}\n", buf.char_at(0));
    buf.char_at(0) = 'h';  // modification via version non-const
    std::print("buf[0] apres modif = {}\n", buf.char_at(0));

    // --- Cache avec mutable (lignes 166-179) ---
    const Cache cache;
    std::print("cache.compute(5) = {}\n", cache.compute(5));
    std::print("cache.compute(5) = {} (cache hit)\n", cache.compute(5));
    std::print("cache.compute(7) = {}\n", cache.compute(7));

    return 0;
}
