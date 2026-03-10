/* ============================================================================
   Section 6.1 : Définition de classes — Fil conducteur
   Description : Première version de DynArray — implémentation
   Fichier source : 01-definition-classes.md
   Compilation : g++ -std=c++17 -Wall -Wextra 01_dynarray_v1.cpp
                     01_dynarray_v1_main.cpp -o 01_dynarray_v1
   ============================================================================ */
#include "01_dynarray_v1.h"

#include <stdexcept>

DynArray::DynArray(std::size_t size)
    : data_(new int[size]{}), size_(size) {}

DynArray::~DynArray() {
    delete[] data_;
}

int& DynArray::operator[](std::size_t index) {
    if (index >= size_) {
        throw std::out_of_range("DynArray: index out of range");
    }
    return data_[index];
}

const int& DynArray::operator[](std::size_t index) const {
    if (index >= size_) {
        throw std::out_of_range("DynArray: index out of range");
    }
    return data_[index];
}
