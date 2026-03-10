/* ============================================================================
   Section 4.3.3 : Passage par référence constante (const &)
   Description : Lecture sans copie, acceptation des temporaires, operator<<
                 pour type personnalisé, const en profondeur (superficiel)
   Fichier source : 03.3-par-reference-const.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

// --- Principe (lignes 13-27) ---
void afficher(const std::string& message) {
    std::cout << message << "\n";
    // message += "!";  // ❌ Erreur de compilation — const
}

// --- Moyenne sans copie (lignes 56-69) ---
double moyenne(const std::vector<double>& donnees) {
    double somme = 0;
    for (double d : donnees) somme += d;
    return somme / static_cast<double>(donnees.size());
}

// --- Accepter les temporaires (lignes 85-98) ---
void traiter(const std::string& s) {
    std::cout << "  Longueur : " << s.size() << "\n";
}

// --- operator<< (lignes 142-150) ---
struct Point {
    double x, y;
};

std::ostream& operator<<(std::ostream& os, const Point& p) {
    os << "(" << p.x << ", " << p.y << ")";
    return os;
}

// --- Fonctions utilitaires (lignes 157-169) ---
bool contient(const std::string& texte, const std::string& motif) {
    return texte.find(motif) != std::string::npos;
}

double distance(const Point& a, const Point& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

int main() {
    std::cout << "--- const ref basique ---\n";
    std::string salut = "Bonjour";
    afficher(salut);

    std::cout << "\n--- Moyenne sans copie ---\n";
    std::vector<double> data = {10.0, 20.0, 30.0, 40.0};
    std::cout << "Moyenne = " << moyenne(data) << "\n";  // 25

    std::cout << "\n--- Accepter les temporaires ---\n";
    std::string nom = "Alice";
    traiter(nom);                        // lvalue
    traiter("littéral C");               // temporaire
    traiter(std::string("temporaire"));  // rvalue explicite
    traiter(nom + " et Bob");            // résultat d'expression

    std::cout << "\n--- operator<< ---\n";
    Point p{3.0, 4.0};
    std::cout << "Point : " << p << "\n";

    std::cout << "\n--- Fonctions utilitaires ---\n";
    std::cout << std::boolalpha;
    std::cout << "contient(\"hello world\", \"world\") = "
              << contient("hello world", "world") << "\n";  // true
    Point a{0.0, 0.0}, b{3.0, 4.0};
    std::cout << "distance(a, b) = " << distance(a, b) << "\n";  // 5

    return 0;
}
