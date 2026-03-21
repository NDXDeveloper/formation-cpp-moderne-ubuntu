🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 25.4 — MessagePack : JSON binaire compact

## Section du Module 8 — Parsing et Formats de Données

---

## Une troisième voie : binaire sans schéma

Protobuf et FlatBuffers reposent sur un schéma formel — un fichier `.proto` ou `.fbs` — compilé en code source avant l'utilisation. Ce schéma est une force (typage, rétrocompatibilité, documentation) mais aussi une contrainte : il impose une étape de génération de code, une synchronisation du schéma entre producteur et consommateur, et une rigidité structurelle.

MessagePack adopte une philosophie différente. C'est un format de sérialisation binaire **auto-descriptif** et **schema-less** — comme JSON, mais en binaire. Les données portent leur propre structure : chaque valeur est précédée d'un tag de type qui indique comment la lire. Pas de fichier de schéma, pas de compilation préalable, pas de génération de code. On sérialise un objet, on obtient un buffer binaire. On reçoit un buffer binaire, on le désérialise. C'est aussi simple que JSON, avec la compacité et la vitesse du binaire.

```
JSON :    {"name":"alice","age":30,"active":true}
          40 octets (texte, guillemets, noms de clés)

MessagePack :  83 a4 6e 61 6d 65 a5 61 6c 69 63 65 a3 61 67 65 1e a6 ...
               27 octets (binaire, tags de type, noms de clés compacts)

Protobuf : (binaire, numéros de champ, pas de noms)
           ~15 octets (nécessite un schéma .proto)
```

MessagePack se situe entre JSON et Protobuf : plus compact et plus rapide que JSON, mais moins compact que Protobuf car il conserve les noms de clés dans le flux. En contrepartie, il n'exige aucun schéma partagé — tout programme capable de lire du MessagePack peut décoder le message sans connaître sa structure à l'avance.

---

## Cas d'usage

MessagePack excelle dans les contextes où JSON est utilisé mais où sa verbosité textuelle devient un problème :

**Cache et stockage temporaire.** Remplacer la sérialisation JSON par MessagePack dans un cache Redis ou mémoire réduit la taille des entrées de 30 à 50% et accélère la sérialisation/désérialisation. La structure des données n'a pas besoin d'être figée dans un schéma — elle peut évoluer librement.

**Protocoles de communication légers.** Les protocoles binaires entre composants internes (workers, agents, collecteurs de métriques) bénéficient de la compacité de MessagePack sans la contrainte d'un schéma partagé. C'est particulièrement utile en phase de prototypage, quand le format évolue rapidement.

**Logs binaires structurés.** Écrire des événements structurés en MessagePack plutôt qu'en JSON-lignes réduit l'espace disque et accélère l'écriture. Fluentd et Fluent Bit utilisent MessagePack comme format interne.

**Échange inter-langages sans schéma.** MessagePack est supporté dans plus de 50 langages. Quand deux services doivent échanger des données structurées sans partager un fichier de schéma, MessagePack est un remplacement naturel de JSON.

**Formats de fichiers compacts.** Certaines bases de données embarquées et formats de configuration binaire utilisent MessagePack comme format de sérialisation des enregistrements.

---

## msgpack-c : la librairie C++ de référence

**msgpack-c** (aussi appelée msgpack-cxx pour la version C++ moderne) est la librairie MessagePack officielle pour C et C++. La version C++ est **header-only** et fournit une API moderne basée sur les templates, avec une intégration transparente des types STL.

### Caractéristiques principales

- **Header-only** — inclusion simple, aucune librairie à linker.  
- **Conversion automatique des types STL** — `std::string`, `std::vector`, `std::map`, `std::tuple`, `std::optional` (C++17) sont sérialisés et désérialisés nativement.  
- **Macro de conversion pour les types utilisateur** — une seule ligne pour rendre un type sérialisable.  
- **Streaming** — support du packing/unpacking incrémental pour les flux de données.  
- **Zéro dépendance** — uniquement la bibliothèque standard C++.

---

## Installation

### Conan 2

```python
def requirements(self):
    self.requires("msgpack-cxx/6.1.1")
```

```cmake
find_package(msgpack-cxx REQUIRED)  
target_link_libraries(myapp PRIVATE msgpack-cxx)  
```

### vcpkg

```bash
vcpkg install msgpack-cxx
```

```cmake
find_package(msgpack-cxx CONFIG REQUIRED)  
target_link_libraries(myapp PRIVATE msgpack-cxx)  
```

### CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    msgpack-cxx
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG        cpp-6.1.1
    GIT_SHALLOW    TRUE
)

set(MSGPACK_USE_BOOST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(msgpack-cxx)

target_link_libraries(myapp PRIVATE msgpack-cxx)
```

L'option `MSGPACK_USE_BOOST OFF` désactive la dépendance optionnelle à Boost, qui n'est pas nécessaire pour les fonctionnalités couvertes ici.

### apt (Ubuntu)

```bash
sudo apt install libmsgpack-cxx-dev
```

---

## Sérialisation et désérialisation de base

### Types primitifs et STL

```cpp
#include <msgpack.hpp>
#include <print>
#include <sstream>

int main() {
    // === Sérialisation ===
    msgpack::sbuffer buffer;  // buffer de sortie (comme std::stringbuf)
    msgpack::packer<msgpack::sbuffer> packer(buffer);

    // Empaqueter des valeurs individuelles
    packer.pack(42);
    packer.pack(std::string("hello"));
    packer.pack(3.14);
    packer.pack(true);

    std::print("Sérialisé : {} octets\n", buffer.size());

    // === Désérialisation ===
    msgpack::object_handle oh =
        msgpack::unpack(buffer.data(), buffer.size());

    // L'objet désérialisé est un variant typé
    msgpack::object obj = oh.get();
    std::print("Type  : {}\n", static_cast<int>(obj.type));
    std::print("Value : {}\n", obj.as<int>());
}
```

Un raccourci plus concis pour la sérialisation utilise `msgpack::pack` et `msgpack::sbuffer` directement :

```cpp
// Sérialisation d'un vector
std::vector<int> scores = {95, 87, 72, 100};

msgpack::sbuffer buffer;  
msgpack::pack(buffer, scores);  

// Désérialisation
auto handle = msgpack::unpack(buffer.data(), buffer.size());  
auto restored = handle.get().as<std::vector<int>>();  

for (int s : restored) {
    std::print("{} ", s);
}
// 95 87 72 100
```

### Maps (équivalent des objets JSON)

```cpp
std::map<std::string, msgpack::object> data;
// Pour des maps simples, utiliser directement std::map :

std::map<std::string, std::string> metadata = {
    {"region", "eu-west"},
    {"env", "production"},
    {"version", "3.2.1"}
};

msgpack::sbuffer buffer;  
msgpack::pack(buffer, metadata);  

auto handle = msgpack::unpack(buffer.data(), buffer.size());  
auto restored = handle.get().as<std::map<std::string, std::string>>();  

for (const auto& [key, value] : restored) {
    std::print("{} = {}\n", key, value);
}
```

### Types STL supportés nativement

msgpack-cxx fournit des conversions automatiques pour la plupart des conteneurs STL :

| Type C++ | Type MessagePack | Notes |
|----------|-----------------|-------|
| `bool` | Boolean | |
| `int`, `int64_t`, etc. | Integer | Entiers signés/non signés |
| `float`, `double` | Float | |
| `std::string` | String/Binary | |
| `std::vector<T>` | Array | Pour tout `T` sérialisable |
| `std::array<T, N>` | Array | |
| `std::list<T>` | Array | |
| `std::deque<T>` | Array | |
| `std::set<T>` | Array | |
| `std::map<K, V>` | Map | |
| `std::unordered_map<K, V>` | Map | |
| `std::pair<A, B>` | Array (2 éléments) | |
| `std::tuple<Ts...>` | Array (N éléments) | |
| `std::optional<T>` | T ou Nil | C++17 |
| `std::nullptr_t` | Nil | |

La composition fonctionne récursivement : un `std::vector<std::map<std::string, std::vector<int>>>` se sérialise et se désérialise sans code supplémentaire.

---

## Sérialisation de types utilisateur

### Avec la macro `MSGPACK_DEFINE`

Pour rendre un type utilisateur sérialisable, la macro `MSGPACK_DEFINE` (intrusive) ou `MSGPACK_DEFINE_MAP` liste les membres à inclure :

```cpp
#include <msgpack.hpp>
#include <string>
#include <vector>
#include <optional>

struct Endpoint {
    std::string path;
    std::string method;
    int timeout_ms;

    // Sérialise comme un tableau MessagePack [path, method, timeout_ms]
    MSGPACK_DEFINE(path, method, timeout_ms);
};

struct ServerConfig {
    std::string host;
    int port;
    int workers;
    std::vector<Endpoint> endpoints;
    std::optional<std::string> description;

    // Sérialise comme un tableau
    MSGPACK_DEFINE(host, port, workers, endpoints, description);
};
```

Utilisation :

```cpp
ServerConfig config{
    .host = "0.0.0.0",
    .port = 8080,
    .workers = 4,
    .endpoints = {
        {"/health", "GET", 1000},
        {"/api/data", "POST", 5000}
    },
    .description = "Production API server"
};

// Sérialisation
msgpack::sbuffer buffer;  
msgpack::pack(buffer, config);  
std::print("Taille : {} octets\n", buffer.size());  

// Désérialisation
auto handle = msgpack::unpack(buffer.data(), buffer.size());  
auto restored = handle.get().as<ServerConfig>();  

std::print("{}:{} ({} workers)\n",
    restored.host, restored.port, restored.workers);
for (const auto& ep : restored.endpoints) {
    std::print("  {} {} ({}ms)\n", ep.method, ep.path, ep.timeout_ms);
}
```

### `MSGPACK_DEFINE` vs `MSGPACK_DEFINE_MAP`

La macro `MSGPACK_DEFINE` sérialise les champs comme un **tableau** MessagePack — compact mais positionnel (comme un tuple). L'ordre des champs doit être identique entre le producteur et le consommateur.

La macro `MSGPACK_DEFINE_MAP` sérialise les champs comme une **map** MessagePack — chaque champ est une paire clé-valeur avec son nom, comme en JSON. Plus volumineux mais résistant au réordonnement des champs et compatible avec les lecteurs qui n'utilisent pas le même schéma :

```cpp
struct ServerConfig {
    std::string host;
    int port;
    int workers;

    // Sérialise comme {"host": "...", "port": 8080, "workers": 4}
    MSGPACK_DEFINE_MAP(host, port, workers);
};
```

| Macro | Format | Compacité | Rétrocompatibilité | Usage recommandé |
|-------|--------|-----------|--------------------|--------------------|
| `MSGPACK_DEFINE` | Tableau `[v1, v2, v3]` | Maximale | Fragile (positionnel) | Communication interne, schéma stable |
| `MSGPACK_DEFINE_MAP` | Map `{k1:v1, k2:v2}` | Moindre (~JSON) | Bonne (par nom) | Stockage, échange inter-équipes |

Pour les systèmes où le producteur et le consommateur évoluent indépendamment, `MSGPACK_DEFINE_MAP` est le choix sûr.

### Macro non intrusive

Pour les types qu'on ne peut pas modifier (librairies tierces), la macro `MSGPACK_ADD_ENUM` gère les enums, et `MSGPACK_DEFINE_EXTERNAL` les types externes :

```cpp
// Enum d'une librairie tierce
enum class LogLevel { trace, debug, info, warn, error, fatal };  
MSGPACK_ADD_ENUM(LogLevel);  
```

---

## Le type `msgpack::object` : inspection dynamique

Lors de la désérialisation, si le type cible n'est pas connu à la compilation, `msgpack::object` permet une inspection dynamique similaire à la navigation dans un `nlohmann::json` :

```cpp
auto handle = msgpack::unpack(buffer.data(), buffer.size());  
msgpack::object obj = handle.get();  

switch (obj.type) {
    case msgpack::type::POSITIVE_INTEGER:
        std::print("Entier : {}\n", obj.as<int64_t>());
        break;

    case msgpack::type::STR:
        std::print("Chaîne : {}\n", obj.as<std::string>());
        break;

    case msgpack::type::ARRAY: {
        std::print("Tableau ({} éléments) :\n", obj.via.array.size);
        for (uint32_t i = 0; i < obj.via.array.size; ++i) {
            std::print("  [{}] type={}\n", i,
                static_cast<int>(obj.via.array.ptr[i].type));
        }
        break;
    }

    case msgpack::type::MAP: {
        std::print("Map ({} entrées) :\n", obj.via.map.size);
        for (uint32_t i = 0; i < obj.via.map.size; ++i) {
            auto& kv = obj.via.map.ptr[i];
            std::print("  {} → {}\n",
                kv.key.as<std::string>(),
                kv.val.as<std::string>());
        }
        break;
    }

    case msgpack::type::BOOLEAN:
        std::print("Booléen : {}\n", obj.as<bool>());
        break;

    case msgpack::type::NIL:
        std::print("Null\n");
        break;

    case msgpack::type::FLOAT32:
    case msgpack::type::FLOAT64:
        std::print("Flottant : {}\n", obj.as<double>());
        break;

    default:
        std::print("Type inconnu : {}\n", static_cast<int>(obj.type));
}
```

Les types MessagePack sont : `NIL`, `BOOLEAN`, `POSITIVE_INTEGER`, `NEGATIVE_INTEGER`, `FLOAT32`, `FLOAT64`, `STR`, `BIN`, `ARRAY`, `MAP`, `EXT`.

Ce mode d'utilisation est utile pour les outils de débogage, les convertisseurs MessagePack ↔ JSON, et les systèmes qui routent des messages sans en connaître la structure.

---

## Streaming : messages multiples

Contrairement à Protobuf, MessagePack est auto-délimitant : le format encode la taille de chaque valeur dans son tag. Plusieurs messages peuvent être concaténés dans un flux et désérialisés séquentiellement sans framing explicite.

### Écriture séquentielle

```cpp
msgpack::sbuffer buffer;  
msgpack::packer<msgpack::sbuffer> packer(buffer);  

// Empaqueter plusieurs messages dans le même buffer
packer.pack(std::string("event_a"));  
packer.pack(42);  
packer.pack(std::map<std::string, int>{{"x", 1}, {"y", 2}});  
```

### Lecture séquentielle avec `msgpack::unpacker`

```cpp
msgpack::unpacker unpacker;

// Simuler la réception de données par morceaux
unpacker.reserve_buffer(buffer.size());  
std::memcpy(unpacker.buffer(), buffer.data(), buffer.size());  
unpacker.buffer_consumed(buffer.size());  

// Extraire les messages un par un
msgpack::object_handle handle;  
while (unpacker.next(handle)) {  
    msgpack::object obj = handle.get();
    std::print("Message type={} : ", static_cast<int>(obj.type));

    switch (obj.type) {
        case msgpack::type::STR:
            std::print("{}\n", obj.as<std::string>());
            break;
        case msgpack::type::POSITIVE_INTEGER:
            std::print("{}\n", obj.as<int>());
            break;
        case msgpack::type::MAP:
            std::print("(map, {} entrées)\n", obj.via.map.size);
            break;
        default:
            std::print("(autre)\n");
    }
}
```

Le `msgpack::unpacker` est conçu pour le streaming réseau : on y verse des morceaux de données au fur et à mesure qu'ils arrivent (`reserve_buffer` + `buffer_consumed`), et on extrait les messages complets avec `next()`. Si un message est incomplet (données pas encore arrivées), `next()` retourne `false` et attend plus de données. C'est un automate de parsing incrémental, idéal pour les sockets non-bloquantes et les boucles d'événements.

---

## Conversion MessagePack ↔ JSON

La correspondance structurelle entre MessagePack et JSON rend la conversion bidirectionnelle naturelle. Bien que msgpack-cxx ne fournisse pas de convertisseur JSON intégré, la conversion est triviale via nlohmann/json :

```cpp
#include <msgpack.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Conversion récursive : msgpack::object → nlohmann::json
json msgpack_to_json(const msgpack::object& obj) {
    switch (obj.type) {
        case msgpack::type::NIL:
            return nullptr;
        case msgpack::type::BOOLEAN:
            return obj.as<bool>();
        case msgpack::type::POSITIVE_INTEGER:
            return obj.as<uint64_t>();
        case msgpack::type::NEGATIVE_INTEGER:
            return obj.as<int64_t>();
        case msgpack::type::FLOAT32:
        case msgpack::type::FLOAT64:
            return obj.as<double>();
        case msgpack::type::STR:
            return obj.as<std::string>();
        case msgpack::type::ARRAY: {
            json arr = json::array();
            for (uint32_t i = 0; i < obj.via.array.size; ++i) {
                arr.push_back(msgpack_to_json(obj.via.array.ptr[i]));
            }
            return arr;
        }
        case msgpack::type::MAP: {
            json map = json::object();
            for (uint32_t i = 0; i < obj.via.map.size; ++i) {
                auto& kv = obj.via.map.ptr[i];
                map[kv.key.as<std::string>()] = msgpack_to_json(kv.val);
            }
            return map;
        }
        default:
            return nullptr;
    }
}

// Utilisation
auto handle = msgpack::unpack(buffer.data(), buffer.size());  
json j = msgpack_to_json(handle.get());  
std::print("{}\n", j.dump(2));  
```

Ce convertisseur est utile pour le débogage (inspecter un buffer MessagePack sous forme JSON lisible), pour les outils d'administration, et pour les systèmes qui acceptent les deux formats en entrée.

---

## Gestion des erreurs

msgpack-cxx utilise des exceptions pour signaler les erreurs. Les principales :

**`msgpack::type_error`** — levée quand `.as<T>()` est appelé avec un type incompatible :

```cpp
try {
    auto handle = msgpack::unpack(buffer.data(), buffer.size());
    // Le buffer contient une chaîne, mais on demande un int
    int value = handle.get().as<int>();
} catch (const msgpack::type_error& e) {
    std::print(stderr, "Type MessagePack incompatible : {}\n", e.what());
}
```

**`msgpack::unpack_error`** — levée quand le buffer est malformé ou tronqué :

```cpp
try {
    auto handle = msgpack::unpack(corrupted_data.data(),
                                  corrupted_data.size());
} catch (const msgpack::unpack_error& e) {
    std::print(stderr, "Buffer MessagePack invalide : {}\n", e.what());
}
```

**`msgpack::insufficient_bytes`** — sous-type d'unpack_error indiquant que le buffer est trop court pour contenir un message complet. Avec le `msgpack::unpacker` en mode streaming, cette situation est gérée par `next()` retournant `false` plutôt que par une exception.

### Limites de sécurité

Pour les données provenant de sources non fiables, il est important de limiter la profondeur de récursion et la taille des objets pour prévenir les attaques par déni de service :

```cpp
// Configurer les limites de l'unpacker
// Le constructeur accepte : array, map, str, bin, ext, depth
// Les valeurs par défaut sont 0xFFFFFFFF (pas de limite effective)
msgpack::unpack_limit limits(
    1000,       // taille max des tableaux
    1000,       // taille max des maps
    1024*1024,  // taille max des chaînes (1 Mo)
    1024*1024,  // taille max des binaires (1 Mo)
    1024*1024,  // taille max des extensions (1 Mo)
    64          // profondeur max de récursion
);

auto handle = msgpack::unpack(
    buffer.data(), buffer.size(),
    nullptr,    // unpack_reference_func (callback optionnel)
    nullptr,    // user_data pour le callback
    limits      // limites de décodage
);
```

---

## Performance

MessagePack est significativement plus rapide que JSON pour la sérialisation et la désérialisation, tout en restant plus lent que Protobuf (qui bénéficie du code généré spécifique à chaque type de message).

### Benchmarks indicatifs

| Opération | MessagePack | JSON (nlohmann) | Protobuf |
|-----------|------------|-----------------|----------|
| Sérialisation (petit message) | ~2-5 M msg/s | ~0.5-1 M msg/s | ~5-10 M msg/s |
| Désérialisation (petit message) | ~1-3 M msg/s | ~0.3-0.8 M msg/s | ~3-8 M msg/s |
| Taille (petit message) | ~30 octets | ~50 octets | ~15 octets |

MessagePack se situe typiquement **2-5x plus rapide que JSON** et produit des messages **30-50% plus compacts**. Protobuf reste **2-3x plus rapide** et **~50% plus compact** que MessagePack, grâce à l'absence de noms de clés et au code de sérialisation généré à la compilation.

### Optimisations

**Réutiliser le `sbuffer`.** Appeler `buffer.clear()` au lieu de créer un nouveau `sbuffer` à chaque message évite les réallocations :

```cpp
msgpack::sbuffer buffer;

for (const auto& record : records) {
    buffer.clear();  // réutilise la mémoire allouée
    msgpack::pack(buffer, record);
    send(buffer.data(), buffer.size());
}
```

**Utiliser `MSGPACK_DEFINE` plutôt que `MSGPACK_DEFINE_MAP`.** Le format tableau est plus compact et plus rapide à sérialiser/désérialiser que le format map (pas de noms de clés à encoder/décoder).

**Pré-allouer le buffer.** Pour les messages de taille prévisible, `sbuffer::reserve()` évite les réallocations pendant le packing.

---

## Quand choisir MessagePack

**MessagePack est le bon choix quand :**

- JSON est utilisé mais sa verbosité ou sa lenteur de parsing pose problème.  
- Le format des données évolue rapidement et un schéma formel serait un frein.  
- L'intégration doit être triviale — header-only, pas de génération de code, pas d'outil de build supplémentaire.  
- Les données doivent rester inspectables (conversion facile vers JSON pour le débogage).  
- Le système implique de nombreux langages et la compatibilité universelle prime.

**MessagePack n'est pas le bon choix quand :**

- Un contrat formel entre producteur et consommateur est nécessaire — Protobuf ou FlatBuffers avec leur schéma sont préférables.  
- La compacité maximale est critique — les noms de clés dans le format map sont de l'overhead que Protobuf élimine.  
- Le projet utilise gRPC — Protobuf est imposé.  
- La validation structurelle est importante — sans schéma, la validation repose entièrement sur le code applicatif.

---

## Résumé comparatif

| Aspect | MessagePack | Protobuf | FlatBuffers |
|--------|-------------|----------|-------------|
| Schéma | Aucun | `.proto` obligatoire | `.fbs` obligatoire |
| Génération de code | Non | Oui (`protoc`) | Oui (`flatc`) |
| Intégration | Header-only, triviale | Librairie + outil de build | Header-only + outil de build |
| Auto-descriptif | Oui (noms de clés en mode map) | Non (numéros de champ) | Non (vtable) |
| Compacité | Bonne | Très bonne | Bonne |
| Vitesse sérialisation | Rapide | Très rapide | Rapide |
| Vitesse désérialisation | Rapide | Très rapide | Quasi-instantanée (zero-copy) |
| Streaming natif | Oui (auto-délimitant) | Non (framing manuel) | Non |
| Inspection/debug | Conversion JSON triviale | `protoc --decode`, `DebugString` | Outils dédiés |
| Rétrocompatibilité | Responsabilité du développeur | Intégrée (numéros de champ) | Intégrée (vtable) |
| Conversion JSON | Structurellement isomorphe | Mapping standardisé | Via `flatc` |
| Multi-langage | 50+ langages | 12+ langages officiels | 18+ langages |

La section 25.4 conclut ce chapitre avec une comparaison de performances mesurée et un arbre de décision pour choisir le bon format selon le contexte du projet.

⏭️ [Comparaison de performances et cas d'usage](/25-formats-binaires/05-comparaison-performances.md)
