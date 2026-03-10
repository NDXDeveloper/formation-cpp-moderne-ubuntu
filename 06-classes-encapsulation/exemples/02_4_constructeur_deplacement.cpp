/* ============================================================================
   Section 6.2.4 : Constructeur de déplacement
   Description : Sémantique de déplacement — transfert de ressources O(1),
                 état moved-from, noexcept, retour de fonction
   Fichier source : 02.4-constructeur-deplacement.md
   ============================================================================ */
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cstddef>
#include <utility>

class DynArray {
public:
    DynArray() = default;

    explicit DynArray(std::size_t size)
        : data_(size > 0 ? new int[size]{} : nullptr)
        , size_(size) {}

    DynArray(std::size_t size, int value)
        : DynArray(size) {
        std::fill_n(data_, size_, value);
    }

    DynArray(const DynArray& other)
        : data_(other.size_ > 0 ? new int[other.size_] : nullptr)
        , size_(other.size_) {
        std::copy_n(other.data_, size_, data_);
    }

    // Constructeur de déplacement — transfert de propriété
    DynArray(DynArray&& other) noexcept
        : data_(other.data_)
        , size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    ~DynArray() { delete[] data_; }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    int& operator[](std::size_t index) {
        if (index >= size_) throw std::out_of_range("out of range");
        return data_[index];
    }

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};

DynArray make_range(std::size_t n) {
    DynArray result(n);
    for (std::size_t i = 0; i < n; ++i) result[i] = static_cast<int>(i);
    return result;
}

int main() {
    DynArray a(5, 42);

    DynArray b = std::move(a);        // Déplacement — b vole les ressources de a
    std::cout << b.size() << "\n";    // 5
    std::cout << b[0] << "\n";        // 42
    std::cout << a.size() << "\n";    // 0 — a est vidé
    std::cout << a.empty() << "\n";   // 1 (true)

    // Retour de fonction → déplacement ou RVO
    DynArray range = make_range(10);
    std::cout << "range size: " << range.size() << "\n";   // 10
    std::cout << "range[9]: " << range[9] << "\n";         // 9

    return 0;
}
// Sortie attendue :
// 5
// 42
// 0
// 1
// range size: 10
// range[9]: 9
