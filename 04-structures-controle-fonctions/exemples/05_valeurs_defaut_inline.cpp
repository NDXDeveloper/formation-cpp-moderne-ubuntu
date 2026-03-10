/* ============================================================================
   Section 4.5 : Valeurs par défaut et fonctions inline
   Description : Paramètres par défaut, règle d'ordre, expressions comme
                 défauts, conflit défauts/surcharge, inline, constexpr,
                 variables inline C++17, macros vs inline
   Fichier source : 05-valeurs-defaut-inline.md
   ============================================================================ */
#include <iostream>
#include <string>

// --- Valeurs par défaut (lignes 15-28) ---
void connecter(const std::string& hote, int port = 8080, bool ssl = false) {
    std::cout << (ssl ? "https" : "http") << "://" << hote << ":" << port << "\n";
}

// --- Formater date (lignes 130-136) ---
std::string formater_date(int jour, int mois, int annee = 2026,
                          char separateur = '/') {
    return std::to_string(jour) + separateur +
           std::to_string(mois) + separateur +
           std::to_string(annee);
}

// --- Puissance avec défaut (lignes 60-69) ---
double puissance(double base, int exposant = 2) {
    double resultat = 1.0;
    for (int i = 0; i < exposant; ++i) {
        resultat *= base;
    }
    return resultat;
}

// --- inline (lignes 180-189) ---
inline int carre(int x) {
    return x * x;
}

// --- constexpr implicitement inline (lignes 228-233) ---
constexpr int factorielle(int n) {
    return (n <= 1) ? 1 : n * factorielle(n - 1);
}

// --- Variables inline C++17 (lignes 243-249) ---
inline constexpr int MAX_CONNECTIONS = 100;

// --- Macro vs inline (lignes 260-272) ---
#define CARRE_MACRO(x) ((x) * (x))

// --- Classe avec méthode implicitement inline (lignes 217-224) ---
class Rectangle {
    double largeur_, hauteur_;
public:
    Rectangle(double l, double h) : largeur_(l), hauteur_(h) {}
    double aire() const { return largeur_ * hauteur_; }  // implicitement inline
};

int main() {
    std::cout << "--- Valeurs par défaut ---\n";
    connecter("example.com");               // port=8080, ssl=false
    connecter("example.com", 443);          // ssl=false
    connecter("example.com", 443, true);    // tout spécifié

    std::cout << "\n--- Formater date ---\n";
    std::cout << formater_date(9, 3) << "\n";           // 9/3/2026
    std::cout << formater_date(9, 3, 2025) << "\n";     // 9/3/2025
    std::cout << formater_date(9, 3, 2026, '-') << "\n"; // 9-3-2026

    std::cout << "\n--- Puissance avec défaut ---\n";
    std::cout << "puissance(5) = " << puissance(5) << "\n";       // 25
    std::cout << "puissance(2, 10) = " << puissance(2, 10) << "\n"; // 1024

    std::cout << "\n--- inline ---\n";
    std::cout << "carre(7) = " << carre(7) << "\n";  // 49

    std::cout << "\n--- constexpr (implicitement inline) ---\n";
    constexpr int f5 = factorielle(5);
    std::cout << "5! = " << f5 << "\n";  // 120
    static_assert(factorielle(5) == 120);

    std::cout << "\n--- Variables inline C++17 ---\n";
    std::cout << "MAX_CONNECTIONS = " << MAX_CONNECTIONS << "\n";  // 100

    std::cout << "\n--- Macro vs inline ---\n";
    int a = 5;
    // Avec la macro : CARRE_MACRO(a++) évaluerait a++ deux fois — dangereux
    int r_inline = carre(a);
    std::cout << "carre(5) = " << r_inline << "\n";  // 25
    int r_macro = CARRE_MACRO(5);
    std::cout << "CARRE_MACRO(5) = " << r_macro << "\n";  // 25

    std::cout << "\n--- Classe (méthode implicitement inline) ---\n";
    Rectangle rect(4.0, 5.0);
    std::cout << "Aire = " << rect.aire() << "\n";  // 20

    return 0;
}
