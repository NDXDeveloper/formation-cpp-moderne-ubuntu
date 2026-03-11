🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 18.3 Logging et traces d'exécution

## Introduction

Les assertions (section 18.1) détectent les violations de contrat et arrêtent le programme. La compilation conditionnelle (section 18.2) permet d'inclure ou d'exclure du code de diagnostic. Mais entre ces deux extrêmes, il existe un besoin fondamental : **observer le comportement d'un programme pendant son exécution sans l'interrompre**.

C'est le rôle du *logging* — l'enregistrement de messages décrivant ce que fait le programme, quand, et dans quel état. Une bonne stratégie de logging est souvent ce qui fait la différence entre un bug résolu en cinq minutes et un bug qui hante une équipe pendant des jours.

Cette section couvre les fondamentaux du logging pour le débogage en C++. Elle pose les bases nécessaires avant d'aborder le logging structuré en production avec `spdlog` (section 40.1).

---

## Pourquoi logger ?

Un débogueur comme GDB (section 29.1) est un outil puissant, mais il présente des limites concrètes. Il ralentit considérablement l'exécution, ce qui peut faire disparaître certains bugs liés au timing. Il est difficilement utilisable dans un contexte multi-thread, où le simple fait de poser un breakpoint modifie l'ordonnancement des threads. Il est inutilisable dans un environnement de production, dans un conteneur Docker sans accès interactif, ou sur un système embarqué distant.

Le logging complète le débogueur interactif en offrant une observation **non intrusive** et **persistante** du comportement du programme. Un fichier de log peut être analysé après coup, partagé avec un collègue, comparé entre deux exécutions, ou filtré automatiquement par un outil.

Les cas d'usage typiques du logging en développement sont les suivants :

- **Tracer le flux d'exécution** : savoir quelles fonctions sont appelées et dans quel ordre.  
- **Observer l'état interne** : valeurs de variables, taille de conteneurs, résultats intermédiaires.  
- **Diagnostiquer les erreurs** : comprendre le contexte qui a mené à un crash ou à un résultat incorrect.  
- **Mesurer les performances** : horodater des étapes pour identifier les goulots d'étranglement.  
- **Déboguer les problèmes intermittents** : capturer des informations sur des bugs qui ne se reproduisent pas à la demande.

---

## Approches naïves et leurs limites

### `std::cout` : le premier réflexe

Le réflexe naturel du débutant est d'insérer des `std::cout` dans le code :

```cpp
#include <iostream>
#include <vector>

int compute_sum(const std::vector<int>& data) {
    std::cout << "compute_sum appelée, taille = " << data.size() << std::endl;
    int sum = 0;
    for (auto val : data) {
        sum += val;
        std::cout << "val = " << val << ", sum = " << sum << std::endl;
    }
    std::cout << "compute_sum retourne " << sum << std::endl;
    return sum;
}
```

Cette approche fonctionne pour un programme de quelques dizaines de lignes, mais elle s'effondre rapidement pour plusieurs raisons.

**Mélange des sorties.** Les messages de débogage et la sortie légitime du programme se retrouvent entrelacés sur `stdout`. Il devient impossible de distinguer les uns des autres, et rediriger la sortie du programme dans un fichier ou un pipe capture aussi le bruit de débogage.

**Pas de métadonnées.** Aucune information sur l'horodatage, le fichier source, le numéro de ligne ou la sévérité du message. Quand le programme produit des centaines de lignes de sortie, retrouver l'origine d'un message devient un casse-tête.

**Pas de filtrage.** Tous les messages s'affichent avec la même importance. On ne peut pas activer les traces pour un module spécifique tout en réduisant le bruit des autres.

**Impact sur les performances.** `std::endl` force un flush du buffer à chaque appel, ce qui est considérablement plus lent que `'\n'`. Dans une boucle serrée, cela peut modifier le comportement temporel du programme au point de masquer ou révéler des bugs.

**Nettoyage fastidieux.** Avant chaque commit, il faut retrouver et supprimer tous les `std::cout` de débogage — un processus manuel et source d'erreurs.

### `std::cerr` : un pas dans la bonne direction

Utiliser `std::cerr` au lieu de `std::cout` résout le premier problème : les messages de débogage vont sur `stderr`, séparé de `stdout`. On peut ainsi rediriger les deux flux indépendamment :

```bash
# La sortie normale va dans output.txt, les traces dans debug.log
./app > output.txt 2> debug.log
```

De plus, `std::cerr` est non bufferisé par défaut — chaque écriture est immédiatement transmise. C'est un avantage pour le débogage (pas de message perdu en cas de crash), mais un inconvénient en termes de performance.

`std::clog` est une alternative bufferisée qui écrit aussi sur `stderr`, offrant un meilleur compromis pour les traces volumineuses :

```cpp
// Non bufferisé — chaque message est immédiatement écrit
std::cerr << "[ERROR] Connexion perdue\n";

// Bufferisé — meilleure performance pour les traces fréquentes
std::clog << "[TRACE] Itération " << i << " terminée\n";
```

Cependant, ces flux restent limités : ni horodatage, ni sévérité, ni filtrage.

---

## Construire un système de logging minimal

Avant d'adopter une bibliothèque tierce, il est instructif de construire un logger élémentaire. Cela permet de comprendre les mécanismes sous-jacents et les décisions de conception qui se posent.

### Niveaux de sévérité

Tout système de logging repose sur une hiérarchie de niveaux de sévérité. Chaque message est associé à un niveau, et le logger peut être configuré pour ne laisser passer que les messages au-dessus d'un certain seuil :

```cpp
#pragma once

#include <iostream>
#include <string_view>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <source_location>

enum class LogLevel {
    Trace,    // Détails fins — flux d'exécution pas à pas
    Debug,    // Informations utiles au développeur
    Info,     // Événements normaux significatifs
    Warning,  // Situations anormales mais gérées
    Error,    // Erreurs qui empêchent une opération
    Fatal     // Erreurs irrécupérables avant arrêt du programme
};

constexpr std::string_view level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
    }
    return "?????";
}
```

La convention de nommage et le nombre de niveaux varient selon les frameworks, mais la hiérarchie Trace < Debug < Info < Warning < Error < Fatal est quasi universelle.

### Un logger simple et thread-safe

```cpp
class SimpleLogger {  
public:  
    static SimpleLogger& instance() {
        static SimpleLogger logger;
        return logger;
    }

    void set_level(LogLevel level) { min_level_ = level; }
    void set_output(std::ostream& os) { output_ = &os; }

    void log(LogLevel level,
             std::string_view message,
             const std::source_location& loc
                 = std::source_location::current())
    {
        if (level < min_level_) return;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        // Le mutex garantit que les lignes ne s'entrelacent pas
        // entre threads
        std::lock_guard<std::mutex> lock(mutex_);

        *output_ << std::put_time(std::localtime(&time), "%H:%M:%S")
                 << '.' << std::setfill('0') << std::setw(3) << ms.count()
                 << " [" << level_to_string(level) << "] "
                 << extract_filename(loc.file_name())
                 << ':' << loc.line() << " — "
                 << message << '\n';
    }

private:
    SimpleLogger() = default;

    static std::string_view extract_filename(const char* path) {
        std::string_view sv(path);
        auto pos = sv.find_last_of("/\\");
        return (pos != std::string_view::npos) ? sv.substr(pos + 1) : sv;
    }

    LogLevel      min_level_ = LogLevel::Trace;
    std::ostream* output_    = &std::cerr;
    std::mutex    mutex_;
};
```

Quelques décisions de conception méritent d'être commentées.

**`std::source_location` (C++20).** Ce type remplace les macros `__FILE__` et `__LINE__` par une alternative intégrée au langage. Utilisé comme paramètre par défaut, il capture automatiquement le site d'appel sans que l'appelant ne doive rien fournir.

**Mutex.** En environnement multi-thread (section 21), plusieurs threads peuvent logger simultanément. Sans synchronisation, les messages s'entrelacent — on obtient des lignes illisibles mêlant des fragments de messages différents. Le `std::lock_guard` garantit que chaque message est écrit atomiquement.

**Singleton.** Le pattern singleton (via une variable `static` locale) assure qu'il existe un unique point de configuration pour le logging. Ce choix est discutable dans une architecture complexe, mais adapté à un logger de débogage.

### Macros d'interface

Pour rendre l'utilisation fluide et permettre la désactivation complète en release, on définit des macros d'interface :

```cpp
#ifdef DEBUG
    #define LOG_TRACE(msg)   SimpleLogger::instance().log(LogLevel::Trace, msg)
    #define LOG_DEBUG(msg)   SimpleLogger::instance().log(LogLevel::Debug, msg)
    #define LOG_INFO(msg)    SimpleLogger::instance().log(LogLevel::Info, msg)
    #define LOG_WARN(msg)    SimpleLogger::instance().log(LogLevel::Warning, msg)
    #define LOG_ERROR(msg)   SimpleLogger::instance().log(LogLevel::Error, msg)
    #define LOG_FATAL(msg)   SimpleLogger::instance().log(LogLevel::Fatal, msg)
#else
    #define LOG_TRACE(msg)   ((void)0)
    #define LOG_DEBUG(msg)   ((void)0)
    #define LOG_INFO(msg)    ((void)0)
    #define LOG_WARN(msg)    ((void)0)
    #define LOG_ERROR(msg)   ((void)0)
    #define LOG_FATAL(msg)   ((void)0)
#endif
```

> ⚠️ **Note.** En production, on souhaite généralement conserver les niveaux `Warning`, `Error` et `Fatal` même en release. La désactivation totale montrée ici est adaptée à un pur logger de débogage. Pour le logging en production, une bibliothèque comme `spdlog` (section 40.1) offre un filtrage par niveau à l'exécution sans recompilation.

### Utilisation

```cpp
#include "simple_logger.h"
#include <vector>

void load_config(const std::string& path) {
    LOG_INFO("Chargement de la configuration");
    LOG_DEBUG("Chemin: " + path);

    // ... lecture du fichier ...

    if (/* fichier absent */) {
        LOG_WARN("Fichier non trouvé, utilisation des valeurs par défaut");
        return;
    }

    LOG_INFO("Configuration chargée avec succès");
}

int main() {
    SimpleLogger::instance().set_level(LogLevel::Debug);

    LOG_INFO("Démarrage de l'application");
    load_config("/etc/app/config.yaml");
    LOG_INFO("Arrêt normal");
}
```

Sortie typique :

```
14:32:07.142 [INFO ] main.cpp:18 — Démarrage de l'application
14:32:07.142 [INFO ] main.cpp:8 — Chargement de la configuration
14:32:07.142 [DEBUG] main.cpp:9 — Chemin: /etc/app/config.yaml
14:32:07.143 [WARN ] main.cpp:13 — Fichier non trouvé, utilisation des valeurs par défaut
14:32:07.143 [INFO ] main.cpp:20 — Arrêt normal
```

Chaque ligne contient l'horodatage à la milliseconde, le niveau de sévérité, le fichier source et la ligne, puis le message. Ce format permet de reconstituer le flux d'exécution complet.

---

## `std::print` et `std::format` pour le logging (C++23)

Depuis C++23, `std::print` et `std::format` (section 12.7) offrent un formatage type-safe et performant qui remplace avantageusement les flux `<<` pour la construction de messages de log :

```cpp
#include <print>
#include <format>
#include <string>
#include <vector>

void process_batch(const std::vector<std::string>& items) {
    // std::format construit la chaîne sans l'envoyer sur un flux
    auto msg = std::format("Traitement du batch : {} éléments", items.size());
    LOG_INFO(msg);

    for (std::size_t i = 0; i < items.size(); ++i) {
        LOG_TRACE(std::format("  [{}] → {}", i, items[i]));
    }

    // std::print écrit directement sur stderr
    // (utile pour un diagnostic rapide sans logger)
    std::print(stderr, "[DIAG] Batch terminé, {} éléments traités\n",
               items.size());
}
```

Les avantages de `std::format` par rapport à la concaténation par `+` ou aux flux `<<` sont significatifs. Le formatage est vérifié à la compilation (type-safety), la syntaxe est plus lisible avec les placeholders `{}`, et les performances sont meilleures que `std::ostringstream` pour construire des chaînes dynamiquement.

---

## Traces d'exécution : observer le flux de contrôle

Au-delà des messages ponctuels, il est souvent utile de tracer **l'entrée et la sortie** des fonctions pour comprendre le cheminement du programme. On peut automatiser cela avec un objet RAII (section 6.3) :

```cpp
#include <string_view>
#include <source_location>

class ScopeTracer {  
public:  
    explicit ScopeTracer(
        std::string_view name,
        const std::source_location& loc = std::source_location::current())
        : name_(name), loc_(loc)
    {
        SimpleLogger::instance().log(
            LogLevel::Trace,
            std::string("→ ENTER ") + std::string(name_),
            loc_);
    }

    ~ScopeTracer() {
        SimpleLogger::instance().log(
            LogLevel::Trace,
            std::string("← EXIT  ") + std::string(name_),
            loc_);
    }

    // Non copiable, non déplaçable
    ScopeTracer(const ScopeTracer&) = delete;
    ScopeTracer& operator=(const ScopeTracer&) = delete;

private:
    std::string_view     name_;
    std::source_location loc_;
};

// Macros auxiliaires pour générer des noms de variables uniques.
// Le double niveau d'indirection est nécessaire car ## empêche
// l'expansion de __LINE__ — CONCAT force l'expansion avant la concaténation.
#define DETAIL_CONCAT(a, b) a##b
#define CONCAT(a, b) DETAIL_CONCAT(a, b)

#ifdef DEBUG
    #define TRACE_SCOPE(name) ScopeTracer CONCAT(_tracer_, __LINE__)(name)
    #define TRACE_FUNCTION()  ScopeTracer _tracer_func_(__func__)
#else
    #define TRACE_SCOPE(name) ((void)0)
    #define TRACE_FUNCTION()  ((void)0)
#endif
```

Le principe est simple : à l'entrée d'un scope, le constructeur de `ScopeTracer` émet un message `ENTER`. À la sortie — qu'elle soit normale, par `return`, ou par exception — le destructeur émet un message `EXIT`. Le mécanisme RAII garantit que la trace de sortie est toujours émise.

### Utilisation

```cpp
void authenticate(const std::string& user) {
    TRACE_FUNCTION();
    LOG_DEBUG(std::format("Authentification de l'utilisateur '{}'", user));

    {
        TRACE_SCOPE("vérification mot de passe");
        // ... vérification ...
    }

    {
        TRACE_SCOPE("chargement permissions");
        // ... chargement ...
    }
}
```

Sortie :

```
14:32:07.200 [TRACE] auth.cpp:2 — → ENTER authenticate
14:32:07.200 [DEBUG] auth.cpp:3 — Authentification de l'utilisateur 'alice'
14:32:07.201 [TRACE] auth.cpp:6 — → ENTER vérification mot de passe
14:32:07.203 [TRACE] auth.cpp:6 — ← EXIT  vérification mot de passe
14:32:07.203 [TRACE] auth.cpp:11 — → ENTER chargement permissions
14:32:07.205 [TRACE] auth.cpp:11 — ← EXIT  chargement permissions
14:32:07.205 [TRACE] auth.cpp:2 — ← EXIT  authenticate
```

Cette trace révèle immédiatement l'ordre d'exécution, la profondeur d'imbrication et le temps passé dans chaque section. Pour un problème de performance, la différence de timestamps entre `ENTER` et `EXIT` donne un premier repère.

---

## Ajouter du chronométrage aux traces

Pour mesurer la durée d'exécution d'un scope, on étend le `ScopeTracer` avec un chronomètre :

```cpp
#include <chrono>
#include <format>

class TimedScope {  
public:  
    explicit TimedScope(
        std::string_view name,
        const std::source_location& loc = std::source_location::current())
        : name_(name)
        , loc_(loc)
        , start_(std::chrono::steady_clock::now())
    {
        SimpleLogger::instance().log(LogLevel::Debug,
            std::format("⏱ START {}", name_), loc_);
    }

    ~TimedScope() {
        auto elapsed = std::chrono::steady_clock::now() - start_;
        auto us = std::chrono::duration_cast<
            std::chrono::microseconds>(elapsed).count();

        SimpleLogger::instance().log(LogLevel::Debug,
            std::format("⏱ END   {} — {} µs", name_, us), loc_);
    }

    TimedScope(const TimedScope&) = delete;
    TimedScope& operator=(const TimedScope&) = delete;

private:
    std::string_view                            name_;
    std::source_location                        loc_;
    std::chrono::steady_clock::time_point       start_;
};

#ifdef DEBUG
    #define TIME_SCOPE(name) TimedScope CONCAT(_timer_, __LINE__)(name)
#else
    #define TIME_SCOPE(name) ((void)0)
#endif
```

```cpp
void process_image(const Image& img) {
    TIME_SCOPE("process_image");

    {
        TIME_SCOPE("décodage");
        // ... décodage ...
    }
    {
        TIME_SCOPE("filtrage");
        // ... application de filtres ...
    }
    {
        TIME_SCOPE("encodage");
        // ... encodage ...
    }
}
```

Sortie :

```
14:32:08.500 [DEBUG] image.cpp:2 — ⏱ START process_image
14:32:08.500 [DEBUG] image.cpp:5 — ⏱ START décodage
14:32:08.523 [DEBUG] image.cpp:5 — ⏱ END   décodage — 23042 µs
14:32:08.523 [DEBUG] image.cpp:9 — ⏱ START filtrage
14:32:08.541 [DEBUG] image.cpp:9 — ⏱ END   filtrage — 17891 µs
14:32:08.541 [DEBUG] image.cpp:13 — ⏱ START encodage
14:32:08.558 [DEBUG] image.cpp:13 — ⏱ END   encodage — 16733 µs
14:32:08.558 [DEBUG] image.cpp:2 — ⏱ END   process_image — 57788 µs
```

> 💡 **Note.** Ce chronométrage est utile pour un premier diagnostic, mais pour des mesures de performance rigoureuses, un micro-benchmarking framework comme Google Benchmark (section 35) ou un profiler comme `perf` (section 31.1) donnent des résultats bien plus fiables.

---

## Rediriger les logs vers un fichier

En phase de débogage, écrire les logs dans un fichier offre plusieurs avantages : on peut les analyser après l'exécution, les filtrer avec `grep`, les comparer entre deux runs, ou les partager. Le logger minimal présenté plus haut accepte n'importe quel `std::ostream` :

```cpp
#include <fstream>

int main() {
    // Écrire les logs dans un fichier
    std::ofstream log_file("debug.log", std::ios::app);
    if (log_file.is_open()) {
        SimpleLogger::instance().set_output(log_file);
    }

    SimpleLogger::instance().set_level(LogLevel::Trace);

    LOG_INFO("Démarrage — logs redirigés vers debug.log");
    // ... exécution ...
}
```

On peut aussi exploiter la redirection shell sans modifier le code, puisque le logger écrit sur `stderr` par défaut :

```bash
# Rediriger stderr vers un fichier
./app 2> debug.log

# Voir les logs en temps réel ET les sauvegarder
./app 2>&1 | tee debug.log

# Filtrer les logs à la volée
./app 2>&1 | grep "\[ERROR\]"

# Ne garder que les logs d'un fichier source précis
./app 2>&1 | grep "network.cpp"
```

---

## Considérations sur la performance

Le logging a un coût, et ce coût peut devenir significatif dans du code exécuté à haute fréquence.

### Éviter le travail inutile

Le filtrage par niveau doit intervenir **avant** la construction du message, pas après. Considérons ces deux approches :

```cpp
// ❌ Mauvais : le std::format est exécuté même si le message sera filtré
void log_bad(LogLevel level, const std::string& message) {
    if (level < min_level_) return;  // Trop tard, message déjà construit
    // ...
}
// Appel :
log_bad(LogLevel::Trace, std::format("état = {}", compute_state()));

// ✅ Bon : vérifier le niveau avant de construire le message
if (SimpleLogger::instance().should_log(LogLevel::Trace)) {
    SimpleLogger::instance().log(LogLevel::Trace,
        std::format("état = {}", compute_state()));
}
```

Les bibliothèques de logging professionnelles comme `spdlog` gèrent cela en interne avec des macros ou des templates qui court-circuitent la construction du message quand le niveau est insuffisant.

### `std::endl` vs `'\n'`

Ce point est souvent répété mais reste une source fréquente de lenteur inutile :

```cpp
// ❌ Lent : std::endl force un flush système à chaque ligne
std::cerr << "[DEBUG] message" << std::endl;

// ✅ Rapide : '\n' écrit un saut de ligne sans forcer le flush
std::cerr << "[DEBUG] message\n";
```

Pour le débogage, un flush explicite n'est nécessaire que juste avant une opération qui risque de faire crasher le programme. Dans ce cas, on peut utiliser `std::flush` ponctuellement.

### Compilation conditionnelle pour le coût zéro

Comme vu en section 18.2, la stratégie la plus efficace est d'éliminer purement et simplement le code de logging en release via `#ifdef` :

```cpp
#ifdef DEBUG
    #define LOG_TRACE(msg) SimpleLogger::instance().log(LogLevel::Trace, msg)
#else
    #define LOG_TRACE(msg) ((void)0)  // Coût nul — rien n'est compilé
#endif
```

En release, `LOG_TRACE("message")` se compile en rien. Aucune construction de chaîne, aucun appel de fonction, aucun test de condition.

---

## Quand passer à une bibliothèque professionnelle

Le logger minimal présenté ici est parfaitement adapté à l'apprentissage et aux petits projets. Mais dès que le projet grandit, ses limites se manifestent. Il ne gère pas l'écriture dans plusieurs destinations simultanément (*sinks* multiples : fichier, console, réseau). Il n'offre pas de rotation automatique des fichiers de log quand ceux-ci deviennent volumineux. Il ne supporte pas le logging asynchrone — le thread appelant est bloqué pendant l'écriture. Son format de sortie est rigide et ne permet pas le logging structuré en JSON pour l'agrégation par des outils comme Elasticsearch ou Loki.

C'est le moment d'adopter une bibliothèque comme **spdlog**, qui est aujourd'hui le standard de facto pour le logging en C++. Elle offre toutes ces fonctionnalités avec des performances remarquables et une API propre. La section 40.1 couvre en détail son installation, sa configuration et ses patterns d'utilisation en production.

Un aperçu de ce à quoi ressemble `spdlog` :

```cpp
#include <spdlog/spdlog.h>

void example() {
    spdlog::info("Démarrage du serveur sur le port {}", 8080);
    spdlog::warn("File d'attente à {} % de capacité", 85);
    spdlog::error("Connexion à la base refusée : {}", "timeout");
}
```

La syntaxe est proche de `std::format`, la performance est optimisée en interne (buffering, logging asynchrone optionnel), et la configuration des niveaux se fait à l'exécution sans recompilation.

---

## Bonnes pratiques

**Utiliser des niveaux de sévérité de manière cohérente.** `Trace` pour le pas-à-pas détaillé (activé ponctuellement), `Debug` pour les informations utiles au développeur, `Info` pour les événements significatifs du cycle de vie, `Warning` pour les anomalies gérées, `Error` pour les échecs d'opération, `Fatal` pour les erreurs irrécupérables. Appliquer cette convention uniformément dans toute la base de code.

**Inclure le contexte dans chaque message.** Un message comme `"Erreur de connexion"` est inutile. Un message comme `"Échec de connexion à db-primary:5432 après 3 tentatives (timeout 5s)"` permet de diagnostiquer immédiatement le problème. Un bon message de log répond aux questions *quoi*, *où* et *pourquoi*.

**Ne pas logger d'informations sensibles.** Mots de passe, tokens d'authentification, données personnelles et numéros de carte bancaire n'ont rien à faire dans un fichier de log, même en mode debug. C'est une habitude à prendre dès le début.

**Logger les cas d'erreur, pas seulement les cas nominaux.** Les branches `catch`, les codes de retour d'erreur et les cas limites sont précisément les endroits où un message de log a le plus de valeur. Le cas nominal fonctionne — c'est l'exception qui nécessite une trace.

**Garder les messages concis et structurés.** Adopter un format prévisible facilite le filtrage avec `grep`, `awk`, ou des outils d'agrégation. Éviter les messages multilignes qui cassent les outils de parsing ligne par ligne.

**Séparer le logging de débogage et le logging opérationnel.** Le premier (couvert ici) est temporaire, conditionnel, et destiné au développeur. Le second (section 40.1) est permanent, structuré, et destiné à la supervision en production. Les deux répondent à des besoins différents et ne doivent pas être confondus.

---

## Résumé

| Outil | Cas d'usage | Avantages | Limites |  
|---|---|---|---|  
| `std::cout` | Diagnostic ponctuel, programmes triviaux | Aucune dépendance | Mélange avec stdout, pas de métadonnées |  
| `std::cerr` / `std::clog` | Traces de débogage simples | Séparation stdout/stderr | Pas de niveaux, pas d'horodatage |  
| `std::print` (C++23) | Formatage de messages de diagnostic | Type-safe, performant, lisible | N'est pas un framework de logging |  
| Logger personnalisé | Projets de taille moyenne, apprentissage | Contrôle total, compréhension du mécanisme | Maintenance, fonctionnalités limitées |  
| `spdlog` (section 40.1) | Production et projets professionnels | Complet, performant, structuré | Dépendance externe |

Le logging est une compétence transversale qui accompagne le développeur C++ tout au long de sa carrière. Les principes fondamentaux restent les mêmes quel que soit l'outil : des messages clairs, des niveaux de sévérité cohérents, un filtrage efficace, et une conscience permanente du coût de chaque instruction de log.

---

> 📎 *Pour le logging structuré en production avec `spdlog`, voir **section 40.1** (Logging structuré : spdlog vs std::print).*  
>  
> 📎 *Pour le système de formatage `std::format` et `std::print`, voir **section 12.7** (std::print et std::format — C++23).*  
>  
> 📎 *Pour les traces de pile d'exécution standardisées, voir **section 18.4** (std::stacktrace — C++23).*  
>  
> 📎 *Pour la compilation conditionnelle et la désactivation du logging en release, voir **section 18.2** (Compilation conditionnelle).*  
>  
> 📎 *Pour le logging structuré en JSON et l'agrégation, voir **section 40.5** (Structured logging : JSON logs pour agrégation).*

⏭️ [std::stacktrace (C++23) : Traces d'exécution standard](/18-assertions-debug/04-stacktrace.md)
