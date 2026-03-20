🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 36.5 — Architecture d'un outil CLI professionnel (à la kubectl, git)

## Module 12 : Création d'Outils CLI — Niveau Avancé

---

## Introduction

Les sections précédentes ont couvert les briques individuelles : parsing d'arguments avec CLI11 (**36.1**), formatage avec {fmt} (**36.3**), et conscience du terminal (**36.4**). Cette section finale assemble ces briques dans une architecture cohérente et maintenable.

Les outils CLI que les développeurs utilisent quotidiennement — `git`, `kubectl`, `cargo`, `docker`, `cmake` — ne sont pas des programmes monolithiques avec un `main()` de 2000 lignes. Ce sont des architectures modulaires où chaque sous-commande est isolée, où la configuration est centralisée, où la sortie est découplée de la logique métier, et où l'ajout d'une nouvelle commande ne nécessite pas de modifier les commandes existantes.

Cette section présente un modèle d'architecture applicable à tout outil CLI C++ de taille moyenne à grande, avec un exemple concret fil rouge.

---

## Principes d'architecture

Avant de plonger dans le code, posons les principes directeurs communs aux outils CLI bien conçus :

**Séparation des responsabilités.** Le parsing, la logique métier et l'affichage sont trois couches distinctes. La logique métier ne sait pas qu'elle est invoquée depuis une CLI — elle pourrait tout aussi bien être appelée depuis un test unitaire ou une API.

**Convention over configuration.** Les valeurs par défaut sont saines, les cas courants ne nécessitent aucun flag. L'outil fait la bonne chose sans qu'on lui demande. Les options existent pour les cas non standard.

**Fail fast, fail loud.** Les erreurs sont détectées au plus tôt (validation CLI, vérification des prérequis) et communiquées clairement (message d'erreur explicite, code de sortie approprié, suggestion de correction).

**Composabilité Unix.** `stdout` contient les données exploitables, `stderr` contient le diagnostic. L'outil fonctionne dans un pipe, accepte l'entrée standard, et respecte les conventions de l'écosystème (`NO_COLOR`, `--`, codes de sortie).

---

## Structure de répertoires

Un outil CLI C++ professionnel suit une organisation prévisible :

```
devtool/
├── CMakeLists.txt
├── README.md
├── .clang-format
├── .clang-tidy
├── cmake/
│   └── FetchDependencies.cmake
├── src/
│   ├── main.cpp                    # Point d'entrée, setup CLI
│   ├── app.hpp / app.cpp           # Classe Application (contexte global)
│   ├── cli/
│   │   ├── cli.hpp / cli.cpp       # Configuration CLI11, dispatch
│   │   ├── options.hpp             # Structs d'options par commande
│   │   └── validators.hpp          # Validateurs CLI personnalisés
│   ├── commands/
│   │   ├── command.hpp             # Interface de base des commandes
│   │   ├── init.hpp / init.cpp     # Commande "init"
│   │   ├── build.hpp / build.cpp   # Commande "build"
│   │   ├── deploy.hpp / deploy.cpp # Commande "deploy"
│   │   └── status.hpp / status.cpp # Commande "status"
│   ├── core/
│   │   ├── config.hpp / config.cpp # Lecture de configuration
│   │   ├── project.hpp / project.cpp
│   │   └── ...                     # Logique métier
│   └── output/
│       ├── terminal.hpp            # Détection TTY (section 36.4)
│       ├── styles.hpp              # Palette sémantique (section 36.3.2)
│       ├── output.hpp / output.cpp # Module de sortie centralisé
│       └── formatters.hpp          # fmt::formatter personnalisés
├── tests/
│   ├── CMakeLists.txt
│   ├── test_init_command.cpp
│   ├── test_build_command.cpp
│   └── test_config.cpp
└── docs/
    └── ...
```

Quelques points notables :

- **`src/cli/`** contient tout ce qui est spécifique au parsing CLI — séparé de la logique métier. Si demain vous exposez la même fonctionnalité via une API REST ou une interface graphique, ce répertoire est le seul à changer.  
- **`src/commands/`** contient une classe par sous-commande, chacune dans son propre fichier. Ajouter une commande = ajouter un fichier + l'enregistrer dans le dispatch.  
- **`src/core/`** contient la logique métier pure, sans dépendance à CLI11, {fmt} ou quoi que ce soit lié à l'interface.  
- **`src/output/`** centralise le formatage et la conscience du terminal.

---

## Couche 1 : le contexte applicatif

Le contexte applicatif est un objet qui encapsule tout l'état partagé entre les commandes : configuration, contexte terminal, options globales. Il est créé une fois dans `main()` et passé à chaque commande.

### `src/app.hpp`

```cpp
#pragma once
#include "output/terminal.hpp"
#include "output/output.hpp"
#include "core/config.hpp"
#include <string>
#include <memory>

namespace devtool {

struct GlobalOptions {
    int verbosity = 0;         // 0 = normal, 1 = verbose, 2+ = debug
    bool color = true;         // Peut être overridé par --no-color
    std::string config_path;   // Chemin du fichier de configuration
    std::string format = "auto"; // text, json, auto
};

class App {  
public:  
    GlobalOptions global;
    cli::TerminalContext term;
    cli::Output out;
    Config config;

    App(GlobalOptions opts, cli::TerminalContext ctx)
        : global(std::move(opts))
        , term(ctx)
        , out(ctx.color_stdout, ctx.color_stderr) {}

    // Charger la configuration depuis le fichier
    bool load_config() {
        if (!global.config_path.empty()) {
            return config.load_from_file(global.config_path);
        }
        // Chercher un fichier par défaut
        return config.load_default();
    }

    // Le format effectif : résoudre "auto"
    std::string effective_format() const {
        if (global.format != "auto") return global.format;
        return term.stdout_tty ? "text" : "json";
    }

    // Raccourcis pour les niveaux de verbosité
    bool verbose() const { return global.verbosity >= 1; }
    bool debug() const { return global.verbosity >= 2; }
};

}  // namespace devtool
```

Ce design centralise les décisions globales dans un seul objet. Les commandes n'ont pas besoin de savoir comment la détection TTY fonctionne ou comment la configuration est chargée — elles reçoivent un `App&` prêt à l'emploi.

---

## Couche 2 : les structs d'options

Chaque sous-commande a son propre struct d'options. Ces structs sont de purs conteneurs de données, sans logique :

### `src/cli/options.hpp`

```cpp
#pragma once
#include <string>
#include <vector>

namespace devtool {

struct InitOptions {
    std::string name;
    std::string tmpl = "default";
    bool force = false;
};

struct BuildOptions {
    std::string target = "release";
    int jobs = 0;              // 0 = auto-detect
    bool clean = false;
    bool dry_run = false;
    std::vector<std::string> defines;
};

struct DeployOptions {
    std::string environment;
    std::string version;
    int replicas = 1;
    bool force = false;
    bool dry_run = false;
    int timeout_sec = 300;
};

struct StatusOptions {
    bool all = false;
    bool watch = false;
    int interval_sec = 5;
};

}  // namespace devtool
```

L'avantage de cette approche : les structs sont testables indépendamment, sérialisables, et ne dépendent d'aucune librairie CLI. On peut construire un `BuildOptions` dans un test unitaire sans passer par CLI11.

---

## Couche 3 : l'interface de commande

Chaque commande implémente une interface commune. Le pattern classique est une classe abstraite avec deux méthodes : l'une pour enregistrer les options CLI, l'autre pour exécuter la logique :

### `src/commands/command.hpp`

```cpp
#pragma once
#include "../app.hpp"
#include <CLI/CLI.hpp>

namespace devtool {

class Command {  
public:  
    virtual ~Command() = default;

    // Enregistrer les options sur la sous-commande CLI11
    virtual void setup(CLI::App& cli) = 0;

    // Exécuter la commande. Retourne le code de sortie.
    virtual int execute(App& app) = 0;

    // Nom de la commande (pour le dispatch)
    virtual std::string name() const = 0;
};

}  // namespace devtool
```

### Exemple : commande `build`

```cpp
// src/commands/build.hpp
#pragma once
#include "command.hpp"
#include "../cli/options.hpp"
#include "../cli/validators.hpp"

namespace devtool {

class BuildCommand : public Command {
    BuildOptions opts_;

public:
    std::string name() const override { return "build"; }

    void setup(CLI::App& cli) override {
        cli.add_option("--target,-t", opts_.target, "Cible de compilation")
            ->check(CLI::IsMember({"debug", "release", "profile"}))
            ->capture_default_str();

        cli.add_option("--jobs,-j", opts_.jobs, "Nombre de jobs (0 = auto)")
            ->check(CLI::Range(0, 128))
            ->capture_default_str();

        cli.add_flag("--clean,-c", opts_.clean, "Nettoyer avant de compiler");

        cli.add_flag("--dry-run", opts_.dry_run, "Afficher sans exécuter");

        cli.add_option("--define,-D", opts_.defines,
                        "Définitions CMake (répétable)")
            ->check(KeyValuePair('='));
    }

    int execute(App& app) override;
};

}  // namespace devtool
```

```cpp
// src/commands/build.cpp
#include "build.hpp"
#include "../core/project.hpp"
#include <thread>

namespace devtool {

int BuildCommand::execute(App& app) {
    // Résoudre les valeurs automatiques
    int jobs = opts_.jobs;
    if (jobs == 0) {
        jobs = static_cast<int>(std::thread::hardware_concurrency());
    }

    if (app.verbose()) {
        app.out.info("Cible: {}, Jobs: {}, Clean: {}",
                     opts_.target, jobs, opts_.clean);
        for (const auto& d : opts_.defines) {
            app.out.info("Define: {}", d);
        }
    }

    if (opts_.dry_run) {
        app.out.info("Mode dry-run — aucune action effectuée");
        return 0;
    }

    // Charger le projet
    Project project;
    if (!project.load(app.config)) {
        app.out.error("Impossible de charger le projet. "
                      "Exécutez 'devtool init' d'abord.");
        return 1;
    }

    if (opts_.clean) {
        app.out.info("Nettoyage du répertoire de build...");
        project.clean();
    }

    // Compilation
    app.out.success("Compilation lancée ({} jobs, cible: {})",
                    jobs, opts_.target);

    auto result = project.build(opts_.target, jobs, opts_.defines);

    if (result.success) {
        app.out.success("Build terminé en {:.1f}s ({} fichiers compilés)",
                        result.elapsed_seconds, result.files_compiled);
        return 0;
    } else {
        app.out.error("Build échoué : {}", result.error_message);
        return 1;
    }
}

}  // namespace devtool
```

Observez comment `execute()` ne contient aucune logique de parsing ni de formatage bas niveau. Il utilise `App` pour le diagnostic (`app.out.info`, `app.out.error`) et délègue le travail réel à `Project` (logique métier pure). La commande est un **orchestrateur mince** entre l'interface CLI et le cœur métier.

---

## Couche 4 : le registre de commandes et le dispatch

Le dispatch connecte les sous-commandes CLI11 aux objets `Command`. Un registre centralise cette association :

### `src/cli/cli.hpp`

```cpp
#pragma once
#include "../commands/command.hpp"
#include <CLI/CLI.hpp>
#include <memory>
#include <vector>
#include <functional>

namespace devtool {

class CommandRegistry {
    struct Entry {
        std::unique_ptr<Command> command;
        CLI::App* subcommand;  // Pointeur non-owning, géré par CLI11
    };

    std::vector<Entry> entries_;

public:
    // Enregistrer une commande
    template <typename T, typename... Args>
    void add(CLI::App& app, const std::string& description, Args&&... args) {
        auto cmd = std::make_unique<T>(std::forward<Args>(args)...);
        auto* sub = app.add_subcommand(cmd->name(), description);
        cmd->setup(*sub);
        entries_.push_back({std::move(cmd), sub});
    }

    // Trouver la commande active après parsing
    Command* active_command() const {
        for (const auto& entry : entries_) {
            if (entry.subcommand->parsed()) {
                return entry.command.get();
            }
        }
        return nullptr;
    }
};

}  // namespace devtool
```

### `src/cli/cli.cpp`

```cpp
#include "cli.hpp"
#include "../commands/init.hpp"
#include "../commands/build.hpp"
#include "../commands/deploy.hpp"
#include "../commands/status.hpp"

namespace devtool {

void register_commands(CommandRegistry& registry, CLI::App& app) {
    registry.add<InitCommand>(app, "Initialiser un nouveau projet");
    registry.add<BuildCommand>(app, "Compiler le projet");
    registry.add<DeployCommand>(app, "Déployer l'application");
    registry.add<StatusCommand>(app, "Afficher l'état du projet");
}

}  // namespace devtool
```

Ajouter une nouvelle commande se résume à trois étapes :

1. Créer `src/commands/nouvelle.hpp` et `src/commands/nouvelle.cpp` implémentant `Command`.
2. Ajouter une ligne dans `register_commands()`.
3. Ajouter le `.cpp` dans `CMakeLists.txt`.

Aucune autre modification n'est nécessaire. Les commandes existantes ne sont pas touchées.

---

## Couche 5 : le point d'entrée

Le `main()` est un assembleur de couches. Il crée l'application CLI11, enregistre les options globales, détecte le contexte terminal, dispatch la commande active :

### `src/main.cpp`

```cpp
#include "app.hpp"
#include "cli/cli.hpp"
#include "cli/options.hpp"
#include "output/terminal.hpp"
#include <CLI/CLI.hpp>
#include <print>

int main(int argc, char* argv[]) {
    // === 1. Configuration CLI11 ===
    CLI::App cli_app{"devtool — Outil de développement C++"};
    cli_app.set_version_flag("--version,-V", "devtool 1.0.0");
    cli_app.require_subcommand(1);

    cli_app.footer(
        "Exemples:\n"
        "  devtool init myproject --template lib\n"
        "  devtool build --target release -j 8\n"
        "  devtool deploy --env production --replicas 3\n"
        "  devtool status --watch\n"
        "\n"
        "Documentation : https://example.com/devtool"
    );

    // === 2. Options globales ===
    devtool::GlobalOptions global;

    cli_app.add_flag("--verbose,-v", global.verbosity,
                     "Niveau de verbosité (répétable : -v, -vv)");
    cli_app.add_flag("--color,!--no-color", global.color,
                     "Activer/désactiver les couleurs");
    cli_app.add_option("--config,-c", global.config_path,
                       "Fichier de configuration")
        ->check(CLI::ExistingFile)
        ->envname("DEVTOOL_CONFIG");
    cli_app.add_option("--format,-f", global.format, "Format de sortie")
        ->check(CLI::IsMember({"auto", "text", "json"}))
        ->capture_default_str();

    // === 3. Enregistrement des commandes ===
    devtool::CommandRegistry registry;
    devtool::register_commands(registry, cli_app);

    // === 4. Parsing ===
    CLI11_PARSE(cli_app, argc, argv);

    // === 5. Détection du contexte terminal ===
    cli::ColorMode cmode = cli::ColorMode::automatic;
    if (cli_app.get_option("--color")->count() > 0) {
        cmode = global.color ? cli::ColorMode::always
                             : cli::ColorMode::never;
    }
    auto term_ctx = cli::detect(cmode);

    // === 6. Construction de l'application ===
    devtool::App app(std::move(global), term_ctx);

    if (!app.load_config()) {
        if (!app.global.config_path.empty()) {
            app.out.error("Impossible de charger la configuration : {}",
                          app.global.config_path);
            return 1;
        }
        // Pas de config trouvée, ce n'est pas forcément une erreur
        if (app.debug()) {
            app.out.info("Aucun fichier de configuration détecté");
        }
    }

    // === 7. Dispatch et exécution ===
    auto* cmd = registry.active_command();
    if (cmd == nullptr) {
        // Ne devrait pas arriver avec require_subcommand(1)
        std::println(stderr, "{}", cli_app.help());
        return 1;
    }

    return cmd->execute(app);
}
```

Le `main()` fait **58 lignes** et ne contient aucune logique métier. Chaque section est clairement identifiée et suit un flux linéaire : configuration → parsing → détection → construction → dispatch. C'est un assembleur, pas un exécuteur.

---

## Diagramme des couches

```
┌─────────────────────────────────────────────────────────────────┐
│  main.cpp                                                       │
│  Assemblage : CLI11 → parsing → contexte → dispatch             │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│  cli/ — Interface CLI                                           │
│  ┌──────────┐ ┌───────────┐ ┌────────────┐ ┌──────────────────┐ │
│  │ cli.hpp  │ │options.hpp│ │validators. │ │ CommandRegistry  │ │
│  │ CLI11    │ │ Structs   │ │   hpp      │ │ Dispatch         │ │
│  └──────────┘ └───────────┘ └────────────┘ └──────────────────┘ │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│  commands/ — Orchestrateurs                                     │
│  ┌──────┐ ┌───────┐ ┌────────┐ ┌────────┐                       │
│  │ init │ │ build │ │ deploy │ │ status │ ...                   │
│  └──┬───┘ └──┬────┘ └──┬─────┘ └──┬─────┘                       │
└─────┼────────┼─────────┼──────────┼─────────────────────────────┘
      │        │         │          │
┌─────▼────────▼─────────▼──────────▼─────────────────────────────┐
│  core/ — Logique métier pure                                    │
│  ┌─────────┐ ┌──────────┐ ┌────────────┐                        │
│  │ Project │ │ Config   │ │ Deployer   │ ...                    │
│  └─────────┘ └──────────┘ └────────────┘                        │
│  Aucune dépendance CLI, fmt, terminal                           │
└─────────────────────────────────────────────────────────────────┘
      │
┌─────▼───────────────────────────────────────────────────────────┐
│  output/ — Présentation                                         │
│  ┌────────────┐ ┌──────────┐ ┌────────────┐ ┌──────────────┐    │
│  │ terminal.  │ │ styles.  │ │  output.   │ │ formatters.  │    │
│  │    hpp     │ │   hpp    │ │ hpp / cpp  │ │    hpp       │    │
│  └────────────┘ └──────────┘ └────────────┘ └──────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

Les flèches descendent : chaque couche ne dépend que des couches inférieures. `core/` ne connaît pas `cli/`. `commands/` connaît `core/` et `output/`, mais pas les détails de CLI11. `main.cpp` connaît tout, mais ne fait que de l'assemblage.

---

## Gestion des codes de sortie

Un outil professionnel retourne des codes de sortie cohérents et documentés. Plutôt que d'utiliser des entiers magiques, définissez des constantes :

```cpp
// src/cli/exit_codes.hpp
#pragma once

namespace devtool::exit_code {

constexpr int success           = 0;  
constexpr int general_error     = 1;  
constexpr int usage_error       = 2;   // Mauvaise utilisation CLI  
constexpr int config_error      = 3;   // Fichier de configuration invalide  
constexpr int project_not_found = 4;   // Pas de projet dans le répertoire courant  
constexpr int build_failed      = 10;  // Échec de compilation  
constexpr int deploy_failed     = 20;  // Échec de déploiement  
constexpr int timeout           = 30;  // Opération en timeout  
constexpr int interrupted       = 130; // Convention Unix : 128 + SIGINT(2)  

}  // namespace devtool::exit_code
```

```cpp
// Dans une commande
int BuildCommand::execute(App& app) {
    // ...
    if (result.success) return exit_code::success;
    return exit_code::build_failed;
}
```

La convention Unix veut que 0 soit le succès, 1 une erreur générale, 2 une erreur d'usage, et 128+N une terminaison par signal N. Au-delà, les codes sont spécifiques à l'outil. Documentez-les dans le `--help` ou la page de documentation.

---

## Gestion des signaux

Un outil CLI qui exécute des opérations longues (build, déploiement, surveillance) doit gérer proprement l'interruption par `Ctrl+C` (`SIGINT`) et la demande de terminaison (`SIGTERM`) :

```cpp
// src/cli/signals.hpp
#pragma once
#include <atomic>
#include <csignal>

namespace devtool {

// Flag global consulté par les opérations longues
inline std::atomic<bool> shutdown_requested{false};

inline void install_signal_handlers() {
    auto handler = [](int) {
        shutdown_requested.store(true, std::memory_order_relaxed);
    };

    struct sigaction sa{};
    sa.sa_handler = handler;
    sa.sa_flags = 0;  // Pas de SA_RESTART : interrompre les appels bloquants
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

}  // namespace devtool
```

```cpp
// Dans main.cpp, au démarrage
devtool::install_signal_handlers();

// Dans une opération longue
while (!devtool::shutdown_requested.load() && has_work()) {
    process_next_item();
}

if (devtool::shutdown_requested.load()) {
    app.out.warn("Interruption demandée, nettoyage en cours...");
    cleanup();
    return exit_code::interrupted;
}
```

Ce pattern (flag atomique + vérification périodique) est la manière canonique de gérer l'interruption dans un programme C++ (voir **section 20.2** pour une couverture approfondie des signaux POSIX). L'opération longue vérifie le flag régulièrement et sort proprement quand il est levé.

---

## Format de sortie multi-mode

Un outil professionnel supporte plusieurs formats de sortie pour s'adapter au contexte — humain interactif, script, pipeline de monitoring :

```cpp
// src/output/output.hpp (extrait)

class Output {
    // ... (membres existants de la section 36.3) ...

public:
    // Sortie de données structurées (stdout)
    void print_data(const std::string& format,
                    const auto& data,
                    const auto& text_renderer,
                    const auto& json_renderer) {
        if (format == "json") {
            fmt::print("{}\n", json_renderer(data));
        } else {
            text_renderer(data);
        }
    }
};
```

```cpp
// Dans une commande
int StatusCommand::execute(App& app) {
    auto status = project.get_status();
    auto fmt = app.effective_format();  // "text" ou "json"

    app.out.print_data(fmt, status,
        // Rendu texte (humain)
        [&](const ProjectStatus& s) {
            fmt::print(maybe(style::heading, app.term.color_stdout),
                       "Projet : {}\n", s.name);
            fmt::print("Version : {}\n", s.version);
            // ... tableau détaillé ...
        },
        // Rendu JSON (machine)
        [&](const ProjectStatus& s) -> std::string {
            return s.to_json();
        }
    );

    return exit_code::success;
}
```

L'option `--format auto` (le défaut) résout automatiquement : texte en terminal, JSON en pipe. L'utilisateur peut forcer un format avec `--format json` ou `--format text`.

---

## Testabilité

L'architecture en couches rend chaque niveau testable indépendamment :

### Tests de la logique métier (core/)

```cpp
// tests/test_project.cpp
#include "core/project.hpp"
#include <gtest/gtest.h>

TEST(Project, BuildRelease) {
    Project project;
    project.load_from_directory("test_fixtures/simple_project");

    auto result = project.build("release", 4, {});

    EXPECT_TRUE(result.success);
    EXPECT_GT(result.files_compiled, 0);
}
```

Aucune dépendance à CLI11 ou {fmt} — la logique métier est testée en isolation.

### Tests des commandes

```cpp
// tests/test_build_command.cpp
#include "commands/build.hpp"
#include "app.hpp"
#include <gtest/gtest.h>

TEST(BuildCommand, DryRunDoesNotBuild) {
    // Construire un App de test avec un contexte minimal
    devtool::GlobalOptions global;
    global.verbosity = 0;
    cli::TerminalContext ctx;  // Valeurs par défaut, pas de TTY
    devtool::App app(global, ctx);

    // Simuler les options de la commande
    // (ici on teste execute() directement, sans CLI11)
    BuildCommand cmd;
    cmd.set_options(BuildOptions{
        .target = "debug",
        .jobs = 1,
        .dry_run = true
    });

    int code = cmd.execute(app);
    EXPECT_EQ(code, exit_code::success);
}
```

La séparation entre le struct d'options et le parsing CLI11 permet de tester la logique d'exécution sans simuler `argc`/`argv`. On construit directement les options et on appelle `execute()`.

### Tests d'intégration CLI

Pour tester le parsing complet de bout en bout :

```cpp
// tests/test_cli_integration.cpp
#include <gtest/gtest.h>
#include <cstdlib>
#include <array>
#include <memory>

std::string run_command(const std::string& cmd) {
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen(cmd.c_str(), "r"), pclose);
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

TEST(CLI, HelpContainsSubcommands) {
    auto output = run_command("./devtool --help 2>&1");
    EXPECT_NE(output.find("build"), std::string::npos);
    EXPECT_NE(output.find("deploy"), std::string::npos);
    EXPECT_NE(output.find("status"), std::string::npos);
}

TEST(CLI, InvalidSubcommandReturnsError) {
    int code = std::system("./devtool invalid_cmd > /dev/null 2>&1");
    EXPECT_NE(WEXITSTATUS(code), 0);
}

TEST(CLI, VersionFlag) {
    auto output = run_command("./devtool --version 2>&1");
    EXPECT_NE(output.find("devtool"), std::string::npos);
}
```

---

## CMakeLists.txt du projet

```cmake
cmake_minimum_required(VERSION 3.20)  
project(devtool VERSION 1.0.0 LANGUAGES CXX)  

set(CMAKE_CXX_STANDARD 23)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  

# --- Dépendances ---
include(cmake/FetchDependencies.cmake)

# --- Exécutable ---
add_executable(devtool
    src/main.cpp
    src/app.cpp
    src/cli/cli.cpp
    src/commands/init.cpp
    src/commands/build.cpp
    src/commands/deploy.cpp
    src/commands/status.cpp
    src/core/config.cpp
    src/core/project.cpp
    src/output/output.cpp
)

target_include_directories(devtool PRIVATE src)  
target_link_libraries(devtool PRIVATE CLI11::CLI11 fmt::fmt)  

target_compile_options(devtool PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)

# Version injectée dans le code
configure_file(src/version.h.in src/version.h)  
target_include_directories(devtool PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src)  

# --- Tests ---
option(BUILD_TESTS "Build tests" ON)  
if(BUILD_TESTS)  
    enable_testing()
    add_subdirectory(tests)
endif()
```

```cmake
# cmake/FetchDependencies.cmake
include(FetchContent)

FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.6.0)

FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.1.4)

FetchContent_MakeAvailable(cli11 fmt)
```

---

## Checklist de qualité d'un outil CLI

Pour conclure ce chapitre, voici une checklist synthétique. Un outil CLI professionnel coche toutes ces cases :

**Interface**  
- `--help` génère une aide complète avec exemples (footer).  
- `--version` affiche la version.  
- `--verbose` / `-v` est empilable pour augmenter le diagnostic.  
- `--color` / `--no-color` contrôle la colorisation.  
- `--format` permet de choisir entre texte humain et JSON machine.  
- Les messages d'erreur sont explicites et suggèrent une correction.

**Comportement**  
- `stdout` contient les données, `stderr` contient le diagnostic.  
- Les couleurs sont désactivées automatiquement en pipe/fichier.  
- La convention `NO_COLOR` est respectée.  
- `Ctrl+C` interrompt proprement avec nettoyage.  
- Les codes de sortie sont cohérents et documentés.  
- Le mode `--format auto` produit du texte en terminal, du JSON en pipe.

**Architecture**  
- Le parsing CLI est séparé de la logique métier.  
- Chaque commande est dans son propre fichier.  
- Ajouter une commande ne modifie pas les commandes existantes.  
- La logique métier est testable sans simuler la CLI.  
- La configuration provient de CLI > variables d'environnement > fichier de config > défauts.

**Distribution**  
- Le binaire est compilé statiquement (ou avec des dépendances minimales).  
- Un `--help` complet tient lieu de documentation de premier niveau.  
- Les scripts de complétion shell sont disponibles (`tool completion bash`).

---

> 🔥 **Ce chapitre vous a donné les outils pour construire des CLIs de qualité industrielle.** En combinant CLI11 pour le parsing, {fmt} pour le formatage, une conscience du terminal complète, et une architecture en couches, vous êtes équipé pour livrer des outils que vos utilisateurs adopteront naturellement — parce qu'ils fonctionnent comme les outils qu'ils connaissent déjà.

⏭️ [Module 13 : C++ dans une Approche DevOps](/module-13-devops.md)
