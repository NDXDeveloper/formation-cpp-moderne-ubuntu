/* ============================================================================
   Section 6.3 : Destructeurs et le principe RAII
   Description : Destructeur — appel automatique, ordre de destruction inversé,
                 temporaires, couple constructeur/destructeur, stack unwinding
   Fichier source : 03-destructeurs-raii.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstddef>

// --- Resource : destructeur automatique ---
class Resource {
public:
    Resource(const std::string& name)
        : name_(name) {
        std::cout << "Acquisition " << name_ << "\n";
    }

    ~Resource() {
        std::cout << "Libération " << name_ << "\n";
    }

private:
    std::string name_;
};

// --- DynArray simplifié pour démonstration ---
class DynArray {
public:
    DynArray() = default;

    explicit DynArray(std::size_t size)
        : data_(size > 0 ? new int[size]{} : nullptr)
        , size_(size) {
        std::cout << "DynArray(" << size << ") construit\n";
    }

    ~DynArray() {
        std::cout << "~DynArray(" << size_ << ") détruit\n";
        delete[] data_;
    }

    std::size_t size() const { return size_; }

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};

// --- Démonstration de l'ordre de destruction ---
void order_demo() {
    std::cout << "--- Ordre de destruction ---\n";
    DynArray x(1);    // Construction 1
    DynArray y(2);    // Construction 2
    DynArray z(3);    // Construction 3
}
// Destruction 3 (z), puis 2 (y), puis 1 (x)

// --- Démonstration scope et durée de vie ---
void scope_demo() {
    std::cout << "\n--- Scope et durée de vie ---\n";
    DynArray a(100);

    {
        DynArray b(50);
        std::cout << "b est en vie, size=" << b.size() << "\n";
    }   // b détruit ici

    std::cout << "b n'existe plus, a est toujours en vie\n";
}   // a détruit ici

// --- Démonstration stack unwinding ---
void risky_operation() {
    std::cout << "\n--- Stack unwinding ---\n";
    Resource data("data");
    Resource conn("conn");

    std::cout << "Lancement exception...\n";
    throw std::runtime_error("Erreur simulée");

    // Ce code n'est jamais atteint
    std::cout << "Done\n";
}

int main() {
    // 1. Resource basique
    std::cout << "--- Resource basique ---\n";
    {
        Resource r("fichier");
        std::cout << "Utilisation\n";
    }

    // 2. Ordre de destruction
    order_demo();

    // 3. Scope
    scope_demo();

    // 4. Temporaire
    std::cout << "\n--- Temporaire ---\n";
    std::cout << "taille temporaire: " << DynArray(5).size() << "\n";

    // 5. Stack unwinding
    try {
        risky_operation();
    } catch (const std::exception& e) {
        std::cout << "Exception attrapée: " << e.what() << "\n";
    }

    return 0;
}
// Sortie attendue :
// --- Resource basique ---
// Acquisition fichier
// Utilisation
// Libération fichier
// --- Ordre de destruction ---
// DynArray(1) construit
// DynArray(2) construit
// DynArray(3) construit
// ~DynArray(3) détruit
// ~DynArray(2) détruit
// ~DynArray(1) détruit
//
// --- Scope et durée de vie ---
// DynArray(100) construit
// DynArray(50) construit
// b est en vie, size=50
// ~DynArray(50) détruit
// b n'existe plus, a est toujours en vie
// ~DynArray(100) détruit
//
// --- Temporaire ---
// DynArray(5) construit
// taille temporaire: 5
// ~DynArray(5) détruit
//
// --- Stack unwinding ---
// Acquisition data
// Acquisition conn
// Lancement exception...
// Libération conn
// Libération data
// Exception attrapée: Erreur simulée
