🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 36.4 — Gestion des couleurs et du TTY

## Module 12 : Création d'Outils CLI — Niveau Avancé

---

## Introduction

La section précédente (**36.3.2**) a couvert l'API de couleurs de {fmt} et introduit la détection TTY comme mécanisme de désactivation conditionnelle. Cette section élargit le sujet : au-delà des couleurs, un outil CLI professionnel doit adapter l'ensemble de son comportement au contexte d'exécution. Un terminal interactif, un pipe, un fichier de redirection, un terminal distant via SSH, un environnement CI — chacun de ces contextes appelle des décisions différentes en matière de couleurs, de largeur de sortie, de pagination, de formatage et d'interactivité.

C'est ce qu'on appelle la **conscience du terminal** (*terminal awareness*) : la capacité d'un programme à détecter son environnement d'exécution et à s'adapter automatiquement, sans que l'utilisateur ait à passer des flags explicites.

---

## Détection TTY : les fondamentaux

### `isatty()` et les descripteurs de fichiers

Le mécanisme de base est la fonction POSIX `isatty()`, qui teste si un descripteur de fichier est connecté à un terminal :

```cpp
#include <unistd.h>

bool stdin_is_tty  = isatty(STDIN_FILENO);   // fd 0  
bool stdout_is_tty = isatty(STDOUT_FILENO);  // fd 1  
bool stderr_is_tty = isatty(STDERR_FILENO);  // fd 2  
```

Chaque descripteur peut être dans un état différent. C'est crucial pour comprendre le comportement attendu :

```bash
# Cas 1 : tout est un terminal
$ mon-outil
# stdin=TTY, stdout=TTY, stderr=TTY

# Cas 2 : stdout redirigé vers un fichier
$ mon-outil > output.txt
# stdin=TTY, stdout=FICHIER, stderr=TTY

# Cas 3 : stdout vers un pipe
$ mon-outil | grep pattern
# stdin=TTY, stdout=PIPE, stderr=TTY

# Cas 4 : stdin depuis un fichier
$ mon-outil < input.txt
# stdin=FICHIER, stdout=TTY, stderr=TTY

# Cas 5 : tout redirigé (typique en CI)
$ mon-outil < input.txt > output.txt 2>&1
# stdin=FICHIER, stdout=FICHIER, stderr=FICHIER

# Cas 6 : pipe complet
$ cat data.csv | mon-outil | sort > result.txt
# stdin=PIPE, stdout=PIPE, stderr=TTY
```

### Décisions par descripteur

Chaque descripteur déclenche des décisions indépendantes :

| Descripteur | Si TTY | Si non-TTY |
|-------------|--------|------------|
| **stdin** | Mode interactif (prompts, confirmation) | Mode batch (lire les données sans prompt) |
| **stdout** | Couleurs, tableaux larges, pagination | Texte brut, format machine, pas de couleurs |
| **stderr** | Couleurs pour le diagnostic, barres de progression | Texte brut pour le diagnostic |

C'est pourquoi tester `isatty` séparément sur chaque descripteur est important. Un outil qui redirige sa sortie vers un fichier (`stdout` non-TTY) doit quand même afficher des barres de progression sur `stderr` (qui reste un TTY).

---

## Dimensions du terminal

### Obtenir la taille avec `ioctl`

Quand `stdout` est un terminal, il a des dimensions — un nombre de colonnes et de lignes. Ces dimensions sont nécessaires pour adapter la largeur des tableaux, tronquer les lignes longues et formater la sortie de manière responsive :

```cpp
#include <sys/ioctl.h>
#include <unistd.h>

struct TerminalSize {
    int cols = 80;   // Valeur par défaut raisonnable
    int rows = 24;
};

TerminalSize get_terminal_size() {
    TerminalSize size;
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (ws.ws_col > 0) size.cols = ws.ws_col;
        if (ws.ws_row > 0) size.rows = ws.ws_row;
    }

    return size;
}
```

```cpp
auto term = get_terminal_size();  
fmt::print(stderr, "Terminal : {}x{}\n", term.cols, term.rows);  
// Terminal : 120x40
```

### Fallback sur les variables d'environnement

Quand `ioctl` échoue (descripteur non-TTY, environnement exotique), les variables d'environnement `COLUMNS` et `LINES` servent de fallback :

```cpp
#include <cstdlib>

TerminalSize get_terminal_size() {
    TerminalSize size;
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0
        && ws.ws_col > 0 && ws.ws_row > 0) {
        size.cols = ws.ws_col;
        size.rows = ws.ws_row;
        return size;
    }

    // Fallback : variables d'environnement
    if (const char* cols = std::getenv("COLUMNS"))
        size.cols = std::atoi(cols);
    if (const char* rows = std::getenv("LINES"))
        size.rows = std::atoi(rows);

    return size;
}
```

### Adaptation de la sortie à la largeur

Une fois la largeur connue, l'outil peut adapter son formatage :

```cpp
void print_file_table(const std::vector<FileInfo>& files,
                      const TerminalSize& term) {
    // Colonnes fixes
    int status_col = 10;
    int size_col = 10;
    int date_col = 12;
    int fixed = status_col + size_col + date_col + 3;  // +3 pour les espaces

    // Le nom du fichier prend le reste
    int name_col = std::max(20, term.cols - fixed);

    fmt::print("{:<{}} {:>{}} {:>{}} {:>{}}\n",
               "FICHIER", name_col,
               "TAILLE", size_col,
               "DATE", date_col,
               "STATUS", status_col);

    for (const auto& f : files) {
        std::string name = f.name;
        // Tronquer le nom si trop long
        if (static_cast<int>(name.size()) > name_col) {
            name = name.substr(0, name_col - 3) + "...";
        }

        fmt::print("{:<{}} {:>{}} {:>{}} {:>{}}\n",
                   name, name_col,
                   f.size_str(), size_col,
                   f.date_str(), date_col,
                   f.status, status_col);
    }
}
```

Ce pattern de "colonnes élastiques" est celui utilisé par `ls`, `docker ps` et `kubectl get` : les colonnes à contenu variable (noms, descriptions) s'adaptent à la largeur disponible, tandis que les colonnes à contenu fixe (tailles, dates, statuts) gardent une largeur constante.

### Réagir au redimensionnement avec `SIGWINCH`

Quand l'utilisateur redimensionne son terminal, le noyau envoie le signal `SIGWINCH` au processus. Pour un outil qui affiche une interface persistante (barre de progression, tableau mis à jour en boucle), il peut être utile de capter ce signal :

```cpp
#include <csignal>
#include <atomic>

std::atomic<bool> terminal_resized{false};

void sigwinch_handler(int) {
    terminal_resized.store(true, std::memory_order_relaxed);
}

// Installation du handler (une seule fois, au démarrage)
void install_resize_handler() {
    struct sigaction sa{};
    sa.sa_handler = sigwinch_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGWINCH, &sa, nullptr);
}
```

```cpp
// Dans la boucle principale
install_resize_handler();

while (running) {
    if (terminal_resized.exchange(false)) {
        auto new_size = get_terminal_size();
        // Réafficher avec la nouvelle taille
        redraw(new_size);
    }
    // ... logique principale ...
}
```

> 💡 Pour la plupart des outils CLI qui produisent une sortie linéaire (pas d'interface persistante), `SIGWINCH` n'est pas nécessaire. Il est principalement utile pour les outils de type `top`, `htop`, ou les barres de progression multilignes. Voir **section 20.2** pour les détails sur l'installation de handlers de signaux.

---

## La variable `TERM` et les capacités du terminal

### Qu'est-ce que `TERM` ?

La variable d'environnement `TERM` identifie le type de terminal. Elle détermine quelles séquences d'échappement sont supportées :

```bash
$ echo $TERM
xterm-256color
```

Les valeurs courantes en 2026 :

| Valeur | Contexte | Couleurs |
|--------|----------|:--------:|
| `xterm-256color` | Terminaux modernes (GNOME Terminal, iTerm2, Windows Terminal) | 256 |
| `xterm-color` | Terminaux plus anciens | 16 |
| `screen` / `screen-256color` | Sessions `screen` | 16 / 256 |
| `tmux-256color` | Sessions `tmux` | 256 |
| `linux` | Console virtuelle Linux (Ctrl+Alt+F1) | 8 |
| `dumb` | Terminal minimal, pas de séquences | 0 |
| `vt100` | Émulation historique | 8 |

### Le cas `TERM=dumb`

Un terminal `dumb` ne supporte aucune séquence d'échappement. C'est le signal que toute colorisation et tout positionnement de curseur doivent être désactivés :

```cpp
bool term_supports_color() {
    const char* term = std::getenv("TERM");
    if (term == nullptr) return false;
    if (std::string_view(term) == "dumb") return false;
    return true;
}
```

Certains environnements CI définissent `TERM=dumb`. C'est un signal que votre outil doit respecter.

### Support des couleurs 256 et True Color

Les terminaux modernes supportent trois niveaux de couleurs :

- **8 couleurs de base** (ANSI) : noir, rouge, vert, jaune, bleu, magenta, cyan, blanc.  
- **256 couleurs** : les 8 de base + 8 "bright" + 216 couleurs RGB (cube 6×6×6) + 24 niveaux de gris.  
- **True Color (24 bits)** : 16,7 millions de couleurs (RGB complet).

L'API `fmt::color` utilise des couleurs nommées qui sont mappées sur les codes ANSI appropriés. En pratique, les couleurs nommées de base (`fmt::color::red`, `fmt::color::green`, etc.) fonctionnent sur tous les terminaux qui supportent la couleur. Pour les couleurs plus spécifiques (nuances CSS), un terminal 256 couleurs ou True Color est nécessaire.

La détection du niveau de support se fait en combinant `TERM` et `COLORTERM` :

```cpp
enum class ColorDepth { none, basic, color256, truecolor };

ColorDepth detect_color_depth() {
    const char* term = std::getenv("TERM");
    if (term == nullptr || std::string_view(term) == "dumb")
        return ColorDepth::none;

    // COLORTERM est défini par les terminaux modernes
    const char* colorterm = std::getenv("COLORTERM");
    if (colorterm) {
        std::string_view ct(colorterm);
        if (ct == "truecolor" || ct == "24bit")
            return ColorDepth::truecolor;
    }

    std::string_view t(term);
    if (t.find("256color") != std::string_view::npos)
        return ColorDepth::color256;

    return ColorDepth::basic;
}
```

Pour un outil CLI typique, cette distinction est rarement nécessaire — les couleurs nommées de base de {fmt} fonctionnent sur tout terminal supportant la couleur. Mais si votre outil produit des visualisations riches (heatmaps, graphiques ASCII, dégradés), connaître le niveau de support permet d'adapter le rendu.

---

## Contexte d'exécution complet

En combinant tous les signaux disponibles, un outil CLI peut détecter précisément son contexte d'exécution :

```cpp
#include <unistd.h>
#include <cstdlib>
#include <sys/ioctl.h>
#include <string_view>

struct TerminalContext {
    // TTY
    bool stdin_tty;
    bool stdout_tty;
    bool stderr_tty;

    // Dimensions (valides seulement si stdout_tty)
    int cols;
    int rows;

    // Couleurs
    bool color_stdout;   // Couleurs sur stdout ?
    bool color_stderr;   // Couleurs sur stderr ?

    // Mode
    bool interactive;    // Peut-on poser des questions à l'utilisateur ?
    bool ci;             // Environnement CI détecté ?
};

TerminalContext detect_context() {
    TerminalContext ctx;

    // 1. Détection TTY
    ctx.stdin_tty  = isatty(STDIN_FILENO);
    ctx.stdout_tty = isatty(STDOUT_FILENO);
    ctx.stderr_tty = isatty(STDERR_FILENO);

    // 2. Dimensions
    ctx.cols = 80;
    ctx.rows = 24;
    struct winsize ws;
    if (ctx.stdout_tty && ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (ws.ws_col > 0) ctx.cols = ws.ws_col;
        if (ws.ws_row > 0) ctx.rows = ws.ws_row;
    } else {
        if (const char* c = std::getenv("COLUMNS")) ctx.cols = std::atoi(c);
        if (const char* r = std::getenv("LINES"))   ctx.rows = std::atoi(r);
    }

    // 3. Couleurs
    bool no_color = std::getenv("NO_COLOR") != nullptr;
    bool force_color = std::getenv("FORCE_COLOR") != nullptr;
    bool term_ok = true;
    if (const char* term = std::getenv("TERM")) {
        if (std::string_view(term) == "dumb") term_ok = false;
    } else {
        term_ok = false;
    }

    if (no_color) {
        ctx.color_stdout = ctx.color_stderr = false;
    } else if (force_color) {
        ctx.color_stdout = ctx.color_stderr = true;
    } else {
        ctx.color_stdout = ctx.stdout_tty && term_ok;
        ctx.color_stderr = ctx.stderr_tty && term_ok;
    }

    // 4. Interactivité
    ctx.interactive = ctx.stdin_tty && ctx.stdout_tty;

    // 5. Détection CI
    ctx.ci = std::getenv("CI") != nullptr
          || std::getenv("GITHUB_ACTIONS") != nullptr
          || std::getenv("GITLAB_CI") != nullptr
          || std::getenv("JENKINS_URL") != nullptr
          || std::getenv("BUILDKITE") != nullptr;

    return ctx;
}
```

### Intégration avec les options CLI

Les options `--color`, `--no-color` et `--width` de l'utilisateur doivent primer sur la détection automatique :

```cpp
// Après CLI11_PARSE
auto ctx = detect_context();

// Override par les options utilisateur
if (app.get_option("--color")->count() > 0) {
    ctx.color_stdout = ctx.color_stderr = use_color;
}
if (width_override > 0) {
    ctx.cols = width_override;
}
if (no_interactive_flag) {
    ctx.interactive = false;
}
```

L'ordre de priorité est clair et prévisible :

```
Options CLI (--color, --width)
    > Variables d'environnement (NO_COLOR, COLUMNS)
        > Détection automatique (isatty, ioctl, TERM)
            > Valeurs par défaut (80 colonnes, pas de couleur)
```

---

## Adaptation du format de sortie

Le contexte du terminal influence non seulement les couleurs, mais aussi le **format** de la sortie. Un pattern courant :

```cpp
void print_results(const std::vector<Result>& results,
                   const TerminalContext& ctx,
                   const std::string& format) {
    // Format explicite demandé par l'utilisateur : respecter le choix
    if (format == "json") {
        print_json(results);
        return;
    }
    if (format == "csv") {
        print_csv(results);
        return;
    }

    // Format "auto" : adapter au contexte
    if (!ctx.stdout_tty) {
        // Sortie non-interactive : format machine par défaut
        print_csv(results);
    } else if (ctx.cols < 60) {
        // Terminal étroit : format compact
        print_compact(results, ctx);
    } else {
        // Terminal large : tableau complet avec couleurs
        print_table(results, ctx);
    }
}
```

Ce comportement adaptatif est ce qui distingue un utilitaire amateur d'un outil professionnel. L'utilisateur en terminal interactif voit un beau tableau coloré ; le même outil dans un pipe produit du CSV exploitable par `awk` ou `jq` — sans flag supplémentaire.

### Pagination automatique

Quand la sortie dépasse la hauteur du terminal, les outils comme `git log` lancent automatiquement un pager (`less`, `more`). Ce pattern est implémentable en C++ :

```cpp
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

// Retourne un FILE* vers un pager si pertinent, sinon stdout
FILE* maybe_pager(const TerminalContext& ctx, int content_lines) {
    // Pas de pager si stdout n'est pas un terminal
    if (!ctx.stdout_tty) return stdout;

    // Pas de pager si le contenu tient dans le terminal
    if (content_lines <= ctx.rows - 2) return stdout;

    // Déterminer le pager (PAGER env, ou less, ou more)
    const char* pager_cmd = std::getenv("PAGER");
    if (pager_cmd == nullptr) pager_cmd = "less -R";

    // -R permet à less de comprendre les codes ANSI
    FILE* pager = popen(pager_cmd, "w");
    return pager ? pager : stdout;
}

void close_pager(FILE* pager) {
    if (pager != stdout) {
        pclose(pager);
    }
}
```

```cpp
auto ctx = detect_context();  
FILE* out = maybe_pager(ctx, total_lines);  

// Écrire la sortie dans le pager (ou stdout)
for (const auto& line : output_lines) {
    fmt::print(out, "{}\n", line);
}

close_pager(out);
```

Le flag `-R` de `less` est essentiel : il demande à `less` d'interpréter les séquences ANSI plutôt que de les afficher littéralement. C'est ce qui permet d'avoir des couleurs dans un pager.

---

## Mode interactif : confirmations et prompts

Quand `stdin` et `stdout` sont tous deux des terminaux, l'outil peut interagir avec l'utilisateur. C'est le cas des confirmations avant une action destructrice :

```cpp
bool confirm(const std::string& message, const TerminalContext& ctx) {
    if (!ctx.interactive) {
        // Mode non-interactif : refuser par défaut (sécurité)
        fmt::print(stderr,
                   "Confirmation requise mais mode non-interactif. "
                   "Utilisez --force pour contourner.\n");
        return false;
    }

    fmt::print(stderr, "{} [y/N] ", message);
    std::fflush(stderr);

    std::string response;
    if (!std::getline(std::cin, response)) return false;

    return response == "y" || response == "Y"
        || response == "yes" || response == "YES";
}
```

```cpp
// Utilisation
if (!force_flag && !confirm("Supprimer 47 fichiers ?", ctx)) {
    fmt::print(stderr, "Annulé.\n");
    return 0;
}
```

Points critiques :

- **Prompt sur `stderr`** : le message de confirmation va sur `stderr`, pas `stdout`. Cela permet à `stdout` de rester propre pour les données, même en mode interactif.  
- **Défaut sécurisé** : `[y/N]` indique que le défaut (Entrée sans rien taper) est "Non". Pour les actions destructrices, le défaut doit toujours être le refus.  
- **Mode non-interactif** : quand stdin n'est pas un TTY (pipe, redirection, CI), la confirmation est impossible. Le comportement sûr est de refuser et d'informer l'utilisateur qu'il peut utiliser `--force`. Ne présumez jamais un "oui" en mode non-interactif.

---

## Comportement en environnement CI

Les environnements CI (GitHub Actions, GitLab CI, Jenkins) ont des caractéristiques spécifiques :

- `stdin` n'est pas un TTY (pas d'interaction possible).  
- `stdout` et `stderr` sont souvent capturés dans des logs.  
- La variable `CI=true` est généralement définie.  
- Certains CI supportent les couleurs ANSI dans leurs logs (GitHub Actions oui, Jenkins partiellement).

Un outil bien conçu adapte son comportement :

```cpp
void configure_for_ci(TerminalContext& ctx) {
    if (!ctx.ci) return;

    // Pas d'interactivité en CI
    ctx.interactive = false;

    // GitHub Actions supporte les couleurs ANSI dans les logs
    if (std::getenv("GITHUB_ACTIONS")) {
        ctx.color_stdout = ctx.color_stderr = true;
    }

    // Largeur généreuse pour les logs CI
    if (ctx.cols == 80) {
        ctx.cols = 120;  // Les logs CI ont souvent plus d'espace
    }
}
```

> 💡 GitHub Actions définit `GITHUB_ACTIONS=true` et supporte les couleurs ANSI. GitLab CI définit `GITLAB_CI=true` et supporte également les couleurs. Jenkins ne supporte les couleurs que via le plugin AnsiColor. Si votre outil est utilisé en CI, tester la colorisation dans ces environnements fait partie de la validation.

---

## Module complet : `terminal.hpp`

En synthèse, voici un module réutilisable qui encapsule toute la logique de détection et d'adaptation couverte dans cette section :

```cpp
// terminal.hpp — Détection et adaptation au contexte terminal
#pragma once

#include <cstdlib>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <unistd.h>

namespace cli {

enum class ColorMode { automatic, always, never };

struct TerminalContext {
    bool stdin_tty   = false;
    bool stdout_tty  = false;
    bool stderr_tty  = false;
    int cols         = 80;
    int rows         = 24;
    bool color_stdout = false;
    bool color_stderr = false;
    bool interactive  = false;
    bool ci           = false;

    // Méthodes utilitaires
    bool is_piped() const { return !stdout_tty; }
    bool is_narrow() const { return cols < 80; }
    bool is_wide() const { return cols >= 120; }
};

inline TerminalContext detect(ColorMode mode = ColorMode::automatic) {
    TerminalContext ctx;

    // TTY
    ctx.stdin_tty  = isatty(STDIN_FILENO);
    ctx.stdout_tty = isatty(STDOUT_FILENO);
    ctx.stderr_tty = isatty(STDERR_FILENO);

    // Dimensions
    struct winsize ws{};
    if (ctx.stdout_tty && ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (ws.ws_col > 0) ctx.cols = ws.ws_col;
        if (ws.ws_row > 0) ctx.rows = ws.ws_row;
    } else {
        if (auto* c = std::getenv("COLUMNS")) ctx.cols = std::atoi(c);
        if (auto* r = std::getenv("LINES"))   ctx.rows = std::atoi(r);
    }

    // Couleurs
    bool term_ok = true;
    if (auto* term = std::getenv("TERM")) {
        if (std::string_view(term) == "dumb") term_ok = false;
    } else {
        term_ok = false;
    }

    switch (mode) {
        case ColorMode::always:
            ctx.color_stdout = ctx.color_stderr = true;
            break;
        case ColorMode::never:
            ctx.color_stdout = ctx.color_stderr = false;
            break;
        case ColorMode::automatic:
            if (std::getenv("NO_COLOR")) {
                ctx.color_stdout = ctx.color_stderr = false;
            } else if (std::getenv("FORCE_COLOR")) {
                ctx.color_stdout = ctx.color_stderr = true;
            } else {
                ctx.color_stdout = ctx.stdout_tty && term_ok;
                ctx.color_stderr = ctx.stderr_tty && term_ok;
            }
            break;
    }

    // Interactivité et CI
    ctx.interactive = ctx.stdin_tty && ctx.stdout_tty;
    ctx.ci = std::getenv("CI") != nullptr
          || std::getenv("GITHUB_ACTIONS") != nullptr
          || std::getenv("GITLAB_CI") != nullptr
          || std::getenv("JENKINS_URL") != nullptr;

    if (ctx.ci) {
        ctx.interactive = false;
        if (std::getenv("GITHUB_ACTIONS") || std::getenv("GITLAB_CI")) {
            if (mode == ColorMode::automatic) {
                ctx.color_stdout = ctx.color_stderr = true;
            }
        }
    }

    return ctx;
}

}  // namespace cli
```

### Utilisation dans un outil CLI

```cpp
#include "terminal.hpp"
#include <CLI/CLI.hpp>

int main(int argc, char* argv[]) {
    CLI::App app{"mytool"};

    bool use_color = true;
    app.add_flag("--color,!--no-color", use_color, "Activer/désactiver les couleurs");

    int width_override = 0;
    app.add_option("--width", width_override, "Forcer la largeur de sortie")
        ->check(CLI::Range(40, 300));

    CLI11_PARSE(app, argc, argv);

    // Détection du contexte
    cli::ColorMode cmode = cli::ColorMode::automatic;
    if (app.get_option("--color")->count() > 0) {
        cmode = use_color ? cli::ColorMode::always : cli::ColorMode::never;
    }
    auto ctx = cli::detect(cmode);

    if (width_override > 0) ctx.cols = width_override;

    // Le contexte est prêt — tout le reste de l'outil l'utilise
    run(ctx);
}
```

Ce `TerminalContext` est ensuite passé (par référence constante) à toutes les fonctions d'affichage. C'est le point de vérité unique pour toutes les décisions de rendu — couleurs, largeur, format, interactivité.

---

## Récapitulatif des signaux et de leur priorité

```
┌──────────────────────────────────────────────────────────────┐
│  1. Options CLI (--color/--no-color, --width, --format)      │
│     → Choix explicite de l'utilisateur, priorité maximale    │
├──────────────────────────────────────────────────────────────┤
│  2. Variables d'environnement                                │
│     NO_COLOR / FORCE_COLOR  → Couleurs                       │
│     COLUMNS / LINES         → Dimensions                     │
│     PAGER                   → Paginateur                     │
│     TERM                    → Capacités du terminal          │
│     CI / GITHUB_ACTIONS     → Environnement CI               │
├──────────────────────────────────────────────────────────────┤
│  3. Détection système                                        │
│     isatty()                → TTY ou non                     │
│     ioctl(TIOCGWINSZ)      → Dimensions réelles              │
├──────────────────────────────────────────────────────────────┤
│  4. Valeurs par défaut                                       │
│     80 colonnes, 24 lignes, pas de couleur                   │
└──────────────────────────────────────────────────────────────┘
```

---

## Bonnes pratiques

**Testez les trois flux indépendamment.** `stdout` peut être un pipe pendant que `stderr` est un TTY. Ne prenez pas de raccourci en testant un seul descripteur.

**Respectez `NO_COLOR`.** C'est une convention communautaire adoptée par des centaines d'outils. Un outil qui l'ignore frustre les utilisateurs qui l'ont définie pour une raison (accessibilité, préférences, scripts).

**Dégradez gracieusement.** Quand `ioctl` échoue, utilisez les variables d'environnement. Quand elles sont absentes, utilisez 80 colonnes. Quand `TERM` est absent ou `dumb`, désactivez les couleurs. Chaque niveau de fallback produit une sortie utilisable.

**Données sur `stdout`, diagnostic sur `stderr`.** C'est la règle fondamentale. Les couleurs, les barres de progression, les confirmations — tout ce qui est interactif va sur `stderr`. Les données exploitables par un pipe vont sur `stdout`, jamais colorées en mode automatique.

**Préférez le refus en mode non-interactif.** Si une confirmation est nécessaire et que `stdin` n'est pas un TTY, refusez l'action et orientez l'utilisateur vers `--force` ou `--yes`. Un outil qui présume un "oui" en CI est un danger.

---

La section suivante (**36.5**) synthétise tous les éléments vus dans ce chapitre — parsing, formatage, couleurs et conscience du terminal — dans l'architecture d'un outil CLI professionnel complet, en suivant les patterns des outils de référence comme `kubectl`, `git` et `cargo`.

⏭️ [Architecture d'un outil CLI professionnel (à la kubectl, git)](/36-interfaces-cli/05-architecture-cli.md)
