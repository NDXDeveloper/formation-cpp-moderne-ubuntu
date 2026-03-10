/* ============================================================================
   Section 6.5.3 : Opérateur d'affectation par copie
   Description : Copy assignment — approche « allouer d'abord », auto-affectation,
                 copy-and-swap, swap/ADL, chaînage d'affectations
   Fichier source : 05.3-copy-assignment.md
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

    // Constructeur de copie
    DynArray(const DynArray& other)
        : data_(other.size_ > 0 ? new int[other.size_] : nullptr)
        , size_(other.size_) {
        std::copy_n(other.data_, size_, data_);
    }

    // Constructeur de déplacement
    DynArray(DynArray&& other) noexcept
        : data_(other.data_)
        , size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    // Opérateur d'affectation par copie — allouer d'abord, libérer ensuite
    DynArray& operator=(const DynArray& other) {
        if (this == &other) return *this;

        int* new_data = nullptr;
        if (other.size_ > 0) {
            new_data = new int[other.size_];
            std::copy_n(other.data_, other.size_, new_data);
        }

        delete[] data_;
        data_ = new_data;
        size_ = other.size_;

        return *this;
    }

    // Opérateur d'affectation par déplacement
    DynArray& operator=(DynArray&& other) noexcept {
        if (this == &other) return *this;

        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;

        other.data_ = nullptr;
        other.size_ = 0;

        return *this;
    }

    ~DynArray() { delete[] data_; }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    int& operator[](std::size_t index) {
        if (index >= size_) throw std::out_of_range("out of range");
        return data_[index];
    }

    const int& operator[](std::size_t index) const {
        if (index >= size_) throw std::out_of_range("out of range");
        return data_[index];
    }

    // Swap membre — noexcept, O(1)
    void swap(DynArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }

    friend std::ostream& operator<<(std::ostream& os, const DynArray& arr) {
        os << "[";
        for (std::size_t i = 0; i < arr.size_; ++i) {
            if (i > 0) os << ", ";
            os << arr.data_[i];
        }
        os << "]";
        return os;
    }

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};

// Swap non-membre (ADL)
inline void swap(DynArray& a, DynArray& b) noexcept {
    a.swap(b);
}

int main() {
    // Affectation par copie
    DynArray a(3, 42);
    DynArray b(5, 0);
    std::cout << "Avant: a=" << a << " b=" << b << "\n";

    b = a;
    std::cout << "Après b=a: a=" << a << " b=" << b << "\n";

    // Vérification de l'indépendance
    b[0] = 99;
    std::cout << "Après b[0]=99: a[0]=" << a[0] << " b[0]=" << b[0] << "\n";

    // Auto-affectation
    a = a;
    std::cout << "Après a=a: a=" << a << "\n";

    // Chaînage d'affectations
    DynArray c(2, 7);
    DynArray d(4, 1);
    DynArray e(1, 0);
    e = d = c;
    std::cout << "Après e=d=c: c=" << c << " d=" << d << " e=" << e << "\n";

    // Swap
    DynArray x(2, 1);
    DynArray y(3, 9);
    std::cout << "Avant swap: x=" << x << " y=" << y << "\n";
    swap(x, y);
    std::cout << "Après swap: x=" << x << " y=" << y << "\n";

    // Affectation par déplacement (rvalue)
    DynArray f(2, 0);
    f = DynArray(4, 100);
    std::cout << "Après f=DynArray(4,100): f=" << f << "\n";

    return 0;
}
// Sortie attendue :
// Avant: a=[42, 42, 42] b=[0, 0, 0, 0, 0]
// Après b=a: a=[42, 42, 42] b=[42, 42, 42]
// Après b[0]=99: a[0]=42 b[0]=99
// Après a=a: a=[42, 42, 42]
// Après e=d=c: c=[7, 7] d=[7, 7] e=[7, 7]
// Avant swap: x=[1, 1] y=[9, 9, 9]
// Après swap: x=[9, 9, 9] y=[1, 1]
// Après f=DynArray(4,100): f=[100, 100, 100, 100]
