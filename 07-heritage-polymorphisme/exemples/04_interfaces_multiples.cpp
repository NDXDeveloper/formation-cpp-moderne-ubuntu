/* ============================================================================
   Section 7.4 : Classes abstraites — Interfaces pures et heritage multiple
   Description : Implementation de plusieurs interfaces (ISerializable,
                 IPrintable) par une meme classe. Principe de segregation
                 des interfaces (ISP — SOLID).
   Fichier source : 04-classes-abstraites.md
   ============================================================================ */
#include <print>
#include <format>
#include <string>
#include <string_view>
#include <ostream>
#include <iostream>

class ISerializable {
public:
    virtual std::string to_json() const = 0;
    virtual void from_json(std::string_view json) = 0;
    virtual ~ISerializable() = default;
};

class IPrintable {
public:
    virtual void print(std::ostream& out) const = 0;
    virtual ~IPrintable() = default;
};

class Rapport : public ISerializable, public IPrintable {
    std::string titre_;
    std::string contenu_;

public:
    Rapport(std::string titre, std::string contenu)
        : titre_{std::move(titre)}, contenu_{std::move(contenu)} {}

    std::string to_json() const override {
        return std::format(R"({{"titre":"{}","contenu":"{}"}})", titre_, contenu_);
    }

    void from_json(std::string_view /*json*/) override {
        // parsing simplifié...
    }

    void print(std::ostream& out) const override {
        out << titre_ << "\n" << contenu_;
    }
};

void sauvegarder(ISerializable const& obj) {
    std::println("JSON: {}", obj.to_json());
}

void afficher(IPrintable const& obj) {
    obj.print(std::cout);
    std::cout << '\n';
}

int main() {
    Rapport r{"Q1 2026", "Résultats positifs"};
    sauvegarder(r);   // vu comme ISerializable
    afficher(r);       // vu comme IPrintable
}
