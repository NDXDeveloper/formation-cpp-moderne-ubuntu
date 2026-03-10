/* ============================================================================
   Section 6.1 : Définition de classes — Fil conducteur
   Description : Première version de DynArray — structure de base avec
                 constructeur, destructeur, operator[], size(), empty()
   Fichier source : 01-definition-classes.md
   ============================================================================ */
#pragma once

#include <cstddef>

class DynArray {
public:
    explicit DynArray(std::size_t size);
    ~DynArray();

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    int& operator[](std::size_t index);
    const int& operator[](std::size_t index) const;

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};
