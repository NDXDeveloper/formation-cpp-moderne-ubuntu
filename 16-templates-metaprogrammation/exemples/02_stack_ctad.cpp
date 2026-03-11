/* ============================================================================
   Section 16.2 : Templates de classes — Stack, CTAD et Pair
   Description : Pile template avec méthodes hors-classe, CTAD avec
                 initializer_list, classe Pair à paramètres multiples
   Fichier source : 02-templates-classes.md
   ============================================================================ */
#include <vector>
#include <stdexcept>
#include <print>
#include <string>
#include <initializer_list>

using namespace std::string_literals;

template <typename T>
class Stack {
public:
    Stack() = default;

    Stack(std::initializer_list<T> init)
        : elements_{init} {}

    void push(const T& valeur);
    void pop();
    const T& top() const;
    bool empty() const { return elements_.empty(); }
    std::size_t size() const { return elements_.size(); }

private:
    std::vector<T> elements_;
};

template <typename T>
void Stack<T>::push(const T& valeur) {
    elements_.push_back(valeur);
}

template <typename T>
void Stack<T>::pop() {
    if (elements_.empty()) {
        throw std::runtime_error("pop() sur une pile vide");
    }
    elements_.pop_back();
}

template <typename T>
const T& Stack<T>::top() const {
    if (elements_.empty()) {
        throw std::runtime_error("top() sur une pile vide");
    }
    return elements_.back();
}

template <typename K, typename V>
class Pair {
public:
    Pair(const K& cle, const V& valeur)
        : cle_{cle}, valeur_{valeur} {}

    const K& cle() const { return cle_; }
    const V& valeur() const { return valeur_; }

private:
    K cle_;
    V valeur_;
};

int main() {
    // Stack basique
    {
        Stack<int> pile_entiers;
        pile_entiers.push(10);
        pile_entiers.push(20);
        pile_entiers.push(30);

        std::print("Sommet : {}\n", pile_entiers.top());   // 30
        std::print("Taille : {}\n", pile_entiers.size());  // 3

        pile_entiers.pop();
        std::print("Sommet après pop : {}\n", pile_entiers.top());  // 20

        Stack<std::string> pile_mots;
        pile_mots.push("hello");
        pile_mots.push("world");
        std::print("Mot au sommet : {}\n", pile_mots.top());  // world
    }

    // CTAD
    {
        Stack pile{1, 2, 3, 4, 5};
        std::print("CTAD Stack top : {}\n", pile.top());  // 5

        Stack mots{"alpha"s, "beta"s};
        std::print("CTAD Stack<string> top : {}\n", mots.top());  // beta
    }

    // Pair
    {
        Pair<std::string, int> score{"Alice", 95};
        std::print("{} : {}\n", score.cle(), score.valeur());

        Pair p2{"Bob"s, 87};
        std::print("{} : {}\n", p2.cle(), p2.valeur());
    }
}
