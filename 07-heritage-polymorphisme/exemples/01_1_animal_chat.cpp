/* ============================================================================
   Section 7.1.1 : Heritage simple — Syntaxe de base
   Description : Heritage simple Animal/Chat, constructeurs, membres proteges,
                 conversion implicite derivee vers base (upcast)
   Fichier source : 01.1-heritage-simple.md
   ============================================================================ */
#include <string>
#include <print>

class Animal {
protected:
    std::string nom_;
    int age_;

public:
    Animal(std::string nom, int age)
        : nom_{std::move(nom)}, age_{age} {}

    void presenter() const {
        std::println("Je suis {}, {} an(s)", nom_, age_);
    }

    std::string const& nom() const { return nom_; }
    int age() const { return age_; }
};

class Chat : public Animal {
    bool interieur_;   // vit en intérieur ?

public:
    Chat(std::string nom, int age, bool interieur)
        : Animal{std::move(nom), age}   // appel au constructeur de la base
        , interieur_{interieur} {}

    void ronronner() const {
        std::println("{} ronronne...", nom_);  // accès à nom_ (protected)
    }

    bool est_interieur() const { return interieur_; }
};

int main() {
    Chat felix{"Félix", 3, true};
    felix.presenter();    // méthode héritée d'Animal
    felix.ronronner();    // méthode propre à Chat

    // Conversion implicite : un Chat "est-un" Animal
    Animal const& ref = felix;
    ref.presenter();      // OK — accès via référence de base

    // sizeof pour vérifier la disposition mémoire
    std::println("sizeof(Animal) = {}", sizeof(Animal));
    std::println("sizeof(Chat)   = {}", sizeof(Chat));
}
