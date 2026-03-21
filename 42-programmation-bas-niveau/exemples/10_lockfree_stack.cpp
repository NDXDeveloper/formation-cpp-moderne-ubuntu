/* ============================================================================
   Section 42.4.1 : Structures Lock-free
   Description : Pile lock-free naive (push/pop via CAS sur pointeur head)
   Fichier source : 04.1-structures-lock-free.md
   ============================================================================ */

#include <atomic>
#include <optional>
#include <iostream>
#include <string>

template <typename T>
class NaiveLockFreeStack {
    struct Node {
        T data;
        Node* next;
        template <typename U>
        Node(U&& val, Node* n) : data(std::forward<U>(val)), next(n) {}
    };

    std::atomic<Node*> head_{nullptr};

public:
    void push(const T& value) {
        Node* new_node = new Node(value, nullptr);
        new_node->next = head_.load(std::memory_order_relaxed);
        while (!head_.compare_exchange_weak(
                    new_node->next, new_node,
                    std::memory_order_release,
                    std::memory_order_relaxed)) {}
    }

    std::optional<T> pop() {
        Node* expected = head_.load(std::memory_order_acquire);
        while (expected != nullptr) {
            if (head_.compare_exchange_weak(
                        expected, expected->next,
                        std::memory_order_acquire,
                        std::memory_order_relaxed)) {
                T value = std::move(expected->data);
                delete expected;
                return value;
            }
        }
        return std::nullopt;
    }

    ~NaiveLockFreeStack() {
        while (pop().has_value()) {}
    }
};

int main() {
    NaiveLockFreeStack<int> stack;

    // Push 1, 2, 3
    stack.push(1);
    stack.push(2);
    stack.push(3);

    // Pop : LIFO -> 3, 2, 1
    auto v1 = stack.pop();
    auto v2 = stack.pop();
    auto v3 = stack.pop();
    auto v4 = stack.pop();  // vide

    std::cout << "Pop 1: " << (v1 ? std::to_string(*v1) : "empty") << "\n";
    std::cout << "Pop 2: " << (v2 ? std::to_string(*v2) : "empty") << "\n";
    std::cout << "Pop 3: " << (v3 ? std::to_string(*v3) : "empty") << "\n";
    std::cout << "Pop 4: " << (v4 ? std::to_string(*v4) : "empty") << "\n";

    bool ok = v1 && *v1 == 3 && v2 && *v2 == 2 && v3 && *v3 == 1 && !v4;
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
}
