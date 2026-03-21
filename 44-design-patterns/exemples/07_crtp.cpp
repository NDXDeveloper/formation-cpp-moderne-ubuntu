/* ============================================================================
   Section 44.3 : CRTP (Curiously Recurring Template Pattern)
   Description : CRTP classique et deducing this (C++23)
   Fichier source : 03-crtp.md
   ============================================================================ */

#include <numbers>
#include <print>

// --- CRTP classique ---
template<typename Derived>
class ShapeCRTP {
public:
    double area() const { return self().area_impl(); }
    void describe() const { std::print("CRTP: aire = {:.2f}\n", self().area_impl()); }
private:
    const Derived& self() const { return static_cast<const Derived&>(*this); }
};

class CircleCRTP : public ShapeCRTP<CircleCRTP> {
    double radius_;
public:
    explicit CircleCRTP(double r) : radius_(r) {}
    double area_impl() const { return std::numbers::pi * radius_ * radius_; }
};

class RectCRTP : public ShapeCRTP<RectCRTP> {
    double w_, h_;
public:
    RectCRTP(double w, double h) : w_(w), h_(h) {}
    double area_impl() const { return w_ * h_; }
};

// --- deducing this (C++23) ---
struct Base23 {
    template<typename Self>
    void describe(this const Self& self) {
        std::print("C++23: aire = {:.2f}\n", self.area());
    }
};

struct Circle23 : Base23 {
    double radius;
    double area() const { return std::numbers::pi * radius * radius; }
};

struct Rect23 : Base23 {
    double width, height;
    double area() const { return width * height; }
};

int main() {
    CircleCRTP c(5.0);
    RectCRTP r(3.0, 4.0);
    c.describe();
    r.describe();

    Circle23 c23{{}, 5.0};
    Rect23 r23{{}, 3.0, 4.0};
    c23.describe();
    r23.describe();
}
