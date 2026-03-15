🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 25.1 — Protocol Buffers (Protobuf) : Sérialisation Google ⭐

## Section du Module 8 — Parsing et Formats de Données

---

## Le standard de facto de la sérialisation structurée

Protocol Buffers — universellement abrégé **Protobuf** — est un mécanisme de sérialisation de données structurées développé par Google. Conçu en 2001 pour remplacer un format ad-hoc interne, il est devenu au fil des ans le système de sérialisation le plus utilisé à l'échelle industrielle. Google l'utilise pour la quasi-totalité de ses communications inter-services, et son adoption en dehors de Google a explosé avec l'émergence de gRPC en 2015.

En 2026, Protobuf est le format de sérialisation binaire par défaut dans les contextes suivants :

- **gRPC** — le framework RPC de Google utilise Protobuf comme format de sérialisation natif (cf. section 22.6).
- **Kubernetes et l'écosystème Cloud Native** — etcd (le store de configuration de Kubernetes) utilise Protobuf pour son stockage interne, et l'API Kubernetes elle-même supporte Protobuf en alternative à JSON.
- **Big Data et streaming** — Apache Kafka, Apache Pulsar et Google Pub/Sub supportent Protobuf nativement via des schema registries.
- **Stockage structuré** — Google Bigtable, Spanner, et de nombreux systèmes de stockage utilisent Protobuf pour sérialiser les enregistrements.
- **Machine Learning** — TensorFlow utilise Protobuf pour ses graphes de modèles (`.pb`), ses configurations et ses formats d'échange.

---

## Principe de fonctionnement

Le workflow Protobuf repose sur un contrat de données — le fichier `.proto` — qui décrit la structure des messages échangés. Un compilateur dédié (`protoc`) transforme cette description en code source dans le langage cible. Le code généré fournit des classes typées avec des méthodes de sérialisation et de désérialisation intégrées.

```
┌───────────────┐                    ┌─────────────────────┐
│  message.proto│     protoc         │  message.pb.h       │
│               │ ─────────────────► │  message.pb.cc      │
│  Schéma IDL   │   --cpp_out=.      │                     │
│  (langage-    │                    │  Classes C++ avec   │
│   neutre)     │                    │  Serialize() /      │
└───────────────┘                    │  ParseFromString()  │
                                     └──────────┬──────────┘
                                                │
                                         compilation g++
                                                │
                                                ▼
                                     ┌─────────────────────┐
                                     │   Application C++   │
                                     │                     │
                                     │   MyMessage msg;    │
                                     │   msg.set_name(..); │
                                     │   msg.SerializeTo.. │
                                     └─────────────────────┘
```

Ce modèle apporte plusieurs avantages fondamentaux par rapport aux formats textuels :

**Typage garanti à la compilation.** Le code généré par `protoc` expose des accesseurs typés (`msg.port()` retourne un `int32`, pas un variant ou un nœud générique). Les erreurs de type sont détectées par le compilateur C++, pas à l'exécution.

**Sérialisation optimisée.** Le code de sérialisation est généré spécifiquement pour chaque type de message, sans introspection ni dispatch dynamique. Le format binaire est conçu pour être encodé et décodé en un minimum de cycles CPU.

**Indépendance du langage.** Le fichier `.proto` est un IDL (Interface Definition Language) neutre. Le même schéma génère du code C++, Python, Go, Java, Rust, C#, et une douzaine d'autres langages. Un service C++ et un client Python partagent le même fichier `.proto` comme contrat.

**Rétrocompatibilité intégrée.** Protobuf a été conçu dès l'origine pour supporter l'évolution des schémas. Des champs peuvent être ajoutés ou retirés sans casser les consommateurs existants, à condition de respecter quelques règles simples. Dans un système distribué où les services sont déployés indépendamment, cette propriété est critique.

---

## Format de sérialisation

Le format binaire Protobuf utilise un encodage compact basé sur des paires **tag-valeur**. Chaque champ est identifié par son numéro de champ (défini dans le `.proto`) et un type filaire (wire type) qui indique comment lire la valeur :

| Wire type | Encodage | Types Protobuf |
|-----------|----------|---------------|
| 0 | Varint (entier de longueur variable) | `int32`, `int64`, `uint32`, `uint64`, `bool`, `enum` |
| 1 | 64 bits fixe | `fixed64`, `sfixed64`, `double` |
| 2 | Longueur-préfixée | `string`, `bytes`, sous-messages, `repeated` packed |
| 5 | 32 bits fixe | `fixed32`, `sfixed32`, `float` |

L'encodage **varint** est l'un des mécanismes clés de la compacité de Protobuf. Un entier est encodé sur un nombre variable d'octets proportionnel à sa grandeur : la valeur `1` occupe 1 octet, `300` occupe 2 octets, et seules les très grandes valeurs nécessitent la taille maximale. Pour les messages où la plupart des entiers sont petits (identifiants, compteurs, timestamps relatifs), le gain est considérable.

Les champs absents ne sont tout simplement pas encodés dans le flux binaire. Un message avec 20 champs définis dont seuls 3 sont renseignés ne paie que pour ces 3 champs. C'est un avantage significatif sur JSON où même les valeurs `null` occupent de l'espace.

### Exemple concret de compacité

Considérons un message simple :

```protobuf
message User {
    int32 id = 1;
    string name = 2;
    string email = 3;
}
```

Avec les valeurs `id=42`, `name="alice"`, `email="alice@example.com"` :

- **JSON** : `{"id":42,"name":"alice","email":"alice@example.com"}` → **52 octets**
- **Protobuf** : encodage binaire → **~28 octets**

Le gain de 46% est typique pour les messages de petite taille. Sur des messages avec de nombreux champs numériques ou des champs absents, le ratio peut dépasser 80%.

---

## Versions de Protobuf : proto2 vs proto3

Deux versions de la syntaxe coexistent. Les nouveaux projets doivent utiliser **proto3**, la version actuelle :

| Aspect | proto2 | proto3 |
|--------|--------|--------|
| Syntaxe | `syntax = "proto2";` | `syntax = "proto3";` |
| Champs requis | `required`, `optional`, `repeated` | Tous optionnels par défaut, `repeated`, `optional` explicite |
| Valeurs par défaut | Personnalisables | Zéro-value du type (0, "", false) |
| Présence de champ | Toujours trackée | Trackée uniquement si `optional` explicite |
| Extensions | Supportées | Remplacées par `Any` |
| Maps | Non natif | Natif (`map<K, V>`) |
| JSON mapping | Limité | Natif et standardisé |

proto3 simplifie le modèle en éliminant les champs `required` (source de problèmes de rétrocompatibilité) et en adoptant des valeurs par défaut systématiques. Le mot-clé `optional` a été réintroduit en proto3 (à partir de protobuf 3.15) pour les cas où la distinction entre « champ absent » et « champ à la valeur zéro » est nécessaire.

> 💡 *Tout le code de cette section utilise la syntaxe proto3. Si le projet doit interfacer avec un système utilisant proto2, les deux syntaxes sont interopérables au niveau du format binaire — un message proto2 peut être lu par du code généré depuis proto3 et inversement.*

---

## Aperçu de l'utilisation en C++

Avant de détailler chaque étape dans les sous-sections, voici un aperçu du workflow complet pour donner une vision d'ensemble :

```protobuf
// config.proto
syntax = "proto3";  
package myapp;  

message ServerConfig {
    string host = 1;
    int32 port = 2;
    int32 workers = 3;

    message TlsConfig {
        string cert_path = 1;
        string key_path = 2;
        bool verify_client = 3;
    }

    optional TlsConfig tls = 4;
    repeated string allowed_origins = 5;
}
```

Après génération du code avec `protoc`, l'utilisation en C++ est naturelle :

```cpp
#include "config.pb.h"
#include <fstream>
#include <print>

int main() {
    // Construction d'un message
    myapp::ServerConfig config;
    config.set_host("0.0.0.0");
    config.set_port(8080);
    config.set_workers(4);

    // Sous-message
    auto* tls = config.mutable_tls();
    tls->set_cert_path("/etc/ssl/server.crt");
    tls->set_key_path("/etc/ssl/server.key");
    tls->set_verify_client(true);

    // Champ repeated
    config.add_allowed_origins("https://app.example.com");
    config.add_allowed_origins("https://admin.example.com");

    // Sérialisation binaire
    std::string binary_data;
    config.SerializeToString(&binary_data);
    std::print("Taille sérialisée : {} octets\n", binary_data.size());

    // Écriture dans un fichier
    std::ofstream file("config.pb", std::ios::binary);
    config.SerializeToOstream(&file);
    file.close();

    // Désérialisation
    myapp::ServerConfig restored;
    std::ifstream input("config.pb", std::ios::binary);
    if (restored.ParseFromIstream(&input)) {
        std::print("Host : {}\n", restored.host());
        std::print("Port : {}\n", restored.port());

        if (restored.has_tls()) {
            std::print("TLS cert : {}\n", restored.tls().cert_path());
        }

        for (const auto& origin : restored.allowed_origins()) {
            std::print("Origin : {}\n", origin);
        }
    }
}
```

Plusieurs observations sur ce code :

- Les accesseurs sont générés automatiquement : `host()`, `set_host()`, `port()`, `set_port()`, `mutable_tls()`, `add_allowed_origins()`.
- Les sous-messages sont manipulés via des pointeurs retournés par `mutable_*()`. Le message parent possède le sous-message — pas de gestion manuelle de mémoire.
- `has_tls()` teste la présence du champ `optional`. Pour les champs non-optional en proto3, cette méthode n'existe pas (le champ a toujours sa valeur par défaut).
- `SerializeToString` et `ParseFromIstream` gèrent la sérialisation/désérialisation en une seule ligne. Ils retournent un `bool` indiquant le succès.
- Les champs `repeated` exposent une interface similaire à `std::vector` avec `add_*()`, `*_size()`, et accès par index.

---

## Avantages et limites

### Avantages

**Performance.** L'encodage/décodage Protobuf est parmi les plus rapides des formats de sérialisation. Le code généré est optimisé spécifiquement pour chaque type de message, sans overhead d'introspection.

**Rétrocompatibilité.** L'ajout de champs (avec de nouveaux numéros) est transparent pour les consommateurs existants. Les champs inconnus sont préservés lors de la désérialisation, permettant le round-trip même avec un schéma partiellement connu.

**Écosystème.** L'intégration avec gRPC, les schema registries (Confluent, Buf), et les outils de validation (`buf lint`, `buf breaking`) forme un écosystème mature pour le développement d'API.

**Multi-langage.** Un seul fichier `.proto` génère du code dans plus de 12 langages. C'est la lingua franca des systèmes distribués polyglottes.

### Limites

**Non lisible par un humain.** Le format binaire est opaque. Le débogage nécessite des outils (`protoc --decode`, `grpcurl`, extensions IDE). Ce n'est pas un format de configuration.

**Étape de génération de code.** Le compilateur `protoc` doit être invoqué avant la compilation C++. Cela ajoute de la complexité au build system et crée une dépendance à l'outil.

**Taille de la dépendance.** La librairie runtime `libprotobuf` est substantielle (~2 Mo compilée). Pour les applications embarquées ou les contextes où la taille du binaire est critique, Protobuf Lite (`libprotobuf-lite`) offre un runtime réduit au prix de quelques fonctionnalités (pas de réflexion, pas de conversion JSON).

**Pas d'accès aléatoire.** Le format est séquentiel — pour lire le dernier champ, il faut parcourir tous les précédents. FlatBuffers (section 25.2) résout ce problème avec son approche zero-copy.

**Pas de schéma auto-descriptif.** Contrairement à JSON où les noms de clés sont dans le flux, un message Protobuf ne contient que des numéros de champs. Sans le fichier `.proto` ou un descriptor set, le contenu est indéchiffrable. Des solutions existent (self-describing messages, descriptor sets), mais elles ajoutent de la complexité.

---

## Installation

### Conan 2

```python
# conanfile.py
def requirements(self):
    self.requires("protobuf/5.27.0")
```

```cmake
find_package(Protobuf REQUIRED)
```

### vcpkg

```bash
vcpkg install protobuf
```

### apt (Ubuntu)

```bash
sudo apt install protobuf-compiler libprotobuf-dev
```

Vérification de l'installation :

```bash
protoc --version
# libprotoc 27.x
```

### Vérification minimale

```bash
# Créer un fichier test.proto minimal
echo 'syntax = "proto3"; message Test { string name = 1; }' > test.proto

# Générer le code C++
protoc --cpp_out=. test.proto

# Vérifier les fichiers générés
ls test.pb.h test.pb.cc
```

---

## Intégration CMake

L'intégration de Protobuf dans CMake nécessite de déclarer les fichiers `.proto` et d'invoquer `protoc` automatiquement lors du build. CMake fournit un module natif `FindProtobuf` avec la fonction `protobuf_generate_cpp` :

```cmake
cmake_minimum_required(VERSION 3.20)  
project(myapp LANGUAGES CXX)  

set(CMAKE_CXX_STANDARD 23)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  

find_package(Protobuf REQUIRED)

# Génération automatique du code C++ depuis les .proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    proto/config.proto
    proto/user.proto
)

add_executable(myapp
    src/main.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

target_include_directories(myapp PRIVATE ${CMAKE_CURRENT_BINARY_DIR})  
target_link_libraries(myapp PRIVATE protobuf::libprotobuf)  
```

Le `target_include_directories` pointant vers `CMAKE_CURRENT_BINARY_DIR` est nécessaire car les fichiers `.pb.h` sont générés dans le répertoire de build, pas dans l'arborescence source.

Pour les projets plus complexes avec des fichiers `.proto` dans des sous-répertoires ou des imports entre fichiers `.proto`, la fonction `protobuf_generate` (CMake 3.13+) offre plus de contrôle :

```cmake
add_library(proto_lib ${PROTO_SRCS} ${PROTO_HDRS})  
target_include_directories(proto_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})  
target_link_libraries(proto_lib PUBLIC protobuf::libprotobuf)  

# L'application linke contre la librairie proto
target_link_libraries(myapp PRIVATE proto_lib)
```

Isoler le code Protobuf généré dans une librairie dédiée est une bonne pratique : cela évite de recompiler le code généré quand seul le code applicatif change, et clarifie les dépendances.

---

## Alternatives dans l'écosystème Protobuf

### Buf

**Buf** est un outil moderne qui remplace et étend `protoc`. Il fournit un linter pour les fichiers `.proto` (`buf lint`), un détecteur de breaking changes (`buf breaking`), et un gestionnaire de dépendances pour les schémas (`buf.build` registry). Pour les projets professionnels utilisant Protobuf à grande échelle, Buf est devenu l'outil de référence.

### protobuf-c et nanopb

Pour les environnements contraints (embarqué, IoT), **nanopb** génère du code C pur avec une empreinte mémoire minimale (quelques Ko de runtime). C'est l'alternative à Protobuf standard quand les ressources sont très limitées.

### Cap'n Proto

Créé par Kenton Varda (l'un des auteurs originaux de Protobuf chez Google), **Cap'n Proto** est un format qui combine les avantages de Protobuf (schéma, rétrocompatibilité) et de FlatBuffers (zero-copy). Il mérite considération pour les nouveaux projets, bien que son écosystème soit plus restreint que celui de Protobuf.

---

## Ce que couvrent les sous-sections

- **25.1.1 — Définition de messages `.proto`** : syntaxe proto3, types scalaires, messages imbriqués, enums, oneof, maps, imports, packages et conventions de nommage.
- **25.1.2 — Génération de code C++** : invocation de `protoc`, intégration CMake avancée, structure du code généré, API des classes de messages.
- **25.1.3 — Sérialisation / Désérialisation** : encodage binaire, encodage JSON, streaming, gestion des erreurs, évolution de schéma et rétrocompatibilité.

---

## Prérequis

- Concepts de sérialisation du chapitre 24 (parsing, validation, conversion de types).
- CMake : `add_library`, `target_link_libraries`, `target_include_directories` *(chapitre 26)*.
- Move semantics *(chapitre 10)* — les classes générées supportent le move.
- gRPC *(section 22.6)* — recommandé mais pas obligatoire. Protobuf est utilisable indépendamment de gRPC.

⏭️ [Définition de messages .proto](/25-formats-binaires/01.1-definition-proto.md)
