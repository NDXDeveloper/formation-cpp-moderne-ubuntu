/* ============================================================================
   Section 3.3.4 : dynamic_cast - Cast polymorphique
   Description : Downcast avec pointeurs et references, cross-cast,
                 typeid, nullptr
   Fichier source : 03.4-dynamic-cast.md
   ============================================================================ */
#include <print>
#include <string>
#include <typeinfo>
#include <memory>

// --- Hierarchie Animal (lignes 35-51) ---
class Animal {
public:
    virtual ~Animal() = default;
    virtual std::string speak() const = 0;
};

class Dog : public Animal {
public:
    std::string speak() const override { return "Woof!"; }
    void fetch() const { std::print("Fetching!\n"); }
};

class Cat : public Animal {
public:
    std::string speak() const override { return "Meow!"; }
    void purr() const { std::print("Purrrr...\n"); }
};

// --- Hierarchie pour cross-cast (lignes 137-153) ---
class Drawable {
public:
    virtual ~Drawable() = default;
    virtual void draw() const = 0;
};

class Clickable {
public:
    virtual ~Clickable() = default;
    virtual void on_click() = 0;
};

class Button : public Drawable, public Clickable {
public:
    void draw() const override { std::print("Drawing button\n"); }
    void on_click() override { std::print("Button clicked\n"); }
};

int main() {
    // --- Downcast avec pointeurs (lignes 64-75) ---
    std::unique_ptr<Animal> dog_ptr = std::make_unique<Dog>();
    std::unique_ptr<Animal> cat_ptr = std::make_unique<Cat>();

    Dog* dog = dynamic_cast<Dog*>(dog_ptr.get());
    if (dog != nullptr) {
        std::print("dog->speak() = {}\n", dog->speak());
        dog->fetch();
    }

    Dog* not_a_dog = dynamic_cast<Dog*>(cat_ptr.get());
    if (not_a_dog == nullptr) {
        std::print("cat_ptr n'est pas un Dog (nullptr)\n");
    }

    // --- Downcast avec references (lignes 99-108) ---
    try {
        Dog& dog_ref = dynamic_cast<Dog&>(*dog_ptr);
        std::print("dog_ref.speak() = {}\n", dog_ref.speak());
    } catch (const std::bad_cast& e) {
        std::print("Conversion echouee : {}\n", e.what());
    }

    try {
        [[maybe_unused]] Dog& bad_ref = dynamic_cast<Dog&>(*cat_ptr);
    } catch (const std::bad_cast& e) {
        std::print("bad_cast sur cat : {}\n", e.what());
    }

    // --- Cross-cast (lignes 156-163) ---
    std::unique_ptr<Drawable> widget = std::make_unique<Button>();
    Clickable* clickable = dynamic_cast<Clickable*>(widget.get());
    if (clickable) {
        clickable->on_click();
    }

    // --- typeid (lignes 180-186) ---
    Animal* animal = dog_ptr.get();
    std::print("Type dynamique : {}\n", typeid(*animal).name());
    std::print("typeid match Dog : {}\n", typeid(*animal) == typeid(Dog));
    std::print("typeid match Cat : {}\n", typeid(*animal) == typeid(Cat));

    // --- dynamic_cast et nullptr (lignes 216-218) ---
    Animal* null_animal = nullptr;
    Dog* null_dog = dynamic_cast<Dog*>(null_animal);
    std::print("dynamic_cast<Dog*>(nullptr) == nullptr : {}\n", null_dog == nullptr);

    return 0;
}
