/* ============================================================================
   Section 6.1 : Définition de classes — Membres et méthodes
   Description : Le pointeur this — accès implicite, désambiguïsation,
                 chaînage de méthodes (QueryBuilder), et Counter
   Fichier source : 01-definition-classes.md
   ============================================================================ */
#include <iostream>
#include <string>

// --- Counter : this explicite vs implicite ---
class Counter {
public:
    void increment() {
        this->count_++;   // Explicite — rarement nécessaire
        count_++;          // Équivalent — forme usuelle
    }

    int count() const {
        return count_;
    }

private:
    int count_ = 0;
};

// --- Widget : this pour désambiguïsation ---
class Widget {
public:
    void set_name(const std::string& name) {
        this->name = name;   // this->name = attribut, name = paramètre
    }
    const std::string& get_name() const { return name; }
private:
    std::string name;
};

// --- QueryBuilder : chaînage de méthodes (return *this) ---
class QueryBuilder {
public:
    QueryBuilder& select(const std::string& field) {
        query_ += "SELECT " + field + " ";
        return *this;
    }

    QueryBuilder& from(const std::string& table) {
        query_ += "FROM " + table + " ";
        return *this;
    }

    QueryBuilder& where(const std::string& condition) {
        query_ += "WHERE " + condition;
        return *this;
    }

    std::string build() const { return query_; }

private:
    std::string query_;
};

int main() {
    // Counter
    Counter c;
    c.increment();   // this->count_++ et count_++ => +2
    std::cout << "Count: " << c.count() << "\n";   // 2

    // Widget désambiguïsation
    Widget w;
    w.set_name("MyWidget");
    std::cout << "Widget name: " << w.get_name() << "\n";

    // QueryBuilder chaînage
    auto sql = QueryBuilder()
        .select("name")
        .from("users")
        .where("age > 18")
        .build();
    std::cout << sql << "\n";

    return 0;
}
// Sortie attendue :
// Count: 2
// Widget name: MyWidget
// SELECT name FROM users WHERE age > 18
