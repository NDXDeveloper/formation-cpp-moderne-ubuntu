/* ============================================================================
   Section 8.3 : Opérateurs de conversion
   Description : OptionalRef — wrapper pointeur avec explicit operator bool(),
                 operator*() et operator->(), pattern smart pointer
   Fichier source : 03-operateurs-conversion.md
   ============================================================================ */
#include <string>
#include <print>

template <typename T>
class OptionalRef {
    T* ptr_;

public:
    OptionalRef() : ptr_{nullptr} {}
    explicit OptionalRef(T& ref) : ptr_{&ref} {}

    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    T& operator*() const {
        return *ptr_;
    }

    T* operator->() const {
        return ptr_;
    }
};

int main() {
    std::string texte = "Hello";
    OptionalRef<std::string> ref{texte};

    if (ref) {                            // explicit operator bool
        std::println("{}", ref->size());  // operator-> → 5
        std::println("{}", *ref);         // operator* → Hello
    }

    OptionalRef<std::string> vide;
    if (!vide) {
        std::println("vide est null");
    }
}
