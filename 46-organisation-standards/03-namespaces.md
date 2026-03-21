🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 46.3 — Namespaces et éviter la pollution globale

> **Chapitre 46 : Organisation et Standards** · Module 17 : Architecture de Projet Professionnel  
> **Niveau** : Expert · **Prérequis** : Section 46.1 (organisation des répertoires), section 46.2 (séparation .h/.cpp)

---

## Le problème de l'espace de noms global

En C++, tout symbole déclaré en dehors d'un namespace vit dans l'**espace de noms global**. C'est un espace unique, partagé par l'intégralité du programme : votre code, la bibliothèque standard, les bibliothèques tierces, les headers système POSIX. Chaque symbole global est une bombe à retardement — il suffit qu'une dépendance introduise un symbole portant le même nom pour provoquer une collision.

Le problème est aggravé par le modèle d'inclusion textuel du C++ : `#include` copie littéralement le contenu du header dans l'unité de traduction. Si un header système définit une macro `MAX` ou une fonction `log`, et que votre code définit ses propres `MAX` ou `log` dans l'espace global, le résultat va de l'erreur de compilation silencieusement résolue en faveur du mauvais symbole au comportement indéfini.

Les namespaces sont le mécanisme du langage pour compartimenter les symboles et éliminer ces collisions. Mais leur utilité ne se limite pas à la prévention des conflits de noms : bien conçus, ils documentent l'architecture du projet, rendent les dépendances explicites dans le code, et facilitent la navigation dans de grandes bases de code.

---

## Déclarer et utiliser un namespace

### Syntaxe de base

Un namespace crée un scope nommé qui encapsule ses déclarations :

```cpp
namespace monprojet {

class Config {
    // ...
};

void initialize();  
int compute(int x, int y);  

} // namespace monprojet
```

Les symboles sont ensuite référencés par leur nom qualifié :

```cpp
monprojet::Config cfg;  
monprojet::initialize();  
int result = monprojet::compute(3, 4);  
```

### Namespaces imbriqués (C++17)

Avant C++17, les namespaces imbriqués nécessitaient une syntaxe verbeuse :

```cpp
// Avant C++17
namespace monprojet {
    namespace core {
        namespace detail {
            void helper();
        }
    }
}
```

C++17 a introduit la syntaxe compacte, qui est désormais la forme préférée :

```cpp
// C++17 et après
namespace monprojet::core::detail {
    void helper();
}
```

Le résultat est identique au niveau du compilateur, mais la lisibilité est considérablement améliorée, notamment pour les namespaces à trois niveaux ou plus.

---

## Concevoir une hiérarchie de namespaces

### Le namespace racine : le nom du projet

Tout projet professionnel devrait encapsuler l'intégralité de son code dans un namespace portant le nom du projet. C'est la première ligne de défense contre les collisions avec le monde extérieur.

```cpp
namespace monprojet {
    // Tout le code du projet vit ici
}
```

Ce namespace racine est l'équivalent du sous-répertoire dans `include/` (section 46.1) : il crée un préfixe unique qui identifie l'origine de chaque symbole. Quand un développeur lit `monprojet::Engine`, il sait immédiatement de quel projet il s'agit, même dans un fichier qui inclut des dizaines de bibliothèques.

### Sous-namespaces par composant

Au-delà du namespace racine, la structure interne reflète l'architecture logique du projet. Le découpage suit généralement les mêmes frontières que l'organisation des répertoires :

```
include/monprojet/
├── core/           →  namespace monprojet::core
├── network/        →  namespace monprojet::network
├── utils/          →  namespace monprojet::utils
└── config/         →  namespace monprojet::config
```

```cpp
namespace monprojet::core {
    class Engine { /* ... */ };
}

namespace monprojet::network {
    class TcpServer { /* ... */ };
}

namespace monprojet::utils {
    std::string trim(std::string_view input);
}
```

Ce parallélisme entre structure physique (répertoires) et structure logique (namespaces) n'est pas obligatoire, mais il réduit la charge cognitive : quand on cherche `monprojet::network::TcpServer`, on sait qu'il faut regarder dans `include/monprojet/network/`.

### Profondeur de la hiérarchie

En pratique, deux à trois niveaux suffisent pour la majorité des projets :

```
monprojet                     # Racine  
monprojet::core               # Composant  
monprojet::core::detail       # Implémentation interne  
```

Au-delà de trois niveaux, les noms qualifiés deviennent pénibles à écrire et à lire. Si vous ressentez le besoin d'aller plus profond, c'est probablement le signe que le projet devrait être découpé en plusieurs bibliothèques distinctes.

---

## Le namespace `detail` : cacher l'implémentation

Par convention largement adoptée dans l'écosystème C++, le namespace `detail` (ou parfois `internal`) signale que son contenu est un **détail d'implémentation** qui ne fait pas partie de l'API publique. Les consommateurs de la bibliothèque ne doivent pas utiliser directement les symboles de `detail`, même s'ils y ont techniquement accès.

```cpp
// include/monprojet/core/engine.h
#pragma once

namespace monprojet::core {

// API publique
class Engine {  
public:  
    void process();
private:
    // ...
};

namespace detail {

// Implémentation interne — ne pas utiliser directement.
// Peut changer sans préavis entre les versions.
struct WorkItem {
    int priority;
    std::string payload;
};

void schedule_work(const WorkItem& item);

} // namespace detail

} // namespace monprojet::core
```

Ce pattern est utilisé massivement dans Boost, dans la bibliothèque standard de GCC et Clang (namespace `__detail`), et dans la plupart des grands projets open source. Il ne fournit pas d'encapsulation au sens strict (rien n'empêche techniquement un utilisateur d'appeler `detail::schedule_work`), mais il établit un contrat social clair : ce qui est dans `detail` peut changer sans préavis et n'est pas couvert par les garanties de stabilité d'API.

Quand c'est possible, la meilleure approche reste de placer les détails d'implémentation dans les headers internes du répertoire `src/` (section 46.1), qui ne sont pas du tout visibles par les consommateurs. Le namespace `detail` est utile quand le code doit rester dans un header public pour des raisons techniques (templates instanciés par le code utilisateur, par exemple).

---

## Namespaces anonymes : la liaison interne

Un namespace anonyme (sans nom) rend ses symboles visibles uniquement dans l'unité de traduction courante. C'est l'équivalent C++ du mot-clé `static` appliqué aux fonctions et variables au niveau fichier en C.

```cpp
// src/core/engine.cpp

namespace {

// Ces symboles ne sont visibles que dans engine.cpp.
// Aucun autre fichier ne peut y accéder, même par accident.

constexpr int kMaxRetries = 3;  
constexpr auto kTimeout = std::chrono::seconds(30);  

bool validate_config(const std::string& name) {
    return !name.empty() && name.size() < 256;
}

} // namespace anonyme

namespace monprojet::core {

Engine::Engine(std::string config_name)
    : impl_(std::make_unique<Impl>())
{
    if (!validate_config(config_name)) {
        throw std::invalid_argument("Invalid config name");
    }
    impl_->config_name = std::move(config_name);
}

} // namespace monprojet::core
```

### Pourquoi préférer les namespaces anonymes à `static`

Le mot-clé `static` appliqué à une fonction libre ou à une variable globale produit le même effet de liaison interne. Mais le namespace anonyme est préféré en C++ moderne pour plusieurs raisons :

```cpp
// Style C — fonctionne mais déconseillé en C++
static int helper_count = 0;  
static void do_internal_work() { /* ... */ }  

// Style C++ — préféré
namespace {  
int helper_count = 0;  
void do_internal_work() { /* ... */ }  
}
```

D'abord, le namespace anonyme fonctionne pour tous les types de symboles : fonctions, variables, classes, enums, typedefs. Le `static` au niveau fichier ne s'applique qu'aux fonctions et variables. Si vous avez besoin de définir une classe auxiliaire locale à un `.cpp`, le namespace anonyme est la seule option propre. Ensuite, un namespace anonyme regroupe visuellement les symboles internes, ce qui améliore la lisibilité.

### Règle d'or : jamais dans un header

Un namespace anonyme dans un header est un bug. Comme le header est inclus dans plusieurs unités de traduction, chaque inclusion crée une copie distincte des symboles — avec des adresses différentes, des instances de variables statiques distinctes, et un comportement potentiellement indéfini si ces symboles sont utilisés dans des contextes qui attendent une identité unique.

```cpp
// ⚠️ ERREUR : namespace anonyme dans un header
// header.h
namespace {  
int global_counter = 0;  // Chaque .cpp qui inclut ce header  
                          // aura son propre global_counter !
}
```

Les namespaces anonymes sont réservés aux fichiers `.cpp`, sans exception.

---

## `using` : les règles d'or

Le mot-clé `using` est l'outil qui permet de raccourcir les noms qualifiés. C'est aussi l'outil qui, mal utilisé, détruit les bénéfices des namespaces. Les règles sont simples mais non négociables.

### Ce qui est interdit : `using namespace` dans un header

```cpp
// ⛔ INTERDIT — include/monprojet/core/engine.h
#pragma once
using namespace std;  // Pollution globale exportée
                      // vers tous les fichiers qui incluent ce header

namespace monprojet::core {  
class Engine {  
    string name_;     // Fonctionne grâce au using ci-dessus
    vector<int> data_;
};
}
```

Ce `using namespace std;` ne se contente pas de polluer l'espace de noms de `engine.h` : il pollue l'espace de noms de **chaque fichier qui inclut `engine.h`**, directement ou transitivement. Si un consommateur définit une fonction `count` ou une classe `array`, elle entre en conflit avec `std::count` ou `std::array`. Le pire : ces conflits peuvent apparaître silencieusement lors de l'ajout d'un nouveau header standard, des mois après l'écriture du code.

Cette règle est universelle : le Google C++ Style Guide l'interdit, les C++ Core Guidelines l'interdisent, le LLVM Style l'interdit. Il n'y a pas de cas légitime pour `using namespace` dans un header.

### Ce qui est acceptable dans un fichier `.cpp`

Dans un fichier source, la portée du `using` est limitée à l'unité de traduction. Les risques sont réduits, mais les pratiques varient selon les standards de codage.

**`using namespace` dans un scope limité** — généralement accepté :

```cpp
// src/core/engine.cpp
void Engine::process() {
    using namespace std::chrono_literals;

    auto timeout = 500ms;  // Lisible grâce au using local
    auto deadline = std::chrono::steady_clock::now() + timeout;
    // ...
}
```

Le `using namespace` est confiné dans le corps de la fonction. Il n'affecte rien en dehors.

**`using` de symboles spécifiques** — la forme la plus sûre :

```cpp
// src/core/engine.cpp
using std::string;  
using std::vector;  
using std::unique_ptr;  

// Ou dans un scope local :
void Engine::process() {
    using std::chrono::steady_clock;
    using std::chrono::milliseconds;

    auto start = steady_clock::now();
    // ...
}
```

Cette forme est préférable au `using namespace` complet car elle ne rend visible que les symboles explicitement nommés. Si un conflit existe, il est détecté à l'ajout du `using`, pas dans un endroit obscur du code.

**`using namespace std;` au niveau du fichier `.cpp`** — toléré par certaines équipes, déconseillé par la plupart :

```cpp
// src/core/engine.cpp
using namespace std;  // Techniquement limité à cette unité de traduction

void Engine::process() {
    vector<string> items;  // Plus court à écrire
    // ...
}
```

C'est un sujet de débat légitime entre équipes. L'argument en faveur est la lisibilité locale : `vector<string>` est plus léger que `std::vector<std::string>`. L'argument contre est le risque de collisions avec des noms courants (`count`, `distance`, `move`, `swap`, `size`, `data`, `begin`, `end`) et le fait que le `std::` qualificatif documente l'origine du symbole. La plupart des grands projets open source et les guides de style industriels déconseillent cette pratique, même dans les `.cpp`.

### Récapitulatif des règles `using`

| Contexte | `using namespace X;` | `using X::symbol;` |
|---|---|---|
| Header (tout scope) | **Interdit** | **Interdit** (sauf `using` de type dans une classe) |
| `.cpp` — scope fichier | Déconseillé | Acceptable |
| `.cpp` — scope local (fonction, bloc) | Acceptable | Acceptable |

---

## `inline namespace` : le versioning d'API

Les `inline namespaces` (C++11) sont un mécanisme subtil mais puissant pour gérer le versioning d'une bibliothèque. Un namespace `inline` rend ses symboles accessibles comme s'ils appartenaient au namespace parent, tout en conservant leur nom qualifié complet.

### Le problème : faire coexister deux versions d'une API

Supposons que votre bibliothèque expose une classe `Config` en version 1. La version 2 change le layout de `Config` (incompatibilité ABI). Vous voulez que les nouveaux utilisateurs obtiennent la v2 par défaut, tout en permettant aux anciens utilisateurs de spécifier explicitement la v1 pendant la période de transition.

### La solution avec `inline namespace`

```cpp
// include/monprojet/config.h
#pragma once

namespace monprojet {

namespace v1 {
    struct Config {
        std::string host;
        int port;
    };
}

inline namespace v2 {
    struct Config {
        std::string host;
        int port;
        int max_connections;    // Nouveau champ en v2
        bool tls_enabled;       // Nouveau champ en v2
    };
}

} // namespace monprojet
```

Avec cette déclaration :

```cpp
// Utilisation par défaut — résout vers v2 grâce au inline
monprojet::Config cfg;           // monprojet::v2::Config

// Accès explicite à v1 pendant la migration
monprojet::v1::Config legacy;    // monprojet::v1::Config

// Le nom qualifié complet fonctionne aussi
monprojet::v2::Config explicit_v2;
```

Le mot-clé `inline` sur le namespace `v2` fait de `v2` le "namespace par défaut" : les symboles de `v2` sont accessibles directement via `monprojet::`, sans qualifier `v2`. Mais `v1` reste accessible via son nom complet `monprojet::v1::`.

### Utilisation en pratique

L'`inline namespace` est utilisé dans la bibliothèque standard elle-même. Par exemple, `std::string_literals` est dans un `inline namespace` pour permettre `using namespace std::string_literals;` sans importer tout `std`. La bibliothèque Abseil de Google utilise massivement les `inline namespaces` pour versionner son ABI.

Pour un projet classique, les `inline namespaces` deviennent pertinents quand la bibliothèque est distribuée en binaire (`.so` ou `.a`) et que la compatibilité ABI entre versions est un enjeu. Pour un projet compilé de bout en bout par ses consommateurs, le mécanisme est moins critique.

---

## Namespace aliases : raccourcis contrôlés

Les alias de namespaces permettent de créer des noms courts pour des namespaces profondément imbriqués :

```cpp
namespace fs = std::filesystem;  
namespace json = nlohmann;  
namespace chrono = std::chrono;  

void process_files() {
    for (const auto& entry : fs::directory_iterator("/tmp")) {
        if (entry.is_regular_file()) {
            auto modified = fs::last_write_time(entry);
            // ...
        }
    }
}
```

Les alias suivent les mêmes règles que les `using` : ils sont bienvenus dans les fichiers `.cpp` et dans les scopes locaux, mais n'ont pas leur place dans les headers publics. Un alias dans un header impose un choix de nommage à tous les consommateurs et peut entrer en conflit avec leurs propres alias.

L'exception est un alias défini à l'intérieur du namespace du projet, pour l'usage interne :

```cpp
// include/monprojet/core/types.h
#pragma once
#include <chrono>

namespace monprojet::core {

// Alias internes au projet — n'affectent pas l'espace global
using Clock = std::chrono::steady_clock;  
using Duration = std::chrono::milliseconds;  
using TimePoint = Clock::time_point;  

} // namespace monprojet::core
```

Ces alias sont encapsulés dans `monprojet::core` et ne polluent pas l'espace global. Ils standardisent les types temporels à travers le projet tout en restant un détail interne.

---

## Argument-Dependent Lookup (ADL) : le mécanisme invisible

L'ADL (aussi appelé Koenig Lookup) est une règle de résolution de noms qui interagit avec les namespaces de manière parfois surprenante. Quand une fonction est appelée sans qualification, le compilateur cherche cette fonction non seulement dans les scopes courants, mais aussi dans les **namespaces des types de ses arguments**.

```cpp
namespace monprojet::network {

struct Packet {
    std::vector<std::byte> data;
};

// Fonction libre dans le même namespace que Packet
void send(const Packet& pkt) {
    // ...
}

} // namespace monprojet::network

// Ailleurs dans le code :
void process() {
    monprojet::network::Packet pkt;

    // ADL trouve monprojet::network::send automatiquement
    // car pkt est de type monprojet::network::Packet
    send(pkt);  // OK : résolu par ADL
}
```

### ADL et `operator<<`

L'ADL est la raison pour laquelle `operator<<` fonctionne avec `std::cout` sans qualification explicite :

```cpp
namespace monprojet::core {

struct Point {
    double x, y;
};

// operator<< dans le même namespace que Point
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "(" << p.x << ", " << p.y << ")";
}

} // namespace monprojet::core

// Ailleurs :
monprojet::core::Point p{3.0, 4.0};  
std::cout << p;  // ADL trouve monprojet::core::operator<<  
```

Si l'`operator<<` était défini dans l'espace global au lieu du namespace `monprojet::core`, il fonctionnerait aussi — mais au prix d'une pollution de l'espace global. La bonne pratique est de toujours définir les opérateurs surchargés dans le même namespace que le type sur lequel ils opèrent, pour que l'ADL les trouve naturellement.

### ADL et `swap`

Le pattern le plus classique où l'ADL est intentionnellement exploité est la personnalisation de `swap` :

```cpp
namespace monprojet::core {

class Engine {  
public:  
    friend void swap(Engine& a, Engine& b) noexcept {
        using std::swap;     // Rend std::swap visible comme fallback
        swap(a.impl_, b.impl_);  // ADL cherche d'abord dans le namespace
    }                             // de impl_, puis tombe sur std::swap

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace monprojet::core
```

Le `using std::swap;` suivi d'un appel non qualifié à `swap` est le pattern canonique : il permet à l'ADL de trouver une version spécialisée de `swap` si elle existe dans le namespace du type, tout en offrant `std::swap` comme fallback.

### Les pièges de l'ADL

L'ADL peut provoquer des résolutions inattendues. Quand vous écrivez un appel de fonction non qualifié, le compilateur explore potentiellement des dizaines de namespaces liés aux types des arguments. Cela peut créer des ambiguïtés ou des appels à des fonctions que vous n'aviez pas l'intention d'utiliser.

Pour éviter les surprises, la règle est simple : **qualifiez explicitement les appels de fonction** sauf quand vous exploitez intentionnellement l'ADL (comme pour `swap` ou les opérateurs). En cas de doute, le nom qualifié `monprojet::network::send(pkt)` est toujours plus sûr que l'appel non qualifié `send(pkt)`.

---

## Patterns d'organisation à l'échelle d'un projet

### Le pattern "un namespace par bibliothèque"

Dans un monorepo multi-bibliothèques (section 46.1), chaque bibliothèque a son propre namespace de premier niveau sous le namespace racine :

```
monprojet::core       ←  libs/core/  
monprojet::network    ←  libs/networking/  
monprojet::storage    ←  libs/storage/  
```

Les dépendances entre bibliothèques sont explicites dans le code :

```cpp
// libs/network/src/http_server.cpp
#include <monprojet/core/engine.h>      // Dépendance inter-libs
#include <monprojet/network/http_server.h>

namespace monprojet::network {

HttpServer::HttpServer(core::Engine& engine)
    : engine_(engine)
{
    // core:: qualifie explicitement le composant externe
}

} // namespace monprojet::network
```

### Le pattern "namespace fonctionnel"

Certains projets organisent leurs namespaces par fonctionnalité plutôt que par couche architecturale :

```
monprojet::http       ←  Tout ce qui touche au HTTP  
monprojet::json       ←  Parsing et sérialisation JSON  
monprojet::logging    ←  Infrastructure de logging  
monprojet::metrics    ←  Collecte de métriques  
```

Ce pattern est adapté aux projets dont les composants sont fortement cohésifs et faiblement couplés — typiquement les bibliothèques utilitaires ou les frameworks.

### Ce qu'il faut éviter

**Des namespaces trop génériques** : `namespace utils`, `namespace helpers`, `namespace common` sont des fourre-tout qui finissent par contenir des centaines de symboles sans rapport entre eux. Si un namespace grossit sans cohérence, c'est le signe qu'il faut le subdiviser.

**Des namespaces qui dupliquent le nom de la classe** : `monprojet::engine::Engine` est redondant. Préférer `monprojet::core::Engine` ou simplement `monprojet::Engine` si le projet est suffisamment petit.

**Des noms de namespaces qui entrent en conflit avec des noms standard** : un namespace `std`, `posix`, `chrono` ou `filesystem` au premier niveau de votre projet est une invitation aux catastrophes. Même `net`, `io` ou `os` peuvent poser problème si des bibliothèques tierces utilisent les mêmes noms.

---

## Résumé

Les namespaces sont le rempart contre la pollution de l'espace global en C++. Un projet professionnel encapsule tout son code dans un namespace racine portant le nom du projet, subdivisé en sous-namespaces qui reflètent l'architecture logique. Le namespace `detail` signale les détails d'implémentation non contractuels. Les namespaces anonymes assurent la liaison interne dans les fichiers `.cpp`. Les `using namespace` sont proscrits dans les headers et utilisés avec parcimonie dans les sources. Les `inline namespaces` permettent le versioning d'API. Enfin, l'ADL est un mécanisme puissant qu'il faut comprendre pour éviter les résolutions de noms inattendues et exploiter correctement dans les patterns comme `swap` et les opérateurs surchargés.

---


⏭️ [Documentation : Doxygen et commentaires](/46-organisation-standards/04-documentation-doxygen.md)
