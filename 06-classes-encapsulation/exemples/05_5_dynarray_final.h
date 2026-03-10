/* ============================================================================
   Section 6.5.5 : Opérateur d'affectation par déplacement
   Description : DynArray version finale — Règle des 5 complète
                 (header)
   Fichier source : 05.5-move-assignment.md
   ============================================================================ */
#pragma once

#include <cstddef>
#include <iosfwd>

class DynArray {
public:
    // --- Constructeurs ---
    DynArray() = default;
    explicit DynArray(std::size_t size);
    DynArray(std::size_t size, int value);

    // --- Règle des 5 ---
    ~DynArray();                                         // 6.5.1 — Destructeur
    DynArray(const DynArray& other);                     // 6.5.2 — Copie ctor
    DynArray& operator=(const DynArray& other);          // 6.5.3 — Copie =
    DynArray(DynArray&& other) noexcept;                 // 6.5.4 — Move ctor
    DynArray& operator=(DynArray&& other) noexcept;      // 6.5.5 — Move =

    // --- Interface publique ---
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    int& operator[](std::size_t index);
    const int& operator[](std::size_t index) const;

    void swap(DynArray& other) noexcept;

    // --- Friend ---
    friend std::ostream& operator<<(std::ostream& os, const DynArray& arr);

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};

inline void swap(DynArray& a, DynArray& b) noexcept { a.swap(b); }
