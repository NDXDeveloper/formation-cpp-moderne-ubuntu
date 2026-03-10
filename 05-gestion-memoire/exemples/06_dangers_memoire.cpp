/* ============================================================================
   Section 5.4 : Dangers — Memory leaks, dangling pointers, double free
   Description : Démonstration des patterns dangereux (fuite par écrasement,
                 fuite par retour anticipé, classe Buffer sans Rule of Five,
                 invalidation d'itérateurs) — code corrigé pour éviter les UB,
                 les patterns problématiques sont commentés et documentés
   Fichier source : 04-dangers-memoire.md
   ============================================================================ */
#include <iostream>
#include <string>
#include <vector>

// --- Fuite par écrasement (lignes 40-44) — version corrigée ---
void fuite_ecrasement_corrigee() {
    int* p = new int(10);     // allocation A
    std::cout << "  A = " << *p << "\n";
    delete p;                 // ✅ libère A avant réaffectation
    p = new int(20);          // allocation B
    std::cout << "  B = " << *p << "\n";
    delete p;                 // libère B
}

// --- Classe Buffer : problème de copie superficielle (lignes 248-258) ---
class Buffer {
public:
    Buffer(int taille) : donnees_(new int[taille]), taille_(taille) {
        for (int i = 0; i < taille; ++i) donnees_[i] = i;
        std::cout << "  Buffer(" << taille << ") construit\n";
    }
    ~Buffer() {
        delete[] donnees_;
        std::cout << "  Buffer(" << taille_ << ") détruit\n";
    }

    // ⚠️ Sans ces lignes, le compilateur génère des copies superficielles
    // → double free si on copie un Buffer
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    int operator[](int i) const { return donnees_[i]; }

private:
    int* donnees_;
    int taille_;
};

// --- Invalidation d'itérateurs (lignes 174-186) — démonstration sûre ---
void demo_invalidation() {
    std::vector<int> nombres = {1, 2, 3};
    std::cout << "  Capacité avant push_back : " << nombres.capacity() << "\n";
    std::cout << "  &nombres[0] avant : " << &nombres[0] << "\n";

    // Forcer une réallocation en ajoutant assez d'éléments
    for (int i = 4; i <= 100; ++i) {
        nombres.push_back(i);
    }
    std::cout << "  Capacité après push_back : " << nombres.capacity() << "\n";
    std::cout << "  &nombres[0] après  : " << &nombres[0] << "\n";
    std::cout << "  → L'adresse a changé : les anciens pointeurs seraient dangling\n";
}

// --- Structure Employe/Equipe : propriété ambiguë (lignes 193-207) ---
struct Employe {
    std::string nom;
};

struct Equipe {
    std::string nom;
    Employe* responsable;   // pointeur brut → propriété ambiguë
};

int main() {
    std::cout << "--- Fuite par écrasement (corrigée) ---\n";
    fuite_ecrasement_corrigee();

    std::cout << "\n--- Buffer avec Rule of Five ---\n";
    {
        Buffer buf(5);
        std::cout << "  buf[3] = " << buf[3] << "\n";  // 3
        // Buffer copie = buf;  // ❌ ne compile pas grâce à = delete
    }

    std::cout << "\n--- Invalidation d'itérateurs ---\n";
    demo_invalidation();

    std::cout << "\n--- Propriété ambiguë (danger illustré) ---\n";
    {
        Employe* alice = new Employe{"Alice"};
        Equipe equipe{"DevOps", alice};
        std::cout << "  Responsable : " << equipe.responsable->nom << "\n";
        // Qui est propriétaire ? Si on delete alice ici,
        // equipe.responsable devient un dangling pointer.
        // Solution : std::shared_ptr ou propriété claire.
        delete alice;
        // equipe.responsable->nom;  // ❌ dangling pointer !
        std::cout << "  (alice supprimé → equipe.responsable est dangling)\n";
    }

    std::cout << "\n--- Résumé ---\n";
    std::cout << "Tous les patterns dangereux illustrés (sans UB).\n";
    std::cout << "Solutions : RAII, smart pointers, Rule of Five.\n";

    return 0;
}
