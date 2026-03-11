/* ============================================================================
   Section 13.1.1 : Importance de noexcept pour le move constructor
   Description : Comparaison du comportement de vector lors des réallocations
                 avec et sans noexcept sur le move constructor
   Fichier source : 01.1-fonctionnement-interne.md
   ============================================================================ */
#include <vector>
#include <print>
#include <string>

class Lent {
    std::string donnees_;
public:
    Lent(std::string s) : donnees_(std::move(s)) {}

    // Move constructor SANS noexcept → le vector utilisera la COPIE
    Lent(Lent&& other) : donnees_(std::move(other.donnees_)) {}

    Lent(const Lent& other) : donnees_(other.donnees_) {
        std::println("  Copie !");
    }
};

class Rapide {
    std::string donnees_;
public:
    Rapide(std::string s) : donnees_(std::move(s)) {}

    // Move constructor AVEC noexcept → le vector utilisera le MOVE
    Rapide(Rapide&& other) noexcept : donnees_(std::move(other.donnees_)) {}

    Rapide(const Rapide& other) : donnees_(other.donnees_) {
        std::println("  Copie !");
    }
};

int main() {
    std::println("=== Sans noexcept (copies lors des réallocations) ===");
    {
        std::vector<Lent> v;
        v.reserve(2);
        v.emplace_back("aaa");
        v.emplace_back("bbb");
        std::println("-- Réallocation imminente --");
        v.emplace_back("ccc");  // réallocation → copies de "aaa" et "bbb"
    }

    std::println("\n=== Avec noexcept (moves lors des réallocations) ===");
    {
        std::vector<Rapide> v;
        v.reserve(2);
        v.emplace_back("aaa");
        v.emplace_back("bbb");
        std::println("-- Réallocation imminente --");
        v.emplace_back("ccc");  // réallocation → moves de "aaa" et "bbb"
    }
}
