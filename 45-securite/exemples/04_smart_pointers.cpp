/* ============================================================================
   Section 45.3 : Use-after-free et temporal safety
   Description : Prevention du UAF via unique_ptr, shared_ptr et weak_ptr
   Fichier source : 03-use-after-free.md
   ============================================================================ */

#include <memory>
#include <print>

struct Widget {
    int id;
    void render() const { std::print("Widget #{}\n", id); }
};

void unique_ptr_demo() {
    auto w = std::make_unique<Widget>(42);
    w->render();

    auto moved = std::move(w);
    // w est maintenant nullptr
    moved->render();
    std::print("unique_ptr: ownership transferred\n");
}

void shared_ptr_demo() {
    auto w = std::make_shared<Widget>(99);
    {
        auto copy = w;  // ref count = 2
        copy->render();
    }
    // copy detruit, ref count = 1
    w->render();  // Toujours vivant
    std::print("shared_ptr: object survived inner scope\n");
}

void weak_ptr_demo() {
    std::weak_ptr<Widget> observer;
    {
        auto w = std::make_shared<Widget>(7);
        observer = w;
        if (auto locked = observer.lock()) {
            locked->render();  // OK
        }
    }
    // w detruit
    if (auto locked = observer.lock()) {
        locked->render();
    } else {
        std::print("weak_ptr: object expired (correct!)\n");
    }
}

int main() {
    unique_ptr_demo();
    std::print("\n");
    shared_ptr_demo();
    std::print("\n");
    weak_ptr_demo();
}
