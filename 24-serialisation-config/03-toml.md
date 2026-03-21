🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 24.3 — TOML : Alternative moderne (toml++)

## Section du Module 8 — Parsing et Formats de Données

---

## Un format conçu pour la configuration

TOML (Tom's Obvious, Minimal Language) est né en 2013 d'un constat simple de Tom Preston-Werner, cofondateur de GitHub : les formats existants pour la configuration étaient soit trop limités (INI), soit trop permissifs et piégeux (YAML), soit inadaptés à l'édition humaine (JSON). TOML a été conçu avec un objectif unique — **être un format de fichier de configuration sans ambiguïté, facile à lire et à écrire pour un humain, et facile à parser pour une machine**.

En 2026, TOML est le format de configuration par défaut de Rust (Cargo.toml), Python (pyproject.toml), Go (de nombreux outils), Hugo, Starship et un nombre croissant de projets. Son adoption continue de progresser dans les écosystèmes où la clarté de la configuration est valorisée.

Voici un fichier TOML typique :

```toml
# Configuration du serveur d'API

[server]
host = "0.0.0.0"  
port = 8080  
workers = 4  
debug = false  

[server.tls]
cert_path = "/etc/ssl/server.crt"  
key_path = "/etc/ssl/server.key"  
verify_client = true  

[database]
host = "db.internal"  
port = 5432  
name = "production"  
connection_pool = 20  
query_timeout_ms = 5000  

[[server.endpoints]]
path = "/health"  
method = "GET"  
timeout_ms = 1000  

[[server.endpoints]]
path = "/api/data"  
method = "POST"  
timeout_ms = 5000  

[logging]
level = "info"  
outputs = ["stdout", "/var/log/api/server.log"]  
```

La syntaxe est immédiatement lisible. Les sections sont délimitées par des en-têtes entre crochets (`[section]`), les valeurs sont typées explicitement (chaînes toujours quotées, entiers et flottants distincts, booléens sans ambiguïté), et les commentaires sont supportés avec `#`. Il n'y a pas de pièges de typage implicite, pas d'indentation structurelle, pas d'ambiguïté sur le type d'une valeur.

---

## TOML vs YAML vs JSON : positionnement

Le tableau suivant résume les différences pertinentes pour le choix d'un format de configuration :

| Critère | TOML | YAML | JSON |
|---------|------|------|------|
| Commentaires | Oui (`#`) | Oui (`#`) | Non |
| Typage explicite | Oui (chaînes toujours quotées) | Non (typage implicite) | Partiellement (chaînes quotées) |
| Indentation structurelle | Non | Oui (source de bugs) | Non |
| Piège `NO` → `false` | Impossible | Oui (YAML 1.1) | Impossible |
| Structures profondément imbriquées | Lisibilité dégradée | Bon support | Bon support |
| Multi-documents | Non | Oui | Non |
| Ancres / références | Non | Oui | Non |
| Dates et heures natives | Oui (RFC 3339) | Oui (implicite) | Non |
| Spécification | Simple (~800 lignes) | Complexe (~80 pages) | Simple (~1 page) |

**TOML excelle** pour les fichiers de configuration à structure plate ou modérément imbriquée, où chaque valeur doit être sans ambiguïté. **YAML reste préférable** pour les structures profondément imbriquées (manifestes Kubernetes) ou quand l'écosystème l'impose. **JSON reste le choix naturel** pour l'échange de données entre programmes.

La règle pratique : si le fichier est principalement écrit par un humain et lu par un programme, et que sa structure ne dépasse pas 2-3 niveaux d'imbrication, TOML est souvent le meilleur choix.

---

## toml++ : la librairie C++ de référence

**toml++** (prononcé « toml plus plus »), créée par Mark Gillard, est la librairie TOML la plus mature et la plus utilisée en C++. Elle implémente la spécification TOML v1.0.0, est **header-only**, et exploite pleinement le C++ moderne (C++17 minimum, tirant parti de C++20 quand disponible).

### Caractéristiques principales

- **Header-only** — un seul `#include <toml++/toml.hpp>`, aucune librairie à linker.  
- **Conformité complète** — passe l'intégralité de la test suite officielle TOML v1.0.0.  
- **API ergonomique** — navigation par `[]`, extraction typée, conversions vers les types STL.  
- **Types natifs riches** — supporte les dates, heures et timestamps TOML comme types de première classe.  
- **Messages d'erreur détaillés** — positions ligne:colonne dans le fichier source.  
- **Performance** — parsing rapide, empreinte mémoire raisonnable.

---

## Installation

### Conan 2

```python
# conanfile.py
def requirements(self):
    self.requires("tomlplusplus/3.4.0")
```

```cmake
find_package(tomlplusplus REQUIRED)  
target_link_libraries(myapp PRIVATE tomlplusplus::tomlplusplus)  
```

### vcpkg

```bash
vcpkg install tomlplusplus
```

```cmake
find_package(tomlplusplus CONFIG REQUIRED)  
target_link_libraries(myapp PRIVATE tomlplusplus::tomlplusplus)  
```

### CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(tomlplusplus)

target_link_libraries(myapp PRIVATE tomlplusplus::tomlplusplus)
```

### Header direct

```bash
mkdir -p third_party  
curl -L -o third_party/toml.hpp \  
    https://raw.githubusercontent.com/marzer/tomlplusplus/v3.4.0/toml.hpp
```

> 💡 *toml++ fournit un single header (`toml.hpp`) et une version multi-headers (`toml++/toml.hpp`). Le single header est pratique pour l'intégration rapide ; la version multi-headers réduit le temps de compilation incrémentale dans les gros projets.*

---

## Parsing

### Depuis un fichier

```cpp
#include <toml++/toml.hpp>
#include <print>

int main() {
    try {
        toml::table config = toml::parse_file("config.toml");

        std::print("Type du nœud racine : table\n");
        std::print("Nombre de clés racine : {}\n", config.size());

    } catch (const toml::parse_error& err) {
        std::print(stderr, "Erreur de parsing TOML :\n  {}\n", err.what());
        // err.source() fournit le fichier, la ligne et la colonne
        if (err.source().path) {
            std::print(stderr, "  Fichier : {}\n", *err.source().path);
        }
        std::print(stderr, "  Position : ligne {}, colonne {}\n",
            err.source().begin.line, err.source().begin.column);
        return 1;
    }
}
```

`toml::parse_file` retourne une `toml::table` — l'équivalent TOML du `nlohmann::json` ou du `YAML::Node`. L'exception `toml::parse_error` fournit nativement le chemin du fichier, la ligne et la colonne de l'erreur, sans conversion manuelle d'offset en position.

### Depuis une chaîne

```cpp
toml::table config = toml::parse(R"(
    [server]
    host = "localhost"
    port = 8080
)");
```

### Depuis un flux

```cpp
std::ifstream file("config.toml");  
toml::table config = toml::parse(file, "config.toml"s);  
// Le deuxième paramètre est le nom du fichier pour les messages d'erreur
// (utiliser le suffixe 's' pour construire un std::string et lever l'ambiguïté)
```

### Parsing sans exception

Pour les contextes où les exceptions ne sont pas souhaitables, toml++ supporte un mode sans exceptions activé par la macro `TOML_EXCEPTIONS=0`. Dans ce mode, `toml::parse` retourne un `toml::parse_result` qui agit comme un variant succès/erreur :

```cpp
// Nécessite la compilation avec TOML_EXCEPTIONS=0
// (par défaut, les exceptions sont activées et parse_result == toml::table)

toml::parse_result result = toml::parse_file("config.toml");

if (!result) {
    const toml::parse_error& err = result.error();
    std::print(stderr, "TOML invalide : {}\n", err.what());
    return 1;
}

// Accès au résultat comme une table
toml::table& config = result.table();  
std::string host = config["server"]["host"].value_or("localhost"s);  
```

> ⚠️ *Par défaut (exceptions activées), `toml::parse_result` est un alias pour `toml::table`. Les méthodes `!result`, `.error()` et `.table()` ne sont disponibles que quand la macro `TOML_EXCEPTIONS` est définie à `0`. C'est une différence importante avec `json::parse(input, nullptr, false)` de nlohmann/json qui fonctionne indépendamment du mode d'exceptions.*

---

## Navigation et extraction

### Le système de types TOML

TOML définit des types plus riches que JSON, avec notamment une distinction nette entre entiers et flottants, et un support natif des dates et heures :

| Type TOML | Type C++ (toml++) | Exemple TOML |
|-----------|------------------|--------------|
| String | `std::string` | `name = "api"` |
| Integer | `int64_t` | `port = 8080` |
| Float | `double` | `rate = 0.75` |
| Boolean | `bool` | `debug = false` |
| Date-Time | `toml::date_time` | `created = 2026-03-14T10:30:00Z` |
| Local Date | `toml::date` | `release = 2026-03-14` |
| Local Time | `toml::time` | `backup_at = 03:00:00` |
| Array | `toml::array` | `ports = [8080, 8443]` |
| Table | `toml::table` | `[server]` |

Le support natif des dates et heures est un avantage significatif de TOML sur JSON (qui les encode comme des chaînes) et sur YAML (qui les détecte par typage implicite, source de bugs).

### Accès par clé

L'accès par `[]` retourne un `toml::node_view` — une vue légère sur le nœud, qui peut être absente (évalue à `false`) :

```cpp
toml::table config = toml::parse_file("config.toml");

// Accès imbriqué
toml::node_view host_node = config["server"]["host"];

if (host_node) {
    std::print("Host trouvé : {}\n", host_node.as_string()->get());
}
```

### Extraction typée avec `.value<T>()`

La méthode `.value<T>()` extrait un `std::optional<T>`. C'est l'approche la plus sûre — pas d'exception, le type de retour encode explicitement l'absence :

```cpp
toml::table config = toml::parse_file("config.toml");

std::optional<std::string> host = config["server"]["host"].value<std::string>();  
std::optional<int64_t> port = config["server"]["port"].value<int64_t>();  

if (host && port) {
    std::print("Serveur : {}:{}\n", *host, *port);
}
```

### Extraction avec valeur par défaut via `.value_or()`

Pour les champs optionnels, `.value_or()` combine l'extraction et le fallback en un seul appel :

```cpp
std::string host = config["server"]["host"].value_or("localhost"s);  
int64_t port     = config["server"]["port"].value_or(int64_t{8080});  
int64_t workers  = config["server"]["workers"].value_or(int64_t{4});  
bool debug       = config["server"]["debug"].value_or(false);  
```

> ⚠️ *Le type du défaut doit correspondre exactement au type TOML attendu. Un entier TOML est `int64_t`, pas `int`. Passer `8080` (un `int`) au lieu de `int64_t{8080}` peut provoquer une ambiguïté de compilation. L'utilisation explicite du type ou d'un suffixe littéral évite ce problème.*

### Accès chemin profond avec notation pointée

toml++ supporte l'accès par chemin pointé sur les tables, ce qui simplifie la navigation dans les structures imbriquées :

```cpp
// Équivalent de config["server"]["tls"]["cert_path"]
auto cert = config.at_path("server.tls.cert_path").value<std::string>();
```

`at_path` est particulièrement utile quand le chemin d'accès est lui-même une donnée (lu depuis une variable ou un argument).

---

## Itération

### Sur une table

```cpp
toml::table config = toml::parse_file("config.toml");

for (auto& [key, value] : config) {
    std::print("{} (type={})\n",
        std::string_view{key},
        static_cast<int>(value.type()));
}
```

L'itération produit des paires `toml::key` / `toml::node&`. La clé se convertit en `std::string_view` pour l'affichage. La méthode `.type()` retourne un `toml::node_type` (énumération) qui identifie le type du nœud (table, array, string, integer, etc.). Le type `toml::node_type` ne possède pas de `std::formatter` ; il faut le caster en entier ou le convertir manuellement en chaîne pour l'afficher.

### Sur un tableau

```cpp
if (auto* endpoints = config["server"]["endpoints"].as_array()) {
    for (const auto& ep : *endpoints) {
        if (const auto* tbl = ep.as_table()) {
            std::string path = (*tbl)["path"].value_or(""s);
            std::string method = (*tbl)["method"].value_or(""s);
            std::print("{} {}\n", method, path);
        }
    }
}
```

### Tableaux de tables (`[[section]]`)

La syntaxe TOML `[[section]]` (double crochets) définit un tableau de tables — l'équivalent d'un tableau d'objets en JSON. C'est la manière idiomatique de représenter une liste de structures en TOML :

```toml
[[services]]
name = "api"  
port = 8080  
critical = true  

[[services]]
name = "worker"  
port = 9090  
critical = false  

[[services]]
name = "scheduler"  
port = 7070  
critical = true  
```

```cpp
if (auto* services = config["services"].as_array()) {
    for (const auto& svc : *services) {
        if (const auto* tbl = svc.as_table()) {
            auto name = (*tbl)["name"].value_or(""s);
            auto port = (*tbl)["port"].value_or(int64_t{0});
            auto critical = (*tbl)["critical"].value_or(false);

            std::print("Service {} sur le port {}{}\n",
                name, port, critical ? " [CRITICAL]" : "");
        }
    }
}
```

---

## Conversion vers des types utilisateur

toml++ ne fournit pas de mécanisme intégré de conversion automatique comme les macros `NLOHMANN_DEFINE_TYPE_*` ou le `YAML::convert<T>`. La conversion vers des types métier s'écrit manuellement sous forme de fonctions de désérialisation :

```cpp
struct TlsConfig {
    std::string cert_path;
    std::string key_path;
    bool verify_client;
};

struct ServerConfig {
    std::string host;
    int port;
    int workers;
    bool debug;
    std::optional<TlsConfig> tls;
    std::vector<std::string> allowed_origins;
};

TlsConfig parse_tls(const toml::table& tbl) {
    return TlsConfig{
        .cert_path = tbl["cert_path"].value_or(""s),
        .key_path = tbl["key_path"].value_or(""s),
        .verify_client = tbl["verify_client"].value_or(false)
    };
}

ServerConfig parse_server_config(const toml::table& root) {
    ServerConfig cfg;

    cfg.host    = root.at_path("server.host").value_or("localhost"s);
    cfg.port    = static_cast<int>(
                      root.at_path("server.port").value_or(int64_t{8080}));
    cfg.workers = static_cast<int>(
                      root.at_path("server.workers").value_or(int64_t{4}));
    cfg.debug   = root.at_path("server.debug").value_or(false);

    // TLS optionnel
    if (auto* tls_tbl = root.at_path("server.tls").as_table()) {
        cfg.tls = parse_tls(*tls_tbl);
    }

    // Tableau de chaînes
    if (auto* origins = root.at_path("server.allowed_origins").as_array()) {
        for (const auto& item : *origins) {
            if (auto val = item.value<std::string>()) {
                cfg.allowed_origins.push_back(*val);
            }
        }
    }

    return cfg;
}
```

Utilisation :

```cpp
toml::table config = toml::parse_file("config.toml");  
ServerConfig server = parse_server_config(config);  

std::print("{}:{} ({} workers)\n", server.host, server.port, server.workers);
```

L'absence de macros est un choix de design : la conversion est explicite, chaque champ est visible, et la gestion des valeurs par défaut et des champs optionnels est sous le contrôle total du développeur. Pour les projets avec un grand nombre de types, une couche de sérialisation peut être factorisée avec des templates :

```cpp
template <typename T>  
T required(const toml::table& tbl, std::string_view key) {  
    auto val = tbl[key].value<T>();
    if (!val) {
        throw std::runtime_error(
            std::format("Champ obligatoire '{}' manquant ou type invalide", key));
    }
    return *val;
}

// Utilisation
std::string host = required<std::string>(server_tbl, "host");  
int64_t port = required<int64_t>(server_tbl, "port");  
```

---

## Gestion des dates et heures

L'un des avantages distinctifs de TOML est le support natif des dates et heures conformes à la RFC 3339. toml++ les expose comme des types dédiés :

```toml
[deployment]
created_at = 2026-03-14T10:30:00Z  
release_date = 2026-03-14  
backup_time = 03:00:00  
```

```cpp
toml::table config = toml::parse_file("config.toml");

auto created = config.at_path("deployment.created_at").value<toml::date_time>();  
auto release = config.at_path("deployment.release_date").value<toml::date>();  
auto backup  = config.at_path("deployment.backup_time").value<toml::time>();  

if (created) {
    std::print("Créé le {}-{:02}-{:02} à {:02}:{:02}:{:02}\n",
        created->date.year, created->date.month, created->date.day,
        created->time.hour, created->time.minute, created->time.second);
}

if (release) {
    std::print("Release : {}-{:02}-{:02}\n",
        release->year, release->month, release->day);
}

if (backup) {
    std::print("Backup quotidien à {:02}:{:02}\n",
        backup->hour, backup->minute);
}
```

Ce support natif élimine la nécessité de parser manuellement des chaînes de date, une source d'erreurs fréquente avec JSON (où les dates sont des chaînes opaques) et YAML (où le typage implicite peut transformer une date en un type inattendu).

---

## Écriture TOML

toml++ permet de construire des tables programmatiquement et de les sérialiser. La construction utilise les méthodes `.insert()` et `.emplace()` sur les tables, ou l'assignation directe :

```cpp
toml::table config;

config.insert("title", "Mon Application");

// Sous-table
toml::table server;  
server.insert("host", "0.0.0.0");  
server.insert("port", 8080);  
server.insert("debug", false);  
config.insert("server", std::move(server));  

// Tableau
toml::array ports;  
ports.push_back(8080);  
ports.push_back(8443);  
config.insert("ports", std::move(ports));  

// Date native
config.insert("created", toml::date_time{
    toml::date{2026, 3, 14},
    toml::time{10, 30, 0}
});
```

### Sérialisation vers chaîne ou fichier

L'opérateur `<<` et `toml::toml_formatter` produisent la sortie TOML :

```cpp
// Vers fichier
std::ofstream file("output.toml");  
file << toml::toml_formatter{config};  

// Vers stdout (via operator<< sur std::cout ou via ostringstream)
std::cout << toml::toml_formatter{config} << "\n";
```

### Formateurs alternatifs

toml++ fournit plusieurs formateurs pour des besoins différents :

```cpp
// Format TOML standard (par défaut)
std::cout << toml::toml_formatter{config} << "\n";

// Format JSON (utile pour l'interopérabilité ou le débogage)
std::cout << toml::json_formatter{config} << "\n";

// Format YAML
std::cout << toml::yaml_formatter{config} << "\n";
```

> ⚠️ *Les formateurs toml++ ne disposent pas de `std::formatter` et ne sont donc pas compatibles avec `std::print` / `std::format`. Utiliser `operator<<` avec `std::cout` ou un `std::ostringstream` pour la conversion en chaîne.*

La possibilité d'émettre en JSON ou YAML depuis un arbre TOML est un outil de débogage utile et facilite la migration entre formats.

---

## Validation

La validation d'un fichier TOML suit les mêmes principes que pour JSON et YAML. Le parsing syntaxique est pris en charge par la librairie avec des messages d'erreur de qualité. La validation sémantique est de la responsabilité du code applicatif :

```cpp
struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;

    void add(std::string msg) {
        valid = false;
        errors.push_back(std::move(msg));
    }
};

ValidationResult validate_config(const toml::table& config) {
    ValidationResult v;

    // Vérification de section obligatoire
    if (!config.contains("server")) {
        v.add("Section [server] obligatoire manquante");
        return v;
    }

    auto* server = config["server"].as_table();
    if (!server) {
        v.add("[server] doit être une table");
        return v;
    }

    // Champ obligatoire avec vérification de type
    if (auto port = (*server)["port"].value<int64_t>()) {
        if (*port < 1 || *port > 65535) {
            v.add(std::format("server.port hors limites : {}", *port));
        }
    } else {
        v.add("server.port obligatoire (entier entre 1 et 65535)");
    }

    if (!(*server)["host"].value<std::string>()) {
        v.add("server.host obligatoire (chaîne)");
    }

    return v;
}
```

Grâce au typage explicite de TOML, certaines classes de bugs n'existent pas : un entier est toujours un entier, une chaîne est toujours une chaîne, un booléen est toujours `true` ou `false`. La validation peut se concentrer sur les contraintes métier (plages de valeurs, champs obligatoires, cohérence entre champs) plutôt que sur la vérification de types, qui est garantie par le format lui-même.

---

## Quand choisir TOML

**TOML est le bon choix quand :**

- Le fichier est un fichier de configuration applicative écrit et maintenu par des humains.  
- La structure est plate ou modérément imbriquée (2-3 niveaux).  
- La clarté et l'absence d'ambiguïté priment sur la compacité.  
- Le projet n'est pas contraint par un écosystème imposant un autre format.  
- Des dates et heures apparaissent dans la configuration.

**TOML n'est pas le bon choix quand :**

- La structure est profondément imbriquée (plus de 3 niveaux) — la syntaxe TOML devient lourde avec les chemins de sections à rallonge (`[a.b.c.d.e]`).  
- Le format est imposé par l'écosystème (Kubernetes → YAML, API REST → JSON).  
- Les ancres, références ou multi-documents sont nécessaires (fonctionnalités YAML absentes de TOML).  
- Le fichier est principalement généré et consommé par des machines — JSON ou un format binaire est plus adapté.

---

## Résumé comparatif des API

| Opération | toml++ | nlohmann/json | yaml-cpp |
|-----------|--------|---------------|----------|
| Parsing fichier | `toml::parse_file(path)` | `json::parse(ifstream)` | `YAML::LoadFile(path)` |
| Parsing chaîne | `toml::parse(str)` | `json::parse(str)` | `YAML::Load(str)` |
| Extraction typée | `.value<T>()` → `optional` | `.get<T>()` → exception | `.as<T>()` → exception |
| Extraction + défaut | `.value_or(def)` | `.value("key", def)` | `.as<T>(def)` |
| Test d'existence | `if (node)` / `.contains()` | `.contains()` | `if (node)` |
| Accès chemin profond | `.at_path("a.b.c")` | `j["a"]["b"]["c"]` | `node["a"]["b"]["c"]` |
| Erreur de parsing | `toml::parse_error` (ligne:col) | `json::parse_error` (byte) | `YAML::ParserException` (ligne:col) |
| Parsing sans exception | `toml::parse_result` | `parse(s, nullptr, false)` | Non disponible |
| Macro de conversion | Non disponible | `NLOHMANN_DEFINE_TYPE_*` | Non disponible |
| Conversion custom | Fonctions manuelles | `to_json`/`from_json` (ADL) | `YAML::convert<T>` |
| Header-only | Oui | Oui | Non |

La section 24.4 aborde XML avec pugixml, un format très différent dans sa philosophie, mais encore incontournable pour l'interfaçage avec les systèmes legacy.

⏭️ [XML : Parsing avec pugixml (legacy systems)](/24-serialisation-config/04-xml-pugixml.md)
