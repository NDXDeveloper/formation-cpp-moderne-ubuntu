/* ============================================================================
   Section 44.4 : Type Erasure et std::any
   Description : Custom type erasure AnyDrawable — collection heterogene sans heritage
   Fichier source : 04-type-erasure.md
   ============================================================================ */

#include <memory>
#include <string>
#include <vector>
#include <format>
#include <print>

class AnyDrawable {
public:
    template<typename T>
    AnyDrawable(T obj)
        : pimpl_(std::make_unique<Model<T>>(std::move(obj))) {}

    AnyDrawable(const AnyDrawable& other)
        : pimpl_(other.pimpl_ ? other.pimpl_->clone() : nullptr) {}

    AnyDrawable& operator=(const AnyDrawable& other) {
        if (this != &other)
            pimpl_ = other.pimpl_ ? other.pimpl_->clone() : nullptr;
        return *this;
    }

    AnyDrawable(AnyDrawable&&) noexcept = default;
    AnyDrawable& operator=(AnyDrawable&&) noexcept = default;

    void draw() const { pimpl_->draw(); }
    std::string describe() const { return pimpl_->describe(); }

private:
    struct Concept {
        virtual ~Concept() = default;
        virtual void draw() const = 0;
        virtual std::string describe() const = 0;
        virtual std::unique_ptr<Concept> clone() const = 0;
    };

    template<typename T>
    struct Model final : Concept {
        T obj_;
        explicit Model(T obj) : obj_(std::move(obj)) {}
        void draw() const override { obj_.draw(); }
        std::string describe() const override { return obj_.describe(); }
        std::unique_ptr<Concept> clone() const override {
            return std::make_unique<Model>(obj_);
        }
    };

    std::unique_ptr<Concept> pimpl_;
};

// Types concrets — AUCUN heritage requis
struct Circle {
    double x, y, radius;
    void draw() const {
        std::print("Drawing circle at ({:.1f}, {:.1f}) r={:.1f}\n", x, y, radius);
    }
    std::string describe() const {
        return std::format("Circle(r={:.1f})", radius);
    }
};

struct Rectangle {
    double x, y, width, height;
    void draw() const {
        std::print("Drawing rect at ({:.1f}, {:.1f}) {}x{}\n", x, y, width, height);
    }
    std::string describe() const {
        return std::format("Rect({}x{})", width, height);
    }
};

struct Text {
    double x, y;
    std::string content;
    void draw() const {
        std::print("Drawing '{}' at ({:.1f}, {:.1f})\n", content, x, y);
    }
    std::string describe() const {
        return std::format("Text('{}')", content);
    }
};

int main() {
    std::vector<AnyDrawable> scene;
    scene.emplace_back(Circle{100, 100, 50});
    scene.emplace_back(Rectangle{200, 150, 80, 40});
    scene.emplace_back(Text{300, 200, "Hello"});

    for (const auto& drawable : scene) {
        drawable.draw();
        std::print("  -> {}\n", drawable.describe());
    }
}
