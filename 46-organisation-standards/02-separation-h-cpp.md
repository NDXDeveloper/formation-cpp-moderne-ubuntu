🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 46.2 — Séparation `.h`/`.cpp` et compilation incrémentale

> **Chapitre 46 : Organisation et Standards** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert · **Prérequis** : Section 1.3 (cycle de compilation), chapitre 26 (CMake), section 2.3 (ccache)

---

## Pourquoi cette séparation existe

Le C++ hérite du C un modèle de compilation fondé sur les **unités de traduction** : chaque fichier `.cpp` est compilé indépendamment des autres, en un fichier objet `.o`. Le linker assemble ensuite ces fichiers objets en un exécutable ou une bibliothèque. Ce modèle a une conséquence architecturale majeure : le compilateur, lorsqu'il traite un fichier `.cpp`, ne voit que ce fichier et tout ce qui y est inclus via `#include`. Il ne sait rien des autres fichiers `.cpp` du projet.

Les fichiers d'en-tête (`.h`, `.hpp`) existent pour résoudre ce problème. Ils contiennent les **déclarations** — signatures de fonctions, définitions de classes, constantes — nécessaires pour que chaque unité de traduction puisse utiliser des symboles définis ailleurs. Les fichiers sources (`.cpp`) contiennent les **définitions** — le corps des fonctions, l'implémentation des méthodes, l'initialisation des variables statiques.

Cette séparation n'est pas qu'une convention : elle est imposée par la **One Definition Rule** (ODR). En C++, une entité (fonction, variable, classe) ne peut être définie qu'une seule fois dans l'ensemble du programme. Si la définition d'une fonction se trouve dans un header inclus par plusieurs fichiers `.cpp`, le linker détectera des définitions multiples et refusera de produire l'exécutable. C'est pourquoi les headers contiennent des déclarations et les sources contiennent des définitions.

---

## Anatomie d'une séparation correcte

### Le header : le contrat

Un header bien conçu expose le minimum nécessaire pour que les consommateurs puissent utiliser le composant. Il contient les déclarations de l'interface publique et rien de plus.

```cpp
// include/mon-projet/core/engine.h
#pragma once

#include <string>
#include <memory>
#include <cstdint>

namespace monprojet::core {

/// Moteur principal du système de traitement.
class Engine {  
public:  
    /// Construit un moteur avec le nom de configuration donné.
    explicit Engine(std::string config_name);

    /// Destructeur.
    ~Engine();

    /// Interdit la copie (le moteur gère des ressources uniques).
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /// Autorise le déplacement.
    Engine(Engine&&) noexcept;
    Engine& operator=(Engine&&) noexcept;

    /// Démarre le moteur. Retourne true si le démarrage a réussi.
    [[nodiscard]] bool start();

    /// Arrête le moteur proprement.
    void stop();

    /// Retourne le nombre d'éléments traités depuis le démarrage.
    [[nodiscard]] std::uint64_t processed_count() const;

private:
    struct Impl;                    // Forward declaration (Pimpl)
    std::unique_ptr<Impl> impl_;   // Pointeur vers l'implémentation
};

} // namespace monprojet::core
```

Plusieurs principes sont à l'œuvre dans cet exemple. Le header n'inclut que les en-têtes strictement nécessaires aux déclarations (`<string>` pour le paramètre du constructeur, `<memory>` pour `std::unique_ptr`, `<cstdint>` pour `std::uint64_t`). La structure `Impl` est déclarée mais pas définie — c'est l'idiome Pimpl, que nous détaillerons plus bas. Les méthodes sont déclarées, pas définies : aucun corps de fonction n'apparaît dans le header.

### Le source : l'implémentation

Le fichier `.cpp` correspondant inclut le header public et fournit les définitions :

```cpp
// src/core/engine.cpp
#include <mon-projet/core/engine.h>

#include <vector>
#include <thread>
#include <atomic>
#include <iostream>

namespace monprojet::core {

// Définition complète de la structure d'implémentation.
struct Engine::Impl {
    std::string config_name;
    std::vector<std::thread> workers;
    std::atomic<bool> running{false};
    std::atomic<std::uint64_t> counter{0};
};

Engine::Engine(std::string config_name)
    : impl_(std::make_unique<Impl>())
{
    impl_->config_name = std::move(config_name);
}

Engine::~Engine() {
    if (impl_ && impl_->running.load()) {
        stop();
    }
}

Engine::Engine(Engine&&) noexcept = default;  
Engine& Engine::operator=(Engine&&) noexcept = default;  

bool Engine::start() {
    if (impl_->running.exchange(true)) {
        return false; // Déjà démarré
    }
    // Lancement des workers...
    return true;
}

void Engine::stop() {
    impl_->running.store(false);
    for (auto& w : impl_->workers) {
        if (w.joinable()) {
            w.join();
        }
    }
    impl_->workers.clear();
}

std::uint64_t Engine::processed_count() const {
    return impl_->counter.load();
}

} // namespace monprojet::core
```

Observez que `<vector>`, `<thread>`, `<atomic>` et `<iostream>` ne sont inclus que dans le `.cpp`. Les consommateurs de `Engine` n'ont pas besoin de ces en-têtes pour utiliser la classe — ils n'ont besoin que de la déclaration dans le header. C'est une réduction directe du couplage de compilation.

---

## Le mécanisme de compilation incrémentale

### Comment le build system décide quoi recompiler

Quand vous modifiez un fichier et relancez le build, CMake (via Make ou Ninja) ne recompile pas l'intégralité du projet. Il compare les timestamps (ou les checksums, selon le build system) des fichiers sources et de leurs dépendances avec les fichiers objets existants. Seules les unités de traduction dont les entrées ont changé sont recompilées.

Le graphe de dépendances fonctionne ainsi :

```
engine.cpp  ──depends on──►  engine.h
                             string.h (système)
                             memory.h (système)
                             ...

main.cpp    ──depends on──►  engine.h
                             ...
```

Les conséquences sont asymétriques selon le type de fichier modifié :

**Modification d'un `.cpp`** : seule cette unité de traduction est recompilée, puis le linker reassemble les fichiers objets. C'est le cas le plus rapide. Si vous modifiez `engine.cpp`, seul `engine.o` est régénéré, et le link final est relancé. Aucun autre `.cpp` n'est touché.

**Modification d'un `.h`** : toutes les unités de traduction qui incluent ce header (directement ou transitivement) sont recompilées. Si `engine.h` est inclus par `engine.cpp`, `main.cpp` et quinze fichiers de test, les dix-sept unités de traduction sont recompilées. C'est le cas coûteux, et c'est pourquoi la minimisation des dépendances dans les headers est critique.

### Le coût réel de la cascade

Pour illustrer l'impact, prenons un projet de taille moyenne avec 200 fichiers `.cpp`. Si un header central comme `types.h` est inclus (directement ou indirectement) par 150 de ces fichiers, la modification d'une seule ligne dans `types.h` déclenche la recompilation de 150 unités de traduction. Sur une machine moderne avec ccache froid, cela peut représenter plusieurs minutes. Sur un serveur CI, c'est du temps de pipeline consommé à chaque commit.

La qualité de l'organisation des headers a donc un impact direct et mesurable sur la productivité du développement et sur les coûts d'infrastructure CI.

---

## Techniques pour minimiser les dépendances de compilation

### Forward declarations

Une forward declaration informe le compilateur qu'un type existe, sans inclure sa définition complète. Elle suffit dans tous les cas où le compilateur n'a pas besoin de connaître la taille ou la structure du type : pointeurs, références, paramètres de retour déclarés mais pas utilisés.

```cpp
// Au lieu de :
#include <mon-projet/network/connection.h>  // Définition complète

// On écrit :
namespace monprojet::network {  
class Connection;  // Forward declaration  
}
```

Quand la forward declaration suffit :

```cpp
class Engine {  
public:  
    void set_connection(std::shared_ptr<network::Connection> conn);
    network::Connection* get_connection() const;
private:
    std::shared_ptr<network::Connection> conn_;
};
```

Ici, le compilateur n'a besoin que de savoir que `Connection` est un type — il n'a pas besoin de connaître sa taille (c'est le pointeur intelligent qui a une taille fixe, indépendante du type pointé).

Quand la forward declaration ne suffit **pas** :

```cpp
class Engine {
    network::Connection conn_;  // Membre par valeur → taille requise
    // ...
    void process() {
        conn_.send("hello");    // Appel de méthode → définition requise
    }
};
```

Dans ces cas, le compilateur doit connaître la taille de `Connection` (pour calculer la taille de `Engine`) ou sa définition (pour résoudre l'appel à `send`). L'include complet est alors nécessaire — mais il peut souvent être déplacé dans le `.cpp`.

### L'idiome Pimpl (Pointer to Implementation)

L'idiome Pimpl est l'application systématique des forward declarations pour isoler complètement l'implémentation d'une classe de son interface. Le header ne contient qu'un pointeur opaque vers une structure d'implémentation définie dans le `.cpp`.

```cpp
// engine.h — Header public
#pragma once
#include <memory>
#include <string>
#include <cstdint>

namespace monprojet::core {

class Engine {  
public:  
    explicit Engine(std::string config_name);
    ~Engine();

    Engine(Engine&&) noexcept;
    Engine& operator=(Engine&&) noexcept;

    [[nodiscard]] bool start();
    void stop();
    [[nodiscard]] std::uint64_t processed_count() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace monprojet::core
```

```cpp
// engine.cpp — Implémentation
#include <mon-projet/core/engine.h>

#include <vector>        // Nécessaire seulement ici
#include <thread>        // Nécessaire seulement ici
#include <atomic>        // Nécessaire seulement ici
#include <unordered_map> // Nécessaire seulement ici

namespace monprojet::core {

struct Engine::Impl {
    std::string config_name;
    std::vector<std::thread> workers;
    std::atomic<bool> running{false};
    std::atomic<std::uint64_t> counter{0};
    std::unordered_map<std::string, double> metrics;
};

// ... définitions des méthodes ...

} // namespace monprojet::core
```

Les bénéfices sont considérables. Le header `engine.h` n'inclut ni `<vector>`, ni `<thread>`, ni `<atomic>`, ni `<unordered_map>`. Toute modification de la structure interne de `Impl` — ajout de champs, changement de conteneur, refactoring interne — ne déclenche que la recompilation de `engine.cpp`. Les centaines de fichiers qui incluent `engine.h` ne sont pas affectés.

Le coût de Pimpl est double : une indirection supplémentaire (l'accès aux membres passe par un pointeur) et une allocation dynamique (le `make_unique` dans le constructeur). En pratique, ce coût est négligeable pour la grande majorité des classes — il ne devient pertinent que pour des objets créés et détruits à très haute fréquence dans des boucles critiques.

Un point technique important : le destructeur, le constructeur de déplacement et l'opérateur d'affectation par déplacement doivent être **déclarés dans le header** et **définis dans le `.cpp`**. C'est une exigence de `std::unique_ptr` avec un type incomplet. Si vous les laissez implicitement générés dans le header, le compilateur tentera d'instancier le destructeur de `unique_ptr<Impl>` à un endroit où `Impl` n'est pas défini, et la compilation échouera.

### Ordre des includes

L'ordre des `#include` dans un fichier `.cpp` n'est pas qu'esthétique — il peut révéler des dépendances manquantes. La convention recommandée (adoptée entre autres par le Google C++ Style Guide) est :

```cpp
// 1. Header correspondant au fichier (vérifie son autonomie)
#include <mon-projet/core/engine.h>

// 2. Headers du projet
#include <mon-projet/utils/string_utils.h>

// 3. Headers de bibliothèques tierces
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

// 4. Headers de la librairie standard
#include <vector>
#include <string>
#include <memory>

// 5. Headers système/POSIX (si nécessaire)
#include <unistd.h>
#include <sys/socket.h>
```

Le premier include est le plus important : en plaçant le header correspondant en tête, vous garantissez qu'il est **auto-suffisant**. Si `engine.h` oublie d'inclure `<string>` mais que `engine.cpp` l'inclut avant `engine.h`, le bug est masqué. En mettant `engine.h` en premier, le compilateur échoue immédiatement si le header n'est pas autonome.

---

## `#pragma once` vs header guards

Deux mécanismes empêchent l'inclusion multiple d'un header :

```cpp
// Header guards traditionnels
#ifndef MONPROJET_CORE_ENGINE_H
#define MONPROJET_CORE_ENGINE_H
// ... contenu du header ...
#endif // MONPROJET_CORE_ENGINE_H
```

```cpp
// Pragma once (mécanisme non standard mais universellement supporté)
#pragma once
// ... contenu du header ...
```

Les header guards sont la solution standard ISO C++. Ils reposent sur le préprocesseur : si le symbole est déjà défini (parce que le fichier a déjà été inclus), le contenu entre `#ifndef` et `#endif` est ignoré. Le nom du symbole doit être unique dans tout le projet — la convention est d'utiliser le chemin complet du fichier en majuscules avec des underscores.

`#pragma once` est une directive non standard mais supportée par GCC, Clang et MSVC depuis des années. Elle demande au compilateur de n'inclure le fichier qu'une seule fois, en se basant sur l'identité du fichier (chemin physique) plutôt que sur un symbole du préprocesseur. C'est plus simple, sans risque de collision de noms, et légèrement plus performant (le compilateur peut court-circuiter l'ouverture du fichier).

En pratique, `#pragma once` est le choix dominant dans les projets modernes. Les header guards restent nécessaires si vous devez garantir la portabilité vers des compilateurs exotiques ou si votre standard de codage l'impose (les C++ Core Guidelines ne prennent pas position, le Google Style Guide accepte les deux).

---

## Ce qui peut aller dans un header : les exceptions à la règle

La règle "déclarations dans les headers, définitions dans les sources" admet des exceptions légitimes. Certains éléments **doivent** être définis dans les headers :

### Templates

Les templates ne sont pas du code compilé : ce sont des modèles que le compilateur instancie pour chaque type utilisé. L'instanciation a lieu dans chaque unité de traduction qui utilise le template, ce qui impose que la définition soit visible — donc dans le header.

```cpp
// utils/algorithm.h
#pragma once

namespace monprojet::utils {

template <typename Container, typename Predicate>  
auto count_matching(const Container& c, Predicate pred) {  
    typename Container::size_type count = 0;
    for (const auto& elem : c) {
        if (pred(elem)) {
            ++count;
        }
    }
    return count;
}

} // namespace monprojet::utils
```

Pour les templates volumineux, il est possible de séparer la déclaration et la définition en deux fichiers distincts, la définition étant dans un fichier `.inl` ou `.tpp` inclus à la fin du header :

```cpp
// algorithm.h
#pragma once

namespace monprojet::utils {

template <typename Container, typename Predicate>  
auto count_matching(const Container& c, Predicate pred);  

} // namespace monprojet::utils

#include "algorithm.inl"  // Définitions des templates
```

```cpp
// algorithm.inl
namespace monprojet::utils {

template <typename Container, typename Predicate>  
auto count_matching(const Container& c, Predicate pred) {  
    // ... implémentation ...
}

} // namespace monprojet::utils
```

Cela garde le header principal lisible tout en fournissant les définitions nécessaires à l'instanciation.

### Fonctions `inline`

Le mot-clé `inline` relâche la One Definition Rule : une fonction `inline` peut être définie dans plusieurs unités de traduction, à condition que toutes les définitions soient identiques. C'est le mécanisme qui permet de définir de petites fonctions utilitaires directement dans les headers :

```cpp
// utils/math.h
#pragma once

namespace monprojet::utils {

inline constexpr double pi = 3.14159265358979323846;

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

} // namespace monprojet::utils
```

Les méthodes définies directement dans le corps d'une classe sont implicitement `inline` :

```cpp
class Point {  
public:  
    double x() const { return x_; }  // Implicitement inline
    double y() const { return y_; }  // Implicitement inline
private:
    double x_, y_;
};
```

### Fonctions `constexpr` et `consteval`

Les fonctions `constexpr` (section 3.5.2) et `consteval` (section 3.5.3) sont évaluées à la compilation. Elles doivent être visibles dans chaque unité de traduction qui les utilise, et sont donc définies dans les headers. Depuis C++17, les fonctions `constexpr` sont implicitement `inline`.

```cpp
// utils/hash.h
#pragma once
#include <cstdint>

namespace monprojet::utils {

constexpr std::uint32_t fnv1a(const char* str) {
    std::uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= static_cast<std::uint32_t>(*str++);
        hash *= 16777619u;
    }
    return hash;
}

} // namespace monprojet::utils
```

### Variables `inline` (C++17)

Avant C++17, définir une variable globale ou un membre statique dans un header violait la ODR. C++17 a introduit les variables `inline`, qui suivent les mêmes règles que les fonctions `inline` :

```cpp
// config/defaults.h
#pragma once
#include <string_view>

namespace monprojet::config {

inline constexpr std::string_view default_host = "localhost";  
inline constexpr int default_port = 8080;  
inline constexpr int max_connections = 1024;  

} // namespace monprojet::config
```

Sans le `inline`, ces variables seraient définies dans chaque unité de traduction qui inclut le header, provoquant des erreurs de linkage.

---

## Bibliothèques header-only : avantages et limites

Certaines bibliothèques C++ célèbres sont entièrement header-only : nlohmann/json, Catch2 (v2), stb, et de nombreuses bibliothèques Boost. Tout le code est dans les headers, il n'y a rien à compiler ni à linker séparément. L'intégration est triviale : un `#include` suffit.

Ce modèle a des avantages réels : distribution simplifiée (un seul fichier à copier), pas de problèmes de linkage, compatibilité ABI garantie (le code est compilé avec les mêmes options que le consommateur).

Mais il a aussi des coûts, qui deviennent visibles à l'échelle :

**Temps de compilation.** Chaque unité de traduction qui inclut le header compile l'intégralité du code de la bibliothèque. Pour nlohmann/json, cela représente des milliers de lignes de templates instanciés à chaque inclusion. Sur un projet de 200 fichiers qui utilisent tous du JSON, le temps de compilation explose.

**Impossibilité d'isoler l'implémentation.** Toute modification dans le code de la bibliothèque (même interne) recompile tous les consommateurs. Il n'y a pas de firewall de compilation.

Pour vos propres projets, le modèle header-only est adapté aux petits utilitaires (quelques fonctions template, des constantes). Pour tout composant substantiel, la séparation `.h`/`.cpp` est préférable. Si vous consommez une bibliothèque header-only volumineuse, une technique courante est de créer un fichier "wrapper" `.cpp` dédié :

```cpp
// src/third_party/json_impl.cpp
// Ce fichier est le seul à inclure le header lourd.
// Les autres fichiers utilisent des forward declarations
// ou un header allégé qui expose une API simplifiée.
#include <nlohmann/json.hpp>

// Fonctions wrapper compilées une seule fois
namespace monprojet::json {

std::string serialize(const Config& cfg) {
    nlohmann::json j;
    j["name"] = cfg.name;
    j["port"] = cfg.port;
    return j.dump();
}

Config deserialize(std::string_view input) {
    auto j = nlohmann::json::parse(input);
    return Config{
        .name = j.at("name").get<std::string>(),
        .port = j.at("port").get<int>()
    };
}

} // namespace monprojet::json
```

Les autres fichiers du projet incluent un header léger (`json_wrapper.h`) qui déclare `serialize` et `deserialize` sans inclure `nlohmann/json.hpp`. La compilation de la bibliothèque JSON lourde n'a lieu qu'une seule fois, dans `json_impl.cpp`.

---

## Modules C++20 : le futur de la séparation

Les modules C++20 (section 12.13) sont conçus pour remplacer à terme le modèle header/source. Un module exporte une interface compilée une seule fois, consommée sous forme binaire par les unités de traduction importatrices. Plus de préprocesseur textuel, plus de cascade de recompilation, plus de header guards.

```cpp
// engine.cppm — Module interface
export module monprojet.core.engine;

import <string>;  
import <memory>;  
import <cstdint>;  

export namespace monprojet::core {

class Engine {  
public:  
    explicit Engine(std::string config_name);
    ~Engine();
    [[nodiscard]] bool start();
    void stop();
    [[nodiscard]] std::uint64_t processed_count() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace monprojet::core
```

En mars 2026, le support des modules a considérablement progressé dans GCC 15 et Clang 20 (section 12.13.2), mais l'écosystème de build (CMake, Conan, les IDE) n'est pas encore au niveau de maturité requis pour une adoption généralisée en production (section 12.13.3). La séparation `.h`/`.cpp` reste donc le modèle standard pour les projets professionnels, et le restera probablement pendant encore plusieurs années.

L'investissement dans une bonne organisation header/source n'est pas perdu : les principes de séparation interface/implémentation et de minimisation des dépendances s'appliquent identiquement aux modules. Un projet bien structuré en headers aujourd'hui sera plus facile à migrer vers les modules demain.

---

## Checklist de bonnes pratiques

Voici un condensé des règles à appliquer systématiquement :

**Pour les headers :**  
- Un header doit être auto-suffisant : il compile seul, sans dépendre de l'ordre d'inclusion.  
- Utiliser `#pragma once` (ou des header guards avec un nom unique basé sur le chemin).  
- Inclure le strict minimum. Préférer les forward declarations quand c'est possible.  
- Ne jamais placer `using namespace` dans un header (détaillé en section 46.3).  
- Ne définir dans un header que ce qui doit l'être : templates, fonctions `inline`/`constexpr`, variables `inline`.

**Pour les sources :**  
- Le premier `#include` est le header correspondant (test d'autonomie).  
- Regrouper les includes par catégorie : header correspondant, headers projet, bibliothèques tierces, standard, système.  
- Les includes coûteux (bibliothèques header-only volumineuses) sont confinés au minimum de fichiers `.cpp`.

**Pour la compilation incrémentale :**  
- Minimiser le contenu des headers fréquemment inclus.  
- Utiliser l'idiome Pimpl pour les classes dont l'implémentation change souvent.  
- Surveiller les temps de compilation : un rebuild partiel qui prend plus de quelques secondes après une modification locale signale un problème de dépendances.  
- Utiliser ccache (section 2.3) pour amortir les recompilations inévitables.  
- Préférer Ninja à Make comme backend de build (section 26.5) — Ninja gère le graphe de dépendances plus efficacement.

---

## Résumé

La séparation `.h`/`.cpp` est le mécanisme fondamental qui rend la compilation incrémentale possible en C++. Les headers exposent les déclarations (le contrat), les sources fournissent les définitions (l'implémentation). Cette séparation a un impact direct et mesurable sur les temps de compilation : chaque include superflu dans un header propage une dépendance de recompilation à tous les fichiers qui l'incluent, directement ou transitivement. Les techniques de forward declaration et l'idiome Pimpl permettent de construire des firewalls de compilation qui maintiennent les temps de build sous contrôle, même sur des projets de grande taille. Les modules C++20 résoudront structurellement ces problèmes à terme, mais la maîtrise du modèle header/source reste indispensable pour tout projet professionnel en 2026.

---


⏭️ [Namespaces et éviter la pollution globale](/46-organisation-standards/03-namespaces.md)
