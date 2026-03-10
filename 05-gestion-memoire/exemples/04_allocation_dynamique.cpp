/* ============================================================================
   Section 5.2 : Allocation dynamique : new/delete, new[]/delete[]
   Description : Allocation et libération d'objets uniques et de tableaux,
                 classe Connexion avec constructeur/destructeur, tableau de
                 Capteurs, delete sur nullptr, allocation const, bad_alloc
   Fichier source : 02-allocation-dynamique.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <new>       // std::bad_alloc, std::nothrow

// --- Classe Connexion (lignes 46-65) ---
class Connexion {
public:
    Connexion(const std::string& hote, int port)
        : hote_(hote), port_(port)
    {
        std::cout << "Connexion ouverte vers " << hote_ << ":" << port_ << "\n";
    }

    ~Connexion() {
        std::cout << "Connexion fermée (" << hote_ << ":" << port_ << ")\n";
    }

    void envoyer(const std::string& message) {
        std::cout << "Envoi: " << message << "\n";
    }

private:
    std::string hote_;
    int port_;
};

// --- Classe Capteur (lignes 137-141) ---
class Capteur {
public:
    Capteur() { std::cout << "  Capteur construit\n"; }
    ~Capteur() { std::cout << "  Capteur détruit\n"; }
};

int main() {
    // --- Allocation de base (lignes 22-33) ---
    std::cout << "--- Allocation de base ---\n";
    int* p = new int;          // non initialisé
    int* q = new int(42);      // initialisé à 42
    int* r = new int{42};      // initialisation uniforme
    std::cout << "*q = " << *q << "\n";   // 42
    delete p;
    delete q;
    delete r;

    // --- Connexion (lignes 67-77) ---
    std::cout << "\n--- Connexion ---\n";
    Connexion* conn = new Connexion("serveur.local", 8080);
    conn->envoyer("ping");
    delete conn;

    // --- Tableaux (lignes 99-117) ---
    std::cout << "\n--- Tableaux ---\n";
    double* temperatures = new double[5];
    temperatures[0] = 18.5;
    temperatures[1] = 20.3;
    temperatures[2] = 22.1;
    temperatures[3] = 19.7;
    temperatures[4] = 21.0;
    for (int i = 0; i < 5; ++i) {
        std::cout << temperatures[i] << " ";
    }
    std::cout << "\n";
    delete[] temperatures;

    // --- Initialisation C++11 (lignes 121-128) ---
    std::cout << "\n--- Initialisation C++11 ---\n";
    int* valeurs = new int[4]{10, 20, 30, 40};
    int* zeros = new int[100]();
    std::cout << "valeurs : ";
    for (int i = 0; i < 4; ++i) std::cout << valeurs[i] << " ";
    std::cout << "\n";
    std::cout << "zeros[50] = " << zeros[50] << "\n";  // 0
    delete[] valeurs;
    delete[] zeros;

    // --- Capteur (lignes 143-151) ---
    std::cout << "\n--- Capteur ---\n";
    std::cout << "Allocation de 3 capteurs :\n";
    Capteur* capteurs = new Capteur[3];
    std::cout << "\nLibération :\n";
    delete[] capteurs;

    // --- delete sur nullptr (lignes 210-212) ---
    std::cout << "\n--- delete nullptr ---\n";
    int* pn = nullptr;
    delete pn;        // ✅ sûr — aucun effet
    delete[] pn;      // ✅ sûr — aucun effet
    std::cout << "delete nullptr : aucun effet (OK)\n";

    // --- Allocation const (lignes 365-371) ---
    std::cout << "\n--- Allocation const ---\n";
    const int* pc = new const int(42);
    std::cout << "*pc = " << *pc << "\n";  // 42
    delete pc;

    // --- bad_alloc (lignes 328-338) ---
    std::cout << "\n--- bad_alloc ---\n";
    try {
        long long* pb = new long long[1'000'000'000'000'000'000LL];
        delete[] pb;
    }
    catch (const std::bad_alloc& e) {
        std::cerr << "Allocation échouée : " << e.what() << "\n";
    }

    // --- nothrow (lignes 344-353) ---
    std::cout << "\n--- nothrow ---\n";
    int* pnt = new(std::nothrow) int[1'000'000'000];
    if (pnt == nullptr) {
        std::cerr << "Allocation échouée (nothrow)\n";
    } else {
        std::cout << "Allocation réussie (nothrow)\n";
        delete[] pnt;
    }

    return 0;
}
