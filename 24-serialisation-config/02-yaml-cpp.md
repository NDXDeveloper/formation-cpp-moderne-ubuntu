🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 24.2 — YAML : Parsing avec yaml-cpp ⭐

## Section du Module 8 — Parsing et Formats de Données

---

## Le format de l'écosystème DevOps

YAML (YAML Ain't Markup Language) est omniprésent dans le monde de l'infrastructure et du déploiement. Kubernetes, Docker Compose, Ansible, GitHub Actions, GitLab CI, Prometheus, Helm — la liste des outils qui utilisent YAML comme format de configuration est considérable. Tout développeur C++ travaillant dans un contexte DevOps ou Cloud Native sera amené à lire ou générer du YAML, que ce soit pour parser les configurations de son propre outil, interagir avec des fichiers de déploiement, ou produire des manifestes à destination d'un orchestrateur.

YAML a été conçu dès l'origine pour la lisibilité humaine. Là où JSON utilise des accolades, des crochets et des guillemets obligatoires, YAML s'appuie sur l'indentation et une syntaxe minimaliste qui rend les fichiers de configuration visuellement clairs :

```yaml
# Configuration du serveur d'API
server:
  host: 0.0.0.0
  port: 8080
  workers: 4

database:
  host: db.internal
  port: 5432
  name: production
  credentials:
    username: api_user
    password: ${DB_PASSWORD}  # résolu par l'application

logging:
  level: info
  outputs:
    - stdout
    - file: /var/log/api/server.log
```

L'équivalent JSON de ce fichier nécessiterait des guillemets autour de chaque clé et chaque valeur chaîne, des accolades pour chaque objet, des crochets pour chaque tableau, et ne supporterait pas les commentaires. En termes de lisibilité pour un opérateur humain, YAML est difficile à battre.

---

## yaml-cpp : la librairie de référence en C++

**yaml-cpp** est la librairie YAML la plus utilisée en C++. Développée par Jesse Beder, elle implémente la spécification YAML 1.2 et fournit une API C++ moderne basée sur un type central `YAML::Node`, analogue au `nlohmann::json` de la section précédente.

Contrairement à nlohmann/json, yaml-cpp **n'est pas header-only** : elle se compile en une librairie statique ou dynamique qu'il faut linker au projet. Ce choix de conception reflète la complexité intrinsèque du parsing YAML (la spécification est nettement plus riche que celle de JSON) et évite de pénaliser le temps de compilation de chaque unité de traduction.

### Modèle de fonctionnement

yaml-cpp opère selon un modèle DOM similaire à nlohmann/json : le fichier YAML est parsé intégralement en mémoire sous forme d'un arbre de `YAML::Node`, puis navigué et converti en types C++.

```
┌──────────────────┐      YAML::LoadFile()      ┌────────────────┐
│  Fichier YAML    │  ───────────────────────►  │  YAML::Node    │
│  (texte UTF-8)   │                            │  (arbre DOM)   │
└──────────────────┘  ◄───────────────────────  └────────────────┘
                         YAML::Emitter            Navigation,
                                                  .as<T>(),
                                                  conversion
```

Le type `YAML::Node` est un nœud polymorphe qui peut représenter les quatre types fondamentaux YAML :

| Type YAML | Description | Exemple YAML | Équivalent C++ typique |
|-----------|-------------|-------------|----------------------|
| **Scalar** | Valeur simple | `port: 8080` | `std::string`, `int`, `double`, `bool` |
| **Sequence** | Liste ordonnée | `- item1`<br>`- item2` | `std::vector<T>` |
| **Map** | Paires clé-valeur | `host: localhost` | `std::map<std::string, T>` |
| **Null** | Absence de valeur | `value: ~` ou `value:` | `nullptr`, absence |

### Aperçu de l'API

Un premier exemple illustre les opérations fondamentales :

```cpp
#include <yaml-cpp/yaml.h>
#include <print>
#include <string>

int main() {
    // Chargement depuis un fichier
    YAML::Node config = YAML::LoadFile("config.yaml");

    // Accès aux scalaires
    std::string host = config["server"]["host"].as<std::string>();
    int port = config["server"]["port"].as<int>();

    std::print("Serveur : {}:{}\n", host, port);

    // Accès avec valeur par défaut (pas d'exception si absent)
    int workers = config["server"]["workers"].as<int>(4);

    // Test d'existence
    if (config["database"]) {
        std::string db = config["database"]["name"].as<std::string>();
        std::print("Base de données : {}\n", db);
    }

    // Itération sur une séquence
    if (config["logging"]["outputs"]) {
        for (const auto& output : config["logging"]["outputs"]) {
            if (output.IsScalar()) {
                std::print("Log output : {}\n", output.as<std::string>());
            } else if (output.IsMap()) {
                for (const auto& kv : output) {
                    std::print("Log output : {} → {}\n",
                        kv.first.as<std::string>(),
                        kv.second.as<std::string>());
                }
            }
        }
    }
}
```

Quelques différences notables avec nlohmann/json apparaissent immédiatement :

- L'extraction typée se fait via `.as<T>()` (au lieu de `.get<T>()`).  
- `.as<T>(default_value)` accepte une valeur par défaut en paramètre, combinant l'accès et le fallback en un seul appel.  
- Le test d'existence d'un nœud utilise la conversion implicite en `bool` (`if (node["key"])`), ce qui est concis mais nécessite une attention particulière (voir les pièges ci-dessous).  
- L'itération sur les maps produit des paires de `YAML::Node` — la clé est elle-même un nœud, pas une `std::string`.

---

## YAML vs JSON : différences structurelles pour le développeur C++

YAML est un surensemble de JSON : tout document JSON valide est aussi du YAML valide. Mais YAML introduit des fonctionnalités supplémentaires dont certaines sont précieuses et d'autres sont source de bugs subtils.

### Fonctionnalités avantageuses

**Commentaires.** YAML supporte les commentaires avec `#`, ce qui est essentiel pour les fichiers de configuration destinés à être maintenus par des humains. C'est souvent la raison principale pour choisir YAML plutôt que JSON pour la configuration.

**Multi-documents.** Un seul fichier YAML peut contenir plusieurs documents, séparés par `---`. Cette fonctionnalité est exploitée par Kubernetes (plusieurs manifestes dans un seul fichier) et par certains formats de documentation (front-matter en YAML suivi du contenu).

```yaml
---
# Premier document : configuration du serveur
server:
  port: 8080
---
# Deuxième document : configuration du monitoring
monitoring:
  enabled: true
  interval: 30s
```

**Ancres et références.** YAML permet de définir une valeur une fois (`&anchor`) et de la réutiliser ailleurs (`*anchor`), évitant la duplication :

```yaml
defaults: &default_settings
  timeout: 30
  retries: 3
  ssl: true

production:
  <<: *default_settings
  host: api.prod.example.com

staging:
  <<: *default_settings
  host: api.staging.example.com
  ssl: false  # override du défaut
```

> ⚠️ *Attention : le merge key `<<` est une extension YAML 1.1 dont le support varie selon les parsers. yaml-cpp 0.8.0 ne résout **pas** le merge key — la clé `<<` est traitée comme une clé littérale ordinaire. Les ancres simples (`*anchor`) fonctionnent en revanche parfaitement. Voir la section 24.2.1 pour les détails et une solution de contournement.*

### Pièges et comportements surprenants

YAML comporte plusieurs comportements contre-intuitifs qui ont causé des bugs dans d'innombrables projets. Un développeur C++ utilisant yaml-cpp doit les connaître.

**Le piège du typage implicite des scalaires.** YAML interprète automatiquement les scalaires non quotés selon des règles de typage implicite. Cela produit des surprises célèbres :

```yaml
# Ces valeurs ne sont PAS des chaînes de caractères !
country_code: NO      # → boolean false (Norway bug)  
version: 1.0          # → float 1.0, pas string "1.0"  
octal_perm: 0755      # → entier 493 (octal)  
timestamp: 2026-03-14 # → date, pas string  

# Pour forcer le type chaîne, il faut quoter
country_code: "NO"  
version: "1.0"  
octal_perm: "0755"  
```

Le cas `NO` interprété comme `false` est particulièrement notoire — il a causé des bugs dans des projets de toutes tailles. En YAML 1.1 (utilisé par défaut dans de nombreux outils), les valeurs `yes`, `no`, `on`, `off`, `y`, `n` et leurs variantes de casse sont interprétées comme des booléens. YAML 1.2 a restreint cette liste à `true` et `false`, mais yaml-cpp supporte les deux versions et le comportement dépend de la configuration.

**Le piège de l'indentation.** YAML utilise l'indentation (espaces uniquement, jamais de tabulations) pour structurer les données. Une erreur d'un seul espace peut changer complètement la sémantique du document :

```yaml
# "port" est un enfant de "server"
server:
  host: localhost
  port: 8080

# Indentation incorrecte : "port" est au niveau racine, pas sous "server"
server:
  host: localhost
port: 8080
```

Ce type d'erreur est silencieux : le document est syntaxiquement valide dans les deux cas. C'est le programme qui doit détecter que `port` n'est pas là où il est attendu.

**Le piège des chaînes multi-lignes.** YAML propose plusieurs syntaxes pour les chaînes multi-lignes (`|`, `>`, `|+`, `|-`, `>+`, `>-`), chacune avec des règles subtiles sur les retours à la ligne et les espaces de fin. C'est puissant mais facile à utiliser incorrectement :

```yaml
# Bloc littéral : préserve les retours à la ligne
description: |
  Première ligne
  Deuxième ligne

# Bloc plié : fusionne les lignes en une seule
summary: >
  Cette phrase est sur
  plusieurs lignes mais
  sera fusionnée.
```

---

## Quand choisir YAML

**YAML est le bon choix quand :**

- Le fichier de configuration est destiné à être écrit et maintenu par des humains, et la lisibilité est prioritaire.  
- Le projet s'insère dans un écosystème qui utilise déjà YAML (Kubernetes, CI/CD, Ansible).  
- Les commentaires dans les fichiers de configuration sont nécessaires.  
- Le format multi-documents est utile.

**YAML n'est pas le bon choix quand :**

- Les données sont échangées entre programmes sans intervention humaine — JSON ou un format binaire est plus simple et sans ambiguïté.  
- La structure du fichier doit être validée strictement — les pièges de typage implicite rendent YAML fragile pour les données critiques.  
- Le projet n'a pas de dépendance existante à YAML — TOML (section 24.3) offre une syntaxe plus simple, sans les pièges de YAML, pour les fichiers de configuration.

---

## Installation

yaml-cpp est disponible via les mêmes canaux que nlohmann/json, avec la différence qu'il nécessite une étape de compilation.

### Conan 2

```python
# conanfile.py
def requirements(self):
    self.requires("yaml-cpp/0.8.0")
```

```cmake
find_package(yaml-cpp REQUIRED)  
target_link_libraries(myapp PRIVATE yaml-cpp::yaml-cpp)  
```

### vcpkg

```bash
vcpkg install yaml-cpp
```

```cmake
find_package(yaml-cpp CONFIG REQUIRED)  
target_link_libraries(myapp PRIVATE yaml-cpp::yaml-cpp)  
```

### CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        0.8.0
    GIT_SHALLOW    TRUE
)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)  
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)  

FetchContent_MakeAvailable(yaml-cpp)

target_link_libraries(myapp PRIVATE yaml-cpp::yaml-cpp)
```

### apt (Ubuntu)

```bash
sudo apt install libyaml-cpp-dev
```

```cmake
find_package(yaml-cpp REQUIRED)  
target_link_libraries(myapp PRIVATE yaml-cpp::yaml-cpp)  
```

> ⚠️ *Comme pour nlohmann/json, la version disponible dans les dépôts Ubuntu peut être en retard. La version 0.7.0 fournie sur Ubuntu 22.04 LTS manque certaines corrections de bugs présentes dans la 0.8.x. Pour les projets professionnels, Conan ou vcpkg garantissent le contrôle de la version.*

---

## Architecture de l'API yaml-cpp

L'API de yaml-cpp s'organise autour de trois axes, correspondant aux trois opérations fondamentales sur du YAML :

### 1. Chargement (parsing)

Les fonctions de chargement transforment une source YAML en arbre de `YAML::Node` :

| Fonction | Source | Usage |
|----------|--------|-------|
| `YAML::LoadFile(path)` | Fichier | Configuration, manifestes |
| `YAML::Load(string)` | Chaîne | Messages, tests, données en mémoire |
| `YAML::LoadAllFromFile(path)` | Fichier multi-doc | Manifestes Kubernetes multiples |
| `YAML::LoadAll(string)` | Chaîne multi-doc | Idem depuis une chaîne |

### 2. Navigation et extraction

Le `YAML::Node` retourné est navigable par clé ou par index, et convertible en types C++ :

| Opération | Méthode | Comportement si absent/invalide |
|-----------|---------|-------------------------------|
| Accès par clé | `node["key"]` | Retourne un nœud `Null` (évalue à `false`) |
| Extraction typée | `node.as<T>()` | Lève `YAML::BadConversion` |
| Extraction avec défaut | `node.as<T>(default)` | Retourne `default` |
| Test d'existence | `if (node["key"])` | Conversion implicite en `bool` |
| Test de type | `node.IsScalar()`, `IsSequence()`, `IsMap()`, `IsNull()` | Retourne `bool` |
| Taille | `node.size()` | Nombre d'enfants (map ou sequence) |

### 3. Émission (génération)

La classe `YAML::Emitter` permet de construire du YAML en sortie, pour la génération de fichiers de configuration ou de manifestes. Cette partie sera couverte dans la section 24.2.2.

---

## Conversion vers les types C++ : le système `convert`

yaml-cpp utilise un mécanisme de spécialisation de template pour la conversion entre `YAML::Node` et les types C++, conceptuellement analogue au système `to_json`/`from_json` de nlohmann/json mais syntaxiquement différent.

La conversion repose sur la spécialisation partielle de la structure `YAML::convert<T>` :

```cpp
#include <yaml-cpp/yaml.h>
#include <string>

struct Endpoint {
    std::string path;
    std::string method;
    int timeout_ms;
};

namespace YAML {  
template <>  
struct convert<Endpoint> {  
    static Node encode(const Endpoint& e) {
        Node node;
        node["path"] = e.path;
        node["method"] = e.method;
        node["timeout_ms"] = e.timeout_ms;
        return node;
    }

    static bool decode(const Node& node, Endpoint& e) {
        if (!node.IsMap()) return false;

        // Champs obligatoires
        if (!node["path"] || !node["method"]) return false;

        e.path = node["path"].as<std::string>();
        e.method = node["method"].as<std::string>();
        e.timeout_ms = node["timeout_ms"].as<int>(5000);  // défaut si absent
        return true;
    }
};
}  // namespace YAML
```

Une fois cette spécialisation définie, la conversion est transparente :

```cpp
YAML::Node config = YAML::LoadFile("endpoints.yaml");

// Désérialisation directe
auto ep = config["api"].as<Endpoint>();

// Désérialisation d'une séquence
auto endpoints = config["endpoints"].as<std::vector<Endpoint>>();

// Sérialisation
Endpoint new_ep{"/health", "GET", 1000};  
YAML::Node node;  
node["endpoint"] = new_ep;  // appelle convert<Endpoint>::encode  
```

### Différences avec nlohmann/json

Le mécanisme `convert` de yaml-cpp diffère du système `to_json`/`from_json` de nlohmann/json sur plusieurs points :

**Retour booléen pour `decode`.** La fonction `decode` retourne `false` pour signaler un échec de conversion, au lieu de lever une exception. C'est un choix de design qui oblige le développeur à gérer les cas d'erreur dans la logique de conversion elle-même. Un `return false` dans `decode` provoque une exception `YAML::BadConversion` au point d'appel `.as<T>()`.

**Spécialisation de template vs ADL.** yaml-cpp utilise la spécialisation explicite dans le namespace `YAML`, tandis que nlohmann/json repose sur des fonctions libres trouvées par ADL. L'approche yaml-cpp est plus verbeuse mais rend le point de personnalisation explicite et visible.

**Pas de macros de génération automatique.** yaml-cpp ne fournit pas d'équivalent aux macros `NLOHMANN_DEFINE_TYPE_*`. Chaque type nécessite une spécialisation manuelle de `convert`, ce qui est plus de code à écrire mais aussi plus de contrôle sur le comportement en cas de champs manquants ou invalides.

---

## Gestion des erreurs

yaml-cpp utilise des exceptions pour signaler les erreurs. Les deux principales sont :

**`YAML::ParserException`** — levée lorsque le document YAML est syntaxiquement invalide. Elle fournit une position (ligne et colonne) dans le document :

```cpp
try {
    YAML::Node config = YAML::LoadFile("broken.yaml");
} catch (const YAML::ParserException& e) {
    std::print(stderr, "Erreur de syntaxe YAML\n");
    std::print(stderr, "  Ligne   : {}\n", e.mark.line + 1);   // 0-indexed
    std::print(stderr, "  Colonne : {}\n", e.mark.column + 1);
    std::print(stderr, "  Message : {}\n", e.what());
}
```

> ⚠️ *Les positions retournées par `e.mark` sont indexées à partir de 0. Pour un affichage cohérent avec les conventions des éditeurs de texte (lignes commençant à 1), il faut ajouter 1.*

**`YAML::BadConversion`** — levée lorsqu'une conversion `.as<T>()` échoue, soit parce que le nœud est absent, soit parce que la valeur n'est pas convertible vers le type demandé :

```cpp
try {
    int port = config["server"]["port"].as<int>();
} catch (const YAML::BadConversion& e) {
    std::print(stderr, "Conversion impossible : {}\n", e.what());
}
```

Les deux héritent de `YAML::Exception`, qui hérite de `std::runtime_error`. Les patterns de gestion (capture granulaire, capture globale, validation préalable) sont les mêmes que ceux détaillés en section 24.1.4 pour nlohmann/json.

---

## Ce que couvrent les sous-sections

Les sous-sections suivantes détaillent les deux opérations principales avec yaml-cpp :

- **24.2.1 — Lecture de fichiers de configuration** : chargement de fichiers, navigation dans l'arbre, gestion des multi-documents, conversion vers les types métier, patterns de validation et traitement des pièges YAML.  
- **24.2.2 — Écriture YAML** : génération de YAML avec `YAML::Emitter`, contrôle du formatage de sortie, écriture vers fichier, génération de manifestes et cas d'usage courants.

---

## Prérequis pour cette section

- Section 24.1 (nlohmann/json) — les concepts de DOM, parsing, sérialisation et gestion d'erreurs sont identiques dans leur principe.  
- Conteneurs STL : `std::vector`, `std::map`, `std::string` *(chapitres 13-14)*  
- Templates et spécialisation *(chapitre 16)* — pour comprendre le mécanisme `YAML::convert<T>`.  
- CMake et gestion des dépendances *(chapitres 26-27)*

⏭️ [Lecture de fichiers de configuration](/24-serialisation-config/02.1-lecture-config.md)
