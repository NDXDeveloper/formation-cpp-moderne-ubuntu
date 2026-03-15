# Exemples — Chapitre 25 : Formats binaires

## Fichiers de schéma

| Fichier | Format | Description |
|---------|--------|-------------|
| `config.proto` | Protobuf | ServerConfig avec TlsConfig, optional, repeated |
| `definition.proto` | Protobuf | Types variés : Cluster, DeploymentSpec, NotificationTarget, ServiceRegistry |
| `user.proto` | Protobuf | User avec Role enum, optional, repeated, tags |
| `schema.fbs` | FlatBuffers | User et UserList avec enum Role, vecteurs, scalaires |

## Fichiers d'exemples

### Protobuf (sections 25.1.x)

| Fichier | Section | Description | Fichier source | Compilation |
|---------|---------|-------------|----------------|-------------|
| `01_protobuf_apercu.cpp` | 25.1 | Aperçu Protobuf — construction de message, sous-message (`mutable_tls`), champ repeated, sérialisation binaire, écriture/lecture fichier, désérialisation | `01-protobuf.md` | `protoc --cpp_out=. config.proto && g++-15 -std=c++23 -O2 -o 01_protobuf_apercu 01_protobuf_apercu.cpp config.pb.cc -lprotobuf` |
| `01_1_definition_proto.cpp` | 25.1.1 | Définitions `.proto` — `optional` vs non-optional (`has_workers`), `repeated` (Cluster), messages imbriqués (DeploymentSpec, ResourceLimits, HealthCheck), `oneof` (NotificationTarget), `map` (ServiceRegistry) | `01.1-definition-proto.md` | `protoc --cpp_out=. definition.proto && g++-15 -std=c++23 -O2 -o 01_1_definition_proto 01_1_definition_proto.cpp definition.pb.cc -lprotobuf` |
| `01_2_generation_code.cpp` | 25.1.2 | Code généré — construction et `DebugString`, désérialisation et lecture, copie/move/swap, `MergeFrom` (fusion de messages), conversion JSON bidirectionnelle (`MessageToJsonString`/`JsonStringToMessage`), Arenas (`Arena::CreateMessage`) | `01.2-generation-code.md` | `protoc --cpp_out=. user.proto && g++-15 -std=c++23 -O2 -o 01_2_generation_code 01_2_generation_code.cpp user.pb.cc -lprotobuf` |
| `01_3_serialisation.cpp` | 25.1.3 | Sérialisation complète — vers chaîne/fichier/buffer, sérialisation partielle, désérialisation depuis chaîne/fichier/buffer, parsing sécurisé (`CodedInputStream`, limites de taille et récursion), streaming delimited (`SerializeDelimitedToZeroCopyStream`), framing manuel (big-endian 4 octets), Text Format, JSON interop | `01.3-serialisation.md` | `protoc --cpp_out=. config.proto user.proto && g++-15 -std=c++23 -O2 -o 01_3_serialisation 01_3_serialisation.cpp config.pb.cc user.pb.cc -lprotobuf` |

### FlatBuffers (section 25.2)

| Fichier | Section | Description | Fichier source | Compilation |
|---------|---------|-------------|----------------|-------------|
| `02_flatbuffers.cpp` | 25.2 | FlatBuffers complet — construction (`FlatBufferBuilder`, offsets, ordre inversé), Builder pattern (`UserBuilder`), lecture zero-copy (`GetUserList`, accesseurs), vérification de buffer (`Verifier`), Object API (`UserT`, `Pack`/`UnPack`), écriture/lecture fichier | `02-flatbuffers.md` | `flatc --cpp --gen-object-api -o . schema.fbs && g++-15 -std=c++23 -O2 -o 02_flatbuffers 02_flatbuffers.cpp -lflatbuffers` |

### MessagePack (section 25.3)

| Fichier | Section | Description | Fichier source | Compilation |
|---------|---------|-------------|----------------|-------------|
| `03_messagepack.cpp` | 25.3 | MessagePack complet — sérialisation primitifs/vector/maps, types utilisateur (`MSGPACK_DEFINE`, `MSGPACK_DEFINE_MAP`), inspection dynamique (`msgpack::object`), streaming (`unpacker`), gestion erreurs (`type_error`, `unpack_error`), limites de sécurité (`unpack_limit`), réutilisation `sbuffer` | `03-messagepack.md` | `g++-15 -std=c++23 -O2 -o 03_messagepack 03_messagepack.cpp` |
| `03_1_msgpack_json.cpp` | 25.3 | Conversion MessagePack ↔ JSON — convertisseur récursif `msgpack_to_json` couvrant tous les types MessagePack (nil, bool, int, float, string, array, map) | `03-messagepack.md` | `g++-15 -std=c++23 -O2 -o 03_1_msgpack_json 03_1_msgpack_json.cpp` |

## Comportement attendu

Chaque programme affiche les résultats de ses tests et termine par `Tous les tests passent !`.

### `01_protobuf_apercu` — Sortie attendue
```
Taille sérialisée : 112 octets  
Host : 0.0.0.0  
Port : 8080  
TLS cert : /etc/ssl/server.crt  
Origin : https://app.example.com  
Origin : https://admin.example.com  

Tous les tests passent !
```

### `01_1_definition_proto` — Sortie attendue (extrait)
```
=== optional ===
port_set=false, workers_set=true, workers_cleared=false

=== repeated ===
count=3, first=10.0.1.1  
Node : 10.0.1.1  

=== oneof ===
Slack : #deployments

=== maps ===
worker → 10.0.2.1:9090  
api → 10.0.1.1:8080  
```

### `01_2_generation_code` — Sortie attendue (extrait)
```
=== Construction et sérialisation ===
Taille binaire : 67 octets  
Debug :  
id: 42  
name: "Alice Martin"  
role: ROLE_ADMIN  

=== MergeFrom ===
name=Alice, role=ROLE_ADMIN  
tags: team-a on-call  

=== Conversion JSON ===
Depuis JSON : Bob (ROLE_USER)

=== Arenas ===
Arena user: id=42, name=Alice
```

### `01_3_serialisation` — Sortie attendue (extrait)
```
=== Sérialisation vers chaîne ===
Sérialisé : 39 octets

=== Streaming delimited ===
  id=0, name=User0
  id=1, name=User1
  id=2, name=User2
Lu 3 messages, clean_eof=true

=== Framing manuel ===
  id=1, name=User1
  id=2, name=User2
  id=3, name=User3

=== Text Format ===
host: "0.0.0.0"  
port: 8080  
workers: 4  
```

### `02_flatbuffers` — Sortie attendue (extrait)
```
=== Construction de messages ===
Buffer : 180 octets

=== Lecture zero-copy ===
ID    : 42  
Name  : Alice Martin  
Score : 98.5  
Tag   : senior  
Tag   : on-call  

=== Vérification buffer ===
Buffer vérifié OK  
Buffer corrompu détecté OK  

=== Object API ===
Name : Alice Martin
```

### `03_messagepack` — Sortie attendue (extrait)
```
=== Types primitifs ===
Sérialisé : 17 octets  
Value : 42  

=== ServerConfig (MSGPACK_DEFINE) ===
0.0.0.0:8080 (4 workers)
  GET /health (1000ms)
  POST /api/data (5000ms)

=== Streaming ===
Message type=5 : event_a  
Message type=2 : 42  
Message type=8 : (map, 2 entrées)  

=== Gestion erreurs ===
type_error : std::bad_cast  
unpack_error : ...  
```

### `03_1_msgpack_json` — Sortie attendue
```
=== MessagePack → JSON ===
{
  "active": true,
  "age": 30,
  "name": "alice",
  "scores": [
    95,
    87,
    72
  ]
}

Tous les tests passent !
```

## Prérequis

- **Compilateur** : g++-15 (ou compatible C++23 avec `<print>`)
- **protobuf** : `sudo apt install protobuf-compiler libprotobuf-dev` (compilateur `protoc` + librairie, linker avec `-lprotobuf`)
- **flatbuffers** : `sudo apt install flatbuffers-compiler libflatbuffers-dev` (compilateur `flatc` + headers, linker avec `-lflatbuffers`)
- **msgpack-cxx** : `sudo apt install libmsgpack-cxx-dev` (header-only, pas de link)
- **nlohmann/json** : `sudo apt install nlohmann-json3-dev` (pour `03_1_msgpack_json.cpp`)

## Compilation complète

```bash
# 1. Générer le code Protobuf et FlatBuffers
protoc --cpp_out=. config.proto  
protoc --cpp_out=. definition.proto  
protoc --cpp_out=. user.proto  
flatc --cpp --gen-object-api -o . schema.fbs  

# 2. Compiler tous les exemples
g++-15 -std=c++23 -O2 -o 01_protobuf_apercu 01_protobuf_apercu.cpp config.pb.cc -lprotobuf  
g++-15 -std=c++23 -O2 -o 01_1_definition_proto 01_1_definition_proto.cpp definition.pb.cc -lprotobuf  
g++-15 -std=c++23 -O2 -o 01_2_generation_code 01_2_generation_code.cpp user.pb.cc -lprotobuf  
g++-15 -std=c++23 -O2 -o 01_3_serialisation 01_3_serialisation.cpp config.pb.cc user.pb.cc -lprotobuf  
g++-15 -std=c++23 -O2 -o 02_flatbuffers 02_flatbuffers.cpp -lflatbuffers  
g++-15 -std=c++23 -O2 -o 03_messagepack 03_messagepack.cpp  
g++-15 -std=c++23 -O2 -o 03_1_msgpack_json 03_1_msgpack_json.cpp  

# 3. Nettoyage des fichiers générés et binaires
rm -f *.pb.h *.pb.cc schema_generated.h  
rm -f 01_protobuf_apercu 01_1_definition_proto 01_2_generation_code  
rm -f 01_3_serialisation 02_flatbuffers 03_messagepack 03_1_msgpack_json  
```
