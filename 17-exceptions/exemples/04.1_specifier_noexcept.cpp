/* ============================================================================
   Section 17.4.1 : Spécifier noexcept
   Description : Syntaxe noexcept simple, forme conditionnelle noexcept(expr),
                 opérateur noexcept(...), pattern noexcept(noexcept(...)),
                 noexcept dans les lambdas, noexcept dans le système de types,
                 vérification avec static_assert
   Fichier source : 04.1-specifier-noexcept.md
   ============================================================================ */

#include <print>
#include <string>
#include <vector>
#include <type_traits>
#include <utility>
#include <functional>

// === Forme simple : noexcept ===
int calculer_hash(int valeur) noexcept {
    return valeur ^ (valeur >> 16);
}

class Ressource {
public:
    std::size_t taille() const noexcept { return taille_; }
    std::string&& extraire() && noexcept { return std::move(nom_); }

private:
    std::size_t taille_ = 0;
    std::string nom_;
};

// === Destructeurs : implicitement noexcept ===
class Widget {
public:
    ~Widget() { /* implicitement noexcept */ }
};
static_assert(std::is_nothrow_destructible_v<Widget>);

// === Fonctions = default : noexcept conditionnel automatique ===
class Point {
public:
    double x, y;
    Point() = default;
    Point(double x, double y) : x(x), y(y) {}
    Point(Point&&) = default;
    Point& operator=(Point&&) = default;
};
static_assert(std::is_nothrow_move_constructible_v<Point>);
static_assert(std::is_nothrow_move_assignable_v<Point>);

class Document {
public:
    std::string titre;
    std::vector<int> pages;
    Document() = default;
    Document(std::string t) : titre(std::move(t)) {}
    Document(Document&&) = default;
};
static_assert(std::is_nothrow_move_constructible_v<Document>);

// === Forme conditionnelle : noexcept(expression) ===
template <typename T>
void echanger(T& a, T& b) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                     std::is_nothrow_move_assignable_v<T>)
{
    T tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}

struct Bizarre {
    Bizarre() = default;
    Bizarre(Bizarre&&) { /* peut lever */ }
    Bizarre& operator=(Bizarre&&) { return *this; }
};

// === Opérateur noexcept(...) : tester si une expression peut lever ===
static_assert(noexcept(42 + 1));                         // arithmétique : noexcept
static_assert(noexcept(std::declval<int&>() = 5));       // affectation int : noexcept
static_assert(noexcept(std::declval<std::string>().size())); // string::size() est noexcept
static_assert(!noexcept(std::declval<std::vector<int>>().push_back(1))); // push_back peut lever
static_assert(!noexcept(new int));                       // operator new peut lever

// === Pattern noexcept(noexcept(...)) : propagation dans le code générique ===
template <typename T>
class Wrapper {
public:
    Wrapper() = default;
    explicit Wrapper(T val) : valeur_(std::move(val)) {}

    // noexcept si T est nothrow-move-constructible
    Wrapper(Wrapper&& other) noexcept(noexcept(T(std::move(other.valeur_))))
        : valeur_(std::move(other.valeur_))
    {}

    // Avec type traits (équivalent mais plus lisible)
    Wrapper& operator=(Wrapper&& other)
        noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        valeur_ = std::move(other.valeur_);
        return *this;
    }

    const T& get() const noexcept { return valeur_; }

private:
    T valeur_{};
};

// === noexcept dans les lambdas ===
auto diviser_lambda = [](double a, double b) noexcept -> double {
    return a / b;
};

auto compteur = [n = 0]() mutable noexcept {
    return ++n;
};

static_assert(noexcept(diviser_lambda(1.0, 2.0)));
static_assert(noexcept(compteur()));

// === noexcept fait partie du système de types (C++17) ===
void f_noexcept() noexcept {}
void g_normal() {}

// f et g ont des types différents
static_assert(!std::is_same_v<decltype(&f_noexcept), decltype(&g_normal)>);

int main() {
    std::print("=== 1. Forme simple noexcept ===\n");
    std::print("  calculer_hash(42) = {}\n", calculer_hash(42));
    std::print("  noexcept(calculer_hash(42)) = {}\n", noexcept(calculer_hash(42)));

    std::print("\n=== 2. Destructeurs implicitement noexcept ===\n");
    std::print("  is_nothrow_destructible<Widget> = {}\n",
               std::is_nothrow_destructible_v<Widget>);

    std::print("\n=== 3. Fonctions = default : noexcept conditionnel ===\n");
    std::print("  Point nothrow-move-constructible : {}\n",
               std::is_nothrow_move_constructible_v<Point>);
    std::print("  Document nothrow-move-constructible : {}\n",
               std::is_nothrow_move_constructible_v<Document>);

    std::print("\n=== 4. noexcept conditionnel (template) ===\n");
    {
        int a = 1, b = 2;
        echanger(a, b);
        std::print("  echanger(int) noexcept : {} (a={}, b={})\n",
                   noexcept(echanger(a, b)), a, b);

        Bizarre ba, bb;
        echanger(ba, bb);
        std::print("  echanger(Bizarre) noexcept : {}\n",
                   noexcept(echanger(ba, bb)));
    }

    std::print("\n=== 5. Opérateur noexcept(...) ===\n");
    std::print("  noexcept(42 + 1) = {}\n", noexcept(42 + 1));
    std::print("  noexcept(string::size()) = {}\n",
               noexcept(std::declval<std::string>().size()));
    std::print("  noexcept(vector::push_back) = {}\n",
               noexcept(std::declval<std::vector<int>>().push_back(1)));
    std::print("  noexcept(new int) = {}\n", noexcept(new int));

    std::print("\n=== 6. Pattern noexcept(noexcept(...)) ===\n");
    std::print("  Wrapper<int> nothrow-move : {}\n",
               std::is_nothrow_move_constructible_v<Wrapper<int>>);
    std::print("  Wrapper<string> nothrow-move : {}\n",
               std::is_nothrow_move_constructible_v<Wrapper<std::string>>);
    std::print("  Wrapper<Bizarre> nothrow-move : {}\n",
               std::is_nothrow_move_constructible_v<Wrapper<Bizarre>>);

    std::print("\n=== 7. noexcept dans les lambdas ===\n");
    std::print("  diviser_lambda(10, 3) = {}\n", diviser_lambda(10.0, 3.0));
    std::print("  noexcept(diviser_lambda) = {}\n", noexcept(diviser_lambda(1.0, 2.0)));
    std::print("  compteur() = {}, {}, {}\n", compteur(), compteur(), compteur());

    std::print("\n=== 8. noexcept dans le système de types ===\n");
    std::print("  f_noexcept et g_normal ont des types différents : {}\n",
               !std::is_same_v<decltype(&f_noexcept), decltype(&g_normal)>);

    // Conversion implicite : noexcept → peut lever
    void (*ptr_normal)() = &f_noexcept;  // OK
    ptr_normal();
    std::print("  Conversion noexcept → normal : OK\n");

    // L'inverse est interdit :
    // void (*ptr_ne)() noexcept = &g_normal;  // erreur de compilation

    std::print("\nTous les static_assert passés !\n");
    return 0;
}
