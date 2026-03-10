🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.12 std::stacktrace (C++23) : Traces d'exécution standard

## Savoir d'où vient l'appel

Quand un programme plante, la première question est toujours la même : « comment est-on arrivé là ? ». La pile d'appels (*stack trace*, *backtrace*) — la chaîne de fonctions qui ont mené au point courant — est l'information de diagnostic la plus précieuse dont un développeur dispose. Elle transforme un crash anonyme en une histoire lisible : `main` a appelé `process_request`, qui a appelé `parse_json`, qui a appelé `validate_field`, où l'erreur s'est produite.

Pourtant, jusqu'à C++23, obtenir une stack trace en C++ nécessitait des outils non-portables et non-standard : `backtrace()` de la glibc sur Linux, `CaptureStackBackTrace` sur Windows, la bibliothèque Boost.Stacktrace, ou l'inspection post-mortem via un débogueur. Chaque plateforme avait sa propre API, ses propres limitations, et son propre format de sortie.

C++23 standardise cette fonctionnalité avec `std::stacktrace` et `std::stacktrace_entry` (header `<stacktrace>`). Pour la première fois, un programme C++ peut capturer et afficher sa propre pile d'appels de manière portable, sans dépendance tierce, avec un code qui compile identiquement sur Linux, Windows et macOS.

> 📎 *Cette section présente `std::stacktrace` dans le contexte des nouveautés C++23. Pour son intégration dans le débogage défensif, voir section 18.4. Pour son usage avancé dans les workflows de débogage, voir section 29.5.*

## Capture et affichage basiques

### Capturer la pile courante

```cpp
#include <stacktrace>
#include <print>

void inner_function() {
    auto trace = std::stacktrace::current();
    std::print("Pile d'appels :\n{}\n", trace);
}

void middle_function() {
    inner_function();
}

int main() {
    middle_function();
}
```

Sortie typique (avec symboles de debug) :

```
Pile d'appels :
 0# inner_function() at main.cpp:5
 1# middle_function() at main.cpp:9
 2# main at main.cpp:13
```

`std::stacktrace::current()` capture un instantané de la pile d'appels au moment précis de l'appel. L'objet retourné est une valeur — il peut être stocké, copié, transmis à un logger, sérialisé, comparé. La capture est indépendante de l'affichage : on peut capturer la trace à un endroit et l'afficher ou la loguer bien plus tard.

### Compatibilité avec std::format et std::print

`std::stacktrace` et `std::stacktrace_entry` sont directement formattables avec `std::format` et `std::print` (section 12.7). La spécialisation de `std::formatter` est fournie par la bibliothèque standard :

```cpp
#include <stacktrace>
#include <format>
#include <string>

auto trace = std::stacktrace::current();

// Formatage dans un string
std::string trace_str = std::format("{}", trace);

// Affichage direct
std::print(stderr, "Stack trace:\n{}\n", trace);

// Formatage d'une entrée individuelle
for (const auto& entry : trace) {
    std::print("  → {}\n", entry);
}
```

Cette intégration avec le système de formatage standard signifie que les stack traces s'insèrent naturellement dans les messages de log, les rapports d'erreur, et les diagnostics — sans conversion manuelle en chaîne.

## Anatomie de std::stacktrace

### std::stacktrace : la pile complète

`std::stacktrace` est un conteneur de `std::stacktrace_entry`. Il se comporte comme une séquence en lecture seule, avec une interface familière :

```cpp
#include <stacktrace>
#include <print>

auto trace = std::stacktrace::current();

trace.size();    // Nombre de frames dans la pile  
trace.empty();   // true si aucune frame capturée  

trace[0];        // Première entrée (le point de capture)  
trace[1];        // L'appelant du point de capture  
// ...

trace.begin();   // Itérateur de début  
trace.end();     // Itérateur de fin  

// Itération standard
for (const std::stacktrace_entry& frame : trace) {
    std::print("{}\n", frame);
}
```

L'entrée à l'index 0 est la frame la plus récente (le point de capture). Les index croissants remontent vers les appelants, jusqu'à `main` (ou le point d'entrée du thread).

### std::stacktrace_entry : une frame individuelle

Chaque entrée de la pile fournit des informations sur une frame d'appel :

```cpp
#include <stacktrace>
#include <print>

auto trace = std::stacktrace::current();

for (const auto& entry : trace) {
    // Description lisible de la frame
    std::string desc = entry.description();    // ex: "inner_function()"
    
    // Fichier source (si disponible)
    std::string file = entry.source_file();     // ex: "main.cpp"
    
    // Ligne source (si disponible)
    uint_least32_t line = entry.source_line();  // ex: 5
    
    // Adresse native de la frame
    // entry.native_handle();  // Adresse brute (implémentation-dépendante)
    
    std::print("  {} ({}:{})\n", desc, file, line);
}
```

Les méthodes `description()`, `source_file()` et `source_line()` retournent des chaînes vides ou zéro si l'information n'est pas disponible — par exemple quand le binaire est compilé sans symboles de debug, ou pour les frames de bibliothèques système.

## Contrôler la capture

### Limiter la profondeur

`std::stacktrace::current()` accepte deux paramètres optionnels pour contrôler la capture :

```cpp
// Signature complète
static std::stacktrace current(size_t skip = 0, size_t max_depth = /* max */) noexcept;
```

Le paramètre `skip` omet les N premières frames (les plus récentes). C'est utile pour exclure les fonctions d'infrastructure de la trace :

```cpp
void log_error(const std::string& message) {
    // skip=1 : exclure log_error elle-même de la trace
    auto trace = std::stacktrace::current(1);
    std::print(stderr, "ERREUR: {}\nStack trace:\n{}\n", message, trace);
}

void process() {
    log_error("quelque chose a mal tourné");
    // La trace commence à process(), pas à log_error()
}
```

Le paramètre `max_depth` limite le nombre de frames capturées. C'est utile pour le logging haute fréquence où seules les frames les plus proches sont pertinentes :

```cpp
// Capturer seulement les 5 frames les plus récentes, en sautant la frame courante
auto trace = std::stacktrace::current(1, 5);
```

### Capture et noexcept

`std::stacktrace::current()` est `noexcept`. Si la capture échoue (mémoire insuffisante, plateforme non supportée), elle retourne un stacktrace vide plutôt que de lancer une exception. C'est une propriété essentielle pour l'utilisation dans les chemins d'erreur — on ne veut pas qu'une tentative de diagnostic provoque une nouvelle erreur.

## Cas d'usage pratiques

### Enrichir les messages d'exception

Un pattern puissant consiste à capturer la stack trace au moment de la construction d'une exception et à l'embarquer dans l'objet exception :

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
        , trace_(std::stacktrace::current(1))  // skip traced_error()
    {}

    const std::stacktrace& trace() const noexcept { return trace_; }

    std::string full_report() const {
        return std::format("Error: {}\nStack trace:\n{}", what(), trace_);
    }
};

// Utilisation
void validate(int value) {
    if (value < 0) {
        throw traced_error(std::format("Valeur négative: {}", value));
    }
}

int main() {
    try {
        validate(-1);
    } catch (const traced_error& e) {
        std::print(stderr, "{}\n", e.full_report());
    }
}
```

Ce pattern résout un problème classique des exceptions C++ : quand on attrape une exception dans un `catch`, la pile d'appels a déjà été déroulée — on ne sait plus d'où venait l'exception. En capturant la trace au `throw`, on préserve cette information.

### Enrichir std::expected avec des traces

Le même principe s'applique à `std::expected` (section 12.8). On peut créer un type d'erreur qui embarque la trace de son point de création :

```cpp
#include <stacktrace>
#include <expected>
#include <string>
#include <format>

struct DiagnosticError {
    std::string message;
    std::stacktrace trace;

    static DiagnosticError make(std::string msg) {
        return {std::move(msg), std::stacktrace::current(1)};
    }
};

std::expected<Config, DiagnosticError> load_config(const std::string& path) {
    auto content = read_file(path);
    if (!content) {
        return std::unexpected(
            DiagnosticError::make(std::format("Cannot read '{}'", path)));
    }
    // ...
}

// Côté appelant : diagnostic riche sans exception
if (auto cfg = load_config("app.yaml"); !cfg) {
    std::print(stderr, "Erreur: {}\nOrigine:\n{}\n", 
               cfg.error().message, cfg.error().trace);
}
```

Ce pattern combine la gestion d'erreurs explicite de `std::expected` avec la richesse diagnostique des stack traces — le meilleur des deux mondes.

### Logging structuré

Dans un système d'observabilité (section 40), les stack traces enrichissent les logs structurés avec le contexte d'exécution :

```cpp
#include <stacktrace>
#include <print>
#include <format>

enum class LogLevel { Trace, Debug, Info, Warning, Error };

void log(LogLevel level, const std::string& message) {
    std::string trace_str;
    if (level >= LogLevel::Error) {
        // Capturer la trace uniquement pour les erreurs (coût non nul)
        auto trace = std::stacktrace::current(1, 10);
        trace_str = std::format("{}", trace);
    }

    std::print(stderr, "{{\"level\":\"{}\",\"message\":\"{}\",\"stacktrace\":\"{}\"}}\n",
               /* level string */"ERROR", message, trace_str);
}
```

La trace est capturée conditionnellement — seulement pour les niveaux d'erreur — pour limiter l'impact sur les performances.

### Assertions avec contexte

`std::stacktrace` rend les assertions maison bien plus utiles que le `assert()` standard, qui ne fournit que le fichier et la ligne :

```cpp
#include <stacktrace>
#include <print>
#include <source_location>

void assert_that(bool condition, 
                 const char* expr,
                 std::source_location loc = std::source_location::current()) {
    if (!condition) {
        auto trace = std::stacktrace::current(1);
        std::print(stderr,
            "Assertion failed: {}\n"
            "  at {}:{}:{}\n"
            "Stack trace:\n{}\n",
            expr, loc.file_name(), loc.line(), loc.column(), trace);
        std::abort();
    }
}

#define ASSERT(expr) assert_that(static_cast<bool>(expr), #expr)

void process(int* data, size_t size) {
    ASSERT(data != nullptr);    // Si échoue : trace complète
    ASSERT(size > 0);
}
```

La combinaison de `std::source_location` (C++20) et `std::stacktrace` (C++23) fournit à la fois le point exact de l'échec et la chaîne d'appels qui y a mené.

## Performance et coût

La capture d'une stack trace n'est pas gratuite. Il est important de comprendre les coûts pour utiliser cette fonctionnalité de manière appropriée.

### Coût de la capture

La capture (`std::stacktrace::current()`) implique la traversée des frames de la pile et la résolution des symboles (nom de fonction, fichier, ligne). Le coût dépend de la profondeur de la pile et de la plateforme :

- **Traversée de la pile** — Typiquement quelques microsecondes pour une pile de 10-20 frames. Le mécanisme utilise les informations de déroulement (`.eh_frame` sur Linux) ou les API système.
- **Résolution des symboles** — C'est la partie la plus coûteuse. Traduire une adresse en nom de fonction et en position source peut prendre des dizaines de microsecondes, voire des millisecondes si les symboles sont chargés depuis un fichier externe.

En ordre de grandeur, capturer et résoudre une stack trace de profondeur typique (10-20 frames) prend entre **1 et 100 microsecondes** selon l'implémentation et la plateforme.

### Résolution paresseuse

Certaines implémentations adoptent une stratégie de résolution paresseuse : `current()` ne capture que les adresses brutes (rapide), et la résolution en noms/fichiers/lignes n'est effectuée que lors de l'appel à `description()`, `source_file()` ou `source_line()`. Cela permet de capturer à faible coût et de ne payer la résolution que si la trace est effectivement affichée — ce qui, pour les erreurs, est acceptable.

### Recommandations de performance

**Ne pas capturer dans les boucles chaudes.** Une capture par itération dans une boucle à haute fréquence est prohibitive. Réserver la capture aux chemins d'erreur et de diagnostic.

**Utiliser `max_depth` pour limiter le coût.** Si seules les 5 frames les plus proches sont utiles, `current(0, 5)` réduit le coût proportionnellement.

**Capturer conditionnellement.** Vérifier le niveau de log ou la condition d'erreur avant de capturer :

```cpp
if (should_log_trace()) {
    auto trace = std::stacktrace::current();
    // ...
}
```

**Capturer tôt, résoudre tard.** Si l'implémentation supporte la résolution paresseuse, stocker le stacktrace en tant qu'objet et ne le formater que lorsqu'il est effectivement nécessaire (affichage, sérialisation).

## Compilation et symboles de debug

Pour que les stack traces soient lisibles (noms de fonctions, fichiers, lignes), le binaire doit contenir des informations de debug. Sans symboles, les traces ne montrent que des adresses mémoire brutes.

### Avec GCC et Clang

```bash
# Compilation avec symboles de debug (indispensable)
g++ -std=c++23 -g -o app main.cpp -lstdc++_libbacktrace

# Symboles de debug + optimisations (production avec diagnostic)
g++ -std=c++23 -O2 -g -o app main.cpp -lstdc++_libbacktrace
```

Le flag `-g` est nécessaire pour que `source_file()` et `source_line()` retournent des informations utiles. L'option `-O2 -g` combine optimisations et symboles — les noms de fonctions seront présents mais certaines frames peuvent être manquantes à cause de l'inlining.

La bibliothèque de liaison (`-lstdc++_libbacktrace` pour GCC, `-lstdc++exp` ou équivalent selon la version) fournit l'implémentation de la résolution de symboles. Vérifier la documentation de votre version du compilateur pour la bibliothèque exacte.

### Symboles séparés

En production, on peut compiler avec des symboles séparés pour ne pas gonfler le binaire déployé tout en gardant la possibilité de résoudre les traces :

```bash
# Compiler avec symboles
g++ -std=c++23 -O2 -g -o app main.cpp -lstdc++_libbacktrace

# Extraire les symboles dans un fichier séparé
objcopy --only-keep-debug app app.debug  
strip --strip-debug app  

# Déployer 'app' (léger) — garder 'app.debug' pour le diagnostic
```

## Portabilité et état du support

`std::stacktrace` est standardisé en C++23, mais le niveau de support varie selon les compilateurs et les plateformes en mars 2026 :

- **GCC 14+** — Support complet via libstdc++ avec `libbacktrace` comme backend. Nécessite `-lstdc++_libbacktrace` ou `-lstdc++exp` à l'édition de liens.
- **Clang/libc++** — Support en cours de maturation. Vérifier la version de libc++ et les flags de liaison requis.
- **MSVC** — Support natif via les API Windows (`DbgHelp`).

Sur les plateformes où le support natif est incomplet, la bibliothèque **Boost.Stacktrace** reste une alternative portable et mature avec une API similaire. La migration de Boost.Stacktrace vers `std::stacktrace` est quasi directe quand le support standard est disponible.

## Bonnes pratiques

**Embarquer une stack trace dans les types d'erreur.** Que ce soit dans des exceptions personnalisées ou dans des types d'erreur pour `std::expected`, la trace capturée au point d'origine de l'erreur est l'information de diagnostic la plus précieuse.

**Capturer avec `skip` pour exclure l'infrastructure.** Les fonctions de logging, d'assertion ou de création d'erreur ne sont pas pertinentes dans la trace. `current(1)` ou `current(2)` élimine ces frames et démarre la trace là où l'erreur a réellement été détectée.

**Réserver les traces aux chemins d'erreur.** Le coût de capture n'est pas anodin. Les traces sont un outil de diagnostic, pas d'instrumentation systématique. Capturer uniquement quand une erreur est détectée ou quand le niveau de log le justifie.

**Compiler avec `-g` même en mode release.** Le surcoût en taille de binaire est modéré et la capacité de produire des traces lisibles en production est inestimable. Pour les contraintes de taille, les symboles séparés offrent le meilleur compromis.

**Formater avec `std::print` et `std::format`.** L'intégration native avec le système de formatage rend l'affichage des traces trivial et cohérent avec le reste du code.

---

>  
> 📎 [18.4 std::stacktrace dans le débogage défensif](/18-assertions-debug/04-stacktrace.md)  
>  
> 📎 [29.5 Traces d'exécution intégrées au débogage](/29-debogage/05-stacktrace-debug.md)

⏭️ [Modules (C++20) : Concept et état en 2026](/12-nouveautes-cpp17-26/13-modules.md)
