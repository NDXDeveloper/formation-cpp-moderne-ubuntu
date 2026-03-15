🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 25.2 — FlatBuffers : Zéro-copy serialization

## Section du Module 8 — Parsing et Formats de Données

---

## L'approche radicale : ne pas désérialiser

Protobuf, nlohmann/json, yaml-cpp — toutes les librairies vues jusqu'ici suivent le même modèle : les données entrantes sont **parsées** et **copiées** dans des structures en mémoire que le programme manipule ensuite. Ce modèle est simple à comprendre, mais il implique un coût incompressible : allocation mémoire, copie des données, construction d'objets.

FlatBuffers, créé par Wouter van Oortmerssen chez Google en 2014, élimine ce coût en proposant une idée fondamentalement différente : **les données sérialisées sont directement accessibles dans le buffer, sans phase de désérialisation**. Le programme lit les champs directement dans le buffer binaire reçu du réseau ou chargé depuis le disque. Il n'y a pas de copie, pas d'allocation, pas de construction d'objets — le buffer *est* la structure de données.

```
            Protobuf / JSON                         FlatBuffers
    ┌──────────┐  parse/copy  ┌──────────┐   ┌──────────────────────────┐
    │  Buffer  │ ──────────►  │ Objets   │   │  Buffer                  │
    │  réseau  │   (alloc,    │ en       │   │  réseau                  │
    │  binaire │    copie)    │ mémoire  │   │  = structure de données  │
    └──────────┘              └──────────┘   │  (lecture directe)       │
                                             └──────────────────────────┘
                   Désérialisation                    Zéro-copy
                   O(n) temps + mémoire               O(1) temps
```

Cette propriété rend FlatBuffers particulièrement adapté aux contextes où la latence de désérialisation est critique : boucles de rendu de jeux vidéo, systèmes de trading haute fréquence, traitement de flux IoT, et tout scénario où des millions de messages sont consommés par seconde.

---

## Principe de fonctionnement

### Le format binaire

Un buffer FlatBuffers est un bloc d'octets contigu contenant les données dans un format navigable par offsets. Chaque table (l'équivalent d'un message Protobuf) commence par un **vtable** (virtual table) qui indique les offsets de chaque champ dans le buffer. Lire un champ revient à suivre un offset depuis la vtable — une opération à coût constant, indépendante de la taille du message.

```
Buffer FlatBuffers (simplifié) :
┌─────────┬──────────┬───────────────────────────┐
│ vtable  │ offsets  │ données inline            │
│ (taille │ champ 1  │ "alice" | 42 | 98.5 | ... │
│  champs)│ champ 2  │                           │
│         │ champ 3  │                           │
└─────────┴──────────┴───────────────────────────┘
         ▲                    ▲
         │     offset         │
         └────────────────────┘
```

Les conséquences de ce design :

**Accès aléatoire O(1).** On peut lire le champ n°15 sans parcourir les champs 1 à 14. Avec Protobuf, le format est séquentiel — il faut scanner tous les champs pour atteindre un champ spécifique.

**Pas d'allocation mémoire à la lecture.** Les données scalaires sont lues directement depuis le buffer. Les chaînes sont retournées comme des pointeurs dans le buffer, pas comme des `std::string` copiées.

**Pas de phase de parsing.** L'accès au premier champ est instantané après la réception du buffer. Avec Protobuf, le message entier doit être parsé avant que le premier champ ne soit accessible.

**Le buffer doit rester en mémoire.** Les accesseurs retournent des pointeurs dans le buffer. Si le buffer est libéré, tous les accès deviennent invalides. C'est le compromis fondamental du zero-copy.

### Workflow

Le workflow est similaire à Protobuf : un fichier de schéma (`.fbs`) est compilé par `flatc` en code C++ :

```
┌───────────┐                    ┌──────────────────────┐
│ schema.fbs│     flatc          │ schema_generated.h   │
│           │ ─────────────────► │                      │
│ Schéma IDL│   --cpp            │ Accesseurs inline    │
│           │                    │ (header-only)        │
└───────────┘                    └──────────────────────┘
```

Une différence notable : le code généré par FlatBuffers est **header-only**. Il ne produit qu'un `.h`, pas de `.cc`. Les accesseurs sont des fonctions inline qui calculent des offsets — il n'y a pas de code de sérialisation/désérialisation à proprement parler.

---

## Définition de schémas `.fbs`

La syntaxe FlatBuffers ressemble à celle de Protobuf, avec quelques différences :

```fbs
// schema.fbs
namespace myapp;

enum Role : byte {
    Unknown = 0,
    Admin = 1,
    User = 2,
    Viewer = 3
}

table User {
    id: int32;
    name: string;
    email: string;
    role: Role = Unknown;
    department: string;
    tags: [string];              // vecteur de chaînes
    score: float = 0.0;
}

table UserList {
    users: [User];               // vecteur de tables
    total_count: int32;
}

root_type UserList;              // type racine du buffer
```

### Différences avec Protobuf

**`table` vs `struct`.** FlatBuffers distingue deux types composites. Une `table` est flexible (champs optionnels, évolution de schéma) — c'est l'équivalent d'un `message` Protobuf. Un `struct` est compact et fixe (pas de champ optionnel, pas d'évolution) — les données sont stockées inline sans vtable, pour une efficacité maximale.

```fbs
// struct : taille fixe, stocké inline, pas d'overhead
struct Vec3 {
    x: float;
    y: float;
    z: float;
}

// table : flexible, avec vtable pour l'évolution
table GameObject {
    position: Vec3;       // inline, pas d'indirection
    name: string;         // via offset
    health: int32 = 100;  // valeur par défaut
}
```

**Pas de numéros de champ explicites.** Les champs sont identifiés par leur position ordinale dans la déclaration. FlatBuffers assigne les identifiants automatiquement. Pour la rétrocompatibilité, les nouveaux champs doivent être ajoutés **à la fin** de la table.

**Vecteurs typés.** La syntaxe `[Type]` déclare un vecteur. Les vecteurs de scalaires sont stockés de manière contiguë en mémoire, ce qui les rend cache-friendly.

**Unions.** FlatBuffers supporte les unions (équivalent de `oneof` Protobuf) avec une syntaxe dédiée :

```fbs
union NotificationTarget {
    EmailTarget,
    SlackTarget,
    WebhookTarget
}

table EmailTarget { address: string; }  
table SlackTarget { channel: string; }  
table WebhookTarget { url: string; }  

table Notification {
    message: string;
    target: NotificationTarget;
}
```

### Types scalaires

| Type FlatBuffers | Taille | Équivalent C++ |
|-----------------|--------|----------------|
| `bool` | 1 octet | `bool` |
| `byte` / `int8` | 1 octet | `int8_t` |
| `ubyte` / `uint8` | 1 octet | `uint8_t` |
| `short` / `int16` | 2 octets | `int16_t` |
| `ushort` / `uint16` | 2 octets | `uint16_t` |
| `int` / `int32` | 4 octets | `int32_t` |
| `uint` / `uint32` | 4 octets | `uint32_t` |
| `long` / `int64` | 8 octets | `int64_t` |
| `ulong` / `uint64` | 8 octets | `uint64_t` |
| `float` / `float32` | 4 octets | `float` |
| `double` / `float64` | 8 octets | `double` |
| `string` | variable | `flatbuffers::String*` |

---

## Installation

### Conan 2

```python
def requirements(self):
    self.requires("flatbuffers/24.3.25")

def build_requirements(self):
    self.tool_requires("flatbuffers/24.3.25")
```

### vcpkg

```bash
vcpkg install flatbuffers
```

### apt (Ubuntu)

```bash
sudo apt install flatbuffers-compiler libflatbuffers-dev
```

### Vérification

```bash
flatc --version
# flatc version 24.3.25
```

---

## Génération de code et intégration CMake

### Génération manuelle

```bash
flatc --cpp --gen-object-api -o generated/ schema.fbs
```

L'option `--gen-object-api` génère en plus des accesseurs zero-copy une API "objet" qui crée des structures C++ classiques (avec copie). C'est utile pour la construction de messages et les cas où le zero-copy n'est pas nécessaire.

### Intégration CMake

```cmake
cmake_minimum_required(VERSION 3.20)  
project(myapp LANGUAGES CXX)  

set(CMAKE_CXX_STANDARD 23)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)  

find_package(flatbuffers REQUIRED)

# Commande personnalisée pour générer le code
set(FBS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/schema/schema.fbs)  
set(FBS_GENERATED ${CMAKE_CURRENT_BINARY_DIR}/schema_generated.h)  

add_custom_command(
    OUTPUT ${FBS_GENERATED}
    COMMAND flatbuffers::flatc
        --cpp
        --gen-object-api
        --scoped-enums
        -o ${CMAKE_CURRENT_BINARY_DIR}/
        ${FBS_FILES}
    DEPENDS ${FBS_FILES}
    COMMENT "Generating FlatBuffers C++ headers"
)

add_custom_target(flatbuf_generated DEPENDS ${FBS_GENERATED})

add_executable(myapp src/main.cpp)  
add_dependencies(myapp flatbuf_generated)  
target_include_directories(myapp PRIVATE ${CMAKE_CURRENT_BINARY_DIR})  
target_link_libraries(myapp PRIVATE flatbuffers::flatbuffers)  
```

L'option `--scoped-enums` génère des `enum class` C++ au lieu de `enum` classiques, ce qui est recommandé pour le code moderne.

Contrairement à Protobuf, il n'y a pas de librairie à compiler — les headers FlatBuffers suffisent. Le `target_link_libraries` fournit principalement les chemins d'inclusion vers les headers du runtime FlatBuffers (`flatbuffers/flatbuffers.h`).

---

## Construction de messages (sérialisation)

La construction d'un buffer FlatBuffers utilise un `FlatBufferBuilder` — un allocateur séquentiel qui construit le buffer de bas en haut :

```cpp
#include "schema_generated.h"
#include <flatbuffers/flatbuffers.h>
#include <print>

int main() {
    flatbuffers::FlatBufferBuilder builder(1024);  // taille initiale

    // Les chaînes et vecteurs doivent être créés AVANT la table qui les référence
    auto name = builder.CreateString("Alice Martin");
    auto email = builder.CreateString("alice@example.com");
    auto dept = builder.CreateString("Engineering");

    // Vecteur de chaînes
    std::vector<flatbuffers::Offset<flatbuffers::String>> tag_offsets;
    tag_offsets.push_back(builder.CreateString("senior"));
    tag_offsets.push_back(builder.CreateString("on-call"));
    auto tags = builder.CreateVector(tag_offsets);

    // Construction de la table User
    auto user = myapp::CreateUser(
        builder,
        42,                    // id
        name,                  // name (offset)
        email,                 // email (offset)
        myapp::Role_Admin,     // role
        dept,                  // department (offset)
        tags,                  // tags (offset vers vecteur)
        98.5f                  // score
    );

    // Construction de UserList
    std::vector<flatbuffers::Offset<myapp::User>> user_offsets;
    user_offsets.push_back(user);
    auto users_vec = builder.CreateVector(user_offsets);

    auto user_list = myapp::CreateUserList(builder, users_vec, 1);

    // Finaliser le buffer
    builder.Finish(user_list);

    // Accès au buffer résultant
    uint8_t* buf = builder.GetBufferPointer();
    size_t size = builder.GetSize();
    std::print("Buffer : {} octets\n", size);

    // Le buffer [buf, buf+size) est prêt à être envoyé ou écrit
}
```

### Contrainte d'ordre de construction

Le point le plus important et le plus déroutant pour les développeurs habitués à Protobuf : **les objets enfants doivent être créés avant les parents**. Le `FlatBufferBuilder` écrit séquentiellement dans le buffer, et chaque objet créé retourne un `Offset<T>` — un entier représentant la position de l'objet dans le buffer. Ces offsets sont ensuite passés au constructeur du parent.

Concrètement, l'ordre de construction est inversé par rapport à l'ordre logique :

```
Ordre logique : UserList → User → strings, tags  
Ordre de construction : strings, tags → User → UserList → Finish  
```

Cette contrainte est le prix du zero-copy : le buffer est construit en une seule passe, sans réallocation ni réorganisation. C'est plus contraignant qu'un `msg.set_name("Alice")` Protobuf, mais c'est ce qui permet la lecture sans désérialisation.

### Construction alternative avec le Builder pattern

Pour les tables avec de nombreux champs, la syntaxe positionnelle de `CreateUser(builder, ...)` est fragile (un argument décalé et tout est faux). FlatBuffers génère aussi un Builder pattern :

```cpp
myapp::UserBuilder user_builder(builder);  
user_builder.add_id(42);  
user_builder.add_name(name);  
user_builder.add_email(email);  
user_builder.add_role(myapp::Role_Admin);  
user_builder.add_department(dept);  
user_builder.add_tags(tags);  
user_builder.add_score(98.5f);  
auto user = user_builder.Finish();  
```

Ce pattern est plus lisible et résiste mieux à l'ajout de nouveaux champs dans le schéma.

---

## Lecture zero-copy (désérialisation)

La lecture est la partie où FlatBuffers brille. Aucune allocation, aucune copie — juste des lectures d'offsets dans le buffer :

```cpp
// buf et size proviennent de recv(), mmap(), ou du builder
const myapp::UserList* user_list = myapp::GetUserList(buf);

std::print("Total : {}\n", user_list->total_count());

const auto* users = user_list->users();  
if (users) {  
    for (size_t i = 0; i < users->size(); ++i) {
        const myapp::User* user = users->Get(i);

        std::print("ID    : {}\n", user->id());
        std::print("Name  : {}\n", user->name()->c_str());
        std::print("Email : {}\n", user->email()->c_str());
        std::print("Role  : {}\n", static_cast<int>(user->role()));
        std::print("Score : {}\n", user->score());

        if (user->department()) {  // test de présence (nullptr si absent)
            std::print("Dept  : {}\n", user->department()->c_str());
        }

        if (user->tags()) {
            for (size_t j = 0; j < user->tags()->size(); ++j) {
                std::print("Tag   : {}\n", user->tags()->Get(j)->c_str());
            }
        }
    }
}
```

### Caractéristiques de l'API de lecture

**Les accesseurs retournent des pointeurs dans le buffer.** `user->name()` retourne un `const flatbuffers::String*`, pas une `std::string`. Le pointeur pointe directement dans le buffer — pas de copie. Pour obtenir un `std::string`, appeler `user->name()->str()`.

**Les champs absents retournent `nullptr` (tables/strings/vectors) ou la valeur par défaut (scalaires).** Le test d'existence se fait par comparaison avec `nullptr` :

```cpp
if (user->department()) {
    // le champ est présent
}
// Les scalaires retournent leur valeur par défaut du schéma
int score = user->score();  // 0.0 si absent (défaut du schéma)
```

**Les vecteurs exposent `size()` et `Get(index)`.** Ils sont aussi itérables avec un range-based for via les helpers FlatBuffers.

**Aucune exception n'est levée.** L'API est entièrement basée sur des retours de valeurs et des tests de nullité. C'est cohérent avec le positionnement performance de FlatBuffers.

---

## Vérification de buffer

Le zero-copy signifie que le programme fait confiance au contenu du buffer pour naviguer via les offsets. Un buffer malformé ou malveillant pourrait contenir des offsets pointant en dehors du buffer, causant des lectures hors limites. FlatBuffers fournit un vérificateur pour se protéger :

```cpp
#include <flatbuffers/verifier.h>

bool process_buffer(const uint8_t* buf, size_t size) {
    // Vérification de l'intégrité du buffer
    flatbuffers::Verifier verifier(buf, size);
    if (!myapp::VerifyUserListBuffer(verifier)) {
        std::print(stderr, "Buffer FlatBuffers invalide ou corrompu\n");
        return false;
    }

    // Accès sécurisé après vérification
    const myapp::UserList* list = myapp::GetUserList(buf);
    // ... utilisation normale
    return true;
}
```

Le `Verifier` parcourt le buffer et vérifie que tous les offsets sont dans les limites, que les vtables sont cohérentes, et que les chaînes sont correctement terminées. C'est une opération O(n) sur la taille du buffer, mais nettement plus rapide qu'un parsing complet.

**Règle : toujours vérifier les buffers provenant de sources non fiables** (réseau, fichiers externes, mémoire partagée). Omettre la vérification pour les buffers construits localement est acceptable si la performance est critique.

Le `Verifier` accepte des options pour contrôler les limites :

```cpp
flatbuffers::Verifier verifier(buf, size,
    64,              // profondeur maximale de récursion
    1000000);        // nombre maximal de tables
```

---

## Object API : quand le zero-copy n'est pas nécessaire

La construction de buffers avec `FlatBufferBuilder` est verbeuse (ordre inversé, offsets manuels). Pour les cas où la performance zero-copy n'est pas critique — construction de messages, tests, conversion de formats — FlatBuffers propose une **Object API** qui génère des structures C++ classiques avec `std::string` et `std::vector` :

```bash
flatc --cpp --gen-object-api schema.fbs
```

Le code généré inclut alors un type `UserT` (suffixe `T`) pour chaque table :

```cpp
// Construction avec l'Object API — aussi simple que Protobuf
auto user = std::make_unique<myapp::UserT>();  
user->id = 42;  
user->name = "Alice Martin";  
user->email = "alice@example.com";  
user->role = myapp::Role_Admin;  
user->department = "Engineering";  
user->tags = {"senior", "on-call"};  
user->score = 98.5f;  

auto user_list = std::make_unique<myapp::UserListT>();  
user_list->users.push_back(std::move(user));  
user_list->total_count = 1;  

// Conversion Object → Buffer
flatbuffers::FlatBufferBuilder builder;  
auto offset = myapp::UserList::Pack(builder, user_list.get());  
builder.Finish(offset);  

// Conversion Buffer → Object
const myapp::UserList* fb = myapp::GetUserList(builder.GetBufferPointer());  
auto restored = fb->UnPack();  // retourne un unique_ptr<UserListT>  

std::print("Name : {}\n", restored->users[0]->name);
```

L'Object API offre le meilleur des deux mondes : la simplicité de manipulation d'un objet C++ natif pour la construction et la modification, combinée avec la possibilité de convertir en buffer zero-copy pour le transport et le stockage.

---

## Écriture et lecture de fichiers

### Écriture

```cpp
// Après builder.Finish(...)
uint8_t* buf = builder.GetBufferPointer();  
size_t size = builder.GetSize();  

std::ofstream file("data.fbs.bin", std::ios::binary);  
file.write(reinterpret_cast<const char*>(buf), size);  
```

### Lecture avec mmap (zero-copy depuis le disque)

Pour une lecture véritablement zero-copy depuis un fichier, `mmap` évite de copier les données du disque vers un buffer en mémoire utilisateur :

```cpp
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

const myapp::UserList* load_from_file(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return nullptr;

    struct stat st;
    fstat(fd, &st);
    size_t size = static_cast<size_t>(st.st_size);

    void* mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);  // le fd peut être fermé après mmap

    if (mapped == MAP_FAILED) return nullptr;

    const uint8_t* buf = static_cast<const uint8_t*>(mapped);

    // Vérification
    flatbuffers::Verifier verifier(buf, size);
    if (!myapp::VerifyUserListBuffer(verifier)) {
        munmap(mapped, size);
        return nullptr;
    }

    // Le pointeur retourné est valide tant que le mapping existe
    return myapp::GetUserList(buf);
}
```

Ce pattern est utilisé dans les jeux (chargement d'assets), les bases de données embarquées, et les systèmes de cache on-disk. Le fichier est projeté en mémoire par le noyau, et FlatBuffers lit directement dans cette projection — zéro copie du disque jusqu'à l'application.

> ⚠️ *Le pointeur retourné par `GetUserList` est une vue sur la mémoire mappée. Il devient invalide après `munmap`. Dans un programme réel, la durée de vie du mapping doit être gérée soigneusement, par exemple via une classe RAII encapsulant le `mmap`/`munmap`.*

---

## Évolution de schéma

FlatBuffers supporte l'évolution de schéma avec des règles similaires à Protobuf.

### Modifications compatibles

- **Ajouter un champ à la fin d'une table.** Les anciens buffers n'auront pas ce champ ; les lecteurs verront la valeur par défaut.
- **Déprécier un champ** (avec l'attribut `deprecated`). Le champ reste dans le schéma pour préserver les offsets, mais les accesseurs ne sont plus générés.

```fbs
table User {
    id: int32;
    name: string;
    email: string;
    role: Role = Unknown;
    department: string (deprecated);  // ne plus utiliser
    tags: [string];
    score: float = 0.0;
    avatar_url: string;               // nouveau champ ajouté à la fin
}
```

### Modifications incompatibles

- **Ajouter un champ au milieu de la table** — décale les identifiants implicites des champs suivants.
- **Supprimer un champ** — utiliser `deprecated` à la place.
- **Changer le type d'un champ.**
- **Modifier la taille ou l'ordre des champs d'un `struct`.**

Pour forcer un identifiant de champ stable (indépendant de la position), l'attribut `id` peut être utilisé, rendant le schéma similaire aux numéros de champs Protobuf :

```fbs
table User {
    name: string (id: 0);
    email: string (id: 1);
    id: int32 (id: 2);          // l'ordre dans le .fbs ne compte plus
    new_field: string (id: 3);  // ajout sûr avec un nouvel id
}
```

---

## Quand choisir FlatBuffers

**FlatBuffers est le bon choix quand :**

- La latence de désérialisation est un goulot d'étranglement mesuré par profiling.
- Les données sont lues beaucoup plus souvent qu'elles ne sont écrites (le coût de construction est plus élevé que Protobuf, mais la lecture est quasi-gratuite).
- L'accès aléatoire à des champs spécifiques est nécessaire sans parser le message entier.
- Le buffer peut être utilisé directement via `mmap` (fichiers de données, cache on-disk, assets de jeu).
- La mémoire est contrainte et les allocations doivent être minimisées.

**FlatBuffers n'est pas le bon choix quand :**

- L'écosystème impose Protobuf (gRPC, Kubernetes, la plupart des schema registries).
- La construction de messages est plus fréquente que la lecture — le `FlatBufferBuilder` est plus complexe que l'API Protobuf.
- La compacité maximale est prioritaire — Protobuf produit des messages légèrement plus compacts grâce à l'encodage varint (FlatBuffers utilise des tailles fixes pour les scalaires et ajoute des vtables).
- L'équipe n'a pas de contrainte de performance justifiant la complexité additionnelle.

---

## Comparaison directe avec Protobuf

| Aspect | FlatBuffers | Protobuf |
|--------|-------------|----------|
| Désérialisation | Zero-copy (O(1)) | Parse complet (O(n)) |
| Sérialisation | Builder + offsets | Setters directs |
| Accès aléatoire | Oui (via vtable) | Non (séquentiel) |
| Compacité | Bonne (mais vtable overhead) | Très bonne (varint) |
| Ergonomie écriture | Moyenne (ordre inversé) | Bonne (setters) |
| Ergonomie lecture | Bonne (accesseurs inline) | Bonne (accesseurs) |
| Strings | `const char*` dans le buffer | `std::string` (copie) |
| Écosystème | Jeux, mobile, embarqué | Cloud, gRPC, Big Data |
| Runtime | Header-only | Librairie (~2 Mo) |
| mmap | Naturel (zero-copy) | Nécessite ParseFromArray |
| JSON interop | `flatc` génère du JSON | Conversion native |
| Vérification | `Verifier` explicite | Implicite au parsing |

Pour de nombreux projets, Protobuf reste le choix par défaut grâce à son écosystème et son ergonomie. FlatBuffers se justifie quand le profiling montre que la désérialisation est un goulot d'étranglement, ou quand l'architecture repose sur le `mmap` et l'accès aléatoire.

La section 25.3 couvre MessagePack, qui adopte une troisième approche : la compacité d'un format binaire sans la contrainte d'un schéma.

⏭️ [MessagePack : JSON binaire compact](/25-formats-binaires/03-messagepack.md)
