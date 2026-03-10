/* ============================================================================
   Section 6.2.3 : Constructeur de copie
   Description : Copie profonde vs copie superficielle — DynArray avec
                 constructeur de copie correct, vérification de l'indépendance
   Fichier source : 02.3-constructeur-copie.md
   ============================================================================ */
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cstddef>

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

    // Constructeur de copie — copie profonde
    DynArray(const DynArray& other)
        : data_(other.size_ > 0 ? new int[other.size_] : nullptr)
        , size_(other.size_) {
        std::copy_n(other.data_, size_, data_);
    }

    ~DynArray() { delete[] data_; }

    std::size_t size() const { return size_; }

    int& operator[](std::size_t index) {
        if (index >= size_) throw std::out_of_range("out of range");
        return data_[index];
    }

    const int& operator[](std::size_t index) const {
        if (index >= size_) throw std::out_of_range("out of range");
        return data_[index];
    }

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};

int main() {
    DynArray a(3, 42);     // a = [42, 42, 42]
    DynArray b = a;        // Copie profonde → b = [42, 42, 42] (bloc séparé)

    b[0] = 99;             // Modifie b uniquement

    std::cout << "a[0]: " << a[0] << "\n";   // 42 — a est intact
    std::cout << "b[0]: " << b[0] << "\n";   // 99 — b a sa propre copie

    // Test copie d'un tableau vide
    DynArray empty;
    DynArray empty_copy = empty;
    std::cout << "empty_copy size: " << empty_copy.size() << "\n";   // 0

    return 0;
}
// Sortie attendue :
// a[0]: 42
// b[0]: 99
// empty_copy size: 0
