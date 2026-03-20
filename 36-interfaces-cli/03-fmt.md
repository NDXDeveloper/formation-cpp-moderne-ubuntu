🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 36.3 — fmt : Formatage avancé (pré-C++23) ⭐

## Module 12 : Création d'Outils CLI — Niveau Avancé

---

## Introduction

Nous avons couvert `std::print` et `std::format` en **section 2.7** (prise en main rapide) et **section 12.7** (formatage approfondi). Ces fonctionnalités standardisées en C++23 sont directement issues de la librairie **{fmt}**, créée par Victor Zverovich. La question naturelle est alors : si `std::format` est dans le standard, pourquoi parler encore de {fmt} en 2026 ?

La réponse tient en trois mots : **couleurs, performances et fonctionnalités avancées**. Le standard C++23 a intégré le cœur du formatage de {fmt}, mais pas tout. Plusieurs capacités essentielles pour les outils CLI restent exclusives à la librairie :

- **Couleurs et styles terminaux** (`fmt::color`, `fmt::emphasis`) — absents du standard.  
- **Formatage vers des buffers arbitraires** (`fmt::format_to`, `fmt::memory_buffer`) avec contrôle fin de l'allocation.  
- **Performances de compilation** — {fmt} offre des temps de compilation significativement meilleurs que les implémentations de `<format>` dans GCC et Clang en 2026, grâce à des optimisations spécifiques.  
- **`fmt::print` vers des `FILE*`** — le `std::print` de C++23 écrit sur `stdout` ou un `std::ostream` ; {fmt} permet d'écrire directement vers n'importe quel `FILE*`, ce qui est parfois plus naturel dans du code système.  
- **Formatage de types non standard** — support natif de `std::chrono` enrichi, conteneurs, tuples, et formateurs personnalisés plus ergonomiques.

Dans le contexte de ce chapitre sur les outils CLI, la fonctionnalité la plus différenciante est le support des **couleurs et styles** — c'est ce qui permet de passer d'une sortie monochrome fonctionnelle à une interface visuellement structurée et professionnelle.

---

## Positionnement : {fmt} vs std::format vs std::print

Pour clarifier les rôles respectifs, voici comment ces trois solutions se situent :

```
{fmt} (librairie)
  │
  ├── Cœur du formatage ──────→ std::format (C++20)
  │                              std::print  (C++23)
  │
  └── Extensions non standardisées :
      ├── Couleurs et styles (fmt::color, fmt::emphasis)
      ├── fmt::memory_buffer (formatage zéro-allocation)
      ├── Formatage vers FILE* (fmt::print(stderr, ...))
      ├── Named arguments (fmt::arg)
      ├── Support étendu de std::chrono
      └── Performances de compilation optimisées
```

La syntaxe de formatage est **identique** entre {fmt} et le standard — les format strings, les spécificateurs de largeur, de précision, d'alignement sont les mêmes. Ce qui change, c'est l'API autour du formatage et les fonctionnalités supplémentaires.

### Quel choix faire en 2026 ?

**Si votre outil n'a pas besoin de couleurs** et que vous ciblez C++23 ou ultérieur, `std::print` et `std::format` suffisent. Pas besoin d'ajouter une dépendance.

**Si votre outil a besoin de couleurs, de styles terminaux, ou de performance de formatage maximale**, {fmt} est la réponse. C'est le cas de la grande majorité des outils CLI professionnels — une sortie colorée n'est pas un luxe, c'est un standard d'ergonomie.

**Si vous devez supporter des compilateurs antérieurs à C++20**, {fmt} est la seule option pour bénéficier du formatage moderne. La librairie supporte C++11 et ultérieur.

En pratique, dans un outil CLI, la recommandation est claire : **utilisez {fmt}** pour les sorties utilisateur (couleurs, styles, diagnostic) et `std::format`/`std::print` pour le formatage interne sans couleurs. Les deux coexistent parfaitement dans le même projet.

---

## Installation

### CMake FetchContent (recommandé)

```cmake
include(FetchContent)  
FetchContent_Declare(  
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        11.1.4   # Vérifier la dernière version stable
)
FetchContent_MakeAvailable(fmt)

add_executable(mon-outil src/main.cpp)  
target_link_libraries(mon-outil PRIVATE fmt::fmt)  
```

Contrairement à CLI11 et argparse, {fmt} n'est **pas header-only** par défaut — elle se compile en une petite librairie statique. Cela réduit les temps de compilation dans les projets avec plusieurs unités de compilation qui utilisent {fmt}. Un mode header-only existe (`FMT_HEADER_ONLY`), mais le mode compilé est recommandé.

### Conan 2.0

```ini
[requires]
fmt/11.1.4

[generators]
CMakeDeps  
CMakeToolchain  
```

```cmake
find_package(fmt REQUIRED)  
target_link_libraries(mon-outil PRIVATE fmt::fmt)  
```

### Paquet système (Ubuntu)

```bash
sudo apt install libfmt-dev
```

Puis dans CMake :

```cmake
find_package(fmt REQUIRED)  
target_link_libraries(mon-outil PRIVATE fmt::fmt)  
```

> ⚠️ Comme pour CLI11, la version dans les dépôts Ubuntu peut être en retard. FetchContent ou Conan garantissent une version récente et épinglée.

### vcpkg

```bash
vcpkg install fmt
```

L'intégration CMake est identique avec `find_package(fmt REQUIRED)`.

---

## Aperçu des capacités

Avant de détailler chaque aspect dans les sous-sections suivantes, voici un tour d'horizon de ce que {fmt} rend possible dans un outil CLI.

### Formatage basique (identique à std::format)

```cpp
#include <fmt/core.h>

fmt::print("Serveur démarré sur {}:{}\n", host, port);  
fmt::print("Traitement : {}/{} fichiers ({:.1f}%)\n",  
           done, total, 100.0 * done / total);
fmt::print("{:<20} {:>10} {:>8}\n", "NOM", "TAILLE", "DATE");
```

La syntaxe est celle que vous connaissez déjà de `std::format` — accolades pour les placeholders, mini-langage de formatage pour l'alignement, la largeur et la précision. Si vous avez lu les **sections 2.7 et 12.7**, tout vous est familier.

### Couleurs et styles (exclusif {fmt})

```cpp
#include <fmt/color.h>

// Texte coloré
fmt::print(fg(fmt::color::green), "✓ Build réussi\n");  
fmt::print(fg(fmt::color::red),   "✗ 3 tests échoués\n");  
fmt::print(fg(fmt::color::yellow), "⚠ 12 warnings\n");  

// Styles combinés
fmt::print(fmt::emphasis::bold, "=== Résumé ===\n");  
fmt::print(fg(fmt::color::cyan) | fmt::emphasis::italic,  
           "Durée totale : {:.2f}s\n", elapsed);

// Couleur de fond
fmt::print(bg(fmt::color::red) | fg(fmt::color::white) | fmt::emphasis::bold,
           " ERREUR CRITIQUE ");
fmt::print("\n");
```

C'est la fonctionnalité phare de {fmt} dans un contexte CLI. Les couleurs ne sont pas des codes ANSI écrits à la main — elles sont typées, composables, et gérées proprement.

### Formatage de conteneurs et types composés

```cpp
#include <fmt/ranges.h>
#include <vector>
#include <map>

std::vector<int> ports = {8080, 8443, 9090};  
fmt::print("Ports actifs : {}\n", ports);  
// Ports actifs : [8080, 8443, 9090]

std::map<std::string, int> stats = {{"ok", 142}, {"error", 3}};  
fmt::print("Statistiques : {}\n", stats);  
// Statistiques : {"ok": 142, "error": 3}
```

L'inclusion de `<fmt/ranges.h>` rend formattables tous les conteneurs standard — vecteurs, maps, sets, paires, tuples. C'est particulièrement utile pour le logging et le débogage.

### Formatage de dates et durées

```cpp
#include <fmt/chrono.h>
#include <chrono>

auto now = std::chrono::system_clock::now();  
fmt::print("Timestamp : {:%Y-%m-%d %H:%M:%S}\n", now);  
// Timestamp : 2026-03-17 14:32:05

auto dur = std::chrono::milliseconds(3742);  
fmt::print("Durée : {}\n", dur);  
// Durée : 3742ms

auto elapsed = std::chrono::duration<double>(2.718);  
fmt::print("Temps : {:.2}\n", elapsed);  
// Temps : 2.72s
```

Le support `chrono` de {fmt} est plus riche que celui du standard et permet un formatage naturel des timestamps et durées — un besoin récurrent dans les outils CLI qui mesurent des performances ou affichent des logs.

### Buffer haute performance

```cpp
#include <fmt/format.h>

fmt::memory_buffer buf;  
for (int i = 0; i < 1000; ++i) {  
    fmt::format_to(std::back_inserter(buf),
                   "ligne {:05d}: {}\n", i, data[i]);
}
// buf contient tout le texte, avec une seule allocation (ou zéro si le
// buffer interne suffit). On peut ensuite écrire en une fois :
fmt::print("{}", fmt::to_string(buf));
```

`fmt::memory_buffer` est un buffer stack-allocated qui grandit dynamiquement en heap seulement si nécessaire. C'est l'outil idéal pour construire de grandes sorties textuelles sans allocations multiples — un pattern courant dans les outils qui génèrent des rapports ou des tableaux volumineux.

---

## {fmt} dans l'architecture d'un outil CLI

Dans un outil CLI bien architecturé, {fmt} intervient à plusieurs niveaux :

```
┌─────────────────────────────────────────────────┐
│  Parsing (CLI11 / argparse)                     │
│  → Valide les entrées                           │
└──────────────────────┬──────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────┐
│  Logique métier                                 │
│  → Traite les données                           │
│  → Utilise std::format pour le formatage interne│
└──────────────────────┬──────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────┐
│  Sortie (fmt)                                   │
│  → Couleurs et styles pour stdout (si TTY)      │
│  → Texte brut pour stdout (si pipe/fichier)     │
│  → Diagnostic coloré sur stderr                 │
│  → JSON / texte structuré pour l'intégration    │
└─────────────────────────────────────────────────┘
```

La couche de sortie est le domaine naturel de {fmt}. Elle centralise la responsabilité du formatage et de la colorisation, et c'est à ce niveau que la détection TTY (couverte en **section 36.4**) détermine si les couleurs doivent être activées ou non.

Un pattern courant consiste à encapsuler les sorties CLI dans un petit module qui abstrait la colorisation :

```cpp
// output.hpp — Module de sortie CLI
#pragma once
#include <fmt/color.h>
#include <fmt/core.h>
#include <unistd.h>

namespace cli {

// Détection TTY (simplifié — voir section 36.4 pour la version complète)
inline bool stdout_is_tty() { return isatty(STDOUT_FILENO); }  
inline bool stderr_is_tty() { return isatty(STDERR_FILENO); }  

// Succès
template <typename... Args>  
void success(fmt::format_string<Args...> fmts, Args&&... args) {  
    if (stderr_is_tty()) {
        fmt::print(stderr, fg(fmt::color::green), "✓ ");
    } else {
        fmt::print(stderr, "[OK] ");
    }
    fmt::print(stderr, fmts, std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
}

// Erreur
template <typename... Args>  
void error(fmt::format_string<Args...> fmts, Args&&... args) {  
    if (stderr_is_tty()) {
        fmt::print(stderr, fg(fmt::color::red) | fmt::emphasis::bold,
                   "✗ ");
    } else {
        fmt::print(stderr, "[ERROR] ");
    }
    fmt::print(stderr, fmts, std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
}

// Warning
template <typename... Args>  
void warn(fmt::format_string<Args...> fmts, Args&&... args) {  
    if (stderr_is_tty()) {
        fmt::print(stderr, fg(fmt::color::yellow), "⚠ ");
    } else {
        fmt::print(stderr, "[WARN] ");
    }
    fmt::print(stderr, fmts, std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
}

// Information (verbose)
template <typename... Args>  
void info(fmt::format_string<Args...> fmts, Args&&... args) {  
    if (stderr_is_tty()) {
        fmt::print(stderr, fg(fmt::color::cyan), "ℹ ");
    } else {
        fmt::print(stderr, "[INFO] ");
    }
    fmt::print(stderr, fmts, std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
}

}  // namespace cli
```

Utilisation dans le code métier :

```cpp
#include "output.hpp"

cli::success("Build terminé en {:.2f}s", elapsed);  
cli::warn("Le fichier {} sera écrasé", output_path);  
cli::error("Impossible de se connecter à {}:{}", host, port);  
cli::info("Utilisation du cache ({} entrées)", cache.size());  
```

```bash
# En terminal interactif (couleurs activées) :
✓ Build terminé en 3.42s
⚠ Le fichier output.txt sera écrasé
✗ Impossible de se connecter à api.example.com:443
ℹ Utilisation du cache (128 entrées)

# En pipe ou fichier (couleurs désactivées) :
[OK] Build terminé en 3.42s
[WARN] Le fichier output.txt sera écrasé
[ERROR] Impossible de se connecter à api.example.com:443
[INFO] Utilisation du cache (128 entrées)
```

Ce module d'une cinquantaine de lignes transforme l'expérience utilisateur d'un outil CLI. La sortie est lisible d'un coup d'œil en terminal, et propre quand elle est redirigée. Notez que tout le diagnostic va sur `stderr`, conformément aux bonnes pratiques décrites en **section 36 (introduction)**.

---

## Relation avec spdlog

Pour les outils qui ont besoin d'un logging plus structuré — niveaux de log persistants, rotation de fichiers, sortie JSON — **spdlog** (couvert en **section 40.1**) est la solution. spdlog utilise {fmt} comme moteur de formatage sous le capot, ce qui signifie que la syntaxe de formatage est identique. La distinction est claire :

- **{fmt}** : formatage et affichage direct pour les sorties CLI interactives.  
- **spdlog** : framework de logging complet avec sinks, niveaux, timestamps et rotation.

Les deux partagent la même syntaxe et coexistent naturellement dans un même projet. Un outil CLI typique utilise {fmt} pour les messages destinés à l'utilisateur (`stdout`/`stderr`) et spdlog pour les logs techniques destinés à l'exploitation.

---

## Plan de la section

Les deux sous-sections suivantes approfondissent les aspects pratiques :

| Sous-section | Contenu |
|-------------|---------|
| **36.3.1** | Syntaxe Python-like — le mini-langage de formatage en détail, arguments nommés, formateurs personnalisés, différences avec `std::format` |
| **36.3.2** | Couleurs et styles — API `fmt::color`, `fmt::emphasis`, composition, détection TTY, tableaux colorés et barres de progression |

---

> ⭐ **{fmt} est omniprésente dans l'écosystème C++ moderne.** Au-delà des outils CLI, elle est utilisée par spdlog, Folly (Facebook), le moteur Unreal Engine, et des centaines de projets open source. La maîtriser, c'est investir dans une compétence transversale qui servira dans n'importe quel contexte C++.

⏭️ [Syntaxe Python-like](/36-interfaces-cli/03.1-syntaxe-python.md)
