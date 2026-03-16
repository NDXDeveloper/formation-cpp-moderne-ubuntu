🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 29.1 — GDB : Commandes essentielles et breakpoints

## Chapitre 29 : Débogage Avancé · Module 10

---

## Introduction

GDB — GNU Debugger — est le débogueur standard sous Linux. Il existe depuis 1986, il est maintenu activement, et il reste en 2026 l'outil sur lequel s'appuient la quasi-totalité des environnements de débogage C++ sous Linux, y compris les débogueurs intégrés de VS Code et CLion qui ne sont, en réalité, que des interfaces graphiques au-dessus de GDB (ou de LLDB, son équivalent dans l'écosystème LLVM).

Comprendre GDB en ligne de commande n'est pas un exercice académique réservé aux puristes du terminal. C'est une nécessité pratique pour trois raisons :

1. **En production et sur serveurs distants**, vous n'avez souvent qu'un accès SSH. Pas d'IDE, pas d'interface graphique — juste un terminal et GDB.
2. **Les interfaces graphiques ont des limites.** Quand VS Code ne vous montre pas ce que vous cherchez, savoir taper la bonne commande GDB directement vous débloque immédiatement.
3. **Comprendre GDB, c'est comprendre ce que fait le débogueur.** Les concepts (breakpoints, watchpoints, stack frames, stepping) sont les mêmes partout. Une fois que vous les maîtrisez en ligne de commande, tout débogueur graphique devient trivial à utiliser.

---

## Ce que GDB permet de faire

À un niveau fondamental, GDB vous donne quatre capacités :

**Contrôler l'exécution.** Vous pouvez lancer votre programme, l'arrêter à des endroits précis, avancer instruction par instruction, et reprendre l'exécution normale. Vous transformez un programme qui s'exécute en une fraction de seconde en un processus que vous pouvez explorer à votre rythme.

**Inspecter l'état.** À tout moment où le programme est en pause, vous pouvez examiner la valeur de n'importe quelle variable, la pile d'appels complète, le contenu de la mémoire à une adresse donnée, et les registres du processeur. Vous voyez le programme tel qu'il est, pas tel que vous pensez qu'il est.

**Modifier l'état en cours d'exécution.** Vous pouvez changer la valeur d'une variable pendant que le programme est en pause, puis continuer l'exécution. C'est un outil puissant pour tester des hypothèses : "est-ce que le bug disparaît si cette variable vaut 0 à cet endroit ?" — sans recompiler.

**Analyser des crashes post-mortem.** En chargeant un core dump (section 29.3), vous pouvez examiner l'état d'un programme au moment exact de son crash, même si ce crash s'est produit il y a des heures sur un autre serveur.

---

## Prérequis : compiler pour le débogage

GDB travaille sur des binaires compilés. Mais par défaut, le compilateur produit des exécutables optimisés et dépouillés d'informations de débogage. Pour que GDB soit réellement utile, le binaire doit contenir les informations qui font le lien entre le code machine et votre code source : noms de variables, numéros de lignes, structure des types.

Ces informations sont générées par le flag `-g` :

```bash
# Compilation minimale pour le débogage
g++ -g -o mon_programme main.cpp

# Compilation recommandée pour le débogage
g++ -g -O0 -o mon_programme main.cpp
```

Le flag `-O0` désactive les optimisations. C'est important : avec `-O2` ou `-O3`, le compilateur réorganise le code, supprime des variables, inline des fonctions. Le résultat est que le débogueur peut vous montrer un comportement qui ne correspond pas à ce que vous lisez dans le source — des variables "optimized out" qu'on ne peut plus inspecter, des lignes exécutées dans un ordre inattendu, des fonctions qui n'existent plus en tant que telles.

Pour un débogage confortable, désactivez les optimisations. Vous réactiverez `-O2` pour le profiling et la production.

### Niveaux d'information de débogage

Le flag `-g` accepte des niveaux de détail :

```bash
g++ -g1 main.cpp   # Minimal : fonctions et fichiers, pas de variables locales  
g++ -g2 main.cpp   # Standard (équivalent à -g) : variables, types, lignes  
g++ -g3 main.cpp   # Maximum : inclut les macros et les constantes du préprocesseur  
```

En pratique, `-g` (niveau 2) suffit dans la majorité des cas. Utilisez `-g3` quand vous devez déboguer du code qui utilise intensivement des macros — vous pourrez alors voir les valeurs des `#define` dans GDB.

Avec GCC, le flag `-ggdb3` génère des informations au format spécifique GDB avec le niveau de détail maximum :

```bash
g++ -ggdb3 -O0 -o mon_programme main.cpp
```

C'est la combinaison recommandée pour une session de débogage approfondie.

---

## Lancer GDB

### Lancement basique

```bash
gdb ./mon_programme
```

GDB charge le binaire et vous présente son prompt `(gdb)`. Le programme n'est pas encore lancé — vous êtes dans un état où vous pouvez poser des breakpoints, configurer l'environnement, puis décider quand démarrer l'exécution.

### Lancement avec arguments

Si votre programme attend des arguments en ligne de commande :

```bash
gdb --args ./mon_programme --config production.yaml --verbose
```

Le `--args` indique à GDB que tout ce qui suit est la commande complète à déboguer. Sans `--args`, GDB interpréterait `--config` comme l'un de ses propres flags.

### Attacher GDB à un processus en cours

Pour déboguer un programme déjà en cours d'exécution :

```bash
gdb -p <PID>
```

Le programme est immédiatement mis en pause. C'est la technique utilisée pour diagnostiquer un processus qui tourne en production et qui se comporte de manière suspecte (consommation CPU anormale, blocage apparent). Vous inspectez, vous détachez (`detach`), et le programme reprend normalement.

### Charger un core dump

```bash
gdb ./mon_programme core
```

Cette commande charge le binaire et le fichier core dump associé. Vous vous retrouvez dans l'état exact du programme au moment du crash. Les commandes d'inspection (`print`, `backtrace`) fonctionnent normalement, mais les commandes de navigation (`step`, `continue`) n'ont pas de sens — le programme est déjà mort.

> 📎 *La configuration et la génération des core dumps sont traitées en détail dans la section 29.3.*

---

## Anatomie d'une session GDB

Voici le flux typique d'une session de débogage. Les commandes individuelles seront détaillées dans les sous-sections, mais il est utile de voir le schéma global d'abord :

```
┌─────────────────────────────────────────────────────────────┐
│  1. Lancer GDB             gdb ./mon_programme              │
│                                                             │
│  2. Poser des breakpoints  break main                       │
│                            break parser.cpp:42              │
│                                                             │
│  3. Lancer l'exécution     run                              │
│       ↓                                                     │
│  Programme s'exécute jusqu'au premier breakpoint            │
│       ↓                                                     │
│  4. Inspecter l'état       print variable                   │
│                            backtrace                        │
│                            info locals                      │
│                                                             │
│  5. Avancer                next        (ligne suivante)     │
│                            step        (entrer dans fn)     │
│                            continue    (jusqu'au prochain   │
│                                         breakpoint)         │
│                                                             │
│  6. Répéter 4-5 jusqu'à comprendre le bug                   │
│                                                             │
│  7. Quitter                quit                             │
└─────────────────────────────────────────────────────────────┘
```

Ce cycle — poser un point d'arrêt, exécuter, inspecter, avancer — est la boucle fondamentale du débogage interactif. Toute la puissance de GDB réside dans la richesse des options disponibles à chaque étape de cette boucle.

---

## Les trois piliers de GDB

Les sous-sections qui suivent organisent les commandes GDB en trois groupes fonctionnels :

### 29.1.1 — Navigation : `run`, `step`, `next`, `continue`

Comment contrôler le flux d'exécution du programme. Quand avancer ligne par ligne, quand entrer dans une fonction, quand sauter jusqu'au prochain point d'intérêt. C'est le volant et les pédales de votre session de débogage.

### 29.1.2 — Inspection : `print`, `display`, `watch`

Comment observer l'état du programme. Afficher la valeur d'une variable, surveiller automatiquement une expression à chaque arrêt, déclencher un breakpoint quand une valeur change. C'est le tableau de bord — les instruments qui vous montrent ce qui se passe réellement.

### 29.1.3 — Breakpoints conditionnels

Comment cibler précisément le moment intéressant. Un programme peut exécuter une boucle 10 000 fois avant que le bug ne se manifeste à l'itération 7 423. Les breakpoints conditionnels vous permettent de dire "arrête-toi uniquement quand `i == 7423`" ou "arrête-toi quand `buffer.size() > capacity`". C'est la différence entre chercher une aiguille dans une botte de foin et aller directement à l'aiguille.

---

## GDB vs LLDB : quel débogueur choisir ?

Si vous utilisez Clang comme compilateur, vous avez accès à LLDB, le débogueur de l'écosystème LLVM. La question du choix se pose légitimement.

En termes de fonctionnalités de base, GDB et LLDB sont comparables. La syntaxe diffère sur certaines commandes, mais les concepts sont identiques. Voici les éléments de décision :

**GDB est le choix par défaut sous Linux.** L'écosystème est plus mature, la documentation plus abondante, le support des fonctionnalités avancées (comme le reverse debugging ou les pretty-printers pour la STL) est plus complet. Quand vous cherchez une solution à un problème GDB sur le web, vous trouvez des réponses.

**LLDB est natif dans l'écosystème Apple** et s'intègre mieux avec les outils LLVM (sanitizers, clang-tidy). Sur macOS, c'est le choix naturel. Sous Linux, il fonctionne mais reste moins utilisé.

**En pratique, GDB sous Linux est le standard.** Ce chapitre se concentre sur GDB. Si vous passez à LLDB, les concepts sont transférables — seule la syntaxe de certaines commandes change.

| Commande | GDB | LLDB |
|---|---|---|
| Lancer le programme | `run` | `run` |
| Breakpoint | `break main.cpp:42` | `breakpoint set -f main.cpp -l 42` |
| Afficher une variable | `print x` | `frame variable x` ou `p x` |
| Pile d'appels | `backtrace` | `thread backtrace` |
| Variable suivante | `next` | `next` |

---

## Configurer GDB pour un usage confortable

GDB en configuration par défaut est fonctionnel mais austère. Quelques ajustements dans le fichier `~/.gdbinit` transforment l'expérience :

```bash
# ~/.gdbinit — Configuration recommandée

# Historique des commandes entre sessions
set history save on  
set history size 10000  
set history filename ~/.gdb_history  

# Affichage amélioré
set print pretty on            # Structures affichées de manière lisible  
set print array on             # Tableaux sur plusieurs lignes  
set print elements 100         # Nombre max d'éléments affichés dans un tableau  
set print object on            # Affiche le type dynamique (polymorphisme)  

# Pagination désactivée (évite le "Press Enter to continue")
set pagination off

# Confirmation désactivée pour quit
set confirm off

# Pretty-printers STL (inclus avec GCC)
# Permet d'afficher std::vector, std::string, etc. de manière lisible
python  
import sys  
sys.path.insert(0, '/usr/share/gcc/python')  
from libstdcxx.v6.printers import register_libstdcxx_printers  
register_libstdcxx_printers(None)  
end  
```

L'activation des **pretty-printers** est particulièrement importante. Sans eux, un `std::vector<int>` s'affiche comme une structure interne incompréhensible avec des pointeurs. Avec eux :

```
# Sans pretty-printer
(gdb) print vec
$1 = {
  _M_impl = {
    _M_start = 0x55555556aeb0,
    _M_finish = 0x55555556aec4,
    _M_end_of_storage = 0x55555556aed0
  }
}

# Avec pretty-printer
(gdb) print vec
$1 = std::vector of length 5, capacity 8 = {10, 20, 30, 40, 50}
```

La différence est évidente. Les pretty-printers sont inclus avec GCC et sont généralement déjà disponibles sur Ubuntu — le bloc Python dans `.gdbinit` les active.

---

## Un programme pour s'entraîner

Les sous-sections qui suivent utilisent un programme simple mais volontairement buggé. Voici le point de départ — un utilitaire qui lit un fichier de configuration et en extrait des paires clé-valeur :

```cpp
// config_parser.cpp
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

struct ConfigEntry {
    std::string key;
    std::string value;
    int line_number;
};

std::vector<ConfigEntry> parse_config(const std::string& filename) {
    std::vector<ConfigEntry> entries;
    std::ifstream file(filename);
    std::string line;
    int line_num = 0;

    while (std::getline(file, line)) {
        ++line_num;

        // Ignorer les commentaires et lignes vides
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Chercher le séparateur '='
        auto pos = line.find('=');
        if (pos == std::string::npos) {
            std::cerr << "Ligne " << line_num
                      << " : format invalide (pas de '=')\n";
            continue;
        }

        ConfigEntry entry;
        entry.key = line.substr(0, pos);
        entry.value = line.substr(pos + 1);
        entry.line_number = line_num;
        entries.push_back(entry);
    }

    return entries;
}

std::string find_value(const std::vector<ConfigEntry>& entries,
                       const std::string& key) {
    for (const auto& entry : entries) {
        if (entry.key == key) {
            return entry.value;
        }
    }
    return "";  // Bug subtil : impossible de distinguer
                // "clé absente" et "clé avec valeur vide"
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    auto entries = parse_config(argv[1]);
    std::cout << "Entrées parsées : " << entries.size() << "\n";

    for (const auto& e : entries) {
        std::cout << "[L" << e.line_number << "] "
                  << e.key << " = " << e.value << "\n";
    }

    // Recherche d'une clé
    auto db_host = find_value(entries, "db_host");
    if (db_host.empty()) {
        std::cerr << "ERREUR : db_host non trouvé\n";
        return 1;
    }
    std::cout << "Connexion à : " << db_host << "\n";

    return 0;
}
```

Compilez-le avec les flags de débogage :

```bash
g++ -ggdb3 -O0 -std=c++20 -Wall -Wextra -o config_parser config_parser.cpp
```

Et créez un fichier de test :

```ini
# database.conf
db_host=localhost  
db_port=5432  
db_name=production  
# Timeout en secondes
timeout=30  
max_connections=100  
```

Ce programme fonctionne dans le cas nominal. Mais il contient plusieurs bugs subtils que vous découvrirez et diagnostiquerez avec GDB dans les sections suivantes : gestion des espaces autour du `=`, comportement avec un fichier inexistant, et la confusion entre "clé absente" et "valeur vide" dans `find_value`.

---

## Structure des fichiers

```
29-debogage/
└── 01-gdb-commandes.md              ← vous êtes ici
    ├── 01.1-navigation.md
    ├── 01.2-inspection.md
    └── 01.3-breakpoints-conditionnels.md
```

---

> **À retenir** : GDB n'est pas un outil qu'on apprend en lisant — c'est un outil qu'on apprend en l'utilisant. Compilez le programme ci-dessus, lancez `gdb ./config_parser`, et suivez les sous-sections les mains sur le clavier. En une heure de pratique, vous aurez acquis des réflexes qui vous feront gagner des jours sur vos futurs bugs.

⏭️ [Navigation : run, step, next, continue](/29-debogage/01.1-navigation.md)
