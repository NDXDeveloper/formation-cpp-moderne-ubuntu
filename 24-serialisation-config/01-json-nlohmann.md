🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 24.1 — JSON : Lecture/Écriture avec nlohmann/json ⭐

## Section du Module 8 — Parsing et Formats de Données

---

## Le format incontournable

JSON (JavaScript Object Notation) est devenu le format d'échange de données le plus répandu au monde. Né en 2001 comme un sous-ensemble de la syntaxe JavaScript, il a transcendé son langage d'origine pour devenir un standard universel. Sa spécification tient en une seule page (RFC 8259), sa grammaire ne comporte que six types de valeurs, et pratiquement tout langage de programmation dispose d'un parser natif ou quasi-natif.

En C++, la situation est historiquement moins confortable. Le langage ne propose aucun support JSON dans sa bibliothèque standard, et les premières librairies disponibles (RapidJSON, JsonCpp) imposaient une API verbeuse, typique du C++ pré-moderne. C'est dans ce contexte que la librairie **nlohmann/json** a changé la donne.

---

## nlohmann/json : la référence de l'écosystème C++

Créée par Niels Lohmann en 2013, la librairie `nlohmann/json` — souvent appelée simplement *JSON for Modern C++* — est devenue la librairie JSON la plus utilisée en C++. Son dépôt GitHub dépasse les 45 000 étoiles, et elle est intégrée comme dépendance dans des milliers de projets open-source et industriels.

Sa philosophie de conception repose sur un principe directeur : **le JSON en C++ doit se manipuler aussi naturellement qu'en Python ou en JavaScript**. Concrètement, cela signifie qu'on peut écrire :

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Création directe, syntaxe intuitive
json config = {
    {"server", {
        {"host", "0.0.0.0"},
        {"port", 8080},
        {"tls", true}
    }},
    {"workers", 4},
    {"tags", {"production", "eu-west"}}
};

// Accès naturel
std::string host = config["server"]["host"];  
int port = config["server"]["port"];  

// Itération comme sur un conteneur STL
for (auto& [key, value] : config["server"].items()) {
    std::print("{} = {}\n", key, value.dump());
}
```

Ce code est valide, type-safe à l'exécution, et ne nécessite aucune étape de génération de code ni de définition de schéma préalable. C'est cette ergonomie qui a fait le succès de la librairie.

---

## Caractéristiques principales

### Header-only

La librairie se compose d'un unique fichier header (`json.hpp`). Aucune étape de compilation séparée, aucune librairie à linker. On inclut le header, et tout fonctionne. Cette propriété simplifie considérablement l'intégration dans un projet existant et le déploiement en CI/CD.

Le revers de cette approche est un temps de compilation plus long dans les unités de traduction qui incluent le header. La section 24.1.1 détaillera les stratégies pour atténuer ce coût, notamment l'utilisation de la version split (headers multiples) et des forward declarations.

### API intuitive basée sur le type `json`

Le cœur de la librairie est le type `nlohmann::json`, un conteneur polymorphe capable de représenter n'importe quelle valeur JSON. Sous le capot, il s'agit d'un variant qui encapsule les six types JSON :

| Type JSON | Représentation C++ interne | Exemple |
|-----------|---------------------------|---------|
| **object** | `std::map<std::string, json>` | `{"key": "value"}` |
| **array** | `std::vector<json>` | `[1, 2, 3]` |
| **string** | `std::string` | `"hello"` |
| **number (int)** | `int64_t` / `uint64_t` | `42` |
| **number (float)** | `double` | `3.14` |
| **boolean** | `bool` | `true` |
| **null** | `std::nullptr_t` | `null` |

L'accès aux valeurs utilise l'opérateur `[]` (comme `std::map`) ou la méthode `.value()` qui accepte une valeur par défaut :

```cpp
// operator[] — lève une exception si le type ne correspond pas
int port = config["server"]["port"];

// .value() — retourne la valeur par défaut si la clé est absente
int timeout = config.value("timeout", 30);

// .contains() — test d'existence sans exception
if (config.contains("debug")) {
    bool debug = config["debug"];
}

// .at() — accès avec vérification, lève json::out_of_range si absent
try {
    auto val = config.at("nonexistent");
} catch (const json::out_of_range& e) {
    std::print(stderr, "Clé manquante : {}\n", e.what());
}
```

### Conversion automatique depuis/vers les types C++

L'un des points forts majeurs de nlohmann/json est son système de conversion bidirectionnelle entre JSON et les types C++. Les types standard sont gérés nativement :

```cpp
// Sérialisation implicite : C++ → JSON
std::vector<int> scores = {95, 87, 72, 100};  
json j_scores = scores;  // → [95, 87, 72, 100]  

std::map<std::string, double> prices = {{"BTC", 68500.0}, {"ETH", 3800.0}};  
json j_prices = prices;  // → {"BTC": 68500.0, "ETH": 3800.0}  

// Désérialisation implicite : JSON → C++
auto restored_scores = j_scores.get<std::vector<int>>();  
auto restored_prices = j_prices.get<std::map<std::string, double>>();  
```

Pour les types utilisateur (structs, classes), la librairie fournit un mécanisme non intrusif basé sur deux fonctions libres, `to_json` et `from_json`, ou la macro `NLOHMANN_DEFINE_TYPE_INTRUSIVE` / `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE` pour les cas simples :

```cpp
struct ServerConfig {
    std::string host;
    int port;
    bool tls;
};

// Macro non intrusive — génère automatiquement to_json/from_json
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServerConfig, host, port, tls)

// Utilisation transparente
ServerConfig cfg{"localhost", 443, true};  
json j = cfg;                              // sérialisation  
auto cfg2 = j.get<ServerConfig>();         // désérialisation  
```

La section 24.1.3 explorera ce mécanisme en profondeur, y compris la gestion des champs optionnels, des valeurs par défaut et de l'héritage.

### Parsing et génération robustes

La librairie supporte le parsing depuis plusieurs sources — chaînes de caractères, flux d'entrée (`std::istream`), itérateurs, et fichiers — avec une gestion d'erreurs détaillée :

```cpp
// Depuis une chaîne
json j1 = json::parse(R"({"status": "ok", "code": 200})");

// Depuis un fichier via std::ifstream
std::ifstream file("config.json");  
json j2 = json::parse(file);  

// Parsing avec callback de gestion d'erreurs (pas d'exception)
json j3 = json::parse(input, nullptr, false);  
if (j3.is_discarded()) {  
    std::print(stderr, "JSON invalide\n");
}
```

Pour la génération, la méthode `.dump()` produit une chaîne JSON avec un contrôle sur l'indentation :

```cpp
std::string compact = j.dump();       // minifié, une seule ligne  
std::string pretty = j.dump(4);       // indenté avec 4 espaces  
std::string ascii = j.dump(-1, ' ', true);  // échappement des caractères non-ASCII  
```

---

## Quand choisir nlohmann/json

nlohmann/json est le bon choix dans la grande majorité des cas d'usage JSON en C++. Sa couverture fonctionnelle est exhaustive, son API est bien documentée, et sa communauté est très active.

**Cas d'usage idéaux :**

- Lecture et écriture de fichiers de configuration JSON.  
- Communication avec des API REST (parsing de réponses HTTP, construction de requêtes).  
- Sérialisation d'état applicatif pour la persistance ou le débogage.  
- Prototypage rapide nécessitant une manipulation JSON flexible.  
- Projets où la clarté du code prime sur la performance brute du parsing.

**Cas où une alternative peut être préférable :**

- **Parsing de très gros volumes JSON (centaines de Mo)** — simdjson exploite les instructions SIMD du processeur pour parser du JSON à plusieurs Go/s. Si le parsing JSON est un goulot d'étranglement mesuré par profiling, simdjson est l'alternative à évaluer.  
- **Contraintes mémoire très strictes** — RapidJSON utilise un allocateur en arène et consomme significativement moins de mémoire que nlohmann/json pour les documents volumineux, au prix d'une API moins ergonomique.  
- **Sérialisation haute fréquence entre services** — si la performance est critique et que les deux extrémités du canal sont sous votre contrôle, un format binaire (Protobuf, FlatBuffers) sera plus approprié que JSON quel que soit le parser.

En pratique, la grande majorité des projets C++ qui manipulent du JSON n'ont pas ces contraintes extrêmes, et nlohmann/json offre le meilleur rapport productivité/fiabilité.

---

## Aperçu du flux de travail typique

Le diagramme suivant résume le cycle de vie des données JSON dans une application C++ utilisant nlohmann/json :

```
                    ┌─────────────────────┐
                    │   Source de données │
                    │  (fichier, réseau,  │
                    │   chaîne, stdin)    │
                    └─────────┬───────────┘
                              │
                      json::parse()
                              │
                              ▼
                    ┌─────────────────────┐
                    │    nlohmann::json   │
                    │  (DOM en mémoire)   │
                    │                     │
                    │  ┌───────────────┐  │
                    │  │ Accès []      │  │
                    │  │ Navigation    │  │
                    │  │ Modification  │  │
                    │  │ Itération     │  │
                    │  └───────────────┘  │
                    └────┬───────────┬────┘
                         │           │
                   .get<T>()     .dump()
                         │           │
                         ▼           ▼
              ┌──────────────┐  ┌──────────────────┐
              │ Structs C++  │  │  Sortie JSON     │
              │ (domaine     │  │  (fichier, HTTP, │
              │  métier)     │  │   logs, stdout)  │
              └──────────────┘  └──────────────────┘
```

La librairie opère selon un modèle **DOM (Document Object Model)** : le JSON est d'abord parsé intégralement en mémoire sous forme d'arbre d'objets `json`, puis navigué et manipulé. Ce modèle est simple à comprendre et à utiliser, mais implique que l'intégralité du document réside en mémoire. Pour les documents de taille modérée (jusqu'à quelques dizaines de Mo), c'est rarement un problème. Pour les flux JSON massifs, nlohmann/json supporte également un mode SAX (event-driven) qui parse le document en streaming sans le charger entièrement.

---

## Ce que couvrent les sous-sections

Les sous-sections suivantes détaillent chaque aspect de l'utilisation de nlohmann/json dans un projet C++ professionnel :

- **24.1.1 — Installation et intégration** : les différentes méthodes d'installation (Conan, vcpkg, FetchContent, header-only direct) et la configuration CMake recommandée.  
- **24.1.2 — Parsing de fichiers JSON** : lecture depuis fichier, chaîne et flux, parsing strict vs tolérant, mode SAX pour les gros documents.  
- **24.1.3 — Sérialisation d'objets C++** : conversion bidirectionnelle avec les types utilisateur, macros vs fonctions manuelles, gestion des champs optionnels et de l'héritage.  
- **24.1.4 — Gestion des erreurs de parsing** : hiérarchie des exceptions, parsing sans exception, messages d'erreur exploitables, et patterns de validation.

---

## Prérequis pour cette section

- Conteneurs STL : `std::vector`, `std::map`, `std::string` *(chapitres 13-14)*  
- `std::optional` *(section 12.2)*  
- Gestion des exceptions en C++ *(chapitre 17)*  
- CMake et gestion des dépendances *(chapitres 26-27)*  
- Flux d'entrée/sortie (`std::ifstream`, `std::ofstream`) ou `std::filesystem` *(section 19.1)*

⏭️ [Installation et intégration](/24-serialisation-config/01.1-installation.md)
