🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 24.4 — XML : Parsing avec pugixml (legacy systems)

## Section du Module 8 — Parsing et Formats de Données

---

## Un format omniprésent dans les systèmes existants

XML (eXtensible Markup Language) a dominé le monde de l'échange de données et de la configuration pendant plus d'une décennie, des années 2000 au milieu des années 2010. Bien que JSON, YAML et TOML l'aient largement remplacé pour les nouveaux projets, XML reste profondément ancré dans de nombreux domaines :

- **Protocoles d'entreprise** — SOAP, SAML, XMPP, les web services WS-*, les flux bancaires (ISO 20022, FIX/FIXML).
- **Formats documentaires** — XHTML, DocBook, SVG, MathML, les formats Office Open XML (.docx, .xlsx).
- **Configuration d'outils** — fichiers Maven (pom.xml), configurations Spring, descripteurs de déploiement Java EE, manifestes Android.
- **Formats industriels** — COLLADA (3D), GML (géospatial), HL7/CDA (santé), UBL (facturation électronique).
- **Systèmes legacy** — toute application d'entreprise développée entre 2000 et 2015 a de fortes chances d'utiliser XML pour ses échanges de données.

Un développeur C++ travaillant dans un environnement professionnel sera tôt ou tard confronté à du XML, non pas par choix mais par nécessité d'interfaçage. Cette section fournit les outils pour le gérer efficacement avec pugixml, sans prétendre que XML est le bon choix pour un nouveau projet.

---

## Anatomie d'un document XML

Pour les développeurs habitués à JSON ou YAML, la syntaxe XML peut paraître verbeuse. Un rappel rapide de sa structure est utile :

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- Configuration du serveur (legacy) -->
<server name="api-prod" version="3.2">
    <host>0.0.0.0</host>
    <port>8080</port>
    <workers>4</workers>

    <database>
        <host>db.internal</host>
        <port>5432</port>
        <credentials username="api_user" password="secret"/>
    </database>

    <endpoints>
        <endpoint path="/health" method="GET" timeout="1000"/>
        <endpoint path="/api/data" method="POST" timeout="5000"/>
    </endpoints>

    <logging level="info">
        <output type="stdout"/>
        <output type="file" path="/var/log/api/server.log"/>
    </logging>
</server>
```

Les concepts clés de XML qui n'ont pas d'équivalent direct dans les formats couverts précédemment :

**Attributs vs éléments enfants.** XML offre deux façons de porter une donnée : comme attribut d'un élément (`<server name="api">`) ou comme élément enfant (`<host>0.0.0.0</host>`). Cette dualité est source de débats de design sans fin et d'incohérences dans les formats existants — un même concept peut être modélisé comme attribut dans un schéma et comme élément dans un autre.

**Namespaces.** XML supporte les espaces de noms (`xmlns`) pour éviter les collisions de noms entre vocabulaires différents. C'est indispensable dans les formats complexes (SOAP enveloppant du SAML, par exemple) mais ajoute une couche de complexité au parsing.

**Prologue et déclaration.** La première ligne (`<?xml version="1.0" encoding="UTF-8"?>`) est une déclaration optionnelle mais conventionnelle. Elle spécifie la version XML et l'encodage.

**Verbosité inhérente.** Chaque élément nécessite une balise ouvrante et fermante (`<host>...</host>`), ce qui rend XML significativement plus volumineux que JSON ou YAML pour la même information. L'équivalent JSON du document ci-dessus ferait environ la moitié de la taille.

---

## pugixml : légère, rapide, pragmatique

Plusieurs librairies XML existent en C++ : Xerces-C++ (lourde, conforme aux standards W3C), libxml2 (API C, complète), TinyXML-2 (minimaliste). **pugixml** se positionne comme le meilleur compromis entre performance, fonctionnalité et ergonomie pour le C++ moderne.

### Caractéristiques principales

- **Header-only** (optionnellement compilable en librairie statique) — un header `pugixml.hpp` et un source `pugixml.cpp`, ou un amalgamé single-file.
- **Très rapide** — l'un des parsers XML les plus performants disponibles, grâce à un parsing in-situ qui minimise les allocations.
- **API C++ idiomatique** — types légers avec sémantique de valeur, itérateurs compatibles STL, pas de gestion manuelle de mémoire.
- **XPath intégré** — requêtes XPath 1.0 pour la navigation dans les documents complexes.
- **Empreinte minimale** — environ 150 Ko compilé, pas de dépendance externe.
- **Pas de validation DTD/XSD** — pugixml est un parser non-validant. C'est un choix délibéré : pour la grande majorité des cas d'utilisation, la validation structurelle applicative est préférable à la validation par schéma XML.

---

## Installation

### Conan 2

```python
def requirements(self):
    self.requires("pugixml/1.15")
```

```cmake
find_package(pugixml REQUIRED)  
target_link_libraries(myapp PRIVATE pugixml::pugixml)  
```

### vcpkg

```bash
vcpkg install pugixml
```

```cmake
find_package(pugixml CONFIG REQUIRED)  
target_link_libraries(myapp PRIVATE pugixml::pugixml)  
```

### CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG        v1.15
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(pugixml)

target_link_libraries(myapp PRIVATE pugixml::pugixml)
```

### apt (Ubuntu)

```bash
sudo apt install libpugixml-dev
```

```cmake
find_package(pugixml REQUIRED)  
target_link_libraries(myapp PRIVATE pugixml::pugixml)  
```

### Intégration directe

pugixml se compose de trois fichiers seulement (`pugixml.hpp`, `pugiconfig.hpp`, `pugixml.cpp`). Les copier dans le projet et les ajouter à la compilation est une option parfaitement viable :

```cmake
add_executable(myapp
    src/main.cpp
    third_party/pugixml/pugixml.cpp
)
target_include_directories(myapp PRIVATE third_party/pugixml)
```

---

## Parsing

### Modèle de fonctionnement

pugixml utilise un modèle DOM : le document XML entier est chargé en mémoire sous forme d'un arbre de nœuds. Le type central est `pugi::xml_document`, qui possède l'arbre et gère sa durée de vie. Les nœuds individuels sont représentés par `pugi::xml_node`, un type léger (un pointeur interne) qui peut être copié librement.

```
┌─────────────────┐    load_file()    ┌───────────────────────┐
│  Fichier XML    │ ────────────────► │  pugi::xml_document   │
│  (texte UTF-8)  │                   │  (propriétaire du DOM)│
└─────────────────┘                   │                       │
                                      │  └─ xml_node (racine) │
                                      │       ├─ xml_node     │
                                      │       ├─ xml_node     │
                                      │       └─ ...          │
                                      └───────────────────────┘
```

Un point important : les `xml_node` sont des vues non-propriétaires. Ils deviennent invalides si le `xml_document` qui les a produits est détruit. Il faut donc s'assurer que le document vit au moins aussi longtemps que les nœuds qu'on manipule.

### Chargement depuis un fichier

```cpp
#include <pugixml.hpp>
#include <print>

int main() {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file("config.xml");

    if (!result) {
        std::print(stderr, "Erreur XML : {}\n", result.description());
        std::print(stderr, "  Position : offset {}\n", result.offset);
        return 1;
    }

    pugi::xml_node server = doc.child("server");
    std::print("Serveur : {}\n", server.attribute("name").as_string());
    std::print("Port    : {}\n", server.child("port").text().as_int());
}
```

`load_file` retourne un `xml_parse_result` — pas d'exception par défaut. Le résultat est convertible en `bool` et fournit un message d'erreur descriptif ainsi que l'offset de l'erreur en cas d'échec. Ce design sans exception est idiomatique pour pugixml et s'intègre bien dans les projets où les exceptions sont évitées.

### Chargement depuis une chaîne

```cpp
const char* xml_content = R"(
<config>
    <name>test</name>
    <value>42</value>
</config>
)";

pugi::xml_document doc;  
pugi::xml_parse_result result = doc.load_string(xml_content);  
```

### Chargement depuis un buffer

Pour les données provenant du réseau ou d'un fichier déjà en mémoire :

```cpp
std::vector<char> buffer = receive_from_network();

pugi::xml_document doc;
// load_buffer copie les données — le buffer peut être libéré après
pugi::xml_parse_result result =
    doc.load_buffer(buffer.data(), buffer.size());

// load_buffer_inplace modifie le buffer en place — plus rapide mais
// le buffer doit rester valide pendant toute la durée de vie du document
// pugi::xml_parse_result result =
//     doc.load_buffer_inplace(buffer.data(), buffer.size());
```

La variante `load_buffer_inplace` évite une copie en parsant directement dans le buffer fourni. C'est plus performant pour les gros documents, mais le buffer doit rester alloué et non modifié tant que le document est utilisé.

### Options de parsing

Le parsing accepte des flags qui contrôlent le comportement :

```cpp
// Parsing par défaut : préserve les CDATA, les commentaires, le prologue
pugi::xml_parse_result result = doc.load_file("data.xml");

// Parsing minimal : ignore commentaires, prologue, déclarations
result = doc.load_file("data.xml",
    pugi::parse_minimal);

// Parsing complet : tout inclus, y compris les espaces blancs
result = doc.load_file("data.xml",
    pugi::parse_full);

// Combinaison personnalisée
result = doc.load_file("data.xml",
    pugi::parse_default | pugi::parse_comments);
```

Les flags les plus utiles : `parse_default` (bon compromis pour la plupart des cas), `parse_minimal` (performance maximale quand seules les données importent), `parse_trim_pcdata` (supprime les espaces autour du texte des éléments).

---

## Navigation dans le DOM

### Éléments enfants

L'accès aux éléments enfants se fait par nom avec `.child()` ou par itération :

```cpp
pugi::xml_node server = doc.child("server");

// Accès direct par nom (premier enfant portant ce nom)
pugi::xml_node host = server.child("host");  
pugi::xml_node db = server.child("database");  

// Test d'existence — un nœud non trouvé est "vide" (évalue à false)
if (!server.child("nonexistent")) {
    std::print("Nœud absent\n");
}
```

Un nœud vide (résultat d'un `.child()` sur un nom inexistant) est un objet valide qui évalue à `false`. Toutes les opérations sur un nœud vide retournent des valeurs par défaut (chaîne vide, 0 pour les entiers, etc.) sans lever d'exception ni provoquer de crash. C'est un design défensif qui simplifie le code mais demande une vérification explicite quand l'absence est une erreur.

### Attributs

Les attributs sont accessibles via `.attribute()` :

```cpp
pugi::xml_node server = doc.child("server");

// Accès à un attribut
std::string name = server.attribute("name").as_string();  
std::string version = server.attribute("version").as_string();  

// Attribut avec valeur par défaut
int timeout = server.attribute("timeout").as_int(30000);

// Test d'existence
if (server.attribute("deprecated")) {
    std::print("Attention : serveur marqué comme deprecated\n");
}
```

### Contenu textuel

Le texte contenu dans un élément (`<host>0.0.0.0</host>`) est accessible via `.text()` :

```cpp
pugi::xml_node server = doc.child("server");

std::string host = server.child("host").text().as_string();  
int port         = server.child("port").text().as_int();  
int workers      = server.child("workers").text().as_int(4);  // défaut  
double rate      = server.child("rate").text().as_double(1.0);  
bool debug       = server.child("debug").text().as_bool(false);  
```

Les méthodes d'extraction typée de `.text()` suivent la même convention que les attributs : `as_string()`, `as_int()`, `as_uint()`, `as_double()`, `as_float()`, `as_bool()`, `as_llong()`, `as_ullong()`. Toutes acceptent un paramètre de valeur par défaut optionnel.

### Itération sur les enfants

```cpp
pugi::xml_node endpoints = doc.child("server").child("endpoints");

// Itérer sur tous les enfants "endpoint"
for (pugi::xml_node ep : endpoints.children("endpoint")) {
    std::string path = ep.attribute("path").as_string();
    std::string method = ep.attribute("method").as_string();
    int timeout = ep.attribute("timeout").as_int(5000);

    std::print("{} {} (timeout: {}ms)\n", method, path, timeout);
}
```

L'itération sans argument de nom parcourt tous les enfants, quel que soit leur nom :

```cpp
// Tous les enfants directs de <server>
for (pugi::xml_node child : doc.child("server").children()) {
    std::print("Élément : {}\n", child.name());
}
```

### Itération sur les attributs

```cpp
pugi::xml_node server = doc.child("server");

for (pugi::xml_attribute attr : server.attributes()) {
    std::print("  {} = {}\n", attr.name(), attr.as_string());
}
```

### Navigation dans l'arbre

pugixml fournit des méthodes de navigation entre frères et vers le parent :

```cpp
pugi::xml_node first = server.first_child();  
pugi::xml_node last = server.last_child();  
pugi::xml_node next = first.next_sibling();  
pugi::xml_node prev = last.previous_sibling();  
pugi::xml_node parent = first.parent();  

// Frère suivant portant un nom spécifique
pugi::xml_node next_endpoint = ep.next_sibling("endpoint");
```

---

## XPath : requêtes sur le document

Pour les documents complexes avec de nombreux niveaux d'imbrication, la navigation manuelle nœud par nœud devient fastidieuse. XPath est un langage de requêtes standardisé (W3C) qui permet de sélectionner des nœuds par expression. pugixml implémente XPath 1.0.

```cpp
pugi::xml_document doc;  
doc.load_file("config.xml");  

// Sélectionner tous les endpoints GET
pugi::xpath_node_set get_endpoints =
    doc.select_nodes("//endpoint[@method='GET']");

for (const auto& xnode : get_endpoints) {
    std::print("GET {}\n",
        xnode.node().attribute("path").as_string());
}

// Sélectionner une valeur unique
pugi::xpath_node result =
    doc.select_node("/server/database/host");

if (result) {
    std::print("DB host : {}\n", result.node().text().as_string());
}

// Évaluer une expression numérique XPath
pugi::xpath_query count_query("count(//endpoint)");  
double endpoint_count = count_query.evaluate_number(doc);  
```

### Expressions XPath courantes

| Expression | Sélection |
|-----------|-----------|
| `/server/host` | Élément `<host>` enfant direct de `<server>` |
| `//endpoint` | Tous les éléments `<endpoint>` à n'importe quel niveau |
| `//endpoint[@method='POST']` | Endpoints avec attribut `method` valant `POST` |
| `/server/database/@username` | Attribut `username` de `<database>` |
| `//endpoint[last()]` | Dernier élément `<endpoint>` |
| `count(//endpoint)` | Nombre d'éléments `<endpoint>` |
| `//output[@type='file']/@path` | Attribut `path` des outputs de type `file` |

XPath est particulièrement utile quand on ne contrôle pas le schéma XML (interfaçage avec un système tiers) et que la structure peut varier. Plutôt que de coder une navigation rigide, une requête XPath s'adapte plus facilement aux variations structurelles.

### Gestion des erreurs XPath

Une expression XPath invalide lève une `pugi::xpath_exception` :

```cpp
try {
    auto nodes = doc.select_nodes("///invalid[xpath");
} catch (const pugi::xpath_exception& e) {
    std::print(stderr, "XPath invalide : {}\n", e.what());
}
```

Pour le mode sans exception, on peut pré-compiler la requête et vérifier sa validité :

```cpp
pugi::xpath_query query("//endpoint[@method='GET']");  
if (!query) {  
    std::print(stderr, "XPath invalide\n");
} else {
    auto nodes = doc.select_nodes(query);
    // ...
}
```

---

## Conversion vers des types métier

Comme pour toml++, pugixml ne fournit pas de mécanisme de conversion automatique. La conversion vers des structures C++ s'écrit manuellement :

```cpp
struct Endpoint {
    std::string path;
    std::string method;
    int timeout_ms;
};

struct DatabaseConfig {
    std::string host;
    int port;
    std::string username;
    std::string password;
};

struct ServerConfig {
    std::string name;
    std::string host;
    int port;
    int workers;
    DatabaseConfig database;
    std::vector<Endpoint> endpoints;
};

Endpoint parse_endpoint(pugi::xml_node node) {
    return Endpoint{
        .path = node.attribute("path").as_string(),
        .method = node.attribute("method").as_string(),
        .timeout_ms = node.attribute("timeout").as_int(5000)
    };
}

DatabaseConfig parse_database(pugi::xml_node node) {
    return DatabaseConfig{
        .host = node.child("host").text().as_string(),
        .port = node.child("port").text().as_int(5432),
        .username = node.child("credentials").attribute("username").as_string(),
        .password = node.child("credentials").attribute("password").as_string()
    };
}

std::optional<ServerConfig> parse_server_config(pugi::xml_document& doc) {
    pugi::xml_node server = doc.child("server");
    if (!server) {
        std::print(stderr, "Élément <server> manquant\n");
        return std::nullopt;
    }

    ServerConfig cfg;
    cfg.name    = server.attribute("name").as_string("unnamed");
    cfg.host    = server.child("host").text().as_string("localhost");
    cfg.port    = server.child("port").text().as_int(8080);
    cfg.workers = server.child("workers").text().as_int(4);

    if (pugi::xml_node db = server.child("database")) {
        cfg.database = parse_database(db);
    }

    for (auto ep : server.child("endpoints").children("endpoint")) {
        cfg.endpoints.push_back(parse_endpoint(ep));
    }

    return cfg;
}
```

Utilisation :

```cpp
pugi::xml_document doc;  
auto result = doc.load_file("config.xml");  
if (!result) {  
    std::print(stderr, "XML invalide : {}\n", result.description());
    return 1;
}

auto config = parse_server_config(doc);  
if (!config) return 1;  

std::print("Serveur '{}' sur {}:{}\n",
    config->name, config->host, config->port);
```

Le pattern est identique à celui de toml++ : des fonctions de parsing dédiées par type, composées pour l'arborescence complète. Le code est verbeux mais explicite, et chaque champ dispose d'une valeur par défaut raisonnable.

---

## Écriture XML

pugixml permet de construire des documents XML programmatiquement et de les sérialiser.

### Construction d'un document

```cpp
pugi::xml_document doc;

// Déclaration XML
pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);  
decl.append_attribute("version") = "1.0";  
decl.append_attribute("encoding") = "UTF-8";  

// Élément racine
pugi::xml_node server = doc.append_child("server");  
server.append_attribute("name") = "api-prod";  

// Éléments enfants avec texte
server.append_child("host").text().set("0.0.0.0");  
server.append_child("port").text().set(8080);  
server.append_child("workers").text().set(4);  

// Sous-structure
pugi::xml_node db = server.append_child("database");  
db.append_child("host").text().set("db.internal");  
db.append_child("port").text().set(5432);  

// Éléments avec attributs (auto-fermants)
pugi::xml_node endpoints = server.append_child("endpoints");

pugi::xml_node ep1 = endpoints.append_child("endpoint");  
ep1.append_attribute("path") = "/health";  
ep1.append_attribute("method") = "GET";  
ep1.append_attribute("timeout") = 1000;  

pugi::xml_node ep2 = endpoints.append_child("endpoint");  
ep2.append_attribute("path") = "/api/data";  
ep2.append_attribute("method") = "POST";  
ep2.append_attribute("timeout") = 5000;  
```

### Sérialisation vers fichier

```cpp
// Écriture avec indentation (par défaut)
bool success = doc.save_file("output.xml");  
if (!success) {  
    std::print(stderr, "Échec de l'écriture\n");
}

// Écriture vers un flux
doc.save(std::cout, "  ", pugi::format_indent);
```

### Contrôle du formatage

```cpp
// Indentation avec tabulation
doc.save_file("output.xml", "\t", pugi::format_indent);

// Pas d'indentation (compact)
doc.save_file("output.xml", "", pugi::format_raw);

// Sans déclaration XML
doc.save_file("output.xml", "  ",
    pugi::format_indent | pugi::format_no_declaration);

// Écriture vers une chaîne
std::ostringstream stream;  
doc.save(stream, "  ", pugi::format_indent);  
std::string xml_string = stream.str();  
```

### Écriture depuis des types métier

En miroir des fonctions de parsing, les fonctions de sérialisation construisent l'arbre XML :

```cpp
void emit_endpoint(pugi::xml_node parent, const Endpoint& ep) {
    pugi::xml_node node = parent.append_child("endpoint");
    node.append_attribute("path") = ep.path.c_str();
    node.append_attribute("method") = ep.method.c_str();
    node.append_attribute("timeout") = ep.timeout_ms;
}

void emit_server_config(pugi::xml_document& doc, const ServerConfig& cfg) {
    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    pugi::xml_node server = doc.append_child("server");
    server.append_attribute("name") = cfg.name.c_str();

    server.append_child("host").text().set(cfg.host.c_str());
    server.append_child("port").text().set(cfg.port);
    server.append_child("workers").text().set(cfg.workers);

    pugi::xml_node endpoints = server.append_child("endpoints");
    for (const auto& ep : cfg.endpoints) {
        emit_endpoint(endpoints, ep);
    }
}
```

> 💡 *À partir de la version 1.15, pugixml accepte nativement `std::string` et `std::string_view` dans les assignations d'attributs et les appels `set()`. L'appel `.c_str()` n'est plus nécessaire mais reste fonctionnel. Les exemples ci-dessus utilisent `.c_str()` par compatibilité avec les versions antérieures.*

---

## Modification de documents existants

Un cas d'usage courant avec XML est de charger un document existant, modifier quelques valeurs, et le réécrire. pugixml gère ce cycle naturellement :

```cpp
pugi::xml_document doc;  
doc.load_file("config.xml");  

// Modifier une valeur existante
pugi::xml_node server = doc.child("server");  
server.child("workers").text().set(16);  

// Ajouter un nouvel élément
pugi::xml_node ep = server.child("endpoints").append_child("endpoint");  
ep.append_attribute("path") = "/metrics";  
ep.append_attribute("method") = "GET";  
ep.append_attribute("timeout") = 2000;  

// Supprimer un élément
pugi::xml_node db = server.child("database");  
server.remove_child(db);  

// Modifier un attribut
server.attribute("version").set_value("3.3");

// Sauvegarder
doc.save_file("config.xml");
```

Contrairement au cycle load-modify-emit de YAML (qui perd les commentaires), pugixml **préserve les commentaires XML** présents dans le document original, car ils font partie de l'arbre DOM. C'est un avantage significatif pour l'édition programmatique de fichiers de configuration XML existants.

---

## Gestion des namespaces

Les documents XML d'entreprise utilisent fréquemment les namespaces. pugixml n'offre pas de support dédié aux namespaces (pas de résolution automatique), mais ils sont accessibles comme des attributs et des préfixes de noms :

```cpp
// Document avec namespaces
const char* xml = R"(
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/">
    <s:Body>
        <GetUserResponse xmlns="http://example.com/api">
            <User>
                <Name>Alice</Name>
                <Email>alice@example.com</Email>
            </User>
        </GetUserResponse>
    </s:Body>
</s:Envelope>
)";

pugi::xml_document doc;  
doc.load_string(xml);  

// Navigation avec préfixes explicites
pugi::xml_node body = doc.child("s:Envelope").child("s:Body");  
pugi::xml_node response = body.child("GetUserResponse");  
pugi::xml_node user = response.child("User");  

std::string name = user.child("Name").text().as_string();  
std::string email = user.child("Email").text().as_string();  
```

Pour les documents avec des namespaces complexes ou variables, XPath offre une approche plus robuste avec la fonction `local-name()` qui ignore les préfixes :

```cpp
// Sélectionner indépendamment du préfixe de namespace
auto body = doc.select_node("//*[local-name()='Body']");  
auto user = doc.select_node("//*[local-name()='User']");  
```

---

## Gestion des erreurs

### Erreurs de parsing

```cpp
pugi::xml_document doc;  
pugi::xml_parse_result result = doc.load_file("config.xml");  

if (!result) {
    std::print(stderr, "Erreur XML : {}\n", result.description());
    std::print(stderr, "  Offset  : {}\n", result.offset);
    std::print(stderr, "  Status  : {}\n", static_cast<int>(result.status));
}
```

Les statuts possibles incluent `status_ok`, `status_file_not_found`, `status_io_error`, `status_out_of_memory`, `status_unrecognized_tag`, `status_bad_start_element`, `status_bad_attribute`, `status_end_element_mismatch`, entre autres. Le champ `description()` fournit un message lisible.

### Nœuds manquants

pugixml ne lève pas d'exception pour les nœuds ou attributs manquants. Un nœud vide retourne des valeurs par défaut silencieusement. C'est pratique pour le code concis mais dangereux si un champ obligatoire est silencieusement remplacé par une valeur par défaut. La validation explicite reste nécessaire :

```cpp
pugi::xml_node server = doc.child("server");  
if (!server) {  
    std::print(stderr, "Élément <server> obligatoire manquant\n");
    return 1;
}

pugi::xml_node host = server.child("host");  
if (!host || std::string_view(host.text().as_string()).empty()) {  
    std::print(stderr, "<host> obligatoire et ne peut pas être vide\n");
    return 1;
}
```

---

## Quand choisir pugixml

**pugixml est le bon choix quand :**

- Le projet doit interfacer avec un système existant qui produit ou consomme du XML.
- Le document XML est relativement simple (configuration, données structurées) et ne nécessite pas de validation par schéma XSD.
- La performance de parsing est importante (pugixml est parmi les plus rapides).
- XPath est nécessaire pour naviguer dans des documents à structure variable.
- Le programme doit modifier et réécrire des documents XML existants en préservant leur structure.

**pugixml n'est pas le bon choix quand :**

- Une validation stricte par DTD ou XSD est requise — utiliser Xerces-C++ ou libxml2.
- Le XML utilise des fonctionnalités avancées comme XSLT — utiliser libxslt.
- Le choix du format est libre — JSON, YAML ou TOML sont préférables pour tout nouveau projet.

---

## Résumé comparatif avec les autres librairies du chapitre

| Opération | pugixml | nlohmann/json | yaml-cpp | toml++ |
|-----------|---------|---------------|----------|--------|
| Parsing fichier | `doc.load_file(path)` | `json::parse(ifstream)` | `YAML::LoadFile(path)` | `toml::parse_file(path)` |
| Type racine | `xml_document` | `json` | `YAML::Node` | `toml::table` |
| Accès enfant | `.child("name")` | `["name"]` | `["name"]` | `["name"]` |
| Accès attribut | `.attribute("name")` | N/A | N/A | N/A |
| Extraction typée | `.text().as_int()` | `.get<int>()` | `.as<int>()` | `.value<int64_t>()` |
| Valeur par défaut | `.as_int(42)` | `.value("k", 42)` | `.as<int>(42)` | `.value_or(42)` |
| Requêtes avancées | XPath | JSON Pointer | N/A | `.at_path()` |
| Erreurs | Code retour | Exceptions | Exceptions | Exception ou `parse_result` |
| Modification | Oui (in-place) | Oui | Oui | Oui |
| Préserve commentaires | Oui | N/A (pas de commentaires) | Non | N/A (commentaires ignorés) |
| Header-only | Quasi (3 fichiers) | Oui | Non | Oui |

La section 24.5 clôt ce chapitre avec les bonnes pratiques de validation de schémas, applicables à tous les formats couverts.

⏭️ [Bonnes pratiques : Validation de schémas](/24-serialisation-config/05-validation-schemas.md)
