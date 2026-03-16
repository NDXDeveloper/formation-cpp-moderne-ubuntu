🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 29.5 — `std::stacktrace` : Traces d'exécution intégrées au débogage

## Chapitre 29 : Débogage Avancé · Module 10

---

## Introduction

Toutes les techniques vues dans ce chapitre — GDB, core dumps, sanitizers — sont des outils externes. Ils observent votre programme de l'extérieur. Mais il existe des situations où vous avez besoin de voir la pile d'appels depuis l'intérieur du programme lui-même, pendant qu'il s'exécute, sans débogueur attaché.

Un serveur qui détecte un état incohérent et veut logger comment il est arrivé là. Un gestionnaire de signal qui veut capturer le contexte d'un crash avant de terminer proprement. Un système d'assertions personnalisé qui veut afficher la pile d'appels quand une précondition est violée. Un framework de tests qui veut montrer le chemin d'exécution exact quand un test échoue.

Avant C++23, obtenir une pile d'appels depuis le code nécessitait des solutions non portables : `backtrace()` et `backtrace_symbols()` sous Linux (API POSIX non standard, résultats bruts avec des adresses hexadécimales), `CaptureStackBackTrace` sous Windows, ou des bibliothèques tierces comme Boost.Stacktrace. Chaque solution avait ses particularités, ses limitations, et ses problèmes de portabilité.

C++23 introduit `std::stacktrace` dans l'en-tête `<stacktrace>` : une API standard, portable, et intégrée au langage pour capturer et manipuler les traces d'exécution. Vous capturez la pile d'appels en une ligne, vous la stockez, vous l'affichez, vous la transmettez — comme n'importe quel objet C++.

> 📎 *Cette section se concentre sur l'utilisation de `std::stacktrace` dans un contexte de débogage. Pour la couverture générale de `std::stacktrace` en tant que fonctionnalité C++23, voir section 12.12.*

---

## Prérequis : support compilateur et linkage

### État du support (mars 2026)

| Compilateur | Support `<stacktrace>` | Bibliothèque de linkage |
|---|---|---|
| GCC 13+ | ✅ Complet | `-lstdc++_libbacktrace` |
| GCC 14+ / GCC 15 | ✅ Complet | `-lstdc++exp` (ou `-lstdc++_libbacktrace`) |
| Clang 20 + libc++ | ✅ Complet | Intégré (pas de flag supplémentaire) |

### Compilation

```bash
# GCC 15 — nécessite le linkage explicite avec la bibliothèque de stacktrace
g++ -std=c++23 -g -O1 -o prog main.cpp -lstdc++exp

# Clang 20 avec libc++
clang++ -std=c++23 -stdlib=libc++ -g -O1 -o prog main.cpp
```

Le flag `-g` est important : `std::stacktrace` résout les noms de fonctions et les numéros de lignes à partir des informations de débogage. Sans `-g`, vous obtenez des adresses brutes au lieu de noms lisibles. Même avec `-O2`, le flag `-g` produit des traces exploitables (les noms de fonctions sont préservés, mais certaines fonctions inlinées peuvent ne pas apparaître).

### Intégration CMake

```cmake
# CMakeLists.txt
add_executable(mon_programme main.cpp)  
target_compile_features(mon_programme PRIVATE cxx_std_23)  

# GCC nécessite le linkage explicite
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(mon_programme PRIVATE stdc++exp)
endif()
```

---

## API fondamentale

### Capturer la pile d'appels

```cpp
#include <stacktrace>
#include <iostream>

void inner_function() {
    // Capture la pile d'appels à cet instant
    auto trace = std::stacktrace::current();
    
    std::cout << trace << '\n';
}

void middle_function() {
    inner_function();
}

int main() {
    middle_function();
    return 0;
}
```

Sortie :

```
 0# inner_function() at main.cpp:6
 1# middle_function() at main.cpp:11
 2# main at main.cpp:15
 3# __libc_start_call_main at libc_start_call_main.h:58
 4# __libc_start_main at libc-start.c:360
 5# _start
```

`std::stacktrace::current()` capture la pile d'appels au point d'appel. L'objet retourné est une valeur — vous pouvez le copier, le stocker, le passer à une fonction, le sérialiser. C'est un objet C++ ordinaire, pas un pointeur vers des ressources internes volatiles.

### Limiter la profondeur de capture

```cpp
// Capturer au maximum 5 frames
auto trace = std::stacktrace::current(0, 5);
```

Le premier argument est le nombre de frames à ignorer en haut de la pile (0 = commencer depuis l'appelant de `current()`). Le second est le nombre maximum de frames à capturer. Limiter la profondeur est utile en production pour réduire le coût de capture.

### Accéder aux entrées individuelles

`std::stacktrace` est un conteneur itérable d'objets `std::stacktrace_entry` :

```cpp
auto trace = std::stacktrace::current();

for (const auto& entry : trace) {
    std::cout << "  Fonction  : " << entry.description() << '\n'
              << "  Fichier   : " << entry.source_file() << '\n'
              << "  Ligne     : " << entry.source_line() << '\n'
              << '\n';
}
```

Sortie :

```
  Fonction  : inner_function()
  Fichier   : main.cpp
  Ligne     : 6

  Fonction  : middle_function()
  Fichier   : main.cpp
  Ligne     : 11

  Fonction  : main
  Fichier   : main.cpp
  Ligne     : 15
```

Les méthodes de `std::stacktrace_entry` :

| Méthode | Retour | Contenu |
|---|---|---|
| `description()` | `std::string` | Nom de la fonction (démanglé) |
| `source_file()` | `std::string` | Chemin du fichier source |
| `source_line()` | `std::uint_least32_t` | Numéro de ligne (0 si inconnu) |
| `native_handle()` | *implementation-defined* | Handle natif pour interop avec des outils système |

### Accès par index et taille

```cpp
auto trace = std::stacktrace::current();

std::cout << "Profondeur : " << trace.size() << '\n';  
std::cout << "Top frame  : " << trace[0] << '\n';  
std::cout << "Main       : " << trace[trace.size() - 3] << '\n';  

// Vide ?
if (trace.empty()) {
    std::cout << "Trace non disponible\n";
}
```

---

## Cas d'usage 1 : Assertions enrichies

L'utilisation la plus immédiate de `std::stacktrace` est d'enrichir vos assertions avec la pile d'appels complète. L'`assert` standard affiche le fichier et la ligne de l'assertion, mais pas le chemin qui y a mené. Dans une base de code où une fonction utilitaire est appelée depuis des dizaines d'endroits, savoir que l'assertion a échoué à `utils.cpp:42` est insuffisant — vous avez besoin de savoir qui a appelé cette fonction avec des arguments invalides.

```cpp
#include <stacktrace>
#include <iostream>
#include <string_view>
#include <cstdlib>

// Assertion personnalisée avec stacktrace
#define ASSERT_MSG(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "ASSERTION FAILED: " << (message) << '\n' \
                      << "  Condition: " #condition << '\n' \
                      << "  Location:  " << __FILE__ << ':' << __LINE__ << '\n' \
                      << "  Stack trace:\n" \
                      << std::stacktrace::current() << '\n'; \
            std::abort(); \
        } \
    } while (false)
```

Utilisation :

```cpp
void process_buffer(const char* data, size_t size) {
    ASSERT_MSG(data != nullptr, "Buffer null passé à process_buffer");
    ASSERT_MSG(size > 0, "Taille de buffer nulle");
    // ...
}

void handle_request(const Request& req) {
    auto buffer = extract_payload(req);
    process_buffer(buffer.data(), buffer.size());    // Assertion ici
}

int main() {
    Request empty_request{};
    handle_request(empty_request);
}
```

Sortie quand `buffer.size() == 0` :

```
ASSERTION FAILED: Taille de buffer nulle
  Condition: size > 0
  Location:  processor.cpp:8
  Stack trace:
 0# process_buffer(char const*, unsigned long) at processor.cpp:8
 1# handle_request(Request const&) at handler.cpp:25
 2# main at main.cpp:14
```

Vous voyez immédiatement que `main` a appelé `handle_request` avec une requête vide, qui a produit un buffer de taille 0. Sans la stacktrace, vous n'auriez que `processor.cpp:8` — insuffisant pour comprendre le contexte.

---

## Cas d'usage 2 : Logging structuré avec contexte d'appel

Dans un système en production, quand une situation anormale est détectée (mais pas fatale), vous voulez logger l'événement avec assez de contexte pour le diagnostiquer plus tard. `std::stacktrace` permet d'inclure la pile d'appels dans les logs structurés :

```cpp
#include <stacktrace>
#include <format>
#include <string>

enum class LogLevel { INFO, WARN, ERROR };

void log_event(LogLevel level, std::string_view message,
               const std::stacktrace& trace = std::stacktrace::current()) {
    
    std::string level_str;
    switch (level) {
        case LogLevel::INFO:  level_str = "INFO";  break;
        case LogLevel::WARN:  level_str = "WARN";  break;
        case LogLevel::ERROR: level_str = "ERROR"; break;
    }
    
    // Première entrée de la trace = l'appelant de log_event
    std::string caller = "unknown";
    std::string file = "unknown";
    int line = 0;
    
    // Index 1 car index 0 est log_event lui-même
    if (trace.size() > 1) {
        caller = trace[1].description();
        file = trace[1].source_file();
        line = static_cast<int>(trace[1].source_line());
    }
    
    std::println(stderr, "[{}] {} ({}:{})", level_str, message, file, line);
    
    // Pour ERROR, inclure la pile complète
    if (level == LogLevel::ERROR) {
        std::println(stderr, "  Call stack:");
        for (size_t i = 1; i < trace.size(); ++i) {
            std::println(stderr, "    #{} {}", i - 1, trace[i].description());
        }
    }
}
```

Utilisation :

```cpp
void process_transaction(int id, double amount) {
    if (amount < 0) {
        log_event(LogLevel::ERROR,
                  std::format("Montant négatif pour transaction {}: {}", id, amount));
        return;
    }
    // ...
}
```

Sortie :

```
[ERROR] Montant négatif pour transaction 1057: -42.5 (transaction.cpp:12)
  Call stack:
    #0 process_transaction(int, double)
    #1 batch_processor(std::vector<Transaction> const&)
    #2 main
```

Le coût de capture de la stacktrace est non négligeable (quelques microsecondes). Pour les logs de niveau ERROR (rares), c'est acceptable. Pour les logs INFO émis des milliers de fois par seconde, capturez la stacktrace uniquement en mode debug ou ne capturez que le frame immédiat.

---

## Cas d'usage 3 : Gestionnaire de signal avec diagnostic

Quand un programme reçoit un signal fatal (SIGSEGV, SIGABRT), le handler de signal peut capturer la pile d'appels avant de terminer. C'est un mini-core-dump en texte, utile quand les core dumps ne sont pas configurés ou quand vous voulez un diagnostic immédiat dans les logs.

```cpp
#include <stacktrace>
#include <csignal>
#include <cstdio>
#include <cstdlib>

// Stockage pour la trace capturée dans le handler
// (signal handler = contexte restreint, voir note ci-dessous)
volatile std::sig_atomic_t signal_received = 0;

void signal_handler(int signum) {
    // ATTENTION : dans un signal handler, les opérations autorisées
    // sont très limitées (async-signal-safe functions uniquement).
    // std::stacktrace::current() n'est PAS garanti async-signal-safe.
    // En pratique, ça fonctionne souvent, mais ce n'est pas portable.
    
    // Approche pragmatique (fonctionne en pratique sous Linux)
    auto trace = std::stacktrace::current();
    
    // write() est async-signal-safe, fprintf ne l'est pas
    // Ici on utilise fprintf pour la lisibilité — en production,
    // préférez write() vers un fd pré-ouvert
    std::fprintf(stderr, "\n=== SIGNAL %d REÇU ===\n", signum);
    for (const auto& entry : trace) {
        std::fprintf(stderr, "  %s at %s:%d\n",
                     entry.description().c_str(),
                     entry.source_file().c_str(),
                     static_cast<int>(entry.source_line()));
    }
    std::fprintf(stderr, "======================\n");
    
    std::_Exit(128 + signum);    // _Exit est async-signal-safe
}

int main() {
    std::signal(SIGSEGV, signal_handler);
    std::signal(SIGABRT, signal_handler);
    
    // ... programme normal ...
}
```

> ⚠️ **Signal safety.** `std::stacktrace::current()` n'est pas garanti async-signal-safe par le standard. L'appeler dans un signal handler est techniquement un comportement indéfini. En pratique, sous Linux avec GCC et Clang, cela fonctionne de manière fiable. Si vous avez besoin d'une garantie stricte, capturez la stacktrace hors du handler via `sigaltstack` et un mécanisme de communication (pipe vers un thread dédié), ou utilisez les core dumps (section 29.3) qui sont le mécanisme garanti pour le post-mortem.

L'approche la plus robuste combine les deux : core dumps pour le diagnostic complet, et un signal handler avec stacktrace pour un diagnostic immédiat dans les logs.

---

## Cas d'usage 4 : Exceptions enrichies

Capturer la pile d'appels au moment du `throw` et la transporter avec l'exception :

```cpp
#include <stacktrace>
#include <stdexcept>
#include <string>
#include <format>

class traced_error : public std::runtime_error {
    std::stacktrace trace_;
    
public:
    explicit traced_error(const std::string& message)
        : std::runtime_error(message)
        , trace_(std::stacktrace::current())
    {}
    
    const std::stacktrace& trace() const noexcept { return trace_; }
    
    std::string full_diagnostic() const {
        return std::format("{}\nStack trace:\n{}", what(), trace_);
    }
};

// Variante pour les erreurs typées
class config_error : public traced_error {  
public:  
    using traced_error::traced_error;
};
```

Utilisation :

```cpp
void load_config(const std::string& path) {
    auto file = open_file(path);
    if (!file) {
        throw config_error(
            std::format("Impossible d'ouvrir le fichier de configuration: {}", path)
        );
    }
    // ...
}

int main() {
    try {
        load_config("/etc/myapp/config.yaml");
    } catch (const traced_error& e) {
        std::cerr << e.full_diagnostic() << '\n';
        return 1;
    }
}
```

Sortie :

```
Impossible d'ouvrir le fichier de configuration: /etc/myapp/config.yaml  
Stack trace:  
 0# traced_error::traced_error(std::string const&) at traced_error.hpp:12
 1# load_config(std::string const&) at config.cpp:8
 2# main at main.cpp:22
```

La pile d'appels est capturée au moment du `throw`, pas au moment du `catch`. C'est exactement ce que vous voulez — vous voyez le code qui a provoqué l'erreur, pas le code qui la gère. C'est un avantage majeur par rapport au `catch` standard, qui a perdu le contexte de la pile après le stack unwinding.

### Coût et considérations

La capture d'une stacktrace implique une allocation (pour stocker les frames) et la résolution des symboles (lecture des informations de débogage). Le coût typique est de l'ordre de quelques microsecondes à quelques dizaines de microsecondes selon la profondeur de la pile.

Pour les exceptions qui sont rares (erreurs de configuration, échecs d'I/O), ce coût est invisible. Pour les exceptions utilisées comme mécanisme de contrôle de flux (ce qui est déjà une mauvaise pratique en C++), le surcoût peut être significatif.

Une stratégie pragmatique :

```cpp
class traced_error : public std::runtime_error {
    std::stacktrace trace_;
    
public:
    // Capture par défaut, mais désactivable
    explicit traced_error(const std::string& message, 
                          bool capture_trace = true)
        : std::runtime_error(message)
        , trace_(capture_trace ? std::stacktrace::current() : std::stacktrace{})
    {}
    
    // ...
};
```

---

## Cas d'usage 5 : Traçage des allocations mémoire

Pour diagnostiquer des fuites mémoire ou des patterns d'allocation excessifs, vous pouvez capturer la stacktrace à chaque allocation et la stocker pour analyse ultérieure :

```cpp
#include <stacktrace>
#include <unordered_map>
#include <mutex>

struct AllocationRecord {
    size_t size;
    std::stacktrace trace;
};

class AllocationTracker {
    std::unordered_map<void*, AllocationRecord> allocations_;
    std::mutex mutex_;
    
public:
    void record_alloc(void* ptr, size_t size) {
        std::lock_guard lock(mutex_);
        allocations_[ptr] = {size, std::stacktrace::current()};
    }
    
    void record_free(void* ptr) {
        std::lock_guard lock(mutex_);
        allocations_.erase(ptr);
    }
    
    void dump_leaks() const {
        std::lock_guard lock(mutex_);
        if (allocations_.empty()) {
            std::println(stderr, "Aucune fuite détectée.");
            return;
        }
        
        std::println(stderr, "=== {} allocations non libérées ===", 
                     allocations_.size());
        
        size_t total = 0;
        for (const auto& [ptr, record] : allocations_) {
            total += record.size;
            std::println(stderr, "\n{} octets à l'adresse {}:",
                         record.size, ptr);
            // Afficher les frames pertinents (ignorer les 2 premiers
            // qui sont record_alloc et le remplacement de new)
            for (size_t i = 2; i < record.trace.size() && i < 7; ++i) {
                std::println(stderr, "  #{} {} at {}:{}",
                             i - 2,
                             record.trace[i].description(),
                             record.trace[i].source_file(),
                             static_cast<int>(record.trace[i].source_line()));
            }
        }
        std::println(stderr, "\nTotal fuité : {} octets", total);
    }
};
```

C'est le principe de fonctionnement de LeakSanitizer et de Valgrind — mais implémenté au niveau applicatif, configurable et ciblé sur votre code. En production, vous pouvez activer ce traçage dynamiquement (via un flag de configuration) pour diagnostiquer une consommation mémoire anormale sans redéployer avec ASan.

> ⚠️ Capturer une stacktrace à chaque allocation a un coût non négligeable. Réservez cette technique au diagnostic ciblé, pas à l'exécution permanente.

---

## Performance de `std::stacktrace`

La capture d'une stacktrace n'est pas gratuite. Voici les ordres de grandeur :

| Opération | Coût typique |
|---|---|
| `std::stacktrace::current()` (capture) | 1-50 μs selon la profondeur |
| Résolution des symboles (noms, lignes) | Inclus dans la capture ou différé selon l'implémentation |
| Affichage (`operator<<`) | Négligeable après capture |
| Copie d'un `std::stacktrace` | Copie profonde — coût proportionnel au nombre de frames |
| Déplacement d'un `std::stacktrace` | Quasi gratuit (transfert de propriété) |

Le coût de capture dépend principalement de la profondeur de la pile et de la méthode de résolution des symboles. Avec `libbacktrace` (utilisé par GCC), la résolution est rapide car les informations DWARF sont parsées efficacement.

### Réduire le coût

```cpp
// Limiter la profondeur — suffisant pour la plupart des diagnostics
auto trace = std::stacktrace::current(0, 10);    // Max 10 frames

// Ignorer les frames internes d'un framework
auto trace = std::stacktrace::current(3);         // Skip 3 frames du haut
```

### Comparaison avec les alternatives pré-C++23

| Solution | Portabilité | Qualité de sortie | Coût |
|---|---|---|---|
| `std::stacktrace` (C++23) | Standard C++ | Noms démanglés, fichiers, lignes | ~1-50 μs |
| `backtrace()` + `backtrace_symbols()` | Linux/macOS (POSIX) | Noms manglés, pas de lignes | ~1-20 μs |
| Boost.Stacktrace | Multi-plateforme | Comparable à std::stacktrace | ~1-50 μs |
| `libunwind` | Linux | Noms démanglés, pas de lignes | ~1-30 μs |
| Capturer dans GDB | Dev uniquement | Complète | Non applicable |

`std::stacktrace` remplace Boost.Stacktrace et les appels POSIX directs comme solution recommandée. Si vous utilisez déjà Boost.Stacktrace, la migration est quasi mécanique — l'API est très similaire (Boost.Stacktrace a d'ailleurs influencé le design de la version standard).

---

## Intégration avec les sanitizers

`std::stacktrace` et les sanitizers sont complémentaires, pas redondants :

- **Les sanitizers** capturent automatiquement des stacktraces dans leurs rapports d'erreur. Vous n'avez rien à coder — ASan, UBSan, TSan, MSan produisent des piles d'appels complètes quand ils détectent un bug.
- **`std::stacktrace`** vous donne une pile d'appels dans votre logique applicative — assertions personnalisées, logging, exceptions, diagnostic — pour des situations que les sanitizers ne couvrent pas (erreurs logiques, états incohérents, violations de préconditions métier).

Les deux outils répondent à des questions différentes. Les sanitizers disent "il y a un bug mémoire/UB/race ici". `std::stacktrace` dit "mon code a détecté une situation anormale, et voici comment on y est arrivé".

---

## Limitations

### Informations de débogage nécessaires

Sans le flag `-g`, les stacktraces contiennent des adresses brutes sans noms de fonctions ni numéros de lignes. En production avec un binaire strippé, la stacktrace n'est utile que si vous pouvez ensuite résoudre les adresses avec les symboles archivés :

```cpp
// Sortie avec -g
//   process_buffer(char const*, unsigned long) at processor.cpp:8

// Sortie sans -g (binaire strippé)
//   0x00005555555552a1
```

Pour les binaires de production strippés, la solution est d'archiver les fichiers de symboles (section 29.3) et de résoudre les adresses a posteriori avec `addr2line` :

```bash
addr2line -e prog.debug -f -C 0x00005555555552a1
# process_buffer(char const*, unsigned long)
# processor.cpp:8
```

### Fonctions inlinées

Quand le compilateur inline une fonction, elle disparaît de la pile d'appels physique. Selon l'implémentation, `std::stacktrace` peut ou non montrer les fonctions inlinées. Avec `-O0` ou `-O1`, la plupart des fonctions ne sont pas inlinées et apparaissent dans la trace. Avec `-O2` ou `-O3`, des frames peuvent manquer.

### Coût non nul

Comme documenté ci-dessus, la capture a un coût. Ne capturez pas une stacktrace dans un chemin critique exécuté des millions de fois par seconde. Réservez la capture aux situations exceptionnelles (erreurs, assertions, logging de diagnostic).

### Signal safety

`std::stacktrace::current()` n'est pas async-signal-safe. L'utiliser dans un signal handler fonctionne en pratique sous Linux mais n'est pas garanti par le standard. Les core dumps (section 29.3) restent la méthode fiable pour le diagnostic post-signal.

---

## Recommandations

**Assertions personnalisées.** Remplacez vos macros `assert` par une version qui inclut `std::stacktrace::current()`. Le coût est nul en fonctionnement normal (la condition est vraie) et le diagnostic est immensément plus riche quand l'assertion échoue.

**Exceptions tracées.** Créez une classe d'exception de base qui capture la stacktrace au moment du `throw`. Utilisez-la pour vos erreurs critiques — pas pour les erreurs de validation courantes.

**Logging d'erreurs.** Incluez la stacktrace dans les logs de niveau ERROR. Ne l'incluez pas dans les logs INFO ou DEBUG (trop coûteux, trop verbeux).

**Diagnostic de production.** Capturez la stacktrace uniquement quand un problème est détecté, jamais systématiquement. Un flag de configuration peut activer un mode "diagnostic verbeux" qui inclut les stacktraces dans les logs.

**Combinez avec les core dumps.** `std::stacktrace` donne un diagnostic immédiat dans les logs. Les core dumps donnent un diagnostic exhaustif pour GDB. Les deux sont complémentaires — l'un ne remplace pas l'autre.

---

> **À retenir** : `std::stacktrace` (C++23) apporte à votre code ce que GDB et les sanitizers apportent de l'extérieur — une pile d'appels complète avec noms de fonctions et numéros de lignes. Intégrez-le dans vos assertions, vos exceptions critiques, et vos logs d'erreur. C'est la pièce qui manquait entre "je sais qu'il y a un problème" et "je sais exactement comment le programme est arrivé là".

⏭️ [Analyse Mémoire](/30-analyse-memoire/README.md)
