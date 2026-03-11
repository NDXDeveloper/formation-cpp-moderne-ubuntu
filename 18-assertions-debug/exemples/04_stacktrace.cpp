/* ============================================================================
   Section 18.4 : std::stacktrace (C++23)
   Description : Capture de pile d'appels standardisée — capture basique,
                 ASSERT_WITH_TRACE (assertion enrichie), TracedException
                 (exception avec trace), SortedBuffer (vérification
                 d'invariant), itération et filtrage des frames,
                 current(skip, max_depth)
   Fichier source : 04-stacktrace.md
   Compilation : g++-15 -std=c++23 -g 04_stacktrace.cpp -o 04_stacktrace \
                 -L/usr/lib/gcc/x86_64-linux-gnu/15 -lstdc++exp
   ============================================================================ */

#include <stacktrace>
#include <iostream>
#include <cstdlib>
#include <string_view>
#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>

// === ASSERT_WITH_TRACE ===
#define ASSERT_WITH_TRACE(condition, message)                            \
    do {                                                                 \
        if (!(condition)) [[unlikely]] {                                 \
            std::cerr << "\n╔══ ASSERTION FAILED ══════════════════\n"   \
                      << "║ Condition : " #condition "\n"                \
                      << "║ Message   : " << (message) << "\n"          \
                      << "║ Fichier   : " << __FILE__ << "\n"           \
                      << "║ Ligne     : " << __LINE__ << "\n"           \
                      << "╠══ STACK TRACE ═══════════════════════\n"    \
                      << std::stacktrace::current() << "\n"             \
                      << "╚══════════════════════════════════════\n";   \
            /* Ne pas abort ici pour continuer la démonstration */       \
        }                                                                \
    } while (false)

// === TracedException ===
class TracedException : public std::runtime_error {
public:
    explicit TracedException(const std::string& message)
        : std::runtime_error(message)
        , trace_(std::stacktrace::current())
    {}

    const std::stacktrace& trace() const noexcept { return trace_; }

    void print_diagnostic(std::ostream& os = std::cerr) const {
        os << "Exception : " << what() << "\n\n"
           << "Capturée à :\n" << trace_ << '\n';
    }

private:
    std::stacktrace trace_;
};

class ConfigError : public TracedException {
public:
    explicit ConfigError(const std::string& msg)
        : TracedException("Erreur de configuration: " + msg) {}
};

// === SortedBuffer avec vérification d'invariant ===
class SortedBuffer {
public:
    void insert(int value) {
        auto pos = std::lower_bound(data_.begin(), data_.end(), value);
        data_.insert(pos, value);
        check_invariant();
    }

    void unsafe_push_back(int value) {
        data_.push_back(value);
        check_invariant();
    }

    void print() const {
        for (auto v : data_) std::cerr << v << ' ';
        std::cerr << '\n';
    }

private:
    std::vector<int> data_;

    void check_invariant() const {
        if (!std::is_sorted(data_.begin(), data_.end())) {
            std::cerr << "[INVARIANT VIOLÉ] SortedBuffer n'est plus trié !\n"
                      << "  Contenu: ";
            for (auto v : data_) std::cerr << v << ' ';
            std::cerr << "\n\n  Pile d'appels:\n"
                      << std::stacktrace::current() << '\n';
            // Ne pas abort pour continuer la démonstration
        }
    }
};

// === Filtrage des frames ===
void log_filtered_trace(const std::stacktrace& trace,
                        const std::string& filter_path) {
    std::cerr << "  Trace filtrée :\n";
    int frame = 0;
    for (const auto& entry : trace) {
        std::string file = entry.source_file();
        if (file.find(filter_path) != std::string::npos) {
            std::cerr << "    #" << frame
                      << " " << entry.description()
                      << " at " << file
                      << ":" << entry.source_line() << '\n';
        }
        ++frame;
    }
}

// === Fonctions pour la démonstration ===

void fonction_profonde() {
    auto trace = std::stacktrace::current();
    std::cerr << "  Pile d'appels :\n" << trace << '\n';
}

void fonction_intermediaire() {
    fonction_profonde();
}

void load_config(const std::string& path) {
    throw ConfigError("Fichier '" + path + "' introuvable");
}

void initialize_service() {
    load_config("/etc/myapp/config.yaml");
}

struct Account {
    std::string name;
    double bal;
    double balance() const { return bal; }
};

void transfer(Account& from, Account& to, double amount) {
    ASSERT_WITH_TRACE(amount > 0,
        "Le montant du transfert doit être positif");
    ASSERT_WITH_TRACE(from.balance() >= amount,
        "Solde insuffisant pour le transfert");
    from.bal -= amount;
    to.bal += amount;
}

void execute_transfers(Account& a, Account& b) {
    transfer(a, b, 100.0);
    transfer(a, b, 5000.0);  // va déclencher l'assertion
}

int main() {
    std::cerr << "=== 1. Capture basique de stacktrace ===\n";
    fonction_intermediaire();

    std::cerr << "=== 2. TracedException (capture au moment du throw) ===\n";
    try {
        initialize_service();
    } catch (const TracedException& ex) {
        ex.print_diagnostic();
    }

    std::cerr << "\n=== 3. ASSERT_WITH_TRACE ===\n";
    {
        Account alice{"Alice", 200.0};
        Account bob{"Bob", 50.0};
        execute_transfers(alice, bob);
    }

    std::cerr << "\n=== 4. SortedBuffer — invariant correct ===\n";
    {
        SortedBuffer buf;
        buf.insert(30);
        buf.insert(10);
        buf.insert(20);
        std::cerr << "  Contenu (trié): ";
        buf.print();
    }

    std::cerr << "\n=== 5. SortedBuffer — invariant violé ===\n";
    {
        SortedBuffer buf;
        buf.insert(10);
        buf.insert(20);
        buf.unsafe_push_back(5);  // Viole l'invariant
    }

    std::cerr << "\n=== 6. Itération et filtrage des frames ===\n";
    {
        auto trace = std::stacktrace::current();
        log_filtered_trace(trace, "04_stacktrace");
    }

    std::cerr << "\n=== 7. current(skip, max_depth) ===\n";
    {
        auto trace = std::stacktrace::current(0, 3);
        std::cerr << "  Trace limitée (max 3 frames) :\n" << trace << '\n';
    }

    std::cerr << "Programme terminé.\n";
    return 0;
}
