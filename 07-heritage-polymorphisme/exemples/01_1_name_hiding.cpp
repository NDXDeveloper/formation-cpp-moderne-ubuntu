/* ============================================================================
   Section 7.1.1 : Heritage simple — Name hiding et masquage de surcharges
   Description : Demonstration du name hiding (resolution statique vs type reel),
                 masquage de surcharges et resolution avec using
   Fichier source : 01.1-heritage-simple.md
   ============================================================================ */
#include <print>
#include <string>

// --- Partie 1 : Name hiding basique ---

class Logger {
public:
    void log(std::string const& message) {
        std::println("[LOG] {}", message);
    }
};

class TimestampLogger : public Logger {
public:
    void log(std::string const& message) {
        std::println("[2026-03-10 14:30:00] {}", message);
    }
};

// --- Partie 2 : Masquage de surcharges ---

class BaseTraiter {
public:
    void traiter(int x)    { std::println("Base::traiter(int): {}", x); }
    void traiter(double x) { std::println("Base::traiter(double): {}", x); }
};

class DeriveeSanUsing : public BaseTraiter {
public:
    void traiter(int x)    { std::println("Derivee::traiter(int): {}", x); }
};

class DeriveeAvecUsing : public BaseTraiter {
public:
    using BaseTraiter::traiter;       // rend visibles TOUTES les surcharges de Base
    void traiter(int x) { std::println("Derivee::traiter(int): {}", x); }
};

int main() {
    std::println("=== Name hiding basique ===");
    TimestampLogger tl;
    tl.log("test");             // appelle TimestampLogger::log

    Logger& ref = tl;
    ref.log("test");            // appelle Logger::log — PAS TimestampLogger::log !

    std::println("\n=== Masquage de surcharges (sans using) ===");
    DeriveeSanUsing d1;
    d1.traiter(42);     // OK — Derivee::traiter(int)
    d1.traiter(3.14);   // Appelle Derivee::traiter(int) avec conversion !

    std::println("\n=== Masquage resolu (avec using) ===");
    DeriveeAvecUsing d2;
    d2.traiter(42);     // Derivee::traiter(int)
    d2.traiter(3.14);   // Base::traiter(double) — correctement dispatche
}
