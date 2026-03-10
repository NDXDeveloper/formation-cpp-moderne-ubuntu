/* ============================================================================
   Section 4.1.3 : Range-based for loop
   Description : Boucles classiques vs range-based for, formes de déclaration
                 (valeur, référence, const ref), conteneurs STL, map avec
                 structured bindings, tableaux C, initializer_list, chaînes,
                 type personnalisé IntRange
   Fichier source : 01.3-range-based-for.md
   ============================================================================ */
#include <iostream>
#include <vector>
#include <map>
#include <string>

// --- Boucles classiques (lignes 13-31) ---
void demo_boucles_classiques() {
    std::vector<std::string> fruits = {"pomme", "banane", "cerise"};

    std::cout << "Style C (index) :\n";
    for (std::size_t i = 0; i < fruits.size(); ++i) {
        std::cout << "  " << fruits[i] << "\n";
    }

    std::cout << "Style C++98 (itérateurs) :\n";
    for (std::vector<std::string>::const_iterator it = fruits.begin();
         it != fruits.end(); ++it) {
        std::cout << "  " << *it << "\n";
    }
}

// --- Range-based for simple (lignes 49-60) ---
void demo_range_based() {
    std::vector<std::string> fruits = {"pomme", "banane", "cerise"};

    for (const auto& fruit : fruits) {
        std::cout << fruit << "\n";
    }
}

// --- Formes de déclaration (lignes 100-119) ---
void demo_formes() {
    std::vector<std::string> fruits = {"pomme", "banane", "cerise"};

    // Par valeur : copie
    std::cout << "Par valeur (copie modifiée) :\n";
    for (auto fruit : fruits) {
        fruit += " mûr(e)";
        std::cout << "  " << fruit << "\n";
    }

    // Par référence : modification en place
    std::vector<int> scores = {72, 85, 91, 68};
    for (auto& score : scores) {
        score += 5;
    }
    std::cout << "Scores modifiés : ";
    for (const auto& s : scores) {
        std::cout << s << " ";
    }
    std::cout << "\n";
    // scores == {77, 90, 96, 73}
}

// --- Map avec structured bindings C++17 (lignes 154-169) ---
void demo_map() {
    std::map<std::string, int> ages = {
        {"Alice", 30},
        {"Bob", 25},
        {"Charlie", 35}
    };

    for (const auto& [nom, age] : ages) {
        std::cout << nom << " a " << age << " ans\n";
    }
}

// --- Tableaux C natifs (lignes 184-190) ---
void demo_tableau_c() {
    int values[] = {10, 20, 30, 40, 50};

    for (int v : values) {
        std::cout << v << " ";
    }
    std::cout << "\n";
}

// --- initializer_list (lignes 213-218) ---
void demo_initializer_list() {
    for (auto x : {1, 2, 3, 5, 8, 13}) {
        std::cout << x << " ";
    }
    std::cout << "\n";
    // Sortie : 1 2 3 5 8 13
}

// --- Chaînes de caractères (lignes 224-231) ---
void demo_string() {
    std::string mot = "Bonjour";

    for (char c : mot) {
        std::cout << c << " ";
    }
    std::cout << "\n";
    // Sortie : B o n j o u r
}

// --- Type personnalisé IntRange (lignes 239-268) ---
class IntRange {
    int start_;
    int end_;
public:
    IntRange(int start, int end) : start_(start), end_(end) {}

    struct Iterator {
        int current;
        int  operator*() const { return current; }
        Iterator& operator++() { ++current; return *this; }
        bool operator!=(const Iterator& other) const {
            return current != other.current;
        }
    };

    Iterator begin() const { return {start_}; }
    Iterator end()   const { return {end_}; }
};

int main() {
    std::cout << "--- Boucles classiques ---\n";
    demo_boucles_classiques();

    std::cout << "\n--- Range-based for ---\n";
    demo_range_based();

    std::cout << "\n--- Formes de déclaration ---\n";
    demo_formes();

    std::cout << "\n--- Map (structured bindings) ---\n";
    demo_map();

    std::cout << "\n--- Tableau C natif ---\n";
    demo_tableau_c();

    std::cout << "\n--- initializer_list ---\n";
    demo_initializer_list();

    std::cout << "\n--- Chaîne de caractères ---\n";
    demo_string();

    std::cout << "\n--- IntRange personnalisé ---\n";
    for (int n : IntRange(1, 6)) {
        std::cout << n << " ";
    }
    std::cout << "\n";
    // Sortie : 1 2 3 4 5

    return 0;
}
