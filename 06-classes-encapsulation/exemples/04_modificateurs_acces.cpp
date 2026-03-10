/* ============================================================================
   Section 6.4 : Modificateurs d'accès : public, private, protected
   Description : Encapsulation — public/private/protected, friend operator<<,
                 accesseurs avec validation (Temperature), Shape/Circle avec
                 protected
   Fichier source : 04-modificateurs-acces.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstddef>
#include <algorithm>

// --- Shape / Circle : protected ---
class Shape {
public:
    virtual double area() const = 0;
    virtual ~Shape() = default;

    std::string color() const { return color_; }

protected:
    void set_color(const std::string& c) { color_ = c; }

private:
    std::string color_ = "black";
};

class Circle : public Shape {
public:
    explicit Circle(double radius) : radius_(radius) {
        if (radius < 0) throw std::invalid_argument("Negative radius");
    }

    double area() const override {
        return 3.14159265358979 * radius_ * radius_;
    }

    void make_red() {
        set_color("red");    // OK — set_color est protected
    }

    double radius() const { return radius_; }

private:
    double radius_;
};

// --- Temperature : accesseurs avec validation ---
class Temperature {
public:
    explicit Temperature(double kelvin) : kelvin_(kelvin) {
        if (kelvin < 0.0) throw std::invalid_argument("Below absolute zero");
    }

    double kelvin() const { return kelvin_; }
    double celsius() const { return kelvin_ - 273.15; }

    void set_kelvin(double k) {
        if (k < 0.0) throw std::invalid_argument("Below absolute zero");
        kelvin_ = k;
    }

private:
    double kelvin_;
};

// --- DynArray avec friend operator<< ---
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

    DynArray(const DynArray& other)
        : data_(other.size_ > 0 ? new int[other.size_] : nullptr)
        , size_(other.size_) {
        std::copy_n(other.data_, size_, data_);
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

    // Friend pour l'affichage — accès aux membres privés
    friend std::ostream& operator<<(std::ostream& os, const DynArray& arr);

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};

std::ostream& operator<<(std::ostream& os, const DynArray& arr) {
    os << "[";
    for (std::size_t i = 0; i < arr.size_; ++i) {
        if (i > 0) os << ", ";
        os << arr.data_[i];
    }
    os << "]";
    return os;
}

// --- Engine / Debugger : friend class ---
class Engine {
    friend class Debugger;

private:
    int internal_state_ = 42;
};

class Debugger {
public:
    void inspect(const Engine& e) {
        std::cout << "Engine state: " << e.internal_state_ << "\n";
    }
};

int main() {
    // Shape / Circle
    Circle c(5.0);
    std::cout << "Circle area: " << c.area() << "\n";
    std::cout << "Circle color: " << c.color() << "\n";
    c.make_red();
    std::cout << "Circle color after make_red: " << c.color() << "\n";

    // Temperature
    Temperature t(373.15);
    std::cout << "Temperature: " << t.kelvin() << " K = "
              << t.celsius() << " °C\n";
    t.set_kelvin(273.15);
    std::cout << "Temperature: " << t.kelvin() << " K = "
              << t.celsius() << " °C\n";

    try {
        t.set_kelvin(-10);
    } catch (const std::invalid_argument& e) {
        std::cout << "Exception: " << e.what() << "\n";
    }

    // DynArray avec friend operator<<
    DynArray arr(5, 42);
    arr[2] = 99;
    std::cout << "DynArray: " << arr << "\n";

    DynArray empty;
    std::cout << "Empty DynArray: " << empty << "\n";

    // Engine / Debugger
    Engine engine;
    Debugger debugger;
    debugger.inspect(engine);

    return 0;
}
// Sortie attendue :
// Circle area: 78.5398
// Circle color: black
// Circle color after make_red: red
// Temperature: 373.15 K = 100 °C
// Temperature: 273.15 K = 0 °C
// Exception: Below absolute zero
// DynArray: [42, 42, 99, 42, 42]
// Empty DynArray: []
// Engine state: 42
