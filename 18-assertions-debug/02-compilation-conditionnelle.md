🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 18.2 Compilation conditionnelle (`#ifdef DEBUG`)

## Introduction

La compilation conditionnelle est un mécanisme du **préprocesseur C++** qui permet d'inclure ou d'exclure des portions de code source **avant même la phase de compilation**. Dans le contexte du débogage défensif, elle constitue un outil fondamental : elle rend possible l'insertion de vérifications, de traces et d'instrumentation dans le code **sans aucun coût en production**, puisque le code concerné est littéralement absent du binaire final.

Contrairement à une condition `if` évaluée à l'exécution, une directive `#ifdef` est résolue au moment de la compilation. Le compilateur ne voit jamais le code exclu — il n'est ni compilé, ni présent dans l'exécutable, ni source de la moindre dégradation de performance.

---

## Le préprocesseur et les directives conditionnelles

Le préprocesseur C++ fournit plusieurs directives pour contrôler l'inclusion de code :

```cpp
#ifdef MACRO       // Vrai si MACRO est définie (quelle que soit sa valeur)
#ifndef MACRO      // Vrai si MACRO n'est PAS définie
#if EXPRESSION     // Vrai si EXPRESSION est non nulle
#elif EXPRESSION   // Sinon si EXPRESSION est non nulle
#else              // Sinon
#endif             // Fin du bloc conditionnel
```

La directive `#if defined(MACRO)` est équivalente à `#ifdef MACRO`, mais offre l'avantage de pouvoir être combinée avec des opérateurs logiques :

```cpp
#if defined(DEBUG) && defined(VERBOSE)
    // Code inclus uniquement si DEBUG ET VERBOSE sont définis
#endif

#if defined(DEBUG) || defined(TESTING)
    // Code inclus si DEBUG OU TESTING est défini
#endif

#if !defined(NDEBUG)
    // Code inclus si NDEBUG n'est PAS défini
#endif
```

---

## La macro `NDEBUG` : le standard C et C++

Avant de définir ses propres macros, il est essentiel de comprendre la macro standard `NDEBUG` (*No Debug*). Elle est définie par la norme C et C++ et contrôle le comportement de `assert()` (voir section 18.1).

La convention est **inversée** par rapport à ce qu'on pourrait attendre :

- **`NDEBUG` non définie** → mode debug, les assertions sont actives.  
- **`NDEBUG` définie** → mode release, les assertions sont désactivées.

En pratique, CMake et la plupart des build systems définissent automatiquement `NDEBUG` pour les configurations `Release` et `RelWithDebInfo` :

```bash
# En mode Debug, NDEBUG n'est PAS définie → assert() est actif
cmake -DCMAKE_BUILD_TYPE=Debug ..

# En mode Release, NDEBUG EST définie → assert() devient un no-op
cmake -DCMAKE_BUILD_TYPE=Release ..
```

On peut s'appuyer sur `NDEBUG` pour du code de débogage personnalisé :

```cpp
#include <iostream>

void process_data(const std::vector<int>& data) {
#ifndef NDEBUG
    std::cerr << "[DEBUG] process_data() appelée avec "
              << data.size() << " éléments\n";
#endif
    // ... traitement réel ...
}
```

> ⚠️ **Attention à la double négation.** `#ifndef NDEBUG` signifie « si on n'est PAS en mode no-debug », autrement dit « si on est en debug ». Cette logique inversée est une source fréquente de confusion. C'est l'une des raisons pour lesquelles de nombreux projets préfèrent définir leur propre macro `DEBUG` à la sémantique plus directe.

---

## Définir et utiliser une macro `DEBUG` personnalisée

### Définition via le compilateur

La manière la plus courante de définir une macro de débogage est de la passer en option au compilateur avec le flag `-D` :

```bash
# Définir DEBUG lors de la compilation
g++ -DDEBUG -g -O0 -std=c++23 main.cpp -o app_debug

# Compilation release : DEBUG n'est pas définie
g++ -DNDEBUG -O2 -std=c++23 main.cpp -o app_release
```

Le flag `-DDEBUG` équivaut à placer `#define DEBUG` en tête de chaque fichier source. On peut aussi attribuer une valeur : `-DDEBUG_LEVEL=2` définit `DEBUG_LEVEL` avec la valeur `2`.

### Utilisation dans le code

```cpp
#include <iostream>
#include <vector>
#include <string_view>

// Macro utilitaire pour les traces conditionnelles
#ifdef DEBUG
    #define DBG_LOG(msg) std::cerr << "[DEBUG " << __FILE__ << ":" \
                                   << __LINE__ << "] " << msg << '\n'
#else
    #define DBG_LOG(msg) ((void)0)
#endif

class ConnectionPool {  
public:  
    void acquire(std::string_view name) {
        DBG_LOG("Acquisition de connexion pour: " << name);

        // ... logique d'acquisition ...

#ifdef DEBUG
        ++debug_acquire_count_;
        DBG_LOG("Total acquisitions: " << debug_acquire_count_);
#endif
    }

    void release(std::string_view name) {
        DBG_LOG("Libération de connexion pour: " << name);
        // ... logique de libération ...
    }

#ifdef DEBUG
    void dump_stats() const {
        std::cerr << "[DEBUG] === Pool Statistics ===\n"
                  << "  Acquisitions totales: " << debug_acquire_count_ << '\n';
    }
#endif

private:
    // ... membres de production ...

#ifdef DEBUG
    int debug_acquire_count_ = 0;  // Compteur uniquement en debug
#endif
};
```

Points importants dans cet exemple :

- La macro `DBG_LOG` se compile en `((void)0)` en release — une expression vide que le compilateur élimine entièrement.  
- Les **membres de données** et les **méthodes entières** peuvent être conditionnels. Attention cependant : cela modifie la taille et le layout de la classe entre debug et release.  
- Les macros prédéfinies `__FILE__` et `__LINE__` sont injectées par le préprocesseur et permettent de localiser précisément l'origine d'un message.

---

## Niveaux de verbosité avec `#if`

Pour des projets de taille significative, un simple toggle debug/release ne suffit pas toujours. On peut définir des **niveaux de débogage** :

```cpp
// Niveaux de débogage :
// 0 = aucun (production)
// 1 = erreurs et warnings critiques
// 2 = informations de flux (entrée/sortie de fonctions)
// 3 = traces détaillées (valeurs de variables, état interne)

#ifndef DEBUG_LEVEL
    #define DEBUG_LEVEL 0
#endif

#if DEBUG_LEVEL >= 1
    #define DBG_ERROR(msg)   std::cerr << "[ERROR] " << msg << '\n'
    #define DBG_WARN(msg)    std::cerr << "[WARN]  " << msg << '\n'
#else
    #define DBG_ERROR(msg)   ((void)0)
    #define DBG_WARN(msg)    ((void)0)
#endif

#if DEBUG_LEVEL >= 2
    #define DBG_INFO(msg)    std::cerr << "[INFO]  " << msg << '\n'
#else
    #define DBG_INFO(msg)    ((void)0)
#endif

#if DEBUG_LEVEL >= 3
    #define DBG_TRACE(msg)   std::cerr << "[TRACE] " << msg << '\n'
#else
    #define DBG_TRACE(msg)   ((void)0)
#endif
```

Utilisation à la compilation :

```bash
# Production : aucun message
g++ -DDEBUG_LEVEL=0 -O2 -std=c++23 main.cpp -o app

# Développement courant : erreurs + infos de flux
g++ -DDEBUG_LEVEL=2 -g -O0 -std=c++23 main.cpp -o app_debug

# Diagnostic approfondi : tout afficher
g++ -DDEBUG_LEVEL=3 -g -O0 -std=c++23 main.cpp -o app_verbose
```

> 💡 **Note pédagogique.** Les niveaux de verbosité via le préprocesseur sont utiles pour le débogage à granularité fine, mais pour le **logging en production**, on préférera une bibliothèque dédiée comme `spdlog` qui offre un filtrage dynamique par niveau à l'exécution (voir section 40.1).

---

## Intégration avec CMake

La gestion manuelle des flags `-D` sur la ligne de commande devient vite fastidieuse. CMake permet d'automatiser cette configuration proprement.

### Configuration de base

```cmake
cmake_minimum_required(VERSION 3.20)  
project(MonProjet LANGUAGES CXX)  

add_executable(app src/main.cpp)

# CMake définit automatiquement NDEBUG en Release.
# On ajoute notre propre macro DEBUG en mode Debug :
target_compile_definitions(app PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:DEBUG_LEVEL=2>
)
```

La syntaxe `$<$<CONFIG:Debug>:DEBUG>` est une **expression génératrice** (*generator expression*) de CMake. Elle signifie : « si la configuration est Debug, alors ajouter la définition `DEBUG` ». En configuration Release, la définition n'est tout simplement pas ajoutée.

### Configuration avancée avec option utilisateur

```cmake
cmake_minimum_required(VERSION 3.20)  
project(MonProjet LANGUAGES CXX)  

# Option exposée à l'utilisateur
option(ENABLE_VERBOSE_DEBUG "Active les traces détaillées (niveau 3)" OFF)

add_executable(app src/main.cpp)

target_compile_definitions(app PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:DEBUG_LEVEL=$<IF:$<BOOL:${ENABLE_VERBOSE_DEBUG}>,3,2>>
)
```

```bash
# Build debug standard (niveau 2)
cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..  
ninja  

# Build debug verbeux (niveau 3)
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_VERBOSE_DEBUG=ON -G Ninja ..  
ninja  
```

> ⭐ **Bonne pratique.** Utiliser `target_compile_definitions` avec le scope `PRIVATE` plutôt que `add_definitions()` qui pollue l'ensemble du projet. Cela garantit que les macros de débogage ne fuient pas vers les bibliothèques ou les cibles qui ne les nécessitent pas.

---

## Vérifier ce que voit le compilateur

Pour s'assurer que la compilation conditionnelle fonctionne comme attendu, on peut inspecter la sortie du préprocesseur :

```bash
# Afficher le code après préprocessing (flag -E)
g++ -E -DDEBUG -DDEBUG_LEVEL=2 -std=c++23 main.cpp | less

# Avec clang, même principe
clang++ -E -DDEBUG -DDEBUG_LEVEL=2 -std=c++23 main.cpp | less
```

Le flag `-E` arrête le processus après le préprocesseur et affiche le résultat sur la sortie standard. Tout le code protégé par des `#ifdef` non satisfaits a disparu. C'est un outil précieux pour diagnostiquer les problèmes de macros imbriquées.

On peut aussi utiliser `g++ -dM -E` pour lister toutes les macros définies à un instant donné :

```bash
# Lister toutes les macros prédéfinies + celles de l'utilisateur
echo | g++ -dM -E -DDEBUG -DDEBUG_LEVEL=2 -std=c++23 -x c++ - | grep -i debug
```

Sortie typique :

```
#define DEBUG 1
#define DEBUG_LEVEL 2
```

---

## Patterns courants et idiomes

### Guard d'inclusion (rappel)

Le pattern `#ifndef` / `#define` / `#endif` le plus connu en C++ est le **header guard**, qui empêche l'inclusion multiple d'un fichier d'en-tête :

```cpp
#ifndef MYPROJECT_LOGGER_H
#define MYPROJECT_LOGGER_H

// Contenu du header...

#endif // MYPROJECT_LOGGER_H
```

Bien que `#pragma once` soit supporté par tous les compilateurs majeurs (GCC, Clang, MSVC), il ne fait pas partie du standard ISO C++. Les deux approches sont acceptables en pratique.

### Vérifications d'intégrité coûteuses

Certaines vérifications sont trop coûteuses pour la production mais précieuses en développement :

```cpp
class SortedContainer {  
public:  
    void insert(int value) {
        // ... insertion ...

#ifdef DEBUG
        // Vérification O(n) que le conteneur reste trié
        // Acceptable en debug, inacceptable en production
        for (std::size_t i = 1; i < data_.size(); ++i) {
            if (data_[i] < data_[i - 1]) {
                std::cerr << "[DEBUG] INVARIANT VIOLÉ : conteneur non trié "
                          << "à l'index " << i << '\n';
                std::abort();
            }
        }
#endif
    }

private:
    std::vector<int> data_;
};
```

### Compilation conditionnelle pour les plateformes

Bien que ce ne soit pas spécifique au débogage, la compilation conditionnelle est aussi utilisée pour gérer les différences entre plateformes. Les macros prédéfinies du compilateur permettent de cibler un OS ou une architecture :

```cpp
#if defined(__linux__)
    #include <sys/epoll.h>
    // Code spécifique Linux
#elif defined(_WIN32)
    #include <winsock2.h>
    // Code spécifique Windows
#elif defined(__APPLE__)
    #include <sys/event.h>
    // Code spécifique macOS (kqueue)
#endif
```

---

## Limites du préprocesseur et alternatives modernes

La compilation conditionnelle via le préprocesseur est puissante, mais elle présente des inconvénients bien connus.

### Problèmes du préprocesseur

**Le code exclu n'est pas vérifié syntaxiquement.** Si une branche `#else` contient une erreur de syntaxe, le compilateur ne la détectera que lorsque cette branche sera activée. Cela peut introduire des régressions silencieuses :

```cpp
#ifdef DEBUG
    log_message("Opération réussie");
#else
    // Cette erreur de syntaxe passe inaperçue tant qu'on compile en DEBUG
    int result = compute( ;  // ← erreur invisible en mode DEBUG
#endif
```

**Les macros ne respectent pas les scopes.** Une macro définie par `#define` est visible dans toute l'unité de compilation, sans notion de namespace ou de portée. Cela peut causer des collisions de noms imprévisibles.

**Le débogage est plus complexe.** Le code source que vous lisez ne correspond pas nécessairement à ce que le compilateur traite. Cela peut rendre le diagnostic d'erreurs de compilation plus difficile.

### `if constexpr` : alternative compile-time (C++17)

Depuis C++17, `if constexpr` permet d'éliminer du code à la compilation tout en bénéficiant de la vérification syntaxique complète par le compilateur :

```cpp
#include <iostream>

// Constante compile-time contrôlée par macro,
// mais utilisée ensuite en C++ pur
#ifdef DEBUG
    inline constexpr bool is_debug = true;
#else
    inline constexpr bool is_debug = false;
#endif

template <typename T>  
void process(const T& data) {  
    if constexpr (is_debug) {
        // Ce bloc est éliminé en release par le compilateur,
        // mais il est TOUJOURS vérifié syntaxiquement
        std::cerr << "[DEBUG] Processing data of size: "
                  << data.size() << '\n';
    }

    // ... traitement ...
}
```

Cette approche combine le meilleur des deux mondes : le code debug est éliminé du binaire en release (comme avec `#ifdef`), mais le compilateur le vérifie toujours, ce qui prévient les régressions silencieuses.

> ⚠️ **Limitation.** `if constexpr` ne permet pas d'ajouter ou retirer des **membres de classe** ou des **déclarations** de manière conditionnelle. Pour ces cas, `#ifdef` reste nécessaire.

### `consteval` et `constexpr` pour les vérifications compile-time

Pour les vérifications qui peuvent être entièrement résolues à la compilation, `static_assert` (voir section 18.1) et les fonctions `consteval` (C++20) offrent des alternatives qui ne dépendent pas du tout du préprocesseur :

```cpp
consteval int checked_divide(int a, int b) {
    if (b == 0) {
        throw "Division par zéro détectée à la compilation";
    }
    return a / b;
}

// Erreur de compilation si b vaut 0
constexpr int result = checked_divide(42, 7);  // OK
// constexpr int bad = checked_divide(42, 0);  // Erreur à la compilation
```

---

## Bonnes pratiques

**Minimiser la surface des blocs conditionnels.** Plus un bloc `#ifdef` est petit et ciblé, plus le code reste lisible. Éviter d'encapsuler des fonctions entières dans des `#ifdef` quand seule une ligne de trace est nécessaire.

**Préférer `if constexpr` quand c'est possible.** Réserver `#ifdef` aux cas où il est indispensable : inclusion conditionnelle de headers, ajout/suppression de membres de classe, ou changement de signatures de fonctions.

**Documenter les macros de configuration.** Maintenir une liste des macros attendues (dans un `README` ou un header dédié `config.h`) pour que les nouveaux contributeurs sachent quels flags activer.

**Tester les deux chemins de compilation.** Les pipelines CI/CD doivent impérativement compiler et tester à la fois en `Debug` et en `Release` pour détecter les divergences de comportement et les erreurs de syntaxe dans les branches inactives (voir section 38.7 — matrix builds).

**Ne jamais placer de logique métier dans un bloc `#ifdef DEBUG`.** Le comportement observable de l'application ne doit pas changer entre debug et release. Les blocs conditionnels doivent se limiter aux traces, vérifications d'intégrité, compteurs de diagnostic et instrumentation.

---

## Résumé

| Mécanisme | Résolution | Code vérifié ? | Cas d'usage |  
|---|---|---|---|  
| `#ifdef` / `#ifndef` | Préprocesseur | Non (branche exclue ignorée) | Headers, membres conditionnels, inclusion de librairies |  
| `#if` / `#elif` | Préprocesseur | Non | Niveaux de verbosité, ciblage de plateforme |  
| `if constexpr` (C++17) | Compilation | Oui (syntaxe vérifiée) | Traces, vérifications dans les templates |  
| `static_assert` | Compilation | Oui | Contraintes sur les types et valeurs |

La compilation conditionnelle via le préprocesseur reste un outil incontournable pour le débogage défensif en C++. Utilisée avec discernement et combinée aux alternatives modernes comme `if constexpr`, elle permet de construire des bases de code robustes où les chemins de debug sont rigoureusement séparés du code de production — sans compromis sur la performance.

---

> 📎 *Pour la gestion de logs en production avec filtrage dynamique des niveaux, voir **section 40.1** (Logging structuré avec spdlog).*  
>  
> 📎 *Pour l'utilisation de `assert` et `static_assert`, voir **section 18.1** (assert et static_assert).*  
>  
> 📎 *Pour l'automatisation des builds multi-configuration en CI, voir **section 38.7** (Matrix builds).*

⏭️ [Logging et traces d'exécution](/18-assertions-debug/03-logging-traces.md)
