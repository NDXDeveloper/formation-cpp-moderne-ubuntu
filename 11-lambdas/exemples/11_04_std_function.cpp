/* ============================================================================
   Section 11.4 : std::function et wrappers de fonctions
   Description : Exemples complets — callable objects, std::function, validité,
                 SBO, coût, callbacks, signal/slot, concepts, std::invoke,
                 std::bind, std::move_only_function, bonnes pratiques
   Fichier source : 04-std-function.md
   ============================================================================ */
#include <print>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <array>
#include <concepts>

// Fonctions libres utilisées dans les exemples
int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

// Foncteur (ligne 39-41)
struct Adder {
    int operator()(int a, int b) const { return a + b; }
};

// Calculator (ligne 44-46)
struct Calculator {
    int add(int a, int b) const { return a + b; }
};

// Widget (ligne 404-408)
struct Widget {
    int value = 42;
    int get_value() const { return value; }
    void set_value(int v) { value = v; }
};

// logged_call template (ligne 429-436)
template<typename F, typename... Args>
auto logged_call(F&& func, Args&&... args) {
    std::print("Calling function...\n");
    auto result = std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
    std::print("Done.\n");
    return result;
}

// process template with concepts (ligne 363-376)
template<typename F>
    requires std::predicate<F, int>
void process_concept(const std::vector<int>& data, F predicate) {
    for (int v : data) {
        if (predicate(v)) std::print("{} ", v);
    }
    std::print("\n");
}

// apply with invocable (ligne 386-394)
template<std::invocable<int, int> F>
auto apply(F&& func, int a, int b) {
    return std::invoke(std::forward<F>(func), a, b);
}

int main() {
    // --- Callable objects (ligne 29-49) ---
    {
        int (*fn_ptr)(int, int) = &add;
        auto lambda = [](int a, int b) { return a + b; };
        auto bound = std::bind(&Calculator::add, Calculator{},
                               std::placeholders::_1, std::placeholders::_2);

        std::print("fn_ptr: {}\n", fn_ptr(3, 4));
        std::print("lambda: {}\n", lambda(3, 4));
        std::print("Adder: {}\n", Adder{}(3, 4));
        std::print("bound: {}\n", bound(3, 4));
    }

    // --- std::function usage (ligne 63-83) ---
    {
        std::function<int(int, int)> operation;

        operation = add;
        std::print("{}\n", operation(3, 4));  // 7

        operation = [](int a, int b) { return a * b; };
        std::print("{}\n", operation(3, 4));  // 12

        operation = Adder{};
        std::print("{}\n", operation(3, 4));  // 7

        int offset = 10;
        operation = [offset](int a, int b) { return a + b + offset; };
        std::print("{}\n", operation(3, 4));  // 17
    }

    // --- Vérification de validité (ligne 93-109) ---
    {
        std::function<void()> callback;

        if (callback) {
            callback();
        } else {
            std::print("callback est vide (test bool)\n");
        }

        if (callback != nullptr) {
            callback();
        } else {
            std::print("callback est vide (test nullptr)\n");
        }

        try {
            callback();
        } catch (const std::bad_function_call& e) {
            std::print("Pas de callback enregistré\n");
        }
    }

    // --- Réinitialisation (ligne 117-121) ---
    {
        std::function<void()> fn = []() { std::print("Active\n"); };
        fn();
        fn = nullptr;
        std::print("fn is empty: {}\n", !static_cast<bool>(fn));
    }

    // --- SBO (ligne 178-191) ---
    {
        std::function<int(int)> small = [x = 42](int v) { return v + x; };
        std::function<int(int)> large = [arr = std::array<int, 100>{}](int v) {
            return v + arr[0];
        };
        std::print("small(1) = {}, large(1) = {}\n", small(1), large(1));
        std::print("sizeof(std::function<void()>) = {}\n", sizeof(std::function<void()>));
    }

    // --- Coût mesurable (ligne 201-207) ---
    {
        auto direct = [](int x) { return x * 2; };
        int result1 = direct(42);

        std::function<int(int)> indirect = [](int x) { return x * 2; };
        int result2 = indirect(42);

        std::print("direct={}, indirect={}\n", result1, result2);
    }

    // --- Button callbacks (ligne 229-251) ---
    {
        class Button {
            std::string label_;
            std::function<void()> on_click_;
        public:
            Button(std::string label) : label_(std::move(label)) {}
            void set_on_click(std::function<void()> handler) {
                on_click_ = std::move(handler);
            }
            void click() {
                if (on_click_) on_click_();
            }
        };

        Button btn("Submit");
        btn.set_on_click([]() { std::print("Form submitted!\n"); });
        btn.click();
    }

    // --- Signal/Slot (ligne 310-330) ---
    {
        class Signal {
            std::vector<std::function<void(int)>> slots_;
        public:
            void connect(std::function<void(int)> slot) {
                slots_.push_back(std::move(slot));
            }
            void emit(int value) {
                for (auto& slot : slots_) {
                    slot(value);
                }
            }
        };

        Signal on_value_changed;
        on_value_changed.connect([](int v) { std::print("Logger: {}\n", v); });
        on_value_changed.connect([](int v) { std::print("UI update: {}\n", v); });
        on_value_changed.emit(42);
    }

    // --- Templates avec Concepts (ligne 363-376) ---
    {
        std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8};
        process_concept(data, [](int v) { return v > 5; });
    }

    // --- std::invocable apply (ligne 386-394) ---
    {
        std::print("{}\n", apply([](int a, int b) { return a + b; }, 3, 4));
        std::print("{}\n", apply(add, 3, 4));
        std::print("{}\n", apply(Adder{}, 3, 4));
    }

    // --- std::invoke (ligne 404-423) ---
    {
        Widget w;
        std::print("{}\n", std::invoke(add, 3, 4));  // 7
        std::print("{}\n", std::invoke([](int x) { return x * 2; }, 21));  // 42
        std::print("{}\n", std::invoke(&Widget::get_value, w));  // 42
        std::print("{}\n", std::invoke(&Widget::value, w));  // 42
    }

    // --- logged_call (ligne 429-441) ---
    {
        Widget w;
        logged_call(add, 3, 4);
        logged_call([](int x) { return x; }, 42);
        logged_call(&Widget::get_value, w);
    }

    // --- std::bind (ligne 451-459) ---
    {
        auto times_ten = std::bind(multiply, 10, std::placeholders::_1);
        std::print("{}\n", times_ten(5));  // 50

        auto times_ten_lambda = [](int x) { return multiply(10, x); };
        std::print("{}\n", times_ten_lambda(5));  // 50
    }

    // --- std::move_only_function (ligne 491-500) ---
    {
        auto ptr = std::make_unique<int>(42);
        std::move_only_function<int()> good = [p = std::move(ptr)]() { return *p; };
        std::print("{}\n", good());  // 42
    }

    // --- move_only_function const vs mutable (ligne 512-516) ---
    {
        std::move_only_function<int() const> const_fn = [](){ return 42; };
        std::move_only_function<int()> mut_fn = [x = 0]() mutable { return ++x; };
        std::print("const: {}\n", const_fn());
        std::print("mut1: {}\n", mut_fn());
        std::print("mut2: {}\n", mut_fn());
    }

    // --- Bonnes pratiques — passage const& vs valeur (ligne 618-630) ---
    {
        auto execute_once = [](const std::function<void()>& fn) {
            fn();
        };

        class EventHandler {
            std::function<void(int)> handler_;
        public:
            void set_handler(std::function<void(int)> h) {
                handler_ = std::move(h);
            }
            void trigger(int v) { if (handler_) handler_(v); }
        };

        execute_once([]() { std::print("executed once\n"); });

        EventHandler eh;
        eh.set_handler([](int v) { std::print("event: {}\n", v); });
        eh.trigger(99);
    }

    return 0;
}
