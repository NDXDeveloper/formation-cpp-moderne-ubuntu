/* ============================================================================
   Section 4.2 : Déclaration et définition de fonctions
   Description : Anatomie d'une fonction, void, forward declaration, main,
                 auto return type, trailing return type, [[nodiscard]],
                 inline, constexpr dans les en-têtes
   Fichier source : 02-declaration-definition.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <vector>
#include <type_traits>

// --- Anatomie d'une fonction (lignes 30-34) ---
double calculer_moyenne(int a, int b) {
    return static_cast<double>(a + b) / 2.0;
}

// --- void et return anticipé (lignes 42-53) ---
void afficher_banniere(const std::string& titre) {
    std::cout << "=== " << titre << " ===\n";
}

void process(int x) {
    if (x < 0) {
        std::cout << "  Valeur négative, sortie anticipée\n";
        return;
    }
    std::cout << "  Traitement de x=" << x << "\n";
}

// --- Forward declaration (lignes 151-165) ---
int carre(int x);  // Déclaration anticipée

void demo_forward_declaration() {
    std::cout << "carre(5) = " << carre(5) << "\n";
}

int carre(int x) {
    return x * x;
}

// --- fibonacci et est_premier (lignes 196-221) ---
int fibonacci(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int tmp = a + b;
        a = b;
        b = tmp;
    }
    return b;
}

bool est_premier(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

// --- Auto return type C++14 (lignes 427-434) ---
auto construire_message(const std::string& nom) {
    return "Bonjour, " + nom;
}

// --- Trailing return type (lignes 443-455) ---
auto calculer_moyenne_trailing(int a, int b) -> double {
    return static_cast<double>(a + b) / 2.0;
}

template <typename A, typename B>
auto additionner(A a, B b) -> decltype(a + b) {
    return a + b;
}

// --- [[nodiscard]] (lignes 466-478) ---
[[nodiscard]] int calculer_checksum(const std::vector<char>& data) {
    int sum = 0;
    for (char c : data) sum += c;
    return sum;
}

// --- constexpr dans les en-têtes (lignes 385-392) ---
constexpr double pi() {
    return 3.14159265358979323846;
}

constexpr double cercle_aire(double rayon) {
    return pi() * rayon * rayon;
}

// --- inline (lignes 370-373) ---
inline int helper() {
    return 42;
}

int main() {
    std::cout << "--- Anatomie d'une fonction ---\n";
    std::cout << "Moyenne(10, 20) = " << calculer_moyenne(10, 20) << "\n";

    std::cout << "\n--- void et return anticipé ---\n";
    afficher_banniere("Test");
    process(-1);
    process(5);

    std::cout << "\n--- Forward declaration ---\n";
    demo_forward_declaration();

    std::cout << "\n--- fibonacci et est_premier ---\n";
    std::cout << "Fib(10) = " << fibonacci(10) << "\n";
    std::cout << "17 premier ? " << std::boolalpha << est_premier(17) << "\n";
    std::cout << "15 premier ? " << std::boolalpha << est_premier(15) << "\n";

    std::cout << "\n--- Auto return type ---\n";
    std::cout << construire_message("Alice") << "\n";

    std::cout << "\n--- Trailing return type ---\n";
    std::cout << "Moyenne trailing(10, 20) = " << calculer_moyenne_trailing(10, 20) << "\n";
    std::cout << "additionner(3, 4.5) = " << additionner(3, 4.5) << "\n";

    std::cout << "\n--- [[nodiscard]] ---\n";
    std::vector<char> data = {'a', 'b', 'c'};
    int cs = calculer_checksum(data);
    std::cout << "checksum = " << cs << "\n";

    std::cout << "\n--- constexpr ---\n";
    constexpr double aire = cercle_aire(5.0);
    std::cout << "Aire cercle(r=5) = " << aire << "\n";
    static_assert(cercle_aire(1.0) > 3.14);

    std::cout << "\n--- inline ---\n";
    std::cout << "helper() = " << helper() << "\n";

    return 0;
}
