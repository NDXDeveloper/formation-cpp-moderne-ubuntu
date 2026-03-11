/* ============================================================================
   Section 16.2 : Templates de classes — Héritage, friend et alias
   Description : LinkedList avec itérateur imbriqué, héritage entre templates,
                 mixin Loggable, operator<< friend, alias using, Boite
   Fichier source : 02-templates-classes.md
   ============================================================================ */
#include <vector>
#include <stdexcept>
#include <print>
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <iostream>

using namespace std::string_literals;

// Stack simplifié pour l'héritage
template <typename T>
class Stack {
public:
    void push(const T& valeur) { elements_.push_back(valeur); }
    void pop() { elements_.pop_back(); }
    const T& top() const { return elements_.back(); }
    bool empty() const { return elements_.empty(); }
    std::size_t size() const { return elements_.size(); }
private:
    std::vector<T> elements_;
};

// LinkedList avec itérateur imbriqué
template <typename T>
class LinkedList {
private:
    struct Node {
        T data;
        Node* next = nullptr;
        Node(const T& val, Node* n = nullptr) : data{val}, next{n} {}
    };
    Node* head_ = nullptr;
    std::size_t size_ = 0;

public:
    ~LinkedList() {
        while (head_) {
            Node* tmp = head_;
            head_ = head_->next;
            delete tmp;
        }
    }

    void push_front(const T& valeur) {
        head_ = new Node(valeur, head_);
        ++size_;
    }

    class Iterator {
    public:
        explicit Iterator(Node* ptr) : current_{ptr} {}
        const T& operator*() const { return current_->data; }
        Iterator& operator++() { current_ = current_->next; return *this; }
        bool operator!=(const Iterator& other) const { return current_ != other.current_; }
    private:
        Node* current_;
    };

    Iterator begin() const { return Iterator(head_); }
    Iterator end()   const { return Iterator(nullptr); }
};

// Héritage Base/Derived
template <typename T>
class Base {
protected:
    T valeur_;
public:
    explicit Base(const T& v) : valeur_{v} {}
    const T& get() const { return valeur_; }
};

template <typename T>
class Derived : public Base<T> {
public:
    explicit Derived(const T& v) : Base<T>(v) {}
    void doubler() { this->valeur_ *= 2; }
};

// Mixin Loggable
class Loggable {
public:
    void log(const std::string& message) const {
        std::print("[LOG] {}\n", message);
    }
};

template <typename T>
class LoggedStack : public Loggable, public Stack<T> {
public:
    void push(const T& valeur) {
        this->log("push");
        Stack<T>::push(valeur);
    }
};

// Wrapper avec friend operator<<
template <typename T>
class Wrapper {
public:
    explicit Wrapper(const T& val) : valeur_{val} {}

    template <typename U>
    friend std::ostream& operator<<(std::ostream&, const Wrapper<U>&);
private:
    T valeur_;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const Wrapper<T>& w) {
    return os << "Wrapper(" << w.valeur_ << ")";
}

// Alias de templates
template <typename V>
using StringMap = std::map<std::string, V>;

// Boite (instanciation paresseuse)
template <typename T>
class Boite {
public:
    explicit Boite(const T& val) : valeur_{val} {}
    T get() const { return valeur_; }
    void afficher() const { std::print("{}\n", valeur_); }
    bool est_positif() const { return valeur_ > T{0}; }
private:
    T valeur_;
};

int main() {
    // LinkedList
    {
        LinkedList<int> liste;
        liste.push_front(30);
        liste.push_front(20);
        liste.push_front(10);
        for (auto val : liste) {
            std::print("{} ", val);
        }
        std::println("");  // 10 20 30
    }

    // Héritage
    {
        Derived<int> d{5};
        std::print("Before: {}\n", d.get());  // 5
        d.doubler();
        std::print("After: {}\n", d.get());   // 10
    }

    // LoggedStack
    {
        LoggedStack<int> ls;
        ls.push(42);
        std::print("LoggedStack top: {}\n", ls.top());  // 42
    }

    // Wrapper friend
    {
        Wrapper w{42};
        std::cout << w << "\n";  // Wrapper(42)
    }

    // StringMap alias
    {
        StringMap<int> ages;
        ages["Alice"] = 30;
        ages["Bob"]   = 25;
        std::print("Alice: {}\n", ages["Alice"]);  // 30
    }

    // Boite (instanciation paresseuse)
    {
        struct Point { int x, y; };
        Boite<Point> b{Point{3, 4}};
        auto p = b.get();
        std::print("Point: ({}, {})\n", p.x, p.y);
        // b.afficher();    // ne compile pas
        // b.est_positif(); // ne compile pas
    }
}
