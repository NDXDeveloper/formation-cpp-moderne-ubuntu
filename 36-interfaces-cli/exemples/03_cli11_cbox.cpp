/* ============================================================================
   Section 36.1.2 : Options, flags et sous-commandes
   Description : Outil cbox — sous-commandes run/ps/stop, options globales,
                 alias, trailing args, exclusions mutuelles
   Fichier source : 01.2-options-flags.md
   ============================================================================ */

#include <CLI/CLI.hpp>
#include <print>
#include <string>
#include <vector>
#include <format>

int main(int argc, char* argv[]) {
    CLI::App app{"cbox — Gestionnaire de conteneurs"};
    app.require_subcommand(1);

    bool verbose = false;
    app.add_flag("--verbose,-v", verbose, "Mode verbeux");

    auto* cmd_run = app.add_subcommand("run", "Lancer un conteneur");
    std::string image;
    cmd_run->add_option("image", image, "Image à exécuter")->required();
    std::string name;
    cmd_run->add_option("--name,-n", name, "Nom du conteneur");
    std::vector<std::string> env_vars;
    cmd_run->add_option("--env,-e", env_vars, "Variables d'environnement");
    std::vector<std::string> ports;
    cmd_run->add_option("--publish,-p", ports, "Mapping de ports");
    bool detach = false;
    cmd_run->add_flag("--detach,-d", detach, "Exécuter en arrière-plan");
    bool rm = false;
    cmd_run->add_flag("--rm", rm, "Supprimer le conteneur à l'arrêt");

    auto* cmd_ps = app.add_subcommand("ps", "Lister les conteneurs");
    bool all = false;
    cmd_ps->add_flag("--all,-a", all, "Inclure les conteneurs arrêtés");
    std::string format = "table";
    cmd_ps->add_option("--format,-f", format, "Format de sortie")
        ->check(CLI::IsMember({"table", "json", "wide"}));

    auto* cmd_stop = app.add_subcommand("stop", "Arrêter des conteneurs");
    std::vector<std::string> containers;
    cmd_stop->add_option("containers", containers, "Conteneurs à arrêter")
        ->required()->expected(1, -1);
    bool force = false;
    cmd_stop->add_flag("--force,-f", force, "Forcer l'arrêt");

    CLI11_PARSE(app, argc, argv);

    if (cmd_run->parsed()) {
        std::println("Lancement de {} {}{}", image,
                     detach ? "(détaché) " : "", rm ? "(auto-suppression)" : "");
    } else if (cmd_ps->parsed()) {
        std::println("Liste des conteneurs{} (format: {})",
                     all ? " (tous)" : " (actifs)", format);
    } else if (cmd_stop->parsed()) {
        std::println("Arrêt de {} conteneur(s){}", containers.size(),
                     force ? " (forcé)" : "");
        for (const auto& c : containers) std::println("  → {}", c);
    }
    return 0;
}
