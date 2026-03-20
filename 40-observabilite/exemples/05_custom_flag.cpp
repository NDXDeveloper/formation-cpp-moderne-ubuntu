/* ============================================================================
   Section 40.1.3 : Pattern de formatage
   Description : Custom flag formatter — injection du hostname via %*
   Fichier source : 01.3-pattern-formatage.md
   ============================================================================ */

#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>
#include <unistd.h>  // gethostname

// Flag personnalisé %* qui affiche le nom du serveur
class server_name_flag : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg&,
                const std::tm&,
                spdlog::memory_buf_t& dest) override {
        static const std::string hostname = []() {
            char buf[256];
            gethostname(buf, sizeof(buf));
            return std::string(buf);
        }();
        dest.append(hostname.data(), hostname.data() + hostname.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return std::make_unique<server_name_flag>();
    }
};

int main() {
    // Enregistrement du flag personnalisé
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<server_name_flag>('*');
    formatter->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%*] [%l] %v");

    auto& sinks = spdlog::default_logger()->sinks();
    auto logger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
    logger->set_formatter(std::move(formatter));
    spdlog::set_default_logger(logger);

    spdlog::info("Server started on port 8080");
    spdlog::warn("High memory usage detected");
    spdlog::error("Connection refused by upstream");

    return 0;
}
