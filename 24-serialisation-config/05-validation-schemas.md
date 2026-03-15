🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 24.5 — Bonnes pratiques : Validation de schémas

## Section du Module 8 — Parsing et Formats de Données

---

## Vue d'ensemble

Les sections précédentes ont montré comment parser et sérialiser des données dans quatre formats textuels. Chaque section abordait la gestion des erreurs propre à sa librairie — erreurs de syntaxe, types incorrects, champs manquants. Mais ces vérifications restaient fragmentées, mêlées au code de désérialisation.

Cette section prend du recul pour traiter la **validation** comme une préoccupation architecturale à part entière. L'objectif est d'établir des pratiques applicables à tous les formats, de distinguer les différents niveaux de validation, et de fournir des patterns réutilisables pour construire une couche de validation solide dans un projet C++ professionnel.

---

## Les trois niveaux de validation

Toute donnée entrante traverse conceptuellement trois niveaux de validation avant d'être exploitable par l'application. Chaque niveau filtre une catégorie d'erreurs différente :

```
     Données brutes (fichier, réseau, stdin)
              │
              ▼
    ┌───────────────────────┐
    │  Niveau 1 : Syntaxe   │  Le document est-il bien formé ?
    │  (parser)             │  JSON/YAML/TOML/XML valide ?
    └─────────┬─────────────┘
              │ ✓
              ▼
    ┌───────────────────────┐
    │  Niveau 2 : Structure │  Les champs attendus sont-ils présents ?
    │  (schéma)             │  Les types sont-ils corrects ?
    └─────────┬─────────────┘
              │ ✓
              ▼
    ┌────────────────────────┐
    │  Niveau 3 : Sémantique │  Les valeurs sont-elles cohérentes ?
    │  (logique métier)      │  Les contraintes métier sont-elles respectées ?
    └─────────┬──────────────┘
              │ ✓
              ▼
       Données validées
       (types C++ métier)
```

### Niveau 1 : Validation syntaxique

C'est le travail du parser. Le document est-il du JSON/YAML/TOML/XML bien formé ? Les guillemets sont-ils fermés, les accolades équilibrées, l'indentation cohérente ? Ce niveau est entièrement pris en charge par les librairies couvertes dans ce chapitre : `json::parse`, `YAML::LoadFile`, `toml::parse_file`, `doc.load_file`. Aucun code applicatif n'est nécessaire à ce stade.

### Niveau 2 : Validation structurelle

Le document est syntaxiquement valide, mais contient-il les données attendues ? Ce niveau vérifie la **présence des champs obligatoires**, le **type de chaque champ** (entier, chaîne, tableau, objet), et les **contraintes de cardinalité** (un tableau doit contenir au moins un élément, une map doit avoir exactement certaines clés).

C'est à ce niveau qu'interviennent les schémas formels (JSON Schema, XSD) ou la validation applicative manuelle.

### Niveau 3 : Validation sémantique

La structure est correcte, mais les valeurs ont-elles du sens ? Un port est-il entre 1 et 65535 ? Le chemin du certificat TLS existe-t-il sur le système de fichiers ? L'URL de la base de données est-elle syntaxiquement valide ? Deux champs mutuellement exclusifs ne sont-ils pas renseignés simultanément ?

Ce niveau est toujours de la responsabilité du code applicatif, quel que soit le format utilisé.

---

## Validation structurelle : approche par schéma formel

### JSON Schema

JSON Schema est le standard de facto pour décrire la structure d'un document JSON. Le schéma est lui-même un document JSON qui spécifie les types, les champs obligatoires, les contraintes de format et les valeurs par défaut :

```json
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "$id": "https://example.com/server-config.schema.json",
    "title": "Server Configuration",
    "type": "object",
    "required": ["server"],
    "properties": {
        "server": {
            "type": "object",
            "required": ["host", "port"],
            "properties": {
                "host": {
                    "type": "string",
                    "minLength": 1
                },
                "port": {
                    "type": "integer",
                    "minimum": 1,
                    "maximum": 65535
                },
                "workers": {
                    "type": "integer",
                    "minimum": 1,
                    "default": 4
                },
                "tls": {
                    "type": "object",
                    "required": ["cert_path", "key_path"],
                    "properties": {
                        "cert_path": { "type": "string" },
                        "key_path": { "type": "string" },
                        "verify_client": {
                            "type": "boolean",
                            "default": false
                        }
                    },
                    "additionalProperties": false
                }
            },
            "additionalProperties": false
        }
    }
}
```

#### Validation en C++ avec json-schema-validator

La librairie **json-schema-validator** (par Patrick Boettcher) est construite au-dessus de nlohmann/json et implémente les drafts JSON Schema 4, 6 et 7 :

```cpp
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <fstream>
#include <print>

using json = nlohmann::json;  
using nlohmann::json_schema::json_validator;  

int main() {
    // Charger le schéma
    std::ifstream schema_file("server-config.schema.json");
    json schema = json::parse(schema_file);

    // Créer le validateur
    json_validator validator;
    try {
        validator.set_root_schema(schema);
    } catch (const std::exception& e) {
        std::print(stderr, "Schéma invalide : {}\n", e.what());
        return 1;
    }

    // Charger et valider le document
    std::ifstream data_file("config.json");
    json config = json::parse(data_file);

    try {
        validator.validate(config);
        std::print("Configuration valide.\n");
    } catch (const std::exception& e) {
        std::print(stderr, "Validation échouée : {}\n", e.what());
        return 1;
    }
}
```

Installation via Conan :

```python
def requirements(self):
    self.requires("nlohmann_json/3.12.0")
    self.requires("json-schema-validator/2.3.0")
```

#### Collecte de toutes les erreurs

Par défaut, le validateur s'arrête à la première erreur. Pour collecter toutes les violations en une passe, on implémente un handler d'erreurs personnalisé :

```cpp
class CollectingHandler : public nlohmann::json_schema::basic_error_handler {
    std::vector<std::string> errors_;

public:
    void error(const nlohmann::json_pointer<nlohmann::basic_json<>>& pointer,
               const json& instance,
               const std::string& message) override {
        nlohmann::json_schema::basic_error_handler::error(pointer, instance, message);
        errors_.push_back(
            std::format("{} : {}", pointer.to_string(), message));
    }

    const std::vector<std::string>& errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }
};

// Utilisation
CollectingHandler handler;  
validator.validate(config, handler);  

if (handler.has_errors()) {
    std::print(stderr, "Validation échouée ({} erreur(s)) :\n",
        handler.errors().size());
    for (const auto& err : handler.errors()) {
        std::print(stderr, "  - {}\n", err);
    }
}
```

### XML Schema (XSD)

Pour XML, la validation par schéma formel utilise XSD (XML Schema Definition). pugixml ne supporte pas la validation XSD — c'est un choix de design assumé (parser non-validant). Si la validation XSD est nécessaire, **libxml2** ou **Xerces-C++** sont les options :

```cpp
// Exemple conceptuel avec libxml2
// (nécessite l'installation de libxml2-dev)
#include <libxml/xmlschemastypes.h>

bool validate_xml_with_xsd(const char* xml_path, const char* xsd_path) {
    xmlSchemaParserCtxtPtr parser_ctx = xmlSchemaNewParserCtxt(xsd_path);
    xmlSchemaPtr schema = xmlSchemaParse(parser_ctx);
    xmlSchemaValidCtxtPtr valid_ctx = xmlSchemaNewValidCtxt(schema);

    int result = xmlSchemaValidateFile(valid_ctx, xml_path, 0);

    xmlSchemaFreeValidCtxt(valid_ctx);
    xmlSchemaFree(schema);
    xmlSchemaFreeParserCtxt(parser_ctx);

    return result == 0;
}
```

En pratique, pour les projets utilisant pugixml, la validation structurelle applicative (niveau 2 en code C++) est plus courante que la validation XSD formelle. La validation XSD se justifie principalement quand le schéma est imposé par un standard externe (ISO 20022, FIXML, UBL).

### YAML et TOML : pas de schéma standardisé

YAML et TOML ne disposent pas de standard de validation de schéma aussi mature que JSON Schema ou XSD. Des initiatives existent — JSON Schema peut être appliqué à YAML (les structures sont isomorphes), et quelques outils de validation TOML émergent — mais aucune n'a atteint la maturité ni l'adoption de JSON Schema.

Pour YAML et TOML, la validation structurelle se fait donc quasi systématiquement en code applicatif, avec les patterns décrits ci-dessous.

---

## Validation structurelle en code C++

Pour les cas où un schéma formel n'est pas disponible ou disproportionné, la validation structurelle s'implémente directement en C++. Les sections 24.1.4, 24.2.1 et 24.3 ont introduit des exemples de validation ponctuelle. Cette section les généralise en un framework réutilisable.

### Un validateur générique

Le cœur du pattern est une classe qui accumule les erreurs et fournit une API fluide pour exprimer les contraintes :

```cpp
#include <string>
#include <vector>
#include <format>
#include <functional>

class ConfigValidator {  
public:  
    struct Error {
        std::string path;     // "server.port", "database.host"
        std::string message;  // description de l'erreur
    };

    bool valid() const { return errors_.empty(); }
    const std::vector<Error>& errors() const { return errors_; }

    // Vérification de présence
    ConfigValidator& require(bool condition,
                             const std::string& path,
                             const std::string& message) {
        if (!condition) {
            errors_.push_back({path, message});
        }
        return *this;
    }

    // Vérification de plage numérique
    template <typename T>
    ConfigValidator& in_range(const std::string& path,
                              T value, T min, T max) {
        if (value < min || value > max) {
            errors_.push_back({path,
                std::format("doit être entre {} et {} (reçu : {})",
                    min, max, value)});
        }
        return *this;
    }

    // Vérification de chaîne non vide
    ConfigValidator& not_empty(const std::string& path,
                               const std::string& value) {
        if (value.empty()) {
            errors_.push_back({path, "ne peut pas être vide"});
        }
        return *this;
    }

    // Vérification parmi un ensemble de valeurs
    ConfigValidator& one_of(const std::string& path,
                            const std::string& value,
                            std::initializer_list<std::string> allowed) {
        for (const auto& a : allowed) {
            if (value == a) return *this;
        }

        std::string joined;
        for (const auto& a : allowed) {
            if (!joined.empty()) joined += ", ";
            joined += "'" + a + "'";
        }
        errors_.push_back({path,
            std::format("doit être l'une de [{}] (reçu : '{}')",
                joined, value)});
        return *this;
    }

    // Vérification personnalisée
    ConfigValidator& check(const std::string& path,
                           std::function<bool()> predicate,
                           const std::string& message) {
        if (!predicate()) {
            errors_.push_back({path, message});
        }
        return *this;
    }

    // Affichage des erreurs
    void report(const std::string& context = "") const {
        if (valid()) return;

        std::string header = context.empty()
            ? "Validation échouée"
            : std::format("Validation échouée pour {}", context);

        std::print(stderr, "{} ({} erreur(s)) :\n",
            header, errors_.size());
        for (const auto& err : errors_) {
            std::print(stderr, "  - {} : {}\n", err.path, err.message);
        }
    }

private:
    std::vector<Error> errors_;
};
```

### Application à nlohmann/json

```cpp
ConfigValidator validate_json_config(const nlohmann::json& j) {
    ConfigValidator v;

    v.require(j.is_object(), "/", "le document racine doit être un objet");
    if (!j.is_object()) return v;

    v.require(j.contains("server"), "server", "section obligatoire manquante");
    if (!j.contains("server")) return v;

    const auto& server = j["server"];
    v.require(server.contains("host"), "server.host", "champ obligatoire")
     .require(server.contains("port"), "server.port", "champ obligatoire");

    if (server.contains("host") && server["host"].is_string()) {
        v.not_empty("server.host", server["host"].get<std::string>());
    } else if (server.contains("host")) {
        v.require(false, "server.host", "doit être une chaîne");
    }

    if (server.contains("port") && server["port"].is_number_integer()) {
        v.in_range("server.port", server["port"].get<int>(), 1, 65535);
    } else if (server.contains("port")) {
        v.require(false, "server.port", "doit être un entier");
    }

    if (server.contains("log_level") && server["log_level"].is_string()) {
        v.one_of("server.log_level",
                 server["log_level"].get<std::string>(),
                 {"trace", "debug", "info", "warn", "error", "fatal"});
    }

    return v;
}
```

### Application à yaml-cpp

```cpp
ConfigValidator validate_yaml_config(const YAML::Node& root) {
    ConfigValidator v;

    v.require(root.IsMap(), "/", "le document racine doit être une map");
    if (!root.IsMap()) return v;

    v.require(root["server"].IsDefined(), "server",
              "section obligatoire manquante");
    if (!root["server"]) return v;

    const auto& server = root["server"];

    v.require(server["host"].IsDefined(), "server.host",
              "champ obligatoire");
    v.require(server["port"].IsDefined(), "server.port",
              "champ obligatoire");

    if (server["host"] && server["host"].IsScalar()) {
        v.not_empty("server.host", server["host"].as<std::string>());
    }

    if (server["port"] && server["port"].IsScalar()) {
        try {
            int port = server["port"].as<int>();
            v.in_range("server.port", port, 1, 65535);
        } catch (const YAML::BadConversion&) {
            v.require(false, "server.port", "doit être un entier");
        }
    }

    return v;
}
```

### Application à toml++

```cpp
ConfigValidator validate_toml_config(const toml::table& config) {
    ConfigValidator v;

    v.require(config.contains("server"), "server",
              "section obligatoire manquante");
    if (!config.contains("server")) return v;

    auto* server = config["server"].as_table();
    v.require(server != nullptr, "server", "doit être une table");
    if (!server) return v;

    auto host = (*server)["host"].value<std::string>();
    auto port = (*server)["port"].value<int64_t>();

    v.require(host.has_value(), "server.host", "champ obligatoire");
    v.require(port.has_value(), "server.port", "champ obligatoire");

    if (host) {
        v.not_empty("server.host", *host);
    }
    if (port) {
        v.in_range<int64_t>("server.port", *port, 1, 65535);
    }

    return v;
}
```

Le `ConfigValidator` est identique dans les trois cas. Seule la couche d'extraction diffère, car chaque librairie a sa propre API d'accès aux nœuds. Ce pattern isole proprement la logique de validation (quelles contraintes ?) de la mécanique de parsing (comment accéder aux données ?).

---

## Validation sémantique

La validation sémantique porte sur la cohérence des valeurs entre elles et avec l'environnement d'exécution. Elle ne peut pas être exprimée par un schéma formel — c'est de la logique métier.

### Cohérence entre champs

```cpp
void validate_tls_consistency(ConfigValidator& v,
                              const ServerConfig& config) {
    if (config.port == 443 && !config.tls.has_value()) {
        v.require(false, "server",
            "le port 443 nécessite une configuration TLS");
    }

    if (config.tls.has_value()) {
        v.require(config.port == 443 || config.port == 8443,
            "server.port",
            "un port TLS standard (443 ou 8443) est recommandé "
            "quand TLS est activé");
    }

    if (config.tls.has_value() && config.tls->verify_client) {
        v.not_empty("server.tls.cert_path", config.tls->cert_path);
        v.not_empty("server.tls.key_path", config.tls->key_path);
    }
}
```

### Vérification de l'environnement

```cpp
namespace fs = std::filesystem;

void validate_paths(ConfigValidator& v, const ServerConfig& config) {
    if (config.tls.has_value()) {
        v.check("server.tls.cert_path",
            [&]{ return fs::exists(config.tls->cert_path); },
            std::format("fichier introuvable : {}",
                config.tls->cert_path));

        v.check("server.tls.key_path",
            [&]{ return fs::exists(config.tls->key_path); },
            std::format("fichier introuvable : {}",
                config.tls->key_path));
    }
}
```

### Validation de format

```cpp
#include <regex>

void validate_formats(ConfigValidator& v, const ServerConfig& config) {
    // Validation basique d'adresse IP ou hostname
    v.check("server.host",
        [&]{
            // Accepter "0.0.0.0", "localhost", ou un hostname valide
            static const std::regex host_re(
                R"(^(\d{1,3}\.){3}\d{1,3}$|^[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?)*$)");
            return std::regex_match(config.host, host_re);
        },
        std::format("'{}' n'est pas un hostname ou une IP valide",
            config.host));
}
```

> ⚠️ *`std::regex` a un coût de construction élevé. Pour les validations appelées fréquemment (parsing de messages en boucle), mieux vaut utiliser un regex statique (`static const`) ou une validation manuelle par parcours de caractères.*

---

## Pattern complet : pipeline de validation

En combinant les trois niveaux, le chargement complet d'une configuration forme un pipeline où chaque étape valide un aspect différent :

```cpp
std::optional<ServerConfig> load_and_validate_config(
        const std::string& path) {

    // === Niveau 1 : Syntaxe ===
    json j;
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::print(stderr, "Fichier introuvable : {}\n", path);
            return std::nullopt;
        }
        j = json::parse(file);
    } catch (const json::parse_error& e) {
        std::print(stderr, "Syntaxe JSON invalide dans {} :\n  {}\n",
            path, e.what());
        return std::nullopt;
    }

    // === Niveau 2 : Structure ===
    auto structural = validate_json_config(j);
    if (!structural.valid()) {
        structural.report(path);
        return std::nullopt;
    }

    // === Désérialisation ===
    ServerConfig config;
    try {
        config = j.get<ServerConfig>();
    } catch (const json::exception& e) {
        std::print(stderr, "Erreur de conversion : {}\n", e.what());
        return std::nullopt;
    }

    // === Niveau 3 : Sémantique ===
    ConfigValidator semantic;
    validate_tls_consistency(semantic, config);
    validate_paths(semantic, config);
    validate_formats(semantic, config);

    if (!semantic.valid()) {
        semantic.report(path);
        return std::nullopt;
    }

    return config;
}
```

Ce pipeline produit des diagnostics adaptés à chaque niveau. Un utilisateur qui a une virgule en trop verra un message de syntaxe avec une position. Un utilisateur qui oublie un champ obligatoire verra la liste complète des champs manquants. Un utilisateur dont le certificat TLS a été déplacé verra un message clair indiquant quel fichier est introuvable.

---

## Validation stricte vs tolérante

Le niveau de rigueur de la validation dépend du contexte :

### Mode strict

En mode strict, toute anomalie est une erreur fatale. Aucune donnée non conforme ne passe. C'est le mode approprié pour les déploiements en production, les fichiers de configuration de services critiques, et les messages entrants dans un système à haute fiabilité.

```cpp
// Rejet des clés inconnues
void reject_unknown_keys(ConfigValidator& v,
                         const json& node,
                         const std::vector<std::string>& known,
                         const std::string& context) {
    if (!node.is_object()) return;

    for (auto& [key, _] : node.items()) {
        bool is_known = false;
        for (const auto& k : known) {
            if (key == k) { is_known = true; break; }
        }
        if (!is_known) {
            v.require(false,
                context + "." + key,
                "clé inconnue (faute de frappe ?)");
        }
    }
}
```

### Mode tolérant

En mode tolérant, les anomalies non critiques produisent des warnings mais ne bloquent pas le chargement. C'est adapté au développement, aux fichiers de configuration amenés à évoluer, et aux systèmes qui doivent rester opérationnels malgré une configuration imparfaite.

```cpp
class ConfigValidator {
    // ... membres existants ...

    std::vector<Error> warnings_;

public:
    ConfigValidator& warn(bool condition,
                          const std::string& path,
                          const std::string& message) {
        if (!condition) {
            warnings_.push_back({path, message});
        }
        return *this;
    }

    const std::vector<Error>& warnings() const { return warnings_; }

    void report(const std::string& context = "") const {
        // Warnings d'abord
        for (const auto& w : warnings_) {
            std::print(stderr, "  ⚠ {} : {}\n", w.path, w.message);
        }
        // Erreurs ensuite
        if (!valid()) {
            std::print(stderr, "{} ({} erreur(s)) :\n",
                context, errors_.size());
            for (const auto& err : errors_) {
                std::print(stderr, "  ✗ {} : {}\n", err.path, err.message);
            }
        }
    }
};
```

```cpp
// Le mode est choisi par l'appelant
void validate_unknown_keys(ConfigValidator& v,
                           const json& node,
                           const std::vector<std::string>& known,
                           const std::string& context,
                           bool strict) {
    for (auto& [key, _] : node.items()) {
        bool is_known = false;
        for (const auto& k : known) {
            if (key == k) { is_known = true; break; }
        }
        if (!is_known) {
            if (strict) {
                v.require(false, context + "." + key,
                    "clé inconnue");
            } else {
                v.warn(false, context + "." + key,
                    "clé inconnue (ignorée)");
            }
        }
    }
}
```

---

## Génération de documentation depuis le schéma

Un schéma de validation, qu'il soit formel (JSON Schema) ou implémenté en code, constitue une source de vérité sur le format de configuration. Dans un projet professionnel, cette source de vérité devrait alimenter la documentation utilisateur.

### Depuis JSON Schema

Un fichier JSON Schema peut être transformé automatiquement en documentation Markdown ou HTML. Des outils comme **json-schema-for-humans** (Python) produisent une documentation lisible à partir du schéma :

```bash
pip install json-schema-for-humans  
generate-schema-doc server-config.schema.json docs/config-reference.md  
```

### Depuis le code de validation

Si la validation est implémentée en code C++, la documentation doit être maintenue manuellement — mais le pattern `ConfigValidator` facilite cette tâche. Chaque appel à `require`, `in_range`, `one_of`, `not_empty` décrit une contrainte documentable. Un effort de nommage cohérent dans les messages d'erreur produit naturellement une documentation lisible.

La règle pratique : **le message d'erreur de validation est la documentation de la contrainte**. Si le message n'est pas clair pour un utilisateur final, la documentation ne le sera pas non plus.

---

## Récapitulatif des stratégies par format

| Format | Schéma formel disponible | Librairie C++ de validation | Alternative |
|--------|--------------------------|---------------------------|-------------|
| **JSON** | JSON Schema (mature) | json-schema-validator | Validation applicative |
| **XML** | XSD (mature) | Xerces-C++, libxml2 | Validation applicative + pugixml |
| **YAML** | JSON Schema (applicable) | json-schema-validator (via conversion) | Validation applicative |
| **TOML** | Pas de standard établi | — | Validation applicative |

Pour les nouveaux projets, deux approches se distinguent :

**Projets avec schéma partagé entre équipes ou systèmes** — JSON Schema est le choix le plus portable. Un seul schéma valide les données en C++, Python, Go, JavaScript et dans les outils CI. Il fonctionne nativement avec JSON et s'applique à YAML (les deux formats partagent le même modèle de données).

**Projets avec validation interne** — Le `ConfigValidator` en C++ offre plus de flexibilité (validation sémantique, vérification de l'environnement) et élimine une dépendance externe. C'est l'approche recommandée quand le fichier de configuration n'est consommé que par le programme C++ lui-même.

Les deux approches ne sont pas mutuellement exclusives. Un projet peut utiliser JSON Schema pour la validation structurelle (niveaux 1-2) et le `ConfigValidator` pour la validation sémantique (niveau 3).

---

## Principes à retenir

**Valider tôt, échouer clairement.** La validation doit intervenir au plus tôt dans le cycle de vie des données — idéalement au chargement. Un message d'erreur à ce stade évite des heures de débogage plus tard quand une valeur invalide produit un comportement inattendu dans une partie éloignée du code.

**Collecter toutes les erreurs en une passe.** Un utilisateur qui corrige une erreur pour en découvrir une nouvelle à chaque tentative perdra patience. Le pattern d'accumulation (collecter toutes les erreurs avant de les rapporter) est toujours préférable à l'abandon à la première erreur.

**Séparer la validation du parsing.** Le code de validation est une couche distincte du code de désérialisation. Il peut être testé indépendamment, réutilisé pour différents formats, et évoluer sans modifier les structures de données.

**Documenter les contraintes par les messages d'erreur.** Chaque message de validation devrait être suffisamment explicite pour qu'un utilisateur puisse corriger le problème sans lire le code source. Inclure la valeur reçue, la contrainte attendue, et si possible un exemple de valeur valide.

**Tester la validation autant que le code métier.** Les fonctions de validation méritent leurs propres tests unitaires : cas nominaux, champs manquants, types incorrects, valeurs limites, combinaisons invalides de champs. Un bug dans la validation est doublement dangereux — il laisse passer des données invalides qui causeront des erreurs silencieuses en aval.

⏭️ [Formats Binaires et Sérialisation Performante](/25-formats-binaires/README.md)
