/* ============================================================================
   Section 44.1 : Singleton, Factory, Builder
   Description : Factory a base de std::variant — polymorphisme sans vtable
   Fichier source : 01-creational-patterns.md
   ============================================================================ */

#include <variant>
#include <string_view>
#include <stdexcept>
#include <numbers>
#include <print>
#include <type_traits>

struct Circle    { double radius; };
struct Rectangle { double width, height; };

using Shape = std::variant<Circle, Rectangle>;

Shape create_shape(std::string_view type, double a, double b) {
    if (type == "circle")    return Circle{a};
    if (type == "rectangle") return Rectangle{a, b};
    throw std::invalid_argument("Unknown shape type");
}

double area(const Shape& s) {
    return std::visit([](const auto& shape) -> double {
        using T = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<T, Circle>)
            return std::numbers::pi * shape.radius * shape.radius;
        else if constexpr (std::is_same_v<T, Rectangle>)
            return shape.width * shape.height;
    }, s);
}

int main() {
    auto c = create_shape("circle", 5.0, 0);
    auto r = create_shape("rectangle", 3.0, 4.0);

    std::print("Circle area: {:.2f}\n", area(c));
    std::print("Rectangle area: {:.2f}\n", area(r));
}
