/* ============================================================================
   Section 36.1 : CLI11 — Parsing d'arguments professionnel
   Description : Outil deptool avec sous-commandes install/list,
                 validation Range, aide auto-générée
   Fichier source : 01-cli11.md
   ============================================================================ */

#include <CLI/CLI.hpp>
#include <string>
#include <print>

int main(int argc, char* argv[]) {
    CLI::App app{"deptool — Gestionnaire de dépendances C++"};
    app.require_subcommand(1);

    auto* install_cmd = app.add_subcommand("install", "Installer une dépendance");
    std::string package_name;
    std::string version = "latest";
    int jobs = 4;
    bool system_wide = false;

    install_cmd->add_option("package", package_name, "Nom du paquet")->required();
    install_cmd->add_option("--version,-v", version, "Version à installer");
    install_cmd->add_option("--jobs,-j", jobs, "Nombre de jobs parallèles")
        ->check(CLI::Range(1, 32));
    install_cmd->add_flag("--system-wide,-s", system_wide, "Installation système");

    auto* list_cmd = app.add_subcommand("list", "Lister les dépendances");
    bool outdated = false;
    list_cmd->add_flag("--outdated", outdated, "Afficher uniquement les obsolètes");

    CLI11_PARSE(app, argc, argv);

    if (install_cmd->parsed()) {
        std::println("Installation de {} v{} ({} jobs, système: {})",
                     package_name, version, jobs, system_wide);
    } else if (list_cmd->parsed()) {
        std::println("Liste des dépendances{}", outdated ? " obsolètes" : "");
    }
    return 0;
}
