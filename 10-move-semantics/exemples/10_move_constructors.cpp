/* ============================================================================
   Section 10.3 : Move constructors et move assignment operators
   Description : Buffer (move constructor, move assignment), Session (membres
                 non-primitifs), copy-and-swap idiom, noexcept vérification,
                 Règle du 0 (UserProfile), ResourceHandle (Règle des 5),
                 = default avec destructeur virtuel
   Fichier source : 03-move-constructors.md
   ============================================================================ */
#include <memory>
#include <print>
#include <string>
#include <vector>
#include <cstring>
#include <utility>
#include <optional>
#include <type_traits>

// === Buffer avec move constructor et move assignment (lignes 37-152) ===
class Buffer {
    char* data_;
    size_t size_;
    size_t capacity_;

public:
    explicit Buffer(size_t cap)
        : data_(new char[cap]), size_(0), capacity_(cap) {
        std::print("[Buffer] Allocation cap={}\n", cap);
    }

    ~Buffer() {
        if (data_)
            std::print("[Buffer] Libération cap={}\n", capacity_);
        delete[] data_;
    }

    Buffer(const Buffer& other)
        : data_(new char[other.capacity_])
        , size_(other.size_)
        , capacity_(other.capacity_)
    {
        std::memcpy(data_, other.data_, size_);
        std::print("[Buffer] Copie cap={}\n", capacity_);
    }

    Buffer(Buffer&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
        , capacity_(other.capacity_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        std::print("[Buffer] Move cap={}\n", capacity_);
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
            std::print("[Buffer] Move assign cap={}\n", capacity_);
        }
        return *this;
    }

    size_t capacity() const { return capacity_; }
};

void test_buffer() {
    std::print("=== Buffer move ===\n");
    Buffer a(1024);

    std::print("--- Copie ---\n");
    Buffer b = a;

    std::print("--- Move constructor ---\n");
    Buffer c = std::move(a);

    std::print("--- Move assignment ---\n");
    Buffer d(512);
    d = std::move(b);
    std::print("--- Fin scope ---\n");
}

// === Session avec membres non-primitifs (lignes 81-95) ===
struct Connection {
    std::string host;
    ~Connection() { std::print("  ~Connection({})\n", host); }
};
struct Message {
    std::string content;
};

class Session {
    std::string id_;
    std::unique_ptr<Connection> conn_;
    std::vector<Message> historique_;

public:
    Session(std::string id, std::unique_ptr<Connection> conn)
        : id_(std::move(id)), conn_(std::move(conn)) {}

    Session(Session&& other) noexcept
        : id_(std::move(other.id_))
        , conn_(std::move(other.conn_))
        , historique_(std::move(other.historique_))
    {}

    void info() const {
        std::print("Session id='{}', conn={}\n", id_,
                   conn_ ? conn_->host : "null");
    }
};

void test_session() {
    std::print("\n=== Session move ===\n");
    Session s1("ABC", std::make_unique<Connection>("localhost"));
    s1.info();

    Session s2 = std::move(s1);
    std::print("Après move:\n");
    s1.info();
    s2.info();
}

// === Copy-and-swap idiom (lignes 174-203) ===
class SwapBuffer {
    char* data_;
    size_t size_;
    size_t capacity_;

public:
    explicit SwapBuffer(size_t cap)
        : data_(new char[cap]{}), size_(0), capacity_(cap) {
        std::print("[SwapBuffer] Alloc cap={}\n", cap);
    }

    ~SwapBuffer() { delete[] data_; }

    SwapBuffer(const SwapBuffer& other)
        : data_(new char[other.capacity_])
        , size_(other.size_)
        , capacity_(other.capacity_)
    {
        std::memcpy(data_, other.data_, size_);
        std::print("[SwapBuffer] Copie cap={}\n", capacity_);
    }

    SwapBuffer(SwapBuffer&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
        , capacity_(other.capacity_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        std::print("[SwapBuffer] Move cap={}\n", capacity_);
    }

    friend void swap(SwapBuffer& a, SwapBuffer& b) noexcept {
        using std::swap;
        swap(a.data_, b.data_);
        swap(a.size_, b.size_);
        swap(a.capacity_, b.capacity_);
    }

    // Un seul opérateur — gère copie ET déplacement
    SwapBuffer& operator=(SwapBuffer other) noexcept {
        swap(*this, other);
        std::print("[SwapBuffer] operator= via swap\n");
        return *this;
    }

    size_t capacity() const { return capacity_; }
};

void test_copy_and_swap() {
    std::print("\n=== Copy-and-swap ===\n");
    SwapBuffer a(1024);
    SwapBuffer b(512);

    std::print("--- Copie via operator= ---\n");
    b = a;  // lvalue → copie + swap

    std::print("--- Move via operator= ---\n");
    SwapBuffer c(256);
    c = std::move(a);  // rvalue → move + swap
}

// === noexcept vérification (lignes 267-272) ===
void test_noexcept() {
    std::print("\n=== noexcept vérification ===\n");
    static_assert(std::is_nothrow_move_constructible_v<Buffer>,
                  "Buffer doit être nothrow move constructible");
    std::print("Buffer: is_nothrow_move_constructible = true\n");

    static_assert(std::is_nothrow_move_constructible_v<SwapBuffer>,
                  "SwapBuffer doit être nothrow move constructible");
    std::print("SwapBuffer: is_nothrow_move_constructible = true\n");
}

// === Règle du 0 (lignes 336-344) ===
class UserProfile {
    std::string name_;
    std::string email_;
    std::vector<std::string> roles_;
    std::optional<std::string> avatar_url_;
public:
    UserProfile(std::string name, std::string email)
        : name_(std::move(name)), email_(std::move(email)) {}
    void info() const { std::print("UserProfile: {} <{}>\n", name_, email_); }
};

void test_rule_of_zero() {
    std::print("\n=== Règle du 0 ===\n");
    UserProfile p("Alice", "alice@example.com");
    p.info();

    UserProfile p2 = std::move(p);
    std::print("Après move:\n");
    p.info();   // vide
    p2.info();  // Alice

    static_assert(std::is_nothrow_move_constructible_v<UserProfile>);
    std::print("UserProfile: nothrow move constructible = true\n");
}

// === ResourceHandle complet — Règle des 5 (lignes 392-471) ===
class ResourceHandle {
    char* data_;
    size_t size_;

public:
    explicit ResourceHandle(size_t size)
        : data_(size > 0 ? new char[size]{} : nullptr)
        , size_(size)
    {}

    ~ResourceHandle() { delete[] data_; }

    ResourceHandle(const ResourceHandle& other)
        : data_(other.size_ > 0 ? new char[other.size_] : nullptr)
        , size_(other.size_)
    {
        if (data_) std::memcpy(data_, other.data_, size_);
    }

    ResourceHandle& operator=(const ResourceHandle& other) {
        if (this != &other) {
            ResourceHandle temp(other);
            swap(*this, temp);
        }
        return *this;
    }

    ResourceHandle(ResourceHandle&& other) noexcept
        : data_(other.data_), size_(other.size_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    ResourceHandle& operator=(ResourceHandle&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    friend void swap(ResourceHandle& a, ResourceHandle& b) noexcept {
        using std::swap;
        swap(a.data_, b.data_);
        swap(a.size_, b.size_);
    }

    const char* data() const noexcept { return data_; }
    size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
};

void test_resource_handle() {
    std::print("\n=== ResourceHandle ===\n");

    static_assert(std::is_nothrow_move_constructible_v<ResourceHandle>);
    static_assert(std::is_nothrow_move_assignable_v<ResourceHandle>);
    std::print("static_assert OK\n");

    ResourceHandle a(100);
    std::print("a.size() = {}, empty = {}\n", a.size(), a.empty());

    ResourceHandle b = a;  // copie
    std::print("b (copie) size = {}\n", b.size());

    ResourceHandle c = std::move(a);  // move
    std::print("c (move) size = {}, a.empty = {}\n", c.size(), a.empty());

    ResourceHandle d(50);
    d = std::move(c);  // move assign
    std::print("d (move assign) size = {}, c.empty = {}\n", d.size(), c.empty());

    ResourceHandle e(200);
    e = b;  // copy assign
    std::print("e (copy assign) size = {}\n", e.size());
}

// === = default avec destructeur virtuel (lignes 354-363) ===
class Base {
public:
    virtual ~Base() = default;
    Base() = default;
    Base(const Base&) = default;
    Base& operator=(const Base&) = default;
    Base(Base&&) noexcept = default;
    Base& operator=(Base&&) noexcept = default;

    virtual void hello() const { std::print("Base::hello\n"); }
};

class Derived : public Base {
    std::string name_;
public:
    Derived(std::string n) : name_(std::move(n)) {}
    void hello() const override { std::print("Derived::hello({})\n", name_); }
};

void test_default() {
    std::print("\n=== = default ===\n");
    static_assert(std::is_nothrow_move_constructible_v<Base>);
    std::print("Base: nothrow move constructible = true\n");

    Derived d("test");
    d.hello();
}

int main() {
    test_buffer();
    test_session();
    test_copy_and_swap();
    test_noexcept();
    test_rule_of_zero();
    test_resource_handle();
    test_default();
    std::print("\n✅ Tous les exemples passés\n");
}
