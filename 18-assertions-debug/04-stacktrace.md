🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 18.4 `std::stacktrace` (C++23) : Traces d'exécution standard

## Introduction

Avant C++23, obtenir une trace d'exécution (*stack trace*) en C++ nécessitait de recourir à des solutions spécifiques à chaque plateforme : `backtrace()` sous Linux, `CaptureStackBackTrace()` sous Windows, ou des bibliothèques tierces comme Boost.Stacktrace. Cette fragmentation rendait le débogage défensif moins portable et plus laborieux à mettre en place.

C++23 introduit `<stacktrace>` dans la bibliothèque standard, offrant enfin un moyen **portable, standardisé et intégrable** de capturer et d'afficher la pile d'appels à n'importe quel point du programme. Dans le contexte du débogage défensif, cette fonctionnalité transforme la qualité des diagnostics : au lieu d'un simple message d'erreur, on peut désormais savoir précisément *par quel chemin d'exécution* on est arrivé à un état invalide.

> 📎 *Cette section se concentre sur l'intégration de `std::stacktrace` dans les stratégies de débogage défensif. Pour une présentation complète de l'API et de ses fonctionnalités, voir **section 12.12** (std::stacktrace C++23).*

---

## Prérequis : compilation et linkage

`std::stacktrace` nécessite un support compilateur C++23 et, selon l'implémentation, un linkage explicite avec une bibliothèque de résolution de symboles.

### Avec GCC (libstdc++)

```bash
# GCC 13+ : linkage avec libstdc++exp (contient le support stacktrace via libbacktrace)
# IMPORTANT : -lstdc++exp doit apparaître APRÈS le(s) fichier(s) source/objet
g++ -std=c++23 -g main.cpp -o app -lstdc++exp

# Si le linker ne trouve pas la bibliothèque (cas des PPA ou installations non standard),
# ajouter le chemin explicite :
g++ -std=c++23 -g main.cpp -o app -L/usr/lib/gcc/x86_64-linux-gnu/15 -lstdc++exp
```

Le flag `-g` est essentiel : sans informations de débogage, la trace contiendra des adresses mémoire brutes au lieu des noms de fonctions, fichiers et numéros de ligne.

### Avec Clang (libc++)

```bash
# Clang 19+ avec libc++
clang++ -std=c++23 -g -stdlib=libc++ main.cpp -o app -lc++experimental
```

### Avec CMake

```cmake
target_compile_features(app PRIVATE cxx_std_23)  
target_compile_options(app PRIVATE -g)  

# GCC : lier la bibliothèque de support stacktrace
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(app PRIVATE stdc++exp)
endif()
```

> ⚠️ **État du support en mars 2026.** GCC 15 offre un support complet de `<stacktrace>`. Le support Clang 20 avec libc++ est fonctionnel mais peut nécessiter des flags de linkage différents selon la distribution. Vérifier la documentation de votre toolchain si la compilation échoue.

---

## Capturer une trace : les bases

L'API repose sur deux types principaux définis dans `<stacktrace>` :

- `std::stacktrace` — représente une capture complète de la pile d'appels.  
- `std::stacktrace_entry` — représente une entrée individuelle (un *frame*) dans la pile.

La capture s'effectue avec la méthode statique `std::stacktrace::current()` :

```cpp
#include <stacktrace>
#include <iostream>

void fonction_profonde() {
    // Capture de la pile d'appels à cet instant précis
    auto trace = std::stacktrace::current();

    std::cerr << "=== Stack Trace ===\n"
              << trace << '\n';
}

void fonction_intermediaire() {
    fonction_profonde();
}

int main() {
    fonction_intermediaire();
    return 0;
}
```

Sortie typique (compilé avec `-g`) :

```
=== Stack Trace ===
 0# fonction_profonde() at /home/dev/project/main.cpp:6
 1# fonction_intermediaire() at /home/dev/project/main.cpp:12
 2# main at /home/dev/project/main.cpp:16
```

Chaque ligne contient le nom démanglé de la fonction, le fichier source et le numéro de ligne — exactement les informations dont on a besoin pour diagnostiquer un problème.

---

## Intégration dans le débogage défensif

### Assertions enrichies avec trace d'exécution

La macro `assert()` standard (section 18.1) affiche un message minimal avant d'appeler `std::abort()`. En combinant `std::stacktrace` avec une macro d'assertion personnalisée, on obtient un diagnostic incomparablement plus riche :

```cpp
#include <stacktrace>
#include <iostream>
#include <cstdlib>
#include <string_view>

#ifdef DEBUG

#define ASSERT_WITH_TRACE(condition, message)                            \
    do {                                                                 \
        if (!(condition)) [[unlikely]] {                                 \
            std::cerr << "\n╔══ ASSERTION FAILED ══════════════════\n"   \
                      << "║ Condition : " #condition "\n"                \
                      << "║ Message   : " << (message) << "\n"          \
                      << "║ Fichier   : " << __FILE__ << "\n"           \
                      << "║ Ligne     : " << __LINE__ << "\n"           \
                      << "╠══ STACK TRACE ═══════════════════════\n"    \
                      << std::stacktrace::current() << "\n"             \
                      << "╚══════════════════════════════════════\n";   \
            std::abort();                                                \
        }                                                                \
    } while (false)

#else
#define ASSERT_WITH_TRACE(condition, message) ((void)0)
#endif
```

Utilisation :

```cpp
void transfer(Account& from, Account& to, double amount) {
    ASSERT_WITH_TRACE(amount > 0,
        "Le montant du transfert doit être positif");
    ASSERT_WITH_TRACE(from.balance() >= amount,
        "Solde insuffisant pour le transfert");

    from.debit(amount);
    to.credit(amount);
}
```

En cas d'échec, la sortie inclut la pile complète, permettant de remonter le chemin d'appel qui a conduit à la violation :

```
╔══ ASSERTION FAILED ══════════════════
║ Condition : from.balance() >= amount
║ Message   : Solde insuffisant pour le transfert
║ Fichier   : /home/dev/project/src/bank.cpp
║ Ligne     : 24
╠══ STACK TRACE ═══════════════════════
 0# transfer(Account&, Account&, double) at bank.cpp:24
 1# BatchProcessor::execute_transfers() at batch.cpp:87
 2# Scheduler::run_daily_batch() at scheduler.cpp:142
 3# main at main.cpp:31
╚══════════════════════════════════════
```

Sans la trace, on sait *quelle* assertion a échoué. Avec la trace, on sait *pourquoi* le code est arrivé là — information souvent cruciale quand une même fonction est appelée depuis de nombreux chemins.

### Vérification d'invariants de classe

Les invariants de classe sont des conditions qui doivent rester vraies tout au long de la vie d'un objet (section 18.1). Quand un invariant est violé, la pile d'appels révèle quelle opération a corrompu l'état :

```cpp
#include <stacktrace>
#include <vector>
#include <iostream>
#include <algorithm>

class SortedBuffer {  
public:  
    void insert(int value) {
        auto pos = std::lower_bound(data_.begin(), data_.end(), value);
        data_.insert(pos, value);
        check_invariant();
    }

    void unsafe_push_back(int value) {
        data_.push_back(value);  // Pas d'insertion triée !
        check_invariant();       // Détecte la violation ici
    }

private:
    std::vector<int> data_;

    void check_invariant() const {
#ifdef DEBUG
        if (!std::is_sorted(data_.begin(), data_.end())) {
            std::cerr << "[INVARIANT VIOLÉ] SortedBuffer n'est plus trié !\n"
                      << "  Contenu: ";
            for (auto v : data_) std::cerr << v << ' ';
            std::cerr << "\n\n  Pile d'appels:\n"
                      << std::stacktrace::current() << '\n';
            std::abort();
        }
#endif
    }
};
```

Si `unsafe_push_back` casse l'ordre de tri, la trace montre exactement qui a appelé cette méthode et depuis quel contexte.

---

## Enrichir les exceptions avec une trace

Une approche particulièrement utile consiste à capturer la pile d'appels au moment où une exception est **lancée**, plutôt qu'au moment où elle est **attrapée**. En effet, lorsque le `catch` s'exécute, la pile a déjà été déroulée (*stack unwinding*) et l'information sur l'origine de l'erreur est perdue.

```cpp
#include <stacktrace>
#include <stdexcept>
#include <string>
#include <iostream>

class TracedException : public std::runtime_error {  
public:  
    explicit TracedException(const std::string& message)
        : std::runtime_error(message)
        , trace_(std::stacktrace::current())
    {}

    const std::stacktrace& trace() const noexcept {
        return trace_;
    }

    void print_diagnostic(std::ostream& os = std::cerr) const {
        os << "Exception : " << what() << "\n\n"
           << "Capturée à :\n" << trace_ << '\n';
    }

private:
    std::stacktrace trace_;
};
```

Utilisation dans une hiérarchie d'erreurs applicatives :

```cpp
class ConfigError : public TracedException {  
public:  
    explicit ConfigError(const std::string& msg)
        : TracedException("Erreur de configuration: " + msg) {}
};

class NetworkError : public TracedException {  
public:  
    explicit NetworkError(const std::string& msg)
        : TracedException("Erreur réseau: " + msg) {}
};

// --- Code applicatif ---

void load_config(const std::string& path) {
    // ... tentative de lecture ...
    throw ConfigError("Fichier '" + path + "' introuvable");
}

void initialize_service() {
    load_config("/etc/myapp/config.yaml");
}

int main() {
    try {
        initialize_service();
    } catch (const TracedException& ex) {
        ex.print_diagnostic();
        return 1;
    }
}
```

Sortie :

```
Exception : Erreur de configuration: Fichier '/etc/myapp/config.yaml' introuvable

Capturée à :
 0# TracedException::TracedException(std::string const&) at traced.h:8
 1# ConfigError::ConfigError(std::string const&) at errors.h:4
 2# load_config(std::string const&) at config.cpp:14
 3# initialize_service() at service.cpp:8
 4# main at main.cpp:5
```

La trace est capturée dans le constructeur de `TracedException`, c'est-à-dire au moment du `throw`. Quand le `catch` dans `main()` exécute `print_diagnostic()`, la pile d'appels originale est intacte, même si le stack unwinding a déjà eu lieu.

---

## Itération sur les frames

`std::stacktrace` se comporte comme un conteneur : on peut itérer sur ses entrées, les filtrer ou les formater selon ses besoins. Chaque `std::stacktrace_entry` expose les méthodes `description()`, `source_file()` et `source_line()` :

```cpp
#include <stacktrace>
#include <iostream>
#include <string>

void log_filtered_trace(const std::stacktrace& trace,
                        const std::string& filter_path) {
    std::cerr << "Trace filtrée (fichiers projet uniquement) :\n";
    int frame = 0;
    for (const auto& entry : trace) {
        // Ne conserver que les frames provenant de notre projet
        std::string file = entry.source_file();
        if (file.find(filter_path) != std::string::npos) {
            std::cerr << "  #" << frame
                      << " " << entry.description()
                      << " at " << file
                      << ":" << entry.source_line() << '\n';
        }
        ++frame;
    }
}
```

Ce filtrage est précieux dans les projets réels : une pile brute peut contenir des dizaines de frames provenant de la bibliothèque standard, du runtime C, ou des bibliothèques tierces. En filtrant sur le chemin du projet, on ne conserve que les frames pertinents pour le diagnostic.

On peut aussi limiter la profondeur de capture pour éviter du bruit inutile :

```cpp
// Capturer au maximum 10 frames, en sautant les 2 premiers
// (utile pour exclure la macro/fonction de capture elle-même)
auto trace = std::stacktrace::current(2, 10);
```

Le premier paramètre (`skip`) indique le nombre de frames à ignorer depuis le sommet de la pile. Le second (`max_depth`) limite le nombre total de frames capturés.

---

## Considérations de performance

La capture d'une trace d'exécution n'est **pas gratuite**. L'opération implique de parcourir les frames de la pile et, pour la résolution des symboles, d'accéder aux informations de débogage du binaire. Cette réalité guide l'usage en débogage défensif.

### Coût typique

L'appel à `std::stacktrace::current()` a un coût de l'ordre de quelques microsecondes à quelques dizaines de microsecondes, selon la profondeur de la pile et la présence d'informations de débogage. C'est négligeable pour un diagnostic ponctuel, mais prohibitif dans une boucle chaude.

### Stratégies de contrôle du coût

**Garder les captures derrière `#ifdef DEBUG`.** C'est la stratégie la plus directe pour le débogage défensif : les traces sont capturées en développement et absentes du binaire de production.

```cpp
#ifdef DEBUG
    #define CAPTURE_TRACE() std::stacktrace::current(1)
#else
    #define CAPTURE_TRACE() std::stacktrace{}
#endif
```

**Capturer uniquement sur les chemins d'erreur.** Même en production, capturer une trace lors d'un événement rare (exception, erreur critique) a un coût acceptable puisque le chemin d'erreur est par définition exceptionnel :

```cpp
void handle_critical_failure(const std::string& reason) {
    // Acceptable en production : cet appel est rare
    auto trace = std::stacktrace::current();
    logger.critical("Défaillance critique: {} | Trace: {}",
                    reason, std::to_string(trace));
    // ... procédure de récupération ...
}
```

**Limiter la profondeur.** Si seuls les quelques premiers frames sont utiles au diagnostic, `current(skip, max_depth)` réduit le travail :

```cpp
// Capturer 5 frames maximum — suffisant pour la plupart des diagnostics
auto trace = std::stacktrace::current(0, 5);
```

---

## Combiner `std::stacktrace` avec les autres outils de débogage

`std::stacktrace` s'inscrit dans un écosystème de débogage défensif plus large. Voici comment il complète les outils abordés dans ce chapitre et les modules liés.

### Avec `assert` et `static_assert` (section 18.1)

`static_assert` opère à la compilation et n'a aucun lien avec l'exécution — `std::stacktrace` ne s'y applique pas. En revanche, les assertions runtime (`assert()`) gagnent considérablement en utilité quand elles sont remplacées par des macros personnalisées capturant la pile, comme montré plus haut avec `ASSERT_WITH_TRACE`.

### Avec la compilation conditionnelle (section 18.2)

Les directives `#ifdef DEBUG` servent naturellement de garde autour des captures de traces, permettant de les éliminer complètement en release :

```cpp
void process(const Data& data) {
#ifdef DEBUG
    auto entry_trace = std::stacktrace::current(0, 3);
    DBG_LOG("Entrée dans process(), appelé depuis:\n" << entry_trace);
#endif
    // ... traitement ...
}
```

### Avec le logging structuré (section 40.1)

En production, les traces capturées sur les chemins d'erreur peuvent être sérialisées dans les logs structurés JSON pour analyse ultérieure :

```cpp
void log_error_with_trace(const std::string& message) {
    auto trace = std::stacktrace::current(1);

    // Construire une représentation sérialisable
    std::string trace_str;
    for (const auto& entry : trace) {
        trace_str += entry.description() + " at "
                   + entry.source_file() + ":"
                   + std::to_string(entry.source_line()) + " | ";
    }

    // Émettre dans le logger structuré (voir section 40.1 / 40.5)
    logger.error(R"({{"error":"{}","stacktrace":"{}"}})",
                 message, trace_str);
}
```

### Avec les sanitizers (section 29.4)

Les sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer) génèrent leurs propres traces lorsqu'ils détectent un problème. `std::stacktrace` ne remplace pas ces outils — il les complète en offrant des traces **à la demande**, là où le développeur décide de les insérer, plutôt qu'uniquement au moment d'un crash ou d'une violation détectée automatiquement.

---

## Bonnes pratiques

**Capturer au moment du `throw`, pas du `catch`.** Le stack unwinding détruit l'information de pile. Stocker la trace dans l'objet exception au moment de sa construction garantit sa disponibilité au point de capture.

**Utiliser `skip` pour nettoyer les frames d'infrastructure.** Les fonctions utilitaires de capture (macros, wrappers) polluent le sommet de la trace. Un `current(1)` ou `current(2)` les élimine et pointe directement sur le code appelant.

**Filtrer les frames en sortie.** Dans un projet utilisant de nombreuses bibliothèques, les traces brutes sont verbeuses. Filtrer par chemin de fichier ou par namespace rend les diagnostics exploitables.

**Garder la capture conditionnelle pour le code chaud.** Pour le code appelé des millions de fois par seconde, réserver les traces aux builds debug. Pour les chemins d'erreur rares, la capture est acceptable même en production.

**Compiler avec `-g` même en release si les traces sont utilisées.** Sans symboles de débogage, les traces ne contiennent que des adresses hexadécimales. L'option `-g` avec `-O2` n'affecte pas la performance du code généré — elle augmente uniquement la taille du binaire. Alternativement, distribuer les symboles dans des fichiers séparés (`.debug` / `debuginfod`) permet de résoudre les adresses a posteriori.

---

## Résumé

| Aspect | Détail |  
|---|---|  
| **Header** | `<stacktrace>` |  
| **Standard** | C++23 |  
| **Capture** | `std::stacktrace::current(skip, max_depth)` |  
| **Itération** | Range-based for sur `std::stacktrace_entry` |  
| **Informations par frame** | `description()`, `source_file()`, `source_line()` |  
| **Coût** | Microsecondes — acceptable sur chemins d'erreur, à éviter en boucle chaude |  
| **Compilation** | `-std=c++23 -g` + linkage bibliothèque de support (GCC : `-lstdc++exp`) |

`std::stacktrace` comble une lacune historique du C++ en matière de débogage. Intégré dans une stratégie de débogage défensif — assertions enrichies, exceptions traçables, vérification d'invariants — il transforme des messages d'erreur opaques en diagnostics exploitables qui montrent non seulement *ce qui* a échoué, mais *comment* le programme est arrivé à ce point de défaillance.

---

> 📎 *Pour la présentation complète de l'API `std::stacktrace` et ses fonctionnalités C++23, voir **section 12.12** (std::stacktrace C++23).*  
>  
> 📎 *Pour l'utilisation de `std::stacktrace` dans le contexte du débogage avancé avec GDB et les sanitizers, voir **section 29.5** (std::stacktrace — traces d'exécution intégrées au débogage).*  
>  
> 📎 *Pour le logging structuré en production, voir **section 40.1** (Logging structuré avec spdlog) et **section 40.5** (JSON logs pour agrégation).*

⏭️ [PARTIE III : PROGRAMMATION SYSTÈME LINUX](/partie-03-programmation-systeme-linux.md)
