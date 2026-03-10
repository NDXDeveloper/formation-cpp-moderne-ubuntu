/* ============================================================================
   Section 7.5 : Cout du polymorphisme — Micro-benchmark
   Description : Comparaison du temps d'execution entre appel direct (inline)
                 et appel virtuel (dispatch dynamique) sur 10M elements.
                 Les resultats varient selon le processeur et le compilateur.
   Fichier source : 05-cout-polymorphisme.md
   ============================================================================ */
#include <vector>
#include <chrono>
#include <print>
#include <memory>

class Operation {
public:
    virtual double executer(double x) const = 0;
    virtual ~Operation() = default;
};

class Doubler final : public Operation {
public:
    double executer(double x) const override { return x * 2.0; }
};

// Version sans virtual — appel direct
double traiter_direct(std::vector<double> const& data) {
    double somme = 0.0;
    for (auto x : data) {
        somme += x * 2.0;   // opération inlinée
    }
    return somme;
}

// Version avec virtual — dispatch dynamique
double traiter_virtuel(std::vector<double> const& data, Operation const& op) {
    double somme = 0.0;
    for (auto x : data) {
        somme += op.executer(x);   // appel virtuel à chaque itération
    }
    return somme;
}

int main() {
    constexpr int N = 10'000'000;
    std::vector<double> data(N, 1.5);
    Doubler d;

    auto t1 = std::chrono::high_resolution_clock::now();
    auto r1 = traiter_direct(data);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto r2 = traiter_virtuel(data, d);
    auto t3 = std::chrono::high_resolution_clock::now();

    auto ms_direct = std::chrono::duration<double, std::milli>(t2 - t1).count();
    auto ms_virtual = std::chrono::duration<double, std::milli>(t3 - t2).count();

    std::println("Direct  : {:.2f} ms (somme={})", ms_direct, r1);
    std::println("Virtual : {:.2f} ms (somme={})", ms_virtual, r2);
    if (ms_direct > 0)
        std::println("Ratio   : {:.1f}x", ms_virtual / ms_direct);
}
