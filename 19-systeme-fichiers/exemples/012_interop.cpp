/* ============================================================================
   Section 19.1.2 : Conversion, interopérabilité et variables d'environnement
   Description : string(), c_str(), API POSIX, XDG, temp_directory_path()
   Fichier source : 01.2-manipulation-chemins.md
   ============================================================================ */
#include <filesystem>
#include <print>
#include <cstdlib>

namespace fs = std::filesystem;

auto get_config_path() -> fs::path {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0') {
        return fs::path(xdg) / "mon_app";
    }

    const char* home = std::getenv("HOME");
    if (home && home[0] != '\0') {
        return fs::path(home) / ".config" / "mon_app";
    }

    return "/tmp/mon_app";
}

auto get_cache_path() -> fs::path {
    const char* xdg = std::getenv("XDG_CACHE_HOME");
    if (xdg && xdg[0] != '\0') {
        return fs::path(xdg) / "mon_app";
    }

    const char* home = std::getenv("HOME");
    if (home && home[0] != '\0') {
        return fs::path(home) / ".cache" / "mon_app";
    }

    return fs::temp_directory_path() / "mon_app";
}

int main() {
    std::println("Config : {}", get_config_path().string());
    std::println("Cache  : {}", get_cache_path().string());
    std::println("Temp   : {}", fs::temp_directory_path().string());
}
