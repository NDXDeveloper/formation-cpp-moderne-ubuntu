/* ============================================================================
   Section 10.5 : Return Value Optimization (RVO) et Copy Elision
   Description : NRVO avec Trace, RVO obligatoire C++17 (NonCopiable),
                 NRVO chemin unique, multiples chemins (move implicite),
                 construire_chemin, RVO vector, retour par valeur, pessimisation
                 return std::move, Builder avec ref-qualifier &&, paramètre
                 move implicite C++20
   Fichier source : 05-rvo-copy-elision.md
   ============================================================================ */
#include <print>
#include <string>
#include <vector>
#include <memory>

// === Trace pour observer la copy elision (lignes 313-327) ===
struct Trace {
    Trace()                    { std::print("[Trace] Construction\n"); }
    Trace(const Trace&)        { std::print("[Trace] COPIE\n"); }
    Trace(Trace&&) noexcept    { std::print("[Trace] MOVE\n"); }
    ~Trace()                   { std::print("[Trace] Destruction\n"); }
};

Trace creer_trace() {
    Trace t;
    return t;  // NRVO attendu
}

void test_nrvo() {
    std::print("=== NRVO (Trace) ===\n");
    Trace resultat = creer_trace();
    std::print("--- fin ---\n");
}

// === RVO prvalue obligatoire C++17 (lignes 62-73) ===
class NonCopiable {
public:
    NonCopiable() { std::print("[NonCopiable] Construction\n"); }
    NonCopiable(const NonCopiable&) = delete;
    NonCopiable(NonCopiable&&) = delete;
};

NonCopiable creer_non_copiable() {
    return NonCopiable{};  // C++17 copy elision obligatoire
}

void test_rvo_obligatoire() {
    std::print("\n=== RVO obligatoire C++17 ===\n");
    auto obj = creer_non_copiable();
    std::print("Objet créé sans copie ni move\n");
}

// === NRVO avec chemin unique (lignes 143-150) ===
std::string construire(bool detaille) {
    std::string resultat = "Base";
    if (detaille) {
        resultat += " — détails complets";
    }
    return resultat;  // NRVO possible — un seul candidat
}

void test_nrvo_chemin_unique() {
    std::print("\n=== NRVO chemin unique ===\n");
    auto s1 = construire(false);
    auto s2 = construire(true);
    std::print("s1 = '{}'\n", s1);
    std::print("s2 = '{}'\n", s2);
}

// === Plusieurs chemins → pas de NRVO → move implicite (lignes 123-135) ===
std::string choisir(bool condition) {
    std::string a = "Alpha";
    std::string b = "Beta";
    if (condition) {
        return a;
    }
    return b;
    // NRVO impossible mais déplacement implicite
}

void test_multi_chemin() {
    std::print("\n=== Plusieurs chemins (move implicite) ===\n");
    auto s1 = choisir(true);
    auto s2 = choisir(false);
    std::print("s1 = '{}'\n", s1);
    std::print("s2 = '{}'\n", s2);
}

// === construire_chemin NRVO (lignes 83-91) ===
std::string construire_chemin(const std::string& base, const std::string& fichier) {
    std::string resultat = base;
    resultat += '/';
    resultat += fichier;
    return resultat;
}

void test_construire_chemin() {
    std::print("\n=== construire_chemin (NRVO) ===\n");
    auto chemin = construire_chemin("/home", "data.txt");
    std::print("chemin = '{}'\n", chemin);
}

// === RVO prvalue vector (lignes 48-54) ===
std::vector<int> generer() {
    return std::vector<int>{1, 2, 3, 4, 5};
}

void test_rvo_vector() {
    std::print("\n=== RVO vector ===\n");
    auto v = generer();
    std::print("v.size() = {}\n", v.size());
    std::print("v = ");
    for (int x : v) std::print("{} ", x);
    std::print("\n");
}

// === Retour par valeur moderne (lignes 376-397) ===
int compute(int i) { return i * i; }

std::vector<int> calculer() {
    std::vector<int> resultats;
    resultats.reserve(100);
    for (int i = 0; i < 100; ++i) {
        resultats.push_back(compute(i));
    }
    return resultats;
}

void test_retour_valeur() {
    std::print("\n=== Retour par valeur ===\n");
    auto r = calculer();
    std::print("r.size() = {}\n", r.size());
    std::print("r[10] = {}\n", r[10]);  // 100
}

// === return std::move pessimisation (lignes 233-246) ===
struct TraceVec {
    std::vector<int> data;
    TraceVec(std::initializer_list<int> il) : data(il) {
        std::print("[TraceVec] Construction ({} éléments)\n", data.size());
    }
    TraceVec(const TraceVec& o) : data(o.data) {
        std::print("[TraceVec] COPIE\n");
    }
    TraceVec(TraceVec&& o) noexcept : data(std::move(o.data)) {
        std::print("[TraceVec] MOVE\n");
    }
    ~TraceVec() = default;
};

TraceVec generer_bon() {
    TraceVec v = {1, 2, 3, 4, 5};
    return v;  // NRVO attendu → 0 move
}

TraceVec generer_mauvais() {
    TraceVec v = {1, 2, 3, 4, 5};
    return std::move(v);  // Force un move, pas de NRVO → pessimisation
}

void test_pessimisation() {
    std::print("\n=== return v (NRVO) vs return std::move(v) ===\n");
    std::print("--- return v; ---\n");
    auto a = generer_bon();
    std::print("--- return std::move(v); ---\n");
    auto b = generer_mauvais();
}

// === Builder avec return std::move justifié (lignes 257-266) ===
class Builder {
    std::string resultat_;
public:
    Builder& ajouter(const std::string& s) {
        resultat_ += s;
        return *this;
    }
    // Qualificateur && → consomme le builder (appelé sur rvalue)
    std::string build() && {
        return std::move(resultat_);  // ✅ Déplacement du membre
    }
};

void test_builder() {
    std::print("\n=== Builder avec std::move justifié ===\n");
    // Builder().build() fonctionne car Builder() est un temporaire (rvalue)
    auto texte1 = Builder().build();
    std::print("texte1 = '{}'\n", texte1);

    // Avec une lvalue, il faut std::move pour appeler build() &&
    Builder b;
    b.ajouter("Hello").ajouter(" World");
    auto texte2 = std::move(b).build();
    std::print("texte2 = '{}'\n", texte2);
}

// === Paramètre → move implicite C++20+ (lignes 156-161) ===
std::string transformer(std::string input) {
    input += " transformé";
    return input;  // C++20 : move implicite pour paramètres
}

void test_parametre_move() {
    std::print("\n=== Paramètre move implicite ===\n");
    auto s = transformer("Hello");
    std::print("s = '{}'\n", s);
}

int main() {
    test_nrvo();
    test_rvo_obligatoire();
    test_nrvo_chemin_unique();
    test_multi_chemin();
    test_construire_chemin();
    test_rvo_vector();
    test_retour_valeur();
    test_pessimisation();
    test_builder();
    test_parametre_move();
    std::print("\n✅ Tous les exemples passés\n");
}
