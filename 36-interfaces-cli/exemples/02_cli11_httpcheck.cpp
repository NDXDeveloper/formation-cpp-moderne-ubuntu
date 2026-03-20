/* ============================================================================
   Section 36.1.1 : Installation et premiers pas
   Description : Outil httpcheck — options, flags mutuellement exclusifs,
                 formats de sortie, headers répétables
   Fichier source : 01.1-installation.md
   ============================================================================ */

#include <CLI/CLI.hpp>
#include <print>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    CLI::App app{"httpcheck — Vérification de disponibilité HTTP"};
    app.set_version_flag("--version,-V", "httpcheck 1.0.0");

    std::string url;
    app.add_option("url", url, "URL à vérifier")
        ->required();

    int timeout_ms = 5000;
    app.add_option("--timeout,-t", timeout_ms, "Timeout en millisecondes")
        ->check(CLI::Range(100, 60'000))
        ->default_str("5000");

    int retries = 3;
    app.add_option("--retries,-r", retries, "Nombre de tentatives")
        ->check(CLI::Range(1, 20));

    int interval_ms = 1000;
    app.add_option("--interval,-i", interval_ms, "Intervalle entre tentatives (ms)")
        ->check(CLI::Range(100, 30'000));

    bool verbose = false;
    app.add_flag("--verbose,-v", verbose, "Afficher les détails");

    bool quiet = false;
    app.add_flag("--quiet,-q", quiet, "Afficher uniquement le résultat final");

    app.get_option("--verbose")->excludes(app.get_option("--quiet"));
    app.get_option("--quiet")->excludes(app.get_option("--verbose"));

    std::string format = "text";
    app.add_option("--format,-f", format, "Format de sortie")
        ->check(CLI::IsMember({"text", "json"}));

    std::vector<std::string> headers;
    app.add_option("--header,-H", headers, "Headers HTTP");

    CLI11_PARSE(app, argc, argv);

    if (verbose) {
        std::println("Configuration :");
        std::println("  URL       : {}", url);
        std::println("  Timeout   : {} ms", timeout_ms);
        std::println("  Tentatives: {}", retries);
        std::println("  Format    : {}", format);
    }

    if (format == "json") {
        std::println(R"({{"url":"{}","status":"ok","attempts":1}})", url);
    } else if (!quiet) {
        std::println("✓ {} — accessible (1/{} tentatives)", url, retries);
    }
    return 0;
}
