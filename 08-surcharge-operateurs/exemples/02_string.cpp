/* ============================================================================
   Section 8.2 : Opérateurs d'affectation
   Description : String simplifiée — Rule of Five complète avec copy-and-swap,
                 operator+= (concaténation) et operator+ (fonction libre)
   Fichier source : 02-operateurs-affectation.md
   ============================================================================ */
#include <algorithm>
#include <cstring>
#include <iostream>
#include <utility>

class String {
    char* data_;
    std::size_t len_;

    void alloc_and_copy(char const* src, std::size_t n) {
        data_ = new char[n + 1];
        std::memcpy(data_, src, n);
        data_[n] = '\0';
        len_ = n;
    }

public:
    // --- Constructeurs ---

    String() : data_{new char[1]{'\0'}}, len_{0} {}

    String(char const* s)
        : data_{nullptr}, len_{0} {
        alloc_and_copy(s, std::strlen(s));
    }

    // Constructeur de copie
    String(String const& other)
        : data_{nullptr}, len_{0} {
        alloc_and_copy(other.data_, other.len_);
    }

    // Constructeur de déplacement
    String(String&& other) noexcept
        : data_{other.data_}, len_{other.len_} {
        other.data_ = nullptr;
        other.len_ = 0;
    }

    // --- Destructeur ---

    ~String() { delete[] data_; }

    // --- swap ---

    void swap(String& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(len_, other.len_);
    }

    // --- operator= (copy-and-swap) ---

    String& operator=(String other) {   // par valeur → copie ou déplacement
        swap(other);
        return *this;
    }

    // --- operator+= (concaténation) ---

    String& operator+=(String const& rhs) {
        char* nouveau = new char[len_ + rhs.len_ + 1];
        std::memcpy(nouveau, data_, len_);
        std::memcpy(nouveau + len_, rhs.data_, rhs.len_);
        nouveau[len_ + rhs.len_] = '\0';
        delete[] data_;
        data_ = nouveau;
        len_ += rhs.len_;
        return *this;
    }

    String& operator+=(char const* rhs) {
        return *this += String{rhs};   // réutilise la version String
    }

    // --- Accesseurs ---

    char const* c_str() const { return data_; }
    std::size_t taille() const { return len_; }

    // --- Flux ---

    friend std::ostream& operator<<(std::ostream& os, String const& s) {
        return os << s.data_;
    }
};

// --- operator+ (fonction libre) ---

String operator+(String lhs, String const& rhs) {
    lhs += rhs;
    return lhs;
}

String operator+(String lhs, char const* rhs) {
    lhs += rhs;
    return lhs;
}

int main() {
    String a{"Hello"};
    String b{" World"};

    String c = a + b;      // operator+ → copie de a, puis +=
    std::cout << c << "\n";  // Hello World

    a += "!";              // operator+= avec char const*
    std::cout << a << "\n";  // Hello!

    String d;
    d = a;                 // operator= par copie (copy-and-swap)
    std::cout << "d = " << d << "\n";  // Hello!

    d = std::move(b);      // operator= par déplacement (move + swap)
    std::cout << "d après move = " << d << "\n";  // World (avec l'espace devant)
}
