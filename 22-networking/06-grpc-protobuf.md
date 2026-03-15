🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 22.6 — gRPC et Protocol Buffers : RPC moderne haute performance 🔥

## Chapitre 22 : Networking et Communication

---

## Introduction

Les sections précédentes ont couvert la communication réseau à différents niveaux d'abstraction : sockets POSIX bruts, multiplexage I/O, Asio pour le transport asynchrone, et clients HTTP pour les API REST. Toutes ces approches partagent un point commun — c'est au développeur de définir le protocole applicatif : quel format de message utiliser, comment sérialiser les données, comment gérer le versioning, comment documenter l'interface entre le client et le serveur.

**gRPC** élimine cette charge. C'est un framework **RPC** (Remote Procedure Call) complet qui vous permet de définir vos services et leurs messages dans un fichier `.proto`, puis de **générer automatiquement** le code client et serveur dans le langage de votre choix — C++, Go, Java, Python, Rust, et une dizaine d'autres. Vous écrivez une définition d'interface, et gRPC produit le code de sérialisation, de transport, de gestion d'erreurs et de streaming.

Développé par Google et rendu open source en 2015, gRPC est devenu le standard de facto pour la communication inter-services dans les architectures microservices modernes. Kubernetes, Envoy, etcd, CockroachDB, TensorFlow Serving — l'infrastructure cloud-native repose massivement sur gRPC.

---

## Pourquoi gRPC plutôt que REST/HTTP ?

### Les limites de REST pour la communication inter-services

REST sur HTTP/JSON est le choix naturel pour les API publiques consommées par des navigateurs web et des applications mobiles. Mais pour la communication **interne** entre services backend, REST présente des limitations significatives :

**Sérialisation inefficace** — JSON est un format texte, lisible par les humains mais coûteux à parser et volumineux sur le réseau. Un message JSON peut être 5 à 10 fois plus gros que son équivalent binaire. Pour des services qui échangent des millions de messages par seconde, cette différence se traduit directement en coûts de CPU, de bande passante et de latence.

**Pas de contrat fort** — Une API REST est décrite (au mieux) par une spécification OpenAPI/Swagger. Mais ce contrat n'est pas appliqué au niveau du code : rien n'empêche le serveur de renvoyer un champ avec un type différent de celui documenté, ni le client d'envoyer un body malformé. Les erreurs de contrat se découvrent à l'exécution, pas à la compilation.

**Pas de streaming natif** — HTTP/1.1 est fondamentalement requête-réponse. Le streaming (Server-Sent Events, chunked transfer) existe mais reste un contournement. Le streaming bidirectionnel (client et serveur envoient des messages simultanément) n'est tout simplement pas possible sans WebSocket — un protocole séparé avec ses propres complexités.

**Pas de génération de code** — Chaque équipe implémente son client HTTP manuellement, avec ses propres choix de sérialisation, de gestion d'erreurs et de retries. L'interface entre deux services est un accord informel, pas un contrat technique exécutable.

### Ce que gRPC résout

gRPC adresse chacune de ces limitations :

**Sérialisation binaire avec Protocol Buffers** — Les messages sont sérialisés en format binaire compact et typé. Le parsing est d'un ordre de grandeur plus rapide que JSON. Les messages sont plus petits sur le réseau. Et le schéma est défini explicitement, avec des règles de compatibilité pour l'évolution.

**Contrat fort et généré** — Le fichier `.proto` est la source de vérité. Le compilateur `protoc` génère le code client et serveur avec des types forts. Si le serveur attend un `int32` et que le client envoie un `string`, l'erreur est détectée à la compilation, pas en production.

**Streaming natif sur HTTP/2** — gRPC utilise HTTP/2 comme transport, ce qui offre nativement le multiplexage de requêtes, la compression des headers, et le streaming bidirectionnel. Un serveur peut envoyer un flux continu de résultats pendant que le client continue d'envoyer des requêtes — dans une seule connexion TCP.

**Multi-langage par conception** — Un service gRPC défini en `.proto` peut être implémenté en C++ et consommé par un client Go, Python ou Java. Le code généré dans chaque langage est idiomatique et type-safe. C'est particulièrement précieux dans les architectures polyglotes.

---

## L'architecture de gRPC

### Vue d'ensemble

```
Développeur                    Build                        Runtime
───────────                    ─────                        ───────

                              protoc
  service.proto  ──────────►  (compilateur   ──────►  Code C++ généré
  (définition       │          protobuf)          │     ├── service.pb.h/.cc
   d'interface)     │                             │     │   (messages)
                    │         grpc_cpp_plugin     │     └── service.grpc.pb.h/.cc
                    └────────► (plugin gRPC) ─────┘        (stubs client/serveur)
                                                                │
                                                    ┌───────────┴───────────┐
                                                    ▼                       ▼
                                               Serveur C++             Client C++
                                               (implémenter            (appeler les
                                                les méthodes)           méthodes)
                                                    │                       │
                                                    └───────── HTTP/2 ──────┘
                                                        Protocol Buffers
                                                        (sérialisation binaire)
```

### Les trois couches

**Protocol Buffers (Protobuf)** — Le langage de définition d'interface (IDL) et le format de sérialisation. Vous définissez vos structures de données (`message`) et vos services (`service`) dans des fichiers `.proto`. Le compilateur `protoc` génère le code de sérialisation/désérialisation pour chaque langage cible. Protobuf est utilisable indépendamment de gRPC — c'est aussi un excellent format de sérialisation autonome (couvert en détail au chapitre 25).

**gRPC framework** — La couche de transport et de gestion RPC. Elle prend en charge la connexion HTTP/2, le multiplexage, la gestion des timeouts (deadlines), la propagation de métadonnées, l'interception (interceptors), le load balancing côté client, et le health checking. En C++, gRPC peut fonctionner en mode synchrone (un thread par appel) ou asynchrone (intégré avec une boucle événementielle).

**HTTP/2** — Le protocole de transport sous-jacent. gRPC exploite les fonctionnalités d'HTTP/2 que HTTP/1.1 n'offre pas : multiplexage de streams (plusieurs RPC en parallèle sur une seule connexion TCP), compression des headers (HPACK), contrôle de flux par stream, et push serveur.

---

## Protocol Buffers : le langage de définition

### Syntaxe de base d'un fichier `.proto`

```protobuf
// greeter.proto
syntax = "proto3";

package greeter;

// Définition d'un message (structure de données)
message HelloRequest {
    string name = 1;           // Champ string, numéro de champ 1
    int32 greeting_count = 2;  // Champ entier, numéro de champ 2
}

message HelloReply {
    string message = 1;
    int64 timestamp = 2;
}

// Définition d'un service (interface RPC)
service Greeter {
    // RPC unaire : une requête, une réponse
    rpc SayHello (HelloRequest) returns (HelloReply);
}
```

### Le système de types Protobuf

Les types Protobuf correspondent directement à des types C++ :

| Type Protobuf | Type C++ | Description |
|---------------|----------|-------------|
| `double` | `double` | Flottant 64 bits |
| `float` | `float` | Flottant 32 bits |
| `int32` | `int32_t` | Entier signé, encodage variable |
| `int64` | `int64_t` | Entier signé 64 bits, encodage variable |
| `uint32` | `uint32_t` | Entier non signé |
| `uint64` | `uint64_t` | Entier non signé 64 bits |
| `sint32` | `int32_t` | Entier signé, optimisé pour les valeurs négatives |
| `bool` | `bool` | Booléen |
| `string` | `std::string` | Chaîne UTF-8 |
| `bytes` | `std::string` | Données binaires arbitraires |
| `repeated T` | `RepeatedField<T>` / `vector`-like | Liste de T |
| `map<K,V>` | `Map<K,V>` / `map`-like | Table associative |
| `MessageType` | `MessageType` | Message imbriqué |

### Numéros de champs et compatibilité

Les numéros de champs (`= 1`, `= 2`, etc.) sont l'identifiant binaire de chaque champ dans le format sérialisé. Ils sont **fondamentaux** pour la compatibilité :

- Les numéros de 1 à 15 sont encodés sur 1 octet — utilisez-les pour les champs les plus fréquents.
- Les numéros de 16 à 2047 sont encodés sur 2 octets.
- **Ne réutilisez jamais** un numéro de champ supprimé. Si vous retirez un champ, réservez son numéro avec `reserved`.
- Vous pouvez **ajouter** de nouveaux champs à tout moment (avec de nouveaux numéros) sans casser la compatibilité. Les anciens clients ignoreront les champs inconnus, les nouveaux clients utiliseront les valeurs par défaut pour les champs absents.

```protobuf
message User {
    string name = 1;
    string email = 2;
    // Champ 3 supprimé (anciennement "phone")
    reserved 3;
    reserved "phone";
    int32 age = 4;       // Ajouté plus tard — compatible
}
```

Ce mécanisme de compatibilité avant/arrière est un avantage majeur de Protobuf : vous pouvez faire évoluer vos messages sans coordination synchrone entre tous les clients et serveurs — une nécessité dans les architectures microservices où les déploiements sont indépendants.

---

## Les quatre patterns de communication gRPC

gRPC supporte quatre patterns RPC, du plus simple au plus flexible :

### 1. Unaire (Unary RPC)

Le pattern classique requête-réponse — une requête, une réponse. C'est l'équivalent d'un appel de fonction distant :

```protobuf
rpc GetUser (GetUserRequest) returns (User);
```

```
Client                  Serveur
──────                  ───────
  ── GetUserRequest ──►
  ◄── User ────────────
```

C'est le pattern le plus courant, adapté à la majorité des opérations CRUD.

### 2. Streaming serveur (Server Streaming)

Le client envoie une requête, le serveur répond avec un flux de messages. Le client lit les messages un par un jusqu'à ce que le serveur signale la fin :

```protobuf
rpc ListUsers (ListUsersRequest) returns (stream User);
```

```
Client                  Serveur
──────                  ───────
  ── ListUsersRequest ─►
  ◄── User (Alice) ─────
  ◄── User (Bob) ───────
  ◄── User (Charlie) ──
  ◄── (fin du stream) ──
```

Cas d'usage : résultats paginés, flux de logs, flux de métriques, notifications push.

### 3. Streaming client (Client Streaming)

Le client envoie un flux de messages, le serveur répond avec un seul message quand le client a terminé :

```protobuf
rpc UploadMetrics (stream Metric) returns (UploadSummary);
```

```
Client                  Serveur
──────                  ───────
  ── Metric (cpu) ─────►
  ── Metric (mem) ─────►
  ── Metric (disk) ────►
  ── (fin du stream) ──►
  ◄── UploadSummary ────
```

Cas d'usage : upload de fichiers par morceaux, envoi batch de métriques, agrégation de données.

### 4. Streaming bidirectionnel (Bidirectional Streaming)

Les deux côtés envoient des flux de messages simultanément et indépendamment. Chaque côté peut lire et écrire dans l'ordre qu'il souhaite :

```protobuf
rpc Chat (stream ChatMessage) returns (stream ChatMessage);
```

```
Client                  Serveur
──────                  ───────
  ── Message ──────────►
  ◄── Message ──────────
  ── Message ──────────►
  ── Message ──────────►
  ◄── Message ──────────
  ◄── Message ──────────
  ── Message ──────────►
  ...
```

Cas d'usage : chat temps réel, jeux multijoueurs, synchronisation bidirectionnelle, protocoles interactifs.

### Déclaration dans le `.proto`

```protobuf
service DataService {
    // Unaire
    rpc GetItem (GetItemRequest) returns (Item);

    // Streaming serveur
    rpc WatchItems (WatchRequest) returns (stream Item);

    // Streaming client
    rpc UploadItems (stream Item) returns (UploadSummary);

    // Bidirectionnel
    rpc SyncItems (stream SyncMessage) returns (stream SyncMessage);
}
```

Le mot-clé `stream` devant le type de requête ou de réponse active le streaming pour ce côté. Les combinaisons `stream`/non-`stream` définissent les quatre patterns.

---

## gRPC vs REST vs sockets custom : positionnement

Pour situer gRPC par rapport aux autres approches couvertes dans ce chapitre :

```
                    Sockets         REST/HTTP          gRPC
                    custom          + JSON
────────────────    ───────────     ──────────────     ──────────────
Niveau              Transport       Application        Application  
abstraction         (bas)           (moyen)            (haut)  

Format données      Libre           JSON (texte)       Protobuf (binaire)

Contrat             Aucun           OpenAPI (optionnel) .proto (obligatoire)
                    (implicite)     (non enforced)     (enforced à la compilation)

Génération code     Non             Optionnelle        Automatique
                                    (OpenAPI codegen)  (protoc)

Streaming           Manuel          Limité             Natif (4 patterns)

Performance         Maximale        Bonne              Très bonne
                    (contrôle       (overhead JSON)    (Protobuf + HTTP/2)
                     total)

Interopérabilité    Nulle           Universelle        Multi-langage
                    (protocole      (tout parle HTTP)  (code généré par
                     propriétaire)                      langage)

Debugging           Difficile       Facile (curl,      Moyen (grpcurl,
                    (hexdump)       navigateur)        outils dédiés)

Cas d'usage         Protocoles      API publiques,     Communication
                    custom,         web, mobile        inter-services,
                    performance                        microservices,
                    extrême                             streaming
```

### Quand choisir gRPC

- **Communication inter-services** — C'est le cas d'usage principal. Deux services backend qui se parlent bénéficient du contrat fort, de la performance, et de la génération de code multi-langage.
- **Streaming de données** — Flux de métriques, événements temps réel, synchronisation continue. Les quatre patterns de streaming couvrent tous les cas.
- **Performance critique** — Quand la latence et le throughput de la sérialisation comptent (trading, gaming, ML serving).
- **Architecture polyglotte** — Un fichier `.proto` génère du code compatible dans tous les langages supportés, garantissant la cohérence de l'interface.

### Quand ne PAS choisir gRPC

- **API publiques consommées par des navigateurs** — Les navigateurs ne supportent pas gRPC nativement (gRPC-Web existe mais ajoute de la complexité). REST/JSON reste le standard pour les API web.
- **Intégration avec des systèmes legacy** — Si vos partenaires s'attendent à du REST/JSON, forcer gRPC crée de la friction inutile.
- **Debugging simple** — Un `curl` suffit pour tester une API REST. Tester un service gRPC nécessite des outils spécifiques (`grpcurl`, `grpcui`, `evans`).
- **Prototypage rapide** — Définir un `.proto`, configurer le build, générer le code — c'est un overhead non négligeable pour un prototype. Un serveur cpp-httplib avec du JSON est plus rapide à mettre en place.

---

## L'outillage gRPC

### Outils de développement et diagnostic

L'écosystème gRPC dispose d'outils spécialisés qui compensent l'absence de la simplicité de `curl` :

**`grpcurl`** — L'équivalent de `curl` pour gRPC. Permet d'envoyer des requêtes gRPC depuis la ligne de commande, avec discovery automatique des services via la **reflection** gRPC :

```bash
# Lister les services disponibles
grpcurl -plaintext localhost:50051 list

# Décrire un service
grpcurl -plaintext localhost:50051 describe greeter.Greeter

# Appeler une méthode
grpcurl -plaintext -d '{"name": "Alice"}' \
    localhost:50051 greeter.Greeter/SayHello
```

**`grpcui`** — Une interface web interactive pour explorer et tester les services gRPC, similaire à Swagger UI pour REST.

**`evans`** — Un client gRPC interactif en terminal, avec auto-complétion et mode REPL.

### Observabilité

gRPC s'intègre nativement avec les standards d'observabilité :

- **OpenTelemetry** (chapitre 40) — Tracing distribué automatique des appels gRPC entre services.
- **Prometheus** — Métriques de latence, throughput et erreurs par méthode RPC.
- **Health checking** — Protocole standard `grpc.health.v1.Health` pour les checks Kubernetes.

---

## Le workflow de développement gRPC

Le cycle de développement gRPC suit un pattern discipliné :

```
1. Définir l'interface
   └─► Écrire le fichier .proto (messages + services)

2. Générer le code
   └─► protoc + grpc_cpp_plugin → headers et sources C++

3. Implémenter le serveur
   └─► Hériter de la classe de service générée
       Implémenter chaque méthode RPC

4. Implémenter le client
   └─► Utiliser le stub client généré
       Appeler les méthodes comme des fonctions locales

5. Tester
   └─► Tests unitaires avec mocks générés
       Tests d'intégration avec grpcurl

6. Déployer
   └─► Conteneur Docker avec le binaire serveur
       Service Kubernetes avec health checks gRPC
```

L'étape 1 est la plus importante : le fichier `.proto` est le **contrat** entre le client et le serveur. Il est versionné, reviewé, et évolue avec les mêmes précautions qu'une API publique. Le reste est en grande partie automatisé par la génération de code.

---

## Plan de la section

Les quatre sous-sections qui suivent vous guident à travers chaque étape :

| Sous-section | Contenu |
|--------------|---------|
| **22.6.1** — Installation et configuration gRPC | Installation de `protoc`, du plugin C++, et configuration CMake complète pour un projet gRPC. |
| **22.6.2** — Définition de services avec `.proto` | Syntaxe Protobuf avancée, types composés, enums, oneof, options de service, bonnes pratiques de design d'API. |
| **22.6.3** — Génération de code et implémentation | Compilation `.proto` → C++, implémentation d'un serveur et d'un client complets, API synchrone et asynchrone. |
| **22.6.4** — Streaming bidirectionnel | Implémentation des quatre patterns de streaming, gestion du backpressure, cas d'usage avancés. |

---

## Prérequis

Avant d'aborder les sous-sections, assurez-vous d'être à l'aise avec :

- **CMake** (chapitre 26) — La configuration du build gRPC est plus complexe que la moyenne. `find_package`, `FetchContent` et les commandes custom de CMake seront utilisés.
- **Héritage et polymorphisme** (chapitre 7) — Le serveur gRPC fonctionne par héritage d'une classe de base générée.
- **Sérialisation** (chapitre 25) — Les concepts de Protocol Buffers sont couverts en profondeur au chapitre 25. Cette section se concentre sur l'aspect RPC.
- **Docker** (chapitre 37) — Les exemples de déploiement utilisent des conteneurs.

---

> **Prochaine étape** → Section 22.6.1 : Installation et configuration gRPC — mise en place de la toolchain de compilation Protobuf et gRPC sur Ubuntu, avec une configuration CMake prête pour la production.

⏭️ [Installation et configuration gRPC](/22-networking/06.1-installation-grpc.md)
