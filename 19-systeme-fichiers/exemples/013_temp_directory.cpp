/* ============================================================================
   Section 19.1.3 : Répertoire temporaire RAII
   Description : Classe TempDirectory avec nettoyage automatique
   Fichier source : 01.3-operations-fichiers.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <format>
#include <unistd.h>
#include <chrono>

namespace fs = std::filesystem;

class TempDirectory {
public:
    explicit TempDirectory(const std::string& prefix = "tmp") {
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        auto name = std::format("{}_{}_{}",prefix, getpid(), now);
        path_ = fs::temp_directory_path() / name;
        fs::create_directories(path_);
    }

    ~TempDirectory() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    // Non copiable, déplaçable
    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;
    TempDirectory(TempDirectory&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }
    TempDirectory& operator=(TempDirectory&& other) noexcept {
        if (this != &other) {
            cleanup();
            path_ = std::move(other.path_);
            other.path_.clear();
        }
        return *this;
    }

    [[nodiscard]] const fs::path& path() const noexcept { return path_; }

    [[nodiscard]] fs::path operator/(const fs::path& child) const {
        return path_ / child;
    }

private:
    void cleanup() noexcept {
        if (!path_.empty()) {
            std::error_code ec;
            fs::remove_all(path_, ec);
        }
    }

    fs::path path_;
};

int main() {
    {
        TempDirectory tmp("build_cache");
        std::println("Répertoire temporaire : {}", tmp.path().string());

        // Utiliser le répertoire
        fs::create_directory(tmp / "objects");
        fs::create_directory(tmp / "deps");

        std::println("Existe : {}", fs::exists(tmp.path()));
        std::println("objects/ : {}", fs::exists(tmp / "objects"));
        std::println("deps/    : {}", fs::exists(tmp / "deps"));

    }  // Nettoyage automatique ici — RAII

    std::println("Répertoire temporaire supprimé");
}
