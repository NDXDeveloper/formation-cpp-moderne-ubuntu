/* ============================================================================
   Section 6.5.5 : Opérateur d'affectation par déplacement
   Description : DynArray version finale — Règle des 5 complète
                 (implémentation)
   Fichier source : 05.5-move-assignment.md
   ============================================================================ */
#include "05_5_dynarray_final.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>

// --- Constructeurs ---

DynArray::DynArray(std::size_t size)
    : data_(size > 0 ? new int[size]{} : nullptr)
    , size_(size) {}

DynArray::DynArray(std::size_t size, int value)
    : DynArray(size) {
    std::fill_n(data_, size_, value);
}

// --- Règle des 5 ---

DynArray::~DynArray() {
    delete[] data_;
}

DynArray::DynArray(const DynArray& other)
    : data_(other.size_ > 0 ? new int[other.size_] : nullptr)
    , size_(other.size_) {
    std::copy_n(other.data_, size_, data_);
}

DynArray& DynArray::operator=(const DynArray& other) {
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

DynArray::DynArray(DynArray&& other) noexcept
    : data_(other.data_)
    , size_(other.size_) {
    other.data_ = nullptr;
    other.size_ = 0;
}

DynArray& DynArray::operator=(DynArray&& other) noexcept {
    if (this == &other) return *this;

    delete[] data_;

    data_ = other.data_;
    size_ = other.size_;

    other.data_ = nullptr;
    other.size_ = 0;

    return *this;
}

// --- Interface publique ---

int& DynArray::operator[](std::size_t index) {
    if (index >= size_) throw std::out_of_range("DynArray: index out of range");
    return data_[index];
}

const int& DynArray::operator[](std::size_t index) const {
    if (index >= size_) throw std::out_of_range("DynArray: index out of range");
    return data_[index];
}

void DynArray::swap(DynArray& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
}

// --- Friend ---

std::ostream& operator<<(std::ostream& os, const DynArray& arr) {
    os << "[";
    for (std::size_t i = 0; i < arr.size_; ++i) {
        if (i > 0) os << ", ";
        os << arr.data_[i];
    }
    os << "]";
    return os;
}
