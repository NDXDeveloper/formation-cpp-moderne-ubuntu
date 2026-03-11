/* ============================================================================
   Section 16.2 : Templates de classes — NTTP, défauts et membres statiques
   Description : FixedBuffer avec NTTP, Grid avec valeurs par défaut,
                 Compteur avec membres statiques, alias de types
   Fichier source : 02-templates-classes.md
   ============================================================================ */
#include <stdexcept>
#include <print>
#include <string>
#include <vector>

// FixedBuffer avec NTTP
template <typename T, std::size_t N>
class FixedBuffer {
public:
    void set(std::size_t index, const T& valeur) {
        if (index >= N) throw std::out_of_range("Index hors limites");
        data_[index] = valeur;
    }
    const T& get(std::size_t index) const {
        if (index >= N) throw std::out_of_range("Index hors limites");
        return data_[index];
    }
    constexpr std::size_t capacity() const { return N; }
private:
    T data_[N];
};

// Grid avec valeurs par défaut
template <typename T = int, std::size_t N = 10>
class Grid {
    T data_[N][N];
public:
    constexpr std::size_t dimension() const { return N; }
};

// Compteur avec membres statiques
template <typename T>
class Compteur {
public:
    Compteur() { ++instances_; }
    ~Compteur() { --instances_; }
    static int instances() { return instances_; }
private:
    static inline int instances_ = 0;
};

// DynamicArray avec alias de types
template <typename T>
class DynamicArray {
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using reference       = T&;
    using const_reference = const T&;
    using iterator        = T*;
    using const_iterator  = const T*;

    iterator begin() { return data_; }
    iterator end()   { return data_ + size_; }
private:
    T* data_     = nullptr;
    size_type size_     = 0;
    size_type capacity_ = 0;
};

// afficher_premier avec typename
template <typename Container>
void afficher_premier(const Container& c) {
    typename Container::value_type premier = *c.begin();
    std::print("{}\n", premier);
}

int main() {
    // FixedBuffer
    {
        FixedBuffer<int, 128> buffer;
        static_assert(buffer.capacity() == 128);
        std::print("buffer capacity: {}\n", buffer.capacity());  // 128
    }

    // Grid avec défauts
    {
        Grid<> g1;
        Grid<double> g2;
        Grid<float, 20> g3;
        std::print("Grid<> dimension: {}\n", g1.dimension());         // 10
        std::print("Grid<double> dimension: {}\n", g2.dimension());   // 10
        std::print("Grid<float,20> dimension: {}\n", g3.dimension()); // 20
    }

    // Compteur
    {
        Compteur<int> a, b, c;
        Compteur<double> x, y;
        std::print("Instances int    : {}\n", Compteur<int>::instances());     // 3
        std::print("Instances double : {}\n", Compteur<double>::instances());  // 2
    }

    // DynamicArray alias
    {
        DynamicArray<double>::value_type val = 3.14;
        DynamicArray<double>::size_type  idx = 0;
        std::print("val={}, idx={}\n", val, idx);
    }

    // afficher_premier
    {
        std::vector<int> v{10, 20, 30};
        afficher_premier(v);  // 10
    }
}
