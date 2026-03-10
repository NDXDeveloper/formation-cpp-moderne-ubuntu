/* ============================================================================
   Section 6.2.1 : Constructeur par défaut
   Description : Constructeur par défaut — génération implicite, = default,
                 piège des types primitifs non initialisés, most vexing parse,
                 DynArray avec constructeur par défaut
   Fichier source : 02.1-constructeur-defaut.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <vector>
#include <cstddef>
#include <stdexcept>

// --- A et B : deux formes de constructeur par défaut ---
class A {
public:
    A() {}
};

class B {
public:
    B(int x = 0, int y = 0) : x_(x), y_(y) {}
    int x() const { return x_; }
    int y() const { return y_; }
private:
    int x_, y_;
};

// --- ImplicitDefault : constructeur implicite + default member initializers ---
class ImplicitDefault {
public:
    int count() const { return count_; }
    const std::string& label() const { return label_; }
private:
    int count_ = 0;
    std::string label_ = "none";
};

// --- Config : = default combiné avec default member initializers ---
class Config {
public:
    Config() = default;
    Config(int timeout, int retries) : timeout_(timeout), retries_(retries) {}
    int timeout() const { return timeout_; }
    int retries() const { return retries_; }
private:
    int timeout_ = 30;
    int retries_ = 3;
};

// --- Widget : contextes d'appel du constructeur par défaut ---
class Widget {
public:
    Widget() { std::cout << "Widget()\n"; }
};

// --- DynArray : constructeur par défaut ---
class DynArray {
public:
    DynArray() = default;
    explicit DynArray(std::size_t size)
        : data_(new int[size]{}), size_(size) {}
    ~DynArray() { delete[] data_; }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    int& operator[](std::size_t index) {
        if (index >= size_) throw std::out_of_range("DynArray: index out of range");
        return data_[index];
    }

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};

int main() {
    // A et B
    A a;
    B b;
    std::cout << "B default: " << b.x() << ", " << b.y() << "\n";

    // ImplicitDefault
    ImplicitDefault obj;
    std::cout << "count: " << obj.count()
              << ", label: " << obj.label() << "\n";

    // Config
    Config c1;
    Config c2(60, 5);
    std::cout << "c1: timeout=" << c1.timeout()
              << " retries=" << c1.retries() << "\n";
    std::cout << "c2: timeout=" << c2.timeout()
              << " retries=" << c2.retries() << "\n";

    // Widget : divers contextes d'appel
    std::cout << "--- Widget contexts ---\n";
    Widget w1;
    Widget w2{};

    // DynArray : constructeur par défaut
    std::cout << "--- DynArray ---\n";
    DynArray empty_arr;
    std::cout << empty_arr.size() << "\n";    // 0
    std::cout << empty_arr.empty() << "\n";   // 1

    DynArray filled(5);
    std::cout << filled.size() << "\n";       // 5
    filled[0] = 42;
    std::cout << filled[0] << "\n";           // 42

    return 0;
}
// Sortie attendue :
// B default: 0, 0
// count: 0, label: none
// c1: timeout=30 retries=3
// c2: timeout=60 retries=5
// --- Widget contexts ---
// Widget()
// Widget()
// --- DynArray ---
// 0
// 1
// 5
// 42
