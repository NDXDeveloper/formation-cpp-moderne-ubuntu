/* ============================================================================
   Section 5.1.3 : Caractéristiques du Heap
   Description : Fragmentation mémoire, durée de vie flexible des objets
                 sur le heap, taille dynamique avec std::vector, et
                 polymorphisme via pointeurs heap
   Fichier source : 01.3-caracteristiques-heap.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <vector>
#include <memory>

// --- Durée de vie : liberté et responsabilité (lignes 104-121) ---
struct Utilisateur {
    std::string nom;
    int age;
};

Utilisateur* creer_utilisateur(const std::string& nom, int age) {
    Utilisateur* u = new Utilisateur{nom, age};
    return u;   // l'objet survit au retour de la fonction
}

// --- Polymorphisme sur le heap (lignes 228-236) ---
struct Animal {
    virtual ~Animal() = default;
    virtual std::string cri() const = 0;
};

struct Chat : Animal {
    std::string cri() const override { return "Miaou"; }
};

struct Chien : Animal {
    std::string cri() const override { return "Wouf"; }
};

std::unique_ptr<Animal> creer_animal(const std::string& type) {
    if (type == "chat") return std::make_unique<Chat>();
    if (type == "chien") return std::make_unique<Chien>();
    return nullptr;
}

int main() {
    std::cout << "--- Fragmentation ---\n";
    // Trois allocations consécutives
    int* a = new int[1000];    // bloc de 4 000 octets
    int* b = new int[1000];    // bloc de 4 000 octets
    int* c = new int[1000];    // bloc de 4 000 octets

    // On libère le bloc du milieu → trou
    delete[] b;

    // On demande un gros bloc qui ne tient pas dans le trou
    int* d = new int[2000];    // besoin de 8 000 octets
    std::cout << "Fragmentation illustrée (pas de crash)\n";

    delete[] a;
    delete[] c;
    delete[] d;

    std::cout << "\n--- Durée de vie flexible ---\n";
    Utilisateur* alice = creer_utilisateur("Alice", 30);
    std::cout << alice->nom << ", " << alice->age << " ans\n";
    delete alice;

    std::cout << "\n--- Taille dynamique (vector) ---\n";
    int n = 10;
    // int tableau[n];          // ❌ VLA — non standard en C++
    std::vector<int> tableau(n); // ✅ allocation heap via vector
    for (int i = 0; i < n; ++i) tableau[i] = i * i;
    for (int v : tableau) std::cout << v << " ";
    std::cout << "\n";

    std::cout << "\n--- Polymorphisme (heap + unique_ptr) ---\n";
    auto chat = creer_animal("chat");
    auto chien = creer_animal("chien");
    std::cout << "Chat : " << chat->cri() << "\n";
    std::cout << "Chien : " << chien->cri() << "\n";

    return 0;
}
