/* ============================================================================
   Section 7.4 : Classes abstraites — Template Method pattern
   Description : Architecture a trois niveaux : interface pure (ILogger) ->
                 squelette abstrait (AbstractLogger) -> implementations
                 concretes (ConsoleLogger, FileLogger). Factory function.
   Fichier source : 04-classes-abstraites.md
   ============================================================================ */
#include <print>
#include <string>
#include <string_view>
#include <memory>
#include <vector>

class ILogger {
public:
    virtual void log(std::string_view message) = 0;
    virtual void set_level(int level) = 0;
    virtual ~ILogger() = default;
};

class AbstractLogger : public ILogger {
    int level_ = 0;
public:
    void set_level(int level) override {
        level_ = level;
    }

    void log(std::string_view message) override {
        if (should_log()) {
            do_log(message);
        }
    }

protected:
    bool should_log() const { return level_ > 0; }
    virtual void do_log(std::string_view message) = 0;
};

class ConsoleLogger final : public AbstractLogger {
protected:
    void do_log(std::string_view message) override {
        std::println("[CONSOLE] {}", message);
    }
};

class FileLogger final : public AbstractLogger {
    std::string chemin_;
protected:
    void do_log(std::string_view message) override {
        std::println("[FILE:{}] {}", chemin_, message);
    }
public:
    explicit FileLogger(std::string chemin) : chemin_{std::move(chemin)} {}
};

// Factory function
enum class LogDestination { Console, File };

std::unique_ptr<ILogger> creer_logger(LogDestination dest,
                                       std::string const& param = "") {
    switch (dest) {
        case LogDestination::Console:
            return std::make_unique<ConsoleLogger>();
        case LogDestination::File:
            return std::make_unique<FileLogger>(param);
    }
    return std::make_unique<ConsoleLogger>();
}

void log_partout(std::vector<std::unique_ptr<ILogger>> const& loggers,
                 std::string_view message) {
    for (auto const& logger : loggers) {
        logger->log(message);   // dispatch dynamique via l'interface
    }
}

int main() {
    std::println("=== Factory function ===");
    auto logger = creer_logger(LogDestination::File, "/tmp/app.log");
    logger->set_level(1);
    logger->log("Application démarrée");

    std::println("\n=== Collection polymorphique ===");
    std::vector<std::unique_ptr<ILogger>> loggers;
    loggers.push_back(creer_logger(LogDestination::Console));
    loggers.push_back(creer_logger(LogDestination::File, "/var/log/app.log"));

    for (auto& l : loggers) l->set_level(1);
    log_partout(loggers, "Message diffusé partout");

    std::println("\n=== Filtrage par niveau (level=0 → pas de log) ===");
    auto silent = creer_logger(LogDestination::Console);
    // level reste à 0 → ne log rien
    silent->log("Ce message ne s'affiche pas");
    std::println("(rien au-dessus)");
}
