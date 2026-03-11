/* ============================================================================
   Section 18.3 : Logging et traces d'exécution
   Description : SimpleLogger thread-safe avec source_location, macros LOG_*,
                 ScopeTracer RAII pour tracer entrée/sortie de fonctions,
                 TimedScope pour chronométrer des blocs, std::format pour
                 les messages de log
   Fichier source : 03-logging-traces.md
   ============================================================================ */

#include <iostream>
#include <string_view>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <source_location>
#include <format>
#include <vector>
#include <thread>

// === Niveaux de sévérité ===
enum class LogLevel {
    Trace, Debug, Info, Warning, Error, Fatal
};

constexpr std::string_view level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
    }
    return "?????";
}

// === SimpleLogger thread-safe ===
class SimpleLogger {
public:
    static SimpleLogger& instance() {
        static SimpleLogger logger;
        return logger;
    }

    void set_level(LogLevel level) { min_level_ = level; }
    void set_output(std::ostream& os) { output_ = &os; }

    void log(LogLevel level,
             std::string_view message,
             const std::source_location& loc
                 = std::source_location::current())
    {
        if (level < min_level_) return;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::lock_guard<std::mutex> lock(mutex_);

        *output_ << std::put_time(std::localtime(&time), "%H:%M:%S")
                 << '.' << std::setfill('0') << std::setw(3) << ms.count()
                 << " [" << level_to_string(level) << "] "
                 << extract_filename(loc.file_name())
                 << ':' << loc.line() << " — "
                 << message << '\n';
    }

private:
    SimpleLogger() = default;

    static std::string_view extract_filename(const char* path) {
        std::string_view sv(path);
        auto pos = sv.find_last_of("/\\");
        return (pos != std::string_view::npos) ? sv.substr(pos + 1) : sv;
    }

    LogLevel      min_level_ = LogLevel::Trace;
    std::ostream* output_    = &std::cerr;
    std::mutex    mutex_;
};

// === Macros d'interface ===
#define LOG_TRACE(msg)   SimpleLogger::instance().log(LogLevel::Trace, msg)
#define LOG_DEBUG(msg)   SimpleLogger::instance().log(LogLevel::Debug, msg)
#define LOG_INFO(msg)    SimpleLogger::instance().log(LogLevel::Info, msg)
#define LOG_WARN(msg)    SimpleLogger::instance().log(LogLevel::Warning, msg)
#define LOG_ERROR(msg)   SimpleLogger::instance().log(LogLevel::Error, msg)
#define LOG_FATAL(msg)   SimpleLogger::instance().log(LogLevel::Fatal, msg)

// === ScopeTracer (RAII) ===
class ScopeTracer {
public:
    explicit ScopeTracer(
        std::string_view name,
        const std::source_location& loc = std::source_location::current())
        : name_(name), loc_(loc)
    {
        SimpleLogger::instance().log(
            LogLevel::Trace,
            std::string("→ ENTER ") + std::string(name_),
            loc_);
    }

    ~ScopeTracer() {
        SimpleLogger::instance().log(
            LogLevel::Trace,
            std::string("← EXIT  ") + std::string(name_),
            loc_);
    }

    ScopeTracer(const ScopeTracer&) = delete;
    ScopeTracer& operator=(const ScopeTracer&) = delete;

private:
    std::string_view     name_;
    std::source_location loc_;
};

// Macros auxiliaires pour noms de variables uniques
#define DETAIL_CONCAT(a, b) a##b
#define CONCAT(a, b) DETAIL_CONCAT(a, b)

#define TRACE_SCOPE(name) ScopeTracer CONCAT(_tracer_, __LINE__)(name)
#define TRACE_FUNCTION()  ScopeTracer _tracer_func_(__func__)

// === TimedScope ===
class TimedScope {
public:
    explicit TimedScope(
        std::string_view name,
        const std::source_location& loc = std::source_location::current())
        : name_(name), loc_(loc)
        , start_(std::chrono::steady_clock::now())
    {
        SimpleLogger::instance().log(LogLevel::Debug,
            std::format("⏱ START {}", name_), loc_);
    }

    ~TimedScope() {
        auto elapsed = std::chrono::steady_clock::now() - start_;
        auto us = std::chrono::duration_cast<
            std::chrono::microseconds>(elapsed).count();
        SimpleLogger::instance().log(LogLevel::Debug,
            std::format("⏱ END   {} — {} µs", name_, us), loc_);
    }

    TimedScope(const TimedScope&) = delete;
    TimedScope& operator=(const TimedScope&) = delete;

private:
    std::string_view                      name_;
    std::source_location                  loc_;
    std::chrono::steady_clock::time_point start_;
};

#define TIME_SCOPE(name) TimedScope CONCAT(_timer_, __LINE__)(name)

// === Fonctions de démonstration ===

void load_config(const std::string& path) {
    LOG_INFO("Chargement de la configuration");
    LOG_DEBUG("Chemin: " + path);
    LOG_WARN("Fichier non trouvé, utilisation des valeurs par défaut");
}

void authenticate(const std::string& user) {
    TRACE_FUNCTION();
    LOG_DEBUG(std::format("Authentification de l'utilisateur '{}'", user));

    {
        TRACE_SCOPE("vérification mot de passe");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    {
        TRACE_SCOPE("chargement permissions");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void process_batch(const std::vector<std::string>& items) {
    TIME_SCOPE("process_batch");

    auto msg = std::format("Traitement du batch : {} éléments", items.size());
    LOG_INFO(msg);

    for (std::size_t i = 0; i < items.size(); ++i) {
        LOG_TRACE(std::format("  [{}] → {}", i, items[i]));
    }
}

int main() {
    SimpleLogger::instance().set_level(LogLevel::Trace);

    LOG_INFO("Démarrage de l'application");

    std::cerr << "\n--- load_config ---\n";
    load_config("/etc/app/config.yaml");

    std::cerr << "\n--- authenticate (ScopeTracer) ---\n";
    authenticate("alice");

    std::cerr << "\n--- process_batch (TimedScope) ---\n";
    std::vector<std::string> items{"alpha", "beta", "gamma"};
    process_batch(items);

    std::cerr << "\n--- Filtrage par niveau ---\n";
    SimpleLogger::instance().set_level(LogLevel::Warning);
    LOG_TRACE("Ce message ne s'affiche pas (filtré)");
    LOG_DEBUG("Ce message ne s'affiche pas (filtré)");
    LOG_INFO("Ce message ne s'affiche pas (filtré)");
    LOG_WARN("Ce message S'AFFICHE (Warning >= Warning)");
    LOG_ERROR("Ce message S'AFFICHE (Error >= Warning)");

    std::cerr << "\n";
    // Remettre pour le message final
    SimpleLogger::instance().set_level(LogLevel::Info);
    LOG_INFO("Arrêt normal");

    return 0;
}
