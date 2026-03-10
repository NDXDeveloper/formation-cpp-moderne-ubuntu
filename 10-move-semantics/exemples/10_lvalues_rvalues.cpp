/* ============================================================================
   Section 10.1 : L-values vs R-values (&&)
   Description : Lvalues, rvalues, références rvalue, paradoxe de la rvalue
                 nommée, surcharge lvalue/rvalue (MyString), forwarding refs
   Fichier source : 01-lvalues-rvalues.md
   ============================================================================ */
#include <print>
#include <string>
#include <cstring>

// === Lvalues (lignes 25-34) ===
void test_lvalues() {
    std::print("=== Lvalues ===\n");
    int x = 42;
    std::string nom = "Alice";
    int tab[10];
    tab[3] = 7;

    int* ptr = &x;
    *ptr = 10;
    std::print("x = {}\n", x);

    int& ref = x;
    std::print("ref = {}\n", ref);
}

// === Rvalues (lignes 43-51) ===
void test_rvalues() {
    std::print("\n=== Rvalues ===\n");
    // 42, 3.14 + 2.0, std::string("Hello") sont des rvalues
    // On ne peut pas prendre l'adresse d'une rvalue
    std::print("42 est une rvalue\n");
    std::print("3.14 + 2.0 = {}\n", 3.14 + 2.0);
}

// === Test d'adresse (lignes 83-89) ===
void test_adresse() {
    std::print("\n=== Test d'adresse ===\n");
    int x = 42;
    &x;                     // ✅ x est une lvalue
    // &(x + 1);           // ❌ Ne compile pas
    // &std::string("Hi"); // ❌ Ne compile pas
    &(*(&x));              // ✅ *(&x) est une lvalue
    std::print("Tests d'adresse OK\n");
}

// === Références rvalue (lignes 136-141) ===
void test_rvalue_refs() {
    std::print("\n=== Références rvalue ===\n");
    int x = 42;
    int&& rref = 42;
    int&& rref2 = x + 1;
    std::string&& rref3 = std::string("Hi");

    // int&& rref4 = x;   // ❌ Ne compile pas : x est une lvalue

    std::print("rref = {}\n", rref);
    std::print("rref2 = {}\n", rref2);
    std::print("rref3 = {}\n", rref3);
}

// === const T& se lie aux rvalues (lignes 122-127) ===
void test_const_ref() {
    std::print("\n=== const T& et rvalues ===\n");
    const int& ref = 42;
    const std::string& r = std::string("Hi");
    std::print("ref = {}\n", ref);
    std::print("r = {}\n", r);
}

// === Paradoxe : référence rvalue nommée est une lvalue (lignes 150-157) ===
void foo(std::string&& s) {
    std::string a = s;              // COPIE — s est une lvalue ici
    std::print("a (copie) = {}\n", a);
    // s n'est pas vidé car c'est une copie

    std::string b = std::move(s);   // DÉPLACEMENT
    std::print("b (move) = {}\n", b);
    std::print("s après move = '{}'\n", s);
}

void test_paradoxe() {
    std::print("\n=== Paradoxe rvalue ref nommée ===\n");
    foo(std::string("Hello"));
}

// === Surcharge lvalue/rvalue (lignes 183-215) ===
class MyString {
    size_t size_;
    char* data_;

public:
    MyString(const char* str) : size_(std::strlen(str)), data_(new char[size_ + 1]) {
        std::memcpy(data_, str, size_ + 1);
    }

    MyString(const MyString& other)
        : size_(other.size_), data_(new char[other.size_ + 1])
    {
        std::memcpy(data_, other.data_, size_ + 1);
        std::print("[copie] {} octets copiés\n", size_);
    }

    MyString(MyString&& other) noexcept
        : size_(other.size_), data_(other.data_)
    {
        other.size_ = 0;
        other.data_ = nullptr;
        std::print("[move] ressources transférées\n");
    }

    ~MyString() { delete[] data_; }

    const char* c_str() const { return data_ ? data_ : ""; }
};

void test_surcharge() {
    std::print("\n=== Surcharge lvalue/rvalue ===\n");
    MyString a("Hello");

    std::print("--- b = a (copie) ---\n");
    MyString b = a;

    std::print("--- c = temporaire (move) ---\n");
    MyString c = MyString("World");

    std::print("--- d = std::move(a) (move) ---\n");
    MyString d = std::move(a);
}

// === Forwarding reference et auto&& (lignes 314-346) ===
template <typename T>
void wrapper(T&& arg) {
    std::print("wrapper appelé\n");
}

void test_forwarding_ref() {
    std::print("\n=== Forwarding reference ===\n");
    int x = 42;
    wrapper(x);       // T déduit comme int& → lvalue ref
    wrapper(42);      // T déduit comme int  → rvalue ref

    auto&& a = x;     // a est int& (lvalue ref)
    auto&& b = 42;    // b est int&& (rvalue ref)
    std::print("a = {}, b = {}\n", a, b);
}

int main() {
    test_lvalues();
    test_rvalues();
    test_adresse();
    test_rvalue_refs();
    test_const_ref();
    test_paradoxe();
    test_surcharge();
    test_forwarding_ref();
    std::print("\n✅ Tous les exemples passés\n");
}
