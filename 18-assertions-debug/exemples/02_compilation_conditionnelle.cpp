/* ============================================================================
   Section 18.2 : Compilation conditionnelle (#ifdef DEBUG)
   Description : Macro DEBUG, DBG_LOG, niveaux de verbosité (DEBUG_LEVEL),
                 ConnectionPool avec compteurs debug, if constexpr avec
                 is_debug, consteval pour vérification compile-time
   Fichier source : 02-compilation-conditionnelle.md
   ============================================================================ */

#include <iostream>
#include <vector>
#include <string_view>
#include <string>

// === Macro DEBUG et DBG_LOG ===
#ifdef DEBUG
    #define DBG_LOG(msg) std::cerr << "[DEBUG " << __FILE__ << ":" \
                                   << __LINE__ << "] " << msg << '\n'
#else
    #define DBG_LOG(msg) ((void)0)
#endif

// === Niveaux de verbosité ===
#ifndef DEBUG_LEVEL
    #define DEBUG_LEVEL 0
#endif

#if DEBUG_LEVEL >= 1
    #define DBG_ERROR(msg)   std::cerr << "[ERROR] " << msg << '\n'
    #define DBG_WARN(msg)    std::cerr << "[WARN]  " << msg << '\n'
#else
    #define DBG_ERROR(msg)   ((void)0)
    #define DBG_WARN(msg)    ((void)0)
#endif

#if DEBUG_LEVEL >= 2
    #define DBG_INFO(msg)    std::cerr << "[INFO]  " << msg << '\n'
#else
    #define DBG_INFO(msg)    ((void)0)
#endif

#if DEBUG_LEVEL >= 3
    #define DBG_TRACE(msg)   std::cerr << "[TRACE] " << msg << '\n'
#else
    #define DBG_TRACE(msg)   ((void)0)
#endif

// === ConnectionPool avec compteurs debug ===
class ConnectionPool {
public:
    void acquire(std::string_view name) {
        DBG_LOG("Acquisition de connexion pour: " << name);
#ifdef DEBUG
        ++debug_acquire_count_;
        DBG_LOG("Total acquisitions: " << debug_acquire_count_);
#endif
    }

    void release(std::string_view name) {
        DBG_LOG("Libération de connexion pour: " << name);
    }

#ifdef DEBUG
    void dump_stats() const {
        std::cerr << "[DEBUG] === Pool Statistics ===\n"
                  << "  Acquisitions totales: " << debug_acquire_count_ << '\n';
    }
#endif

private:
#ifdef DEBUG
    int debug_acquire_count_ = 0;
#endif
};

// === if constexpr avec is_debug ===
#ifdef DEBUG
    inline constexpr bool is_debug = true;
#else
    inline constexpr bool is_debug = false;
#endif

template <typename T>
void process(const T& data) {
    if constexpr (is_debug) {
        std::cerr << "[DEBUG] Processing data of size: "
                  << data.size() << '\n';
    }
    // traitement silencieux en release
}

// === consteval pour vérification compile-time ===
consteval int checked_divide(int a, int b) {
    if (b == 0) {
        throw "Division par zéro détectée à la compilation";
    }
    return a / b;
}

constexpr int result = checked_divide(42, 7);  // OK
// constexpr int bad = checked_divide(42, 0);  // Erreur à la compilation

// === Vérification coûteuse en debug uniquement ===
class SortedContainer {
public:
    void insert(int value) {
        auto pos = std::lower_bound(data_.begin(), data_.end(), value);
        data_.insert(pos, value);

#ifdef DEBUG
        // Vérification O(n) que le conteneur reste trié
        for (std::size_t i = 1; i < data_.size(); ++i) {
            if (data_[i] < data_[i - 1]) {
                std::cerr << "[DEBUG] INVARIANT VIOLÉ : conteneur non trié "
                          << "à l'index " << i << '\n';
                std::abort();
            }
        }
#endif
    }

    std::size_t size() const { return data_.size(); }
    void print() const {
        for (auto v : data_) std::cout << v << ' ';
        std::cout << '\n';
    }

private:
    std::vector<int> data_;
};

int main() {
    std::cout << "=== 1. ConnectionPool avec DEBUG ===\n";
    {
        ConnectionPool pool;
        pool.acquire("user_service");
        pool.acquire("auth_service");
        pool.release("user_service");
#ifdef DEBUG
        pool.dump_stats();
#endif
    }

    std::cout << "\n=== 2. Niveaux de verbosité (DEBUG_LEVEL=" << DEBUG_LEVEL << ") ===\n";
    DBG_TRACE("Entering main");
    DBG_INFO("Application démarrée");
    DBG_WARN("Config absente, utilisation des défauts");
    DBG_ERROR("Connexion à la base échouée");
    DBG_TRACE("Exiting main");

    std::cout << "\n=== 3. if constexpr (is_debug=" << is_debug << ") ===\n";
    {
        std::vector<int> v{1, 2, 3, 4, 5};
        process(v);
        std::string s = "hello";
        process(s);
    }

    std::cout << "\n=== 4. consteval ===\n";
    std::cout << "  checked_divide(42, 7) = " << result << '\n';

    std::cout << "\n=== 5. SortedContainer avec vérification debug ===\n";
    {
        SortedContainer sc;
        sc.insert(30);
        sc.insert(10);
        sc.insert(20);
        sc.insert(5);
        sc.insert(25);
        std::cout << "  Contenu trié (" << sc.size() << " éléments): ";
        sc.print();
    }

    std::cout << "\nProgramme terminé.\n";
    return 0;
}
