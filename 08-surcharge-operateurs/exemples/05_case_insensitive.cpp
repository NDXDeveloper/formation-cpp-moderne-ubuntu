/* ============================================================================
   Section 8.5 : Opérateur spaceship <=> (C++20)
   Description : weak_ordering — CaseInsensitiveString où "Hello" et "HELLO"
                 sont équivalents mais pas égaux
   Fichier source : 05-operateur-spaceship.md
   ============================================================================ */
#include <compare>
#include <algorithm>
#include <string>
#include <print>
#include <cctype>

class CaseInsensitiveString {
    std::string data_;
public:
    explicit CaseInsensitiveString(std::string s) : data_{std::move(s)} {}

    std::weak_ordering operator<=>(CaseInsensitiveString const& rhs) const {
        // "Hello" et "HELLO" sont équivalents mais pas égaux
        auto cmp = [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a))
               <=> std::tolower(static_cast<unsigned char>(b));
        };
        return std::lexicographical_compare_three_way(
            data_.begin(), data_.end(),
            rhs.data_.begin(), rhs.data_.end(), cmp);
    }

    bool operator==(CaseInsensitiveString const& rhs) const {
        // Cohérent avec <=> : comparaison insensible à la casse
        return (*this <=> rhs) == 0;
    }

    std::string const& str() const { return data_; }
};

int main() {
    CaseInsensitiveString a{"Hello"};
    CaseInsensitiveString b{"HELLO"};
    CaseInsensitiveString c{"World"};

    std::println("{} == {} → {}", a.str(), b.str(), a == b);   // true
    std::println("{} <  {} → {}", a.str(), c.str(), a < c);    // true
    std::println("{} >  {} → {}", c.str(), a.str(), c > a);    // true
}
