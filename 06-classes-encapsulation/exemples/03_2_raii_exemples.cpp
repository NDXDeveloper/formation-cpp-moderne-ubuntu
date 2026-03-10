/* ============================================================================
   Section 6.3.2 : Exemples pratiques de RAII
   Description : ScopedTimer, CoutFlagsGuard, ScopeGuard — wrappers RAII
                 pour le profiling, la restauration d'état, et les actions
                 de fin de portée
   Fichier source : 03.2-raii-exemples.md
   ============================================================================ */
#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>

// --- ScopedTimer : timer de profiling automatique ---
class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& label)
        : label_(label)
        , start_(std::chrono::steady_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start_
        );
        std::cerr << "[TIMER] " << label_ << ": "
                  << elapsed.count() << " µs\n";
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    std::string label_;
    std::chrono::steady_clock::time_point start_;
};

// --- CoutFlagsGuard : restauration d'état de std::cout ---
class CoutFlagsGuard {
public:
    explicit CoutFlagsGuard(std::ostream& os = std::cout)
        : os_(os)
        , saved_flags_(os.flags())
        , saved_precision_(os.precision())
        , saved_fill_(os.fill()) {}

    ~CoutFlagsGuard() {
        os_.flags(saved_flags_);
        os_.precision(saved_precision_);
        os_.fill(saved_fill_);
    }

    CoutFlagsGuard(const CoutFlagsGuard&) = delete;
    CoutFlagsGuard& operator=(const CoutFlagsGuard&) = delete;

private:
    std::ostream& os_;
    std::ios_base::fmtflags saved_flags_;
    std::streamsize saved_precision_;
    char saved_fill_;
};

// --- ScopeGuard : action de fin de portée ---
class ScopeGuard {
public:
    explicit ScopeGuard(std::function<void()> on_exit)
        : on_exit_(std::move(on_exit)) {}

    ~ScopeGuard() {
        if (on_exit_) on_exit_();
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    void dismiss() { on_exit_ = nullptr; }

private:
    std::function<void()> on_exit_;
};

// --- Fonctions de démonstration ---

void print_hex_table(const std::vector<int>& values) {
    CoutFlagsGuard guard;   // Sauvegarde l'état actuel de std::cout

    std::cout << std::hex << std::uppercase << std::setfill('0');
    for (int v : values) {
        std::cout << "0x" << std::setw(4) << v << "\n";
    }
}   // guard restaure les flags

int main() {
    // 1. ScopedTimer
    std::cout << "=== ScopedTimer ===\n";
    {
        ScopedTimer timer("generate");
        std::vector<int> data(100000);
        std::iota(data.begin(), data.end(), 0);
    }

    {
        ScopedTimer timer("sort");
        std::vector<int> data(100000);
        std::iota(data.begin(), data.end(), 0);
        std::sort(data.rbegin(), data.rend());
    }

    // 2. CoutFlagsGuard
    std::cout << "\n=== CoutFlagsGuard ===\n";
    std::cout << "Avant hex: " << 255 << "\n";         // décimal
    print_hex_table({10, 255, 4096});                   // hexa
    std::cout << "Après hex: " << 255 << "\n";          // décimal restauré

    // 3. ScopeGuard
    std::cout << "\n=== ScopeGuard ===\n";

    // Cas 1 : la garde s'exécute (pas de dismiss)
    {
        ScopeGuard guard([]() {
            std::cout << "Rollback exécuté\n";
        });
        std::cout << "Opération en cours...\n";
        // Pas de dismiss → rollback s'exécute
    }

    // Cas 2 : la garde est annulée (dismiss)
    {
        ScopeGuard guard([]() {
            std::cout << "Rollback exécuté (ne devrait pas apparaître)\n";
        });
        std::cout << "Opération réussie\n";
        guard.dismiss();   // Tout a réussi → on annule le rollback
    }

    std::cout << "Fin du programme\n";
    return 0;
}
// Sortie attendue (les temps du timer varient) :
// === ScopedTimer ===
// [TIMER] generate: <N> µs
// [TIMER] sort: <N> µs
//
// === CoutFlagsGuard ===
// Avant hex: 255
// 0x000A
// 0x00FF
// 0x1000
// Après hex: 255
//
// === ScopeGuard ===
// Opération en cours...
// Rollback exécuté
// Opération réussie
// Fin du programme
