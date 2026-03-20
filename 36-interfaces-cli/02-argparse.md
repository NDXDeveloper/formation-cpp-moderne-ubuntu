🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 36.2 — argparse : Alternative légère

## Module 12 : Création d'Outils CLI — Niveau Avancé

---

## Introduction

La section précédente a couvert CLI11, la librairie de référence pour les outils CLI complexes. Mais tous les programmes n'ont pas besoin de sous-commandes imbriquées, de groupes d'options mutuellement exclusives ou de fichiers de configuration. Pour un utilitaire avec quelques options bien définies, CLI11 peut sembler surdimensionné.

**argparse** est une librairie header-only de parsing d'arguments en C++17, directement inspirée du module `argparse` de Python. Son API est volontairement minimaliste et familière : si vous avez déjà utilisé `argparse` en Python, vous serez productif en C++ en quelques minutes. Elle offre un excellent compromis entre le parsing manuel (trop fragile) et CLI11 (plus riche que nécessaire pour un petit outil).

---

## Quand choisir argparse plutôt que CLI11

Le choix entre les deux librairies se résume à une question de complexité de l'interface :

| Critère | argparse | CLI11 |
|---------|----------|-------|
| Nombre d'options | Quelques-unes (< 10) | Nombre quelconque |
| Sous-commandes | Support basique | Imbrication multi-niveaux |
| Validation | Basique (type + choix) | Validateurs composables, personnalisés |
| Fichiers de config | Non | Oui (INI, TOML) |
| Variables d'environnement | Non natif | Oui (`envname`) |
| Exclusions mutuelles | Groupes mutuellement exclusifs | `excludes()`, `needs()`, groupes |
| API | Style Python | Style fluent C++ |
| Familiarité Python | Fort atout | Neutre |

En résumé : **argparse** pour les utilitaires simples et les développeurs venant de Python, **CLI11** pour les outils professionnels complexes. Les deux sont header-only et s'intègrent de la même façon dans un projet CMake.

---

## Installation

### CMake FetchContent (recommandé)

```cmake
include(FetchContent)  
FetchContent_Declare(  
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG        v3.2       # Vérifier la dernière version stable
)
FetchContent_MakeAvailable(argparse)

add_executable(mon-outil src/main.cpp)  
target_link_libraries(mon-outil PRIVATE argparse::argparse)  
```

### Paquet système (Ubuntu)

```bash
sudo apt install libargparse-dev
```

### Header unique

argparse se distribue également en un seul fichier header, téléchargeable depuis les releases GitHub. L'intégration est identique à celle décrite pour CLI11 en **section 36.1.1** (méthode 5).

---

## Premier programme

Commençons par un utilitaire classique — un outil qui compte les lignes, mots ou caractères d'un fichier (un `wc` simplifié) :

```cpp
#include <argparse/argparse.hpp>
#include <print>
#include <fstream>
#include <string>

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("wcount", "1.0.0");
    program.add_description("Compter les lignes, mots ou caractères d'un fichier");

    // Argument positionnel obligatoire
    program.add_argument("file")
        .help("Fichier à analyser");

    // Flags mutuellement exclusifs (par convention, pas par contrainte)
    program.add_argument("-l", "--lines")
        .help("Compter les lignes")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-w", "--words")
        .help("Compter les mots")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-c", "--chars")
        .help("Compter les caractères")
        .default_value(false)
        .implicit_value(true);

    // Parsing
    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::println(stderr, "Erreur : {}", e.what());
        std::println(stderr, "{}", program.help().str());
        return 1;
    }

    // Récupération des valeurs
    auto filepath = program.get<std::string>("file");
    bool count_lines = program.get<bool>("--lines");
    bool count_words = program.get<bool>("--words");
    bool count_chars = program.get<bool>("--chars");

    // Si aucun flag, compter tout
    if (!count_lines && !count_words && !count_chars) {
        count_lines = count_words = count_chars = true;
    }

    // Logique métier
    std::ifstream file(filepath);
    if (!file) {
        std::println(stderr, "Erreur : impossible d'ouvrir '{}'", filepath);
        return 1;
    }

    int lines = 0, words = 0, chars = 0;
    std::string line;
    while (std::getline(file, line)) {
        ++lines;
        chars += static_cast<int>(line.size()) + 1;  // +1 pour le '\n'
        bool in_word = false;
        for (char c : line) {
            if (std::isspace(c)) { in_word = false; }
            else if (!in_word) { ++words; in_word = true; }
        }
    }

    if (count_lines) std::print("{:>8}", lines);
    if (count_words) std::print("{:>8}", words);
    if (count_chars) std::print("{:>8}", chars);
    std::println("  {}", filepath);

    return 0;
}
```

```bash
$ wcount README.md
      47     312    2048  README.md

$ wcount --lines README.md
      47  README.md

$ wcount -wc README.md
     312    2048  README.md

$ wcount --help
Usage: wcount [-h] [-v] [-l] [-w] [-c] file

Compter les lignes, mots ou caractères d'un fichier

Positional arguments:
  file          Fichier à analyser

Optional arguments:
  -h, --help    shows help message and exits
  -v, --version prints version information and exits
  -l, --lines   Compter les lignes
  -w, --words   Compter les mots
  -c, --chars   Compter les caractères
```

Quelques observations immédiates sur la différence avec CLI11 :

- L'aide et le flag `--version` sont ajoutés automatiquement (comme CLI11).  
- Le parsing lève une `std::exception` standard plutôt qu'une `CLI::ParseError` spécialisée.  
- La récupération des valeurs passe par `program.get<T>()` plutôt que par binding direct à des variables.

---

## API en détail

### Arguments positionnels

Les arguments sans tiret sont des positionnels, traités dans l'ordre de déclaration :

```cpp
program.add_argument("source")
    .help("Fichier source");

program.add_argument("destination")
    .help("Fichier destination");
```

```bash
$ mon-outil input.txt output.txt
# source = "input.txt", destination = "output.txt"
```

### Arguments positionnels variadiques

Pour accepter un nombre variable d'arguments positionnels, on utilise `nargs` ou `remaining` :

```cpp
// Nombre fixe
program.add_argument("coords")
    .help("Coordonnées x y")
    .nargs(2)
    .scan<'g', double>();

// Nombre variable (1 ou plus)
program.add_argument("files")
    .help("Fichiers à traiter")
    .nargs(argparse::nargs_pattern::at_least_one);

// Tout le reste (après les options)
program.add_argument("rest")
    .help("Arguments restants")
    .remaining();
```

```bash
$ mon-outil --verbose a.txt b.txt c.txt
# files = {"a.txt", "b.txt", "c.txt"}
```

La récupération d'un argument multi-valeur utilise `get<std::vector<T>>` :

```cpp
auto files = program.get<std::vector<std::string>>("files");  
auto coords = program.get<std::vector<double>>("coords");  
```

### Options avec valeur

```cpp
// Option string
program.add_argument("--output", "-o")
    .help("Fichier de sortie")
    .default_value(std::string("result.txt"));

// Option entière avec conversion explicite
program.add_argument("--port", "-p")
    .help("Port d'écoute")
    .default_value(8080)
    .scan<'i', int>();

// Option flottante
program.add_argument("--threshold")
    .help("Seuil de détection")
    .default_value(0.95)
    .scan<'g', double>();
```

La méthode `scan<>()` est le mécanisme de conversion de type d'argparse. Le premier paramètre template est le spécificateur de format (inspiré de `std::from_chars`) et le second est le type cible :

| Spécificateur | Types | Exemple |
|:---:|---------|---------|
| `'i'` | Entiers (décimal) | `scan<'i', int>()` |
| `'d'` | Entiers (décimal) | `scan<'d', int>()` |
| `'o'` | Entiers (octal) | `scan<'o', int>()` |
| `'x'` | Entiers (hexadécimal) | `scan<'x', int>()` |
| `'g'` | Flottants | `scan<'g', double>()` |
| `'f'` | Flottants (fixe) | `scan<'f', float>()` |

### Flags booléens

argparse traite les flags différemment de CLI11. Il n'y a pas de méthode `add_flag` dédiée — on utilise `add_argument` avec `default_value(false)` et `implicit_value(true)` :

```cpp
program.add_argument("--verbose", "-v")
    .help("Mode verbeux")
    .default_value(false)
    .implicit_value(true);
```

`implicit_value(true)` signifie : "si le flag est présent sans valeur explicite, la valeur est `true`". C'est plus verbeux que le `add_flag` de CLI11, mais le pattern est toujours le même.

### Flag compteur

Pour un flag empilable (`-vvv`), on utilise `append()` et on compte ensuite :

```cpp
program.add_argument("-v", "--verbose")
    .help("Niveau de verbosité (répétable)")
    .action([](const auto&) { return std::string("1"); })
    .append()
    .default_value(std::vector<std::string>{});
```

C'est un cas où CLI11 est nettement plus élégant : le flag compteur y est natif avec un simple `int`.

### Restriction de choix

argparse ne dispose pas de validateurs composables comme CLI11, mais il offre une restriction aux valeurs autorisées via un initializer list :

```cpp
program.add_argument("--format", "-f")
    .help("Format de sortie")
    .default_value(std::string("text"))
    .choices("text", "json", "csv", "table");
```

```bash
$ mon-outil --format xml
Invalid argument "xml" - allowed options: {text, json, csv, table}
```

### Valeur requise

```cpp
program.add_argument("--config")
    .help("Fichier de configuration")
    .required();
```

```bash
$ mon-outil
config is a required argument
```

---

## Récupération des valeurs : `get<T>()` vs binding direct

C'est la différence architecturale principale entre argparse et CLI11. Avec CLI11, les options sont liées directement à des variables — après le parsing, les variables contiennent les bonnes valeurs. Avec argparse, on interroge l'objet parser après coup :

```cpp
// CLI11 — binding direct
int port = 8080;  
app.add_option("--port,-p", port, "Port");  
CLI11_PARSE(app, argc, argv);  
// port contient la valeur ici

// argparse — get<T>() après parsing
program.add_argument("--port", "-p")
    .default_value(8080)
    .scan<'i', int>();
program.parse_args(argc, argv);  
int port = program.get<int>("--port");  // Extraction explicite  
```

L'approche de CLI11 est plus concise et type-safe à la compilation. L'approche d'argparse est plus proche du Python et offre une flexibilité différente — on peut par exemple tester la présence d'un argument avant de le récupérer :

```cpp
if (program.is_used("--output")) {
    auto output = program.get<std::string>("--output");
    // L'utilisateur a explicitement passé --output
} else {
    // L'utilisateur n'a pas passé --output (valeur par défaut active)
}
```

`is_used()` distingue "l'utilisateur a passé la valeur par défaut explicitement" de "l'utilisateur n'a rien passé" — une nuance utile dans certains scénarios. C'est l'équivalent du pattern `std::optional` avec CLI11.

---

## Groupes mutuellement exclusifs

argparse supporte les groupes d'arguments mutuellement exclusifs, ce qui est un avantage par rapport à un parsing purement manuel :

```cpp
auto& group = program.add_mutually_exclusive_group();

group.add_argument("--json")
    .help("Sortie JSON")
    .default_value(false)
    .implicit_value(true);

group.add_argument("--csv")
    .help("Sortie CSV")
    .default_value(false)
    .implicit_value(true);

group.add_argument("--table")
    .help("Sortie tableau")
    .default_value(false)
    .implicit_value(true);
```

```bash
$ mon-outil --json --csv
Argument '--csv' not allowed with '--json'
```

Le groupe peut être rendu obligatoire (exactement une option du groupe doit être spécifiée) :

```cpp
auto& group = program.add_mutually_exclusive_group(true);  // required = true
```

---

## Sous-commandes

argparse supporte les sous-commandes, mais avec une API plus simple que CLI11 — pas d'imbrication multi-niveaux ni de callbacks intégrés :

```cpp
argparse::ArgumentParser program("mytool", "1.0.0");

argparse::ArgumentParser cmd_build("build");  
cmd_build.add_description("Compiler le projet");  
cmd_build.add_argument("--target", "-t")  
    .help("Cible de compilation")
    .default_value(std::string("release"))
    .choices("debug", "release");

argparse::ArgumentParser cmd_clean("clean");  
cmd_clean.add_description("Nettoyer les artefacts");  
cmd_clean.add_argument("--all", "-a")  
    .help("Tout supprimer")
    .default_value(false)
    .implicit_value(true);

program.add_subparser(cmd_build);  
program.add_subparser(cmd_clean);  

try {
    program.parse_args(argc, argv);
} catch (const std::exception& e) {
    std::println(stderr, "Erreur : {}", e.what());
    return 1;
}

if (program.is_subcommand_used("build")) {
    auto target = cmd_build.get<std::string>("--target");
    std::println("Compilation en mode {}", target);
} else if (program.is_subcommand_used("clean")) {
    bool all = cmd_clean.get<bool>("--all");
    std::println("Nettoyage{}", all ? " complet" : "");
}
```

```bash
$ mytool build --target debug
Compilation en mode debug

$ mytool clean --all
Nettoyage complet

$ mytool --help
Usage: mytool [-h] [-v] {build,clean}

Optional arguments:
  -h, --help    shows help message and exits
  -v, --version prints version information and exits

Subcommands:
  build         Compiler le projet
  clean         Nettoyer les artefacts
```

La différence principale avec CLI11 : les sous-commandes sont des objets `ArgumentParser` séparés ajoutés via `add_subparser()`, et la détection de la sous-commande active passe par `is_subcommand_used()`. Il n'y a pas de mécanisme de callback — le dispatch est toujours explicite.

---

## Exemple comparatif : le même outil avec argparse et CLI11

Pour ancrer la comparaison, voici le même utilitaire — un outil de conversion de formats — implémenté avec les deux librairies.

### Version argparse

```cpp
#include <argparse/argparse.hpp>
#include <print>

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("convert", "1.0.0");
    program.add_description("Convertir un fichier entre formats");

    program.add_argument("input")
        .help("Fichier d'entrée");

    program.add_argument("--output", "-o")
        .help("Fichier de sortie")
        .required();

    program.add_argument("--from", "-f")
        .help("Format source")
        .default_value(std::string("auto"))
        .choices("auto", "json", "yaml", "toml", "csv");

    program.add_argument("--to", "-t")
        .help("Format cible")
        .required()
        .choices("json", "yaml", "toml", "csv");

    program.add_argument("--pretty")
        .help("Indenter la sortie")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("--indent")
        .help("Nombre d'espaces d'indentation")
        .default_value(2)
        .scan<'i', int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::println(stderr, "Erreur : {}", e.what());
        std::println(stderr, "\n{}", program.help().str());
        return 1;
    }

    auto input  = program.get<std::string>("input");
    auto output = program.get<std::string>("--output");
    auto from   = program.get<std::string>("--from");
    auto to     = program.get<std::string>("--to");
    auto pretty = program.get<bool>("--pretty");
    auto indent = program.get<int>("--indent");

    std::println("Conversion : {} ({}) → {} ({}){}",
                 input, from, output, to,
                 pretty ? std::format(" [indent={}]", indent) : "");
    return 0;
}
```

### Version CLI11

```cpp
#include <CLI/CLI.hpp>
#include <print>

int main(int argc, char* argv[]) {
    CLI::App app{"convert — Convertir un fichier entre formats"};
    app.set_version_flag("--version,-V", "convert 1.0.0");

    std::string input, output;
    std::string from = "auto", to;
    bool pretty = false;
    int indent = 2;

    app.add_option("input", input, "Fichier d'entrée")->required();
    app.add_option("--output,-o", output, "Fichier de sortie")->required();
    app.add_option("--from,-f", from, "Format source")
        ->check(CLI::IsMember({"auto", "json", "yaml", "toml", "csv"}))
        ->capture_default_str();
    app.add_option("--to,-t", to, "Format cible")
        ->required()
        ->check(CLI::IsMember({"json", "yaml", "toml", "csv"}));
    app.add_flag("--pretty", pretty, "Indenter la sortie");
    app.add_option("--indent", indent, "Nombre d'espaces d'indentation")
        ->check(CLI::Range(1, 8))
        ->capture_default_str();

    CLI11_PARSE(app, argc, argv);

    std::println("Conversion : {} ({}) → {} ({}){}",
                 input, from, output, to,
                 pretty ? std::format(" [indent={}]", indent) : "");
    return 0;
}
```

Les deux versions font exactement la même chose. Les différences concrètes :

- **Déclaration** : CLI11 utilise le binding direct (variables déclarées avant, liées aux options), argparse utilise `get<T>()` après parsing. CLI11 est plus concis sur la déclaration, argparse impose un bloc d'extraction.  
- **Flags** : CLI11 a `add_flag`, argparse nécessite `default_value(false)` + `implicit_value(true)`.  
- **Validation** : CLI11 permet de chaîner `Range` et `IsMember` sur la même option ; argparse offre `choices` mais pas de validateurs composables.  
- **Erreurs** : CLI11 utilise `CLI11_PARSE` (ou `CLI::ParseError`), argparse lève une `std::exception` standard.  
- **Volume** : pour cet exemple simple, les deux versions font sensiblement la même longueur. L'écart se creuse quand la complexité augmente — c'est là que CLI11 brille.

---

## Limites d'argparse

Pour être complet, voici les limitations principales d'argparse par rapport à CLI11 :

**Pas de variables d'environnement en fallback.** Si votre outil doit respecter la convention Twelve-Factor Apps (configuration par variables d'environnement), vous devrez implémenter le fallback manuellement ou passer à CLI11.

**Pas de fichiers de configuration.** argparse parse uniquement la ligne de commande. Pour lire un fichier de config en plus, il faut une librairie séparée et la fusion manuelle des valeurs.

**Validation limitée.** `choices` couvre la restriction à un ensemble, mais il n'y a pas d'équivalent à `CLI::Range`, `CLI::ExistingFile`, `CLI::IsMember` ou aux validateurs composables. La validation de chemins, d'intervalles numériques ou de formats personnalisés doit être écrite à la main après le parsing.

**Sous-commandes non imbriquées.** Le support des sous-commandes existe mais reste à un seul niveau de profondeur. Pour une hiérarchie `tool remote add`, CLI11 est nécessaire.

**Pas de contraintes inter-options avancées.** Les groupes mutuellement exclusifs couvrent le cas de base, mais `needs()` (une option qui en requiert une autre) n'a pas d'équivalent natif.

Ces limitations ne sont pas des défauts — elles sont le reflet d'un choix de conception assumé : rester simple. Pour les outils qui n'ont pas besoin de ces fonctionnalités, argparse est parfaitement adapté et son API Python-like est un avantage concret pour les équipes polyglotes.

---

## Recommandation de choix

Pour conclure le positionnement entre les deux librairies :

**Choisissez argparse si** votre outil a une interface plate (pas de sous-commandes, ou une seule couche), moins d'une dizaine d'options, et que votre équipe est familière avec Python. C'est le bon choix pour les scripts de build, les utilitaires de conversion, les outils de test ponctuels.

**Choisissez CLI11 si** votre outil a des sous-commandes, des validations non triviales, doit lire des fichiers de configuration ou des variables d'environnement, ou est destiné à une distribution large. C'est le bon choix pour les outils d'infrastructure, les CLIs de produit, les agents système.

**Dans le doute, commencez par CLI11.** La migration d'argparse vers CLI11 quand les besoins grandissent nécessite une réécriture du parsing. Commencer par CLI11 ne coûte presque rien de plus et ne limite jamais.

⏭️ [fmt : Formatage avancé (pré-C++23)](/36-interfaces-cli/03-fmt.md)
