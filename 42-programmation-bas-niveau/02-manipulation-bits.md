🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 42.2 — Manipulation de Bits et Bitfields

> **Niveau** : Expert  
> **Prérequis** : Chapitre 3.2 (Types primitifs et représentation mémoire), Chapitre 41.1 (Cache CPU et localité des données), Section 42.1 (Inline Assembly)  
> **Fichiers source** : `42-programmation-bas-niveau/02-manipulation-bits/`

---

## Introduction

La manipulation de bits est l'art de travailler directement avec la représentation binaire des données. Là où le reste du langage opère sur des abstractions — entiers, flottants, objets, conteneurs —, les opérations bit à bit descendent au niveau du bit individuel pour extraire, modifier, combiner ou tester des informations encodées dans la structure même des mots machine.

Cette discipline est fondamentale dans plusieurs domaines : protocoles réseau (où chaque bit d'un en-tête a une signification précise), systèmes embarqués (où la mémoire est mesurée en octets, pas en mégaoctets), moteurs de jeux (où des milliers d'entités doivent stocker des flags d'état de manière compacte), cryptographie (où les permutations et rotations de bits sont au cœur des algorithmes), et optimisation de performance (où l'encodage compact améliore la localité de cache).

C++ offre deux approches complémentaires pour travailler au niveau des bits : les **opérateurs bit à bit** du langage (`&`, `|`, `^`, `~`, `<<`, `>>`) qui opèrent sur des entiers de taille standard, et les **bitfields** qui permettent de déclarer des champs de taille arbitraire à l'intérieur d'une structure. C++20 a également enrichi la bibliothèque standard avec le header `<bit>`, qui fournit des fonctions portables pour les opérations courantes sur les bits.

---

## Les opérateurs bit à bit

C++ fournit six opérateurs qui travaillent directement sur la représentation binaire des entiers. Ils opèrent sur chaque bit indépendamment (sauf les décalages, qui déplacent l'ensemble des bits).

### Tableau de référence

| Opérateur | Nom | Description | Exemple (8 bits) |
|-----------|-----|-------------|-------------------|
| `&` | AND bit à bit | 1 si les deux bits sont 1 | `0b11001010 & 0b10101100` → `0b10001000` |
| `\|` | OR bit à bit | 1 si au moins un bit est 1 | `0b11001010 \| 0b10101100` → `0b11101110` |
| `^` | XOR bit à bit | 1 si exactement un bit est 1 | `0b11001010 ^ 0b10101100` → `0b01100110` |
| `~` | NOT bit à bit | Inverse chaque bit | `~0b11001010` → `0b00110101` |
| `<<` | Décalage à gauche | Décale les bits vers la gauche, remplit par des 0 | `0b00001010 << 2` → `0b00101000` |
| `>>` | Décalage à droite | Décale les bits vers la droite | `0b00101000 >> 2` → `0b00001010` |

Chaque opérateur a sa version composée avec affectation : `&=`, `|=`, `^=`, `<<=`, `>>=`.

### Comportement du décalage à droite

Le décalage à droite `>>` se comporte différemment selon que le type est signé ou non signé :

- **Type non signé** (`unsigned int`, `uint32_t`…) : décalage logique. Les bits vacants à gauche sont remplis par des 0.  
- **Type signé** (`int`, `int32_t`…) : le comportement est *implementation-defined* en C++17 et avant. En pratique, GCC et Clang effectuent un décalage arithmétique (les bits vacants sont remplis par le bit de signe), mais le standard ne le garantissait pas formellement. **À partir de C++20**, la représentation en complément à deux est garantie par le standard, ce qui rend le comportement plus prévisible, mais le décalage à droite d'un entier signé négatif reste *implementation-defined*.

**Règle** : travaillez exclusivement avec des types non signés (`uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`) pour toute manipulation de bits. Cela élimine les ambiguïtés et évite le comportement indéfini lié au décalage d'un nombre négatif.

```cpp
#include <cstdint>

uint32_t value = 0xFF00'00FF;

// Décalage à gauche : multiplication par 2^n
uint32_t shifted_left = value << 4;   // 0xF000'0FF0

// Décalage à droite (logique, car unsigned) : division par 2^n
uint32_t shifted_right = value >> 8;  // 0x00FF'0000
```

### Comportement indéfini à surveiller

Deux situations produisent un **comportement indéfini** (UB) avec les décalages :

```cpp
uint32_t x = 1;

// ❌ UB : décalage >= largeur du type (32 bits ici)
uint32_t bad1 = x << 32;

// ❌ UB : décalage par un montant négatif
int shift = -1;  
uint32_t bad2 = x << shift;  
```

UBSan (`-fsanitize=undefined`) détecte ces erreurs à l'exécution (section 29.4.2).

---

## Techniques classiques de manipulation de bits

Les opérations suivantes forment le vocabulaire de base de tout développeur travaillant au niveau des bits. Elles sont omniprésentes dans le code système, les protocoles et les moteurs de jeux.

### Tester si un bit est actif

L'opérateur AND avec un masque contenant un seul bit à 1 permet de tester la valeur d'un bit à une position donnée :

```cpp
bool is_bit_set(uint32_t value, int position) {
    return (value & (1u << position)) != 0;
}

// Exemple : tester le bit 3 (en partant de 0)
uint32_t flags = 0b0000'1010;  
bool bit3 = is_bit_set(flags, 3);  // true  (le bit 3 vaut 1)  
bool bit2 = is_bit_set(flags, 2);  // false (le bit 2 vaut 0)  
```

> ⚠️ Notez le `1u` (littéral non signé). Écrire `1 << 31` sur un `int` 32 bits est un comportement indéfini car cela déborde la représentation signée. Utilisez systématiquement `1u << n` ou `uint32_t{1} << n`.

### Activer un bit (Set)

L'opérateur OR force un bit à 1 sans modifier les autres :

```cpp
uint32_t set_bit(uint32_t value, int position) {
    return value | (1u << position);
}

uint32_t flags = 0b0000'1010;  
flags = set_bit(flags, 2);  // 0b0000'1110  
```

### Désactiver un bit (Clear)

L'opérateur AND avec le complément d'un masque force un bit à 0 :

```cpp
uint32_t clear_bit(uint32_t value, int position) {
    return value & ~(1u << position);
}

uint32_t flags = 0b0000'1110;  
flags = clear_bit(flags, 1);  // 0b0000'1100  
```

### Inverser un bit (Toggle)

L'opérateur XOR inverse un bit sans affecter les autres :

```cpp
uint32_t toggle_bit(uint32_t value, int position) {
    return value ^ (1u << position);
}

uint32_t flags = 0b0000'1010;  
flags = toggle_bit(flags, 1);  // 0b0000'1000  
flags = toggle_bit(flags, 1);  // 0b0000'1010 (retour à l'état initial)  
```

### Extraire un champ de bits

Pour extraire un groupe de bits contigus, on décale à droite puis on applique un masque :

```cpp
// Extrait 'width' bits à partir de la position 'start'
uint32_t extract_bits(uint32_t value, int start, int width) {
    uint32_t mask = (1u << width) - 1;  // Ex: width=4 → mask = 0b1111
    return (value >> start) & mask;
}

// Exemple : un pixel RGB565 (16 bits)
//   Bits [15:11] = Rouge (5 bits)
//   Bits [10:5]  = Vert  (6 bits)
//   Bits [4:0]   = Bleu  (5 bits)
uint16_t pixel = 0b11010'110100'01011;  
uint8_t red   = extract_bits(pixel, 11, 5);  // 0b11010 = 26  
uint8_t green = extract_bits(pixel, 5, 6);   // 0b110100 = 52  
uint8_t blue  = extract_bits(pixel, 0, 5);   // 0b01011 = 11  
```

### Insérer un champ de bits

L'opération inverse : écrire une valeur dans un champ sans modifier le reste :

```cpp
uint32_t insert_bits(uint32_t target, uint32_t value, int start, int width) {
    uint32_t mask = ((1u << width) - 1) << start;
    target &= ~mask;              // Efface le champ cible
    target |= (value << start) & mask;  // Insère la nouvelle valeur
    return target;
}

// Modifier le canal vert du pixel RGB565
uint16_t pixel = 0b11010'110100'01011;  
pixel = insert_bits(pixel, 0b111111, 5, 6);  // Vert au maximum  
// pixel == 0b11010'111111'01011
```

---

## Idiomes avancés

Au-delà des opérations élémentaires, certains patterns bit à bit reviennent fréquemment dans le code expert. Beaucoup exploitent des propriétés mathématiques de la représentation binaire.

### Tester si un entier est une puissance de 2

Une puissance de 2 a exactement un bit à 1. L'expression `n & (n - 1)` efface le bit de poids le plus faible. Si le résultat est 0, il n'y avait qu'un seul bit :

```cpp
bool is_power_of_2(uint32_t n) {
    return n != 0 && (n & (n - 1)) == 0;
}

// is_power_of_2(8)  → true   (0b1000)
// is_power_of_2(6)  → false  (0b0110)
// is_power_of_2(0)  → false  (cas spécial)
```

> 💡 C++20 fournit `std::has_single_bit(n)` dans `<bit>`, qui fait exactement cela de manière portable et expressive.

### Arrondir à la puissance de 2 supérieure

Utile pour les allocateurs mémoire, les tables de hachage et les buffers alignés :

```cpp
uint32_t next_power_of_2(uint32_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

// next_power_of_2(5)   → 8
// next_power_of_2(16)  → 16
// next_power_of_2(100) → 128
```

> 💡 C++20 : `std::bit_ceil(n)` remplace avantageusement cette implémentation manuelle.

### Isoler le bit de poids le plus faible (Lowest Set Bit)

L'expression `n & (-n)` isole le bit le plus bas qui vaut 1. Cela exploite la représentation en complément à deux :

```cpp
uint32_t lowest_set_bit(uint32_t n) {
    return n & (-n);
}

// lowest_set_bit(0b10110100) → 0b00000100 (bit 2)
// lowest_set_bit(0b10000000) → 0b10000000 (bit 7)
```

Ce pattern est au cœur des Fenwick Trees (Binary Indexed Trees) utilisés en programmation compétitive et dans certaines structures de données d'indexation.

### Effacer le bit de poids le plus faible

Déjà vu dans le test de puissance de 2, `n & (n - 1)` efface le bit le plus bas :

```cpp
uint32_t clear_lowest_bit(uint32_t n) {
    return n & (n - 1);
}

// clear_lowest_bit(0b10110100) → 0b10110000
```

Ce pattern permet d'itérer sur les bits actifs d'un mot sans les scanner un par un :

```cpp
void for_each_set_bit(uint32_t mask, auto&& callback) {
    while (mask != 0) {
        int bit = std::countr_zero(mask);  // Position du bit le plus bas (C++20)
        callback(bit);
        mask &= mask - 1;                 // Efface ce bit
    }
}

// Exemple : itérer sur les bits actifs de 0b10110100
// Appelle callback(2), callback(4), callback(5), callback(7)
```

### Swap sans variable temporaire

Le XOR permet d'échanger deux valeurs sans allocation supplémentaire. C'est un classique historique, mais déconseillé en pratique car `std::swap` est plus lisible et le compilateur l'optimise tout aussi bien :

```cpp
void xor_swap(uint32_t& a, uint32_t& b) {
    a ^= b;
    b ^= a;
    a ^= b;
}
// ⚠️ Ne fonctionne PAS si a et b référencent la même variable (a ^= a → 0)
```

---

## Le header `<bit>` (C++20)

C++20 a standardisé les opérations bit à bit les plus courantes dans le header `<bit>`. Ces fonctions sont portables, constexpr-friendly, et le compilateur les traduit en instructions matérielles optimales quand elles existent (`popcnt`, `lzcnt`, `tzcnt`, `bswap`…).

### Fonctions de comptage

```cpp
#include <bit>
#include <cstdint>
#include <print>

uint32_t value = 0b0000'0000'0010'1100'0000'0000'0000'0000;

// Nombre de bits à 1
int pop = std::popcount(value);          // 3

// Nombre de zéros à droite (trailing zeros)
int tz = std::countr_zero(value);        // 18

// Nombre de zéros à gauche (leading zeros)
int lz = std::countl_zero(value);        // 10

// Nombre de uns à droite (trailing ones)
int to = std::countr_one(0b0111u);       // 3

// Nombre de uns à gauche (leading ones)
int lo = std::countl_one(0xFF00'0000u);  // 8
```

### Fonctions de puissance de 2

```cpp
#include <bit>
#include <cstdint>

// Puissance de 2 supérieure ou égale
uint32_t ceil = std::bit_ceil(100u);     // 128

// Puissance de 2 inférieure ou égale
uint32_t floor = std::bit_floor(100u);   // 64

// Largeur en bits (position du bit le plus haut + 1)
int w = std::bit_width(100u);            // 7 (car 100 < 2^7 = 128)

// Exactement un bit à 1 ? (= puissance de 2)
bool single = std::has_single_bit(64u);  // true
```

### Rotation de bits

Les rotations circulaires sont essentielles en cryptographie. Contrairement aux décalages, les bits qui « sortent » d'un côté reviennent de l'autre :

```cpp
#include <bit>
#include <cstdint>

uint8_t val = 0b1011'0001;

uint8_t rl = std::rotl(val, 2);  // Rotation gauche de 2 → 0b1100'0110  
uint8_t rr = std::rotr(val, 3);  // Rotation droite de 3 → 0b0011'0110  
```

### Endianness

C++20 permet enfin de détecter l'endianness de la plateforme à la compilation :

```cpp
#include <bit>

if constexpr (std::endian::native == std::endian::little) {
    // x86-64, ARM en mode little-endian (le cas courant)
} else if constexpr (std::endian::native == std::endian::big) {
    // PowerPC, SPARC, ARM en mode big-endian
} else {
    // Architecture exotique avec endianness mixte
}
```

### `std::byteswap` (C++23)

C++23 ajoute l'inversion des octets, indispensable pour les conversions réseau (big-endian) ↔ hôte (souvent little-endian) :

```cpp
#include <bit>
#include <cstdint>

uint32_t host_value = 0x0102'0304;  
uint32_t network_value = std::byteswap(host_value);  // 0x0403'0201  
```

Cela remplace avantageusement les macros POSIX `htonl`/`ntohl` et les builtins `__builtin_bswap32`.

---

## Bitfields

Les bitfields permettent de déclarer des champs de taille inférieure à celle d'un type entier à l'intérieur d'une structure. Le compilateur se charge de générer les masques et décalages nécessaires pour lire et écrire ces champs.

### Syntaxe de base

```cpp
struct PermissionFlags {
    uint8_t read    : 1;  // 1 bit
    uint8_t write   : 1;  // 1 bit
    uint8_t execute : 1;  // 1 bit
    uint8_t admin   : 1;  // 1 bit
    uint8_t _pad    : 4;  // 4 bits de padding explicite
};

static_assert(sizeof(PermissionFlags) == 1);  // 8 bits = 1 octet

PermissionFlags perms{};  
perms.read = 1;  
perms.write = 1;  
perms.execute = 0;  
perms.admin = 0;  
// Représentation mémoire : 0b0000'0011 (sur x86-64, little-endian)
```

La syntaxe `type nom : largeur;` déclare un champ de `largeur` bits. Le compilateur regroupe les champs adjacents dans le même mot sous-jacent quand c'est possible.

### Application classique : en-tête de protocole réseau

Les bitfields permettent de mapper directement des structures sur des en-têtes binaires. Voici un exemple simplifié d'en-tête TCP (les 32 premiers bits) :

```cpp
#include <cstdint>

struct TcpFlags {
    uint16_t source_port;      // 16 bits
    uint16_t dest_port;        // 16 bits
    uint32_t sequence_number;  // 32 bits
    uint32_t ack_number;       // 32 bits

    // Quatrième mot de 32 bits
    uint16_t data_offset : 4;  // 4 bits
    uint16_t reserved    : 3;  // 3 bits
    uint16_t ns          : 1;  // 1 bit (ECN-nonce)
    uint16_t cwr         : 1;  // 1 bit
    uint16_t ece         : 1;  // 1 bit
    uint16_t urg         : 1;  // 1 bit
    uint16_t ack         : 1;  // 1 bit
    uint16_t psh         : 1;  // 1 bit
    uint16_t rst         : 1;  // 1 bit
    uint16_t syn         : 1;  // 1 bit
    uint16_t fin         : 1;  // 1 bit

    uint16_t window_size;      // 16 bits
};
```

> ⚠️ **Avertissement critique** : cet exemple est pédagogique. En réalité, utiliser des bitfields pour mapper des données réseau est **dangereux** à cause des problèmes de portabilité décrits dans la section suivante. Le code réseau en production utilise des masques et décalages manuels, ou des bibliothèques de sérialisation.

### Application classique : flags compacts pour moteur de jeux

```cpp
struct EntityFlags {
    uint32_t visible      : 1;
    uint32_t collidable   : 1;
    uint32_t destructible : 1;
    uint32_t animated     : 1;
    uint32_t ai_enabled   : 1;
    uint32_t player_owned : 1;
    uint32_t team_id      : 4;   // 0-15
    uint32_t layer        : 5;   // 0-31
    uint32_t health_pct   : 7;   // 0-127 (santé en pourcentage approché)
    uint32_t _reserved    : 6;
};

static_assert(sizeof(EntityFlags) == 4);  // 32 bits = 4 octets
```

Avec 32 bits par entité, on encode 12 champs d'information. Sur un million d'entités, cela représente 4 Mo — contre potentiellement 48 Mo si chaque champ utilisait un `int` séparé (en comptant le padding). La différence est significative pour le cache CPU (chapitre 41.1).

---

## Problèmes de portabilité des bitfields

Les bitfields ont une réputation sulfureuse en C++, et pour cause : le standard laisse de nombreux aspects à la discrétion de l'implémentation. Deux compilateurs peuvent produire des layouts mémoire différents pour la même structure.

### Ce que le standard ne garantit PAS

**Ordre d'allocation des bits au sein d'un mot.** Sur x86-64 avec GCC et Clang, les bits sont alloués du bit de poids faible vers le bit de poids fort (LSB first). Mais un compilateur sur une architecture big-endian (ou même un autre compilateur sur la même architecture) pourrait les allouer dans l'ordre inverse. Le standard dit simplement que l'ordre est *implementation-defined*.

**Padding entre les champs.** Le compilateur peut insérer des bits de padding entre les champs ou à la fin de la structure pour satisfaire les contraintes d'alignement. Deux compilateurs peuvent ne pas insérer le même padding.

**Chevauchement des mots sous-jacents.** Quand un champ ne tient pas dans l'espace restant du mot courant, le compilateur peut soit le placer à cheval sur deux mots, soit insérer du padding et commencer un nouveau mot. Le comportement dépend de l'implémentation.

**Signedness des champs sans qualification explicite.** Un `int x : 5;` peut être signé ou non signé selon le compilateur. Qualifiez toujours explicitement : `uint32_t x : 5;` ou `int32_t x : 5;`.

### Conséquence pratique

Les bitfields ne doivent **jamais** être utilisés pour des données qui traversent une frontière de processus, de machine ou de compilateur :

- Données sérialisées sur le réseau ou sur disque  
- Mémoire partagée entre processus compilés séparément  
- Structures échangées entre du code C++ et du code Rust, Go ou Python

Pour ces cas d'usage, utilisez des masques et décalages manuels (ou une bibliothèque de sérialisation comme Protobuf — section 25.1) dont le comportement est entièrement contrôlé par votre code.

### Quand les bitfields sont-ils sûrs ?

Les bitfields sont parfaitement adaptés quand la structure reste à l'intérieur d'un même programme compilé avec un seul compilateur pour une seule architecture. Les cas typiques sont les flags d'état internes, les structures de données compactes en mémoire, et les registres matériels pour une cible embarquée spécifique (où le layout est vérifié une fois et documenté).

---

## Bitfields anonymes et padding explicite

Un bitfield sans nom permet de réserver des bits sans y associer un identifiant. C'est utile pour aligner les champs suivants ou pour marquer des zones réservées :

```cpp
struct StatusRegister {
    uint32_t enabled      : 1;
    uint32_t mode         : 3;
    uint32_t              : 4;   // 4 bits réservés (inaccessibles)
    uint32_t error_code   : 8;
    uint32_t              : 0;   // Force l'alignement au prochain mot
    uint32_t next_field   : 16;  // Commence dans un nouveau uint32_t
};
```

Le bitfield de largeur 0 (`uint32_t : 0;`) est une syntaxe spéciale qui force le compilateur à aligner le champ suivant sur la prochaine frontière du type sous-jacent. C'est un outil puissant mais rarement nécessaire.

---

## Enum class comme flags type-safe

Les opérations bit à bit sur des `enum class` (scoped enumerations) fournissent un moyen type-safe de manipuler des flags, contrairement aux macros `#define` ou aux `enum` non-scopés du C :

```cpp
#include <cstdint>
#include <type_traits>

enum class FilePermission : uint8_t {
    None    = 0,
    Read    = 1 << 0,   // 0b001
    Write   = 1 << 1,   // 0b010
    Execute = 1 << 2,   // 0b100
    All     = Read | Write | Execute  // ❌ Ne compile pas !
};
```

Le problème : les opérateurs bit à bit ne sont pas définis pour les `enum class`. Il faut les surcharger explicitement. Un pattern courant consiste à écrire un ensemble d'opérateurs génériques activables par un trait :

```cpp
#include <type_traits>

// Trait pour activer les opérateurs bit à bit sur un enum
template <typename E>  
struct EnableBitmask : std::false_type {};  

// Opérateurs génériques
template <typename E>
    requires EnableBitmask<E>::value
constexpr E operator|(E lhs, E rhs) {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template <typename E>
    requires EnableBitmask<E>::value
constexpr E operator&(E lhs, E rhs) {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template <typename E>
    requires EnableBitmask<E>::value
constexpr E operator^(E lhs, E rhs) {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
}

template <typename E>
    requires EnableBitmask<E>::value
constexpr E operator~(E val) {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(~static_cast<U>(val));
}

template <typename E>
    requires EnableBitmask<E>::value
constexpr E& operator|=(E& lhs, E rhs) { return lhs = lhs | rhs; }

template <typename E>
    requires EnableBitmask<E>::value
constexpr E& operator&=(E& lhs, E rhs) { return lhs = lhs & rhs; }

template <typename E>
    requires EnableBitmask<E>::value
constexpr E& operator^=(E& lhs, E rhs) { return lhs = lhs ^ rhs; }
```

L'activation se fait par spécialisation du trait pour chaque enum concerné :

```cpp
enum class FilePermission : uint8_t {
    None    = 0,
    Read    = 1 << 0,
    Write   = 1 << 1,
    Execute = 1 << 2,
};

// Active les opérateurs bit à bit pour FilePermission
template <>  
struct EnableBitmask<FilePermission> : std::true_type {};  

// Utilisation — type-safe et expressive
FilePermission perms = FilePermission::Read | FilePermission::Write;

// Tester un flag
bool can_read = (perms & FilePermission::Read) != FilePermission::None;

// Ajouter un flag
perms |= FilePermission::Execute;

// Retirer un flag
perms &= ~FilePermission::Write;
```

Ce pattern est largement utilisé dans l'industrie. On le retrouve dans Qt (`Q_DECLARE_FLAGS`), Vulkan, DirectX, et de nombreux frameworks.

---

## `std::bitset` : manipulation de bits en taille fixe

La STL fournit `std::bitset<N>`, un conteneur de N bits avec une interface orientée objet. Il est adapté quand le nombre de bits est connu à la compilation et ne dépasse pas quelques centaines :

```cpp
#include <bitset>
#include <print>

std::bitset<8> flags("10110010");

flags.set(0);              // Active le bit 0 → 10110011  
flags.reset(4);            // Désactive le bit 4 → 10100011  
flags.flip(7);             // Inverse le bit 7 → 00100011  

bool b5 = flags.test(5);  // Teste le bit 5 → true  
bool b5_alt = flags[5];   // Syntaxe alternative (pas de bounds check)  

size_t count = flags.count();       // Nombre de bits à 1 → 4  
bool any = flags.any();             // Au moins un bit à 1 → true  
bool none = flags.none();           // Aucun bit à 1 → false  
bool all = flags.all();             // Tous les bits à 1 → false  

std::println("{}", flags.to_string());   // "00100011"  
std::println("{}", flags.to_ulong());    // 35  
```

### Limites de `std::bitset`

Malgré sa commodité, `std::bitset` a des limitations significatives :

- **Taille fixée à la compilation** — Pas de redimensionnement dynamique.  
- **Pas d'itération sur les bits actifs** — Contrairement à l'idiome `n & (n-1)` sur un entier, il n'y a pas de moyen efficace standard de parcourir uniquement les bits à 1.  
- **Performance variable** — Pour les petites tailles (≤ 64 bits), un simple `uint64_t` avec les opérateurs bit à bit est plus performant. Pour les grandes tailles, l'implémentation repose sur un tableau de mots machine, ce qui peut être efficace mais ajoute une indirection.  
- **Pas de constexpr complet** — Selon le compilateur et la version du standard, certaines opérations ne sont pas `constexpr`.

**Recommandation** : utilisez `uint32_t` ou `uint64_t` avec les opérateurs bit à bit pour les flags (≤ 64 bits). Réservez `std::bitset` pour les cas où N est significativement plus grand que 64 et où la lisibilité du code prime sur la performance brute.

---

## Application complète : allocation bitmap

Pour illustrer la combinaison des techniques vues dans cette section, voici un allocateur simplifié utilisant un bitmap pour suivre les blocs libres. Ce type de structure se retrouve dans les allocateurs mémoire, les systèmes de fichiers et les pools de ressources GPU :

```cpp
#include <bit>
#include <cstdint>
#include <array>
#include <optional>
#include <print>

// Allocateur bitmap : gère 256 blocs via 4 mots de 64 bits
class BitmapAllocator {
    static constexpr size_t NumBlocks = 256;
    static constexpr size_t WordBits = 64;
    static constexpr size_t NumWords = NumBlocks / WordBits;

    // Chaque bit à 1 = bloc libre, bit à 0 = bloc occupé
    std::array<uint64_t, NumWords> bitmap_;

public:
    BitmapAllocator() {
        bitmap_.fill(~uint64_t{0});  // Tous les blocs sont libres
    }

    // Alloue un bloc, retourne son index
    std::optional<size_t> allocate() {
        for (size_t w = 0; w < NumWords; ++w) {
            if (bitmap_[w] == 0) continue;  // Pas de bloc libre dans ce mot

            // Trouve le premier bit à 1 (premier bloc libre)
            int bit = std::countr_zero(bitmap_[w]);
            bitmap_[w] &= ~(uint64_t{1} << bit);  // Marque comme occupé
            return w * WordBits + bit;
        }
        return std::nullopt;  // Plus de blocs disponibles
    }

    // Libère un bloc
    void deallocate(size_t index) {
        size_t w = index / WordBits;
        size_t bit = index % WordBits;
        bitmap_[w] |= (uint64_t{1} << bit);  // Marque comme libre
    }

    // Nombre de blocs libres
    size_t free_count() const {
        size_t count = 0;
        for (auto word : bitmap_) {
            count += std::popcount(word);
        }
        return count;
    }
};
```

Cet allocateur trouve un bloc libre en O(N/64) dans le pire cas, grâce au saut rapide des mots entièrement occupés (`== 0`) et à l'instruction `tzcnt` émise par `std::countr_zero`. Sur une architecture avec 64 bits par mot, scanner 4096 blocs ne nécessite que 64 tests de mots — bien plus rapide qu'une liste chaînée de blocs libres.

---

## Résumé

La manipulation de bits en C++ repose sur un socle d'opérateurs simples (`&`, `|`, `^`, `~`, `<<`, `>>`) qui, combinés avec les idiomes classiques (masquage, isolation, comptage), permettent de résoudre efficacement des problèmes d'encodage compact, de flags, de protocoles et d'allocation de ressources.

Les points clés à retenir :

- Travaillez exclusivement avec des types non signés (`uint32_t`, `uint64_t`) pour éviter les comportements indéfinis liés aux décalages et au complément à deux.  
- Utilisez le header `<bit>` de C++20/23 (`std::popcount`, `std::countr_zero`, `std::bit_ceil`, `std::byteswap`…) plutôt que les builtins GCC ou l'assembleur inline — le compilateur génère les mêmes instructions matérielles.  
- Les bitfields sont pratiques pour les structures internes compactes mais ne doivent jamais être utilisés pour des données sérialisées ou échangées entre systèmes.  
- Le pattern `enum class` + trait `EnableBitmask` fournit des flags type-safe sans sacrifier la concision des opérateurs bit à bit.  
- `std::bitset` est adapté aux ensembles de bits de taille fixe supérieure à 64 ; pour les flags simples, un `uint64_t` nu est préférable.

> 📎 *La section suivante, **42.3 — Memory Ordering et Barrières Mémoire**, explore comment le compilateur et le processeur réordonnent les accès mémoire, et comment les opérations atomiques permettent de contrôler cet ordre — un prérequis indispensable avant d'aborder la programmation lock-free (section 42.4).*

⏭️ [Memory ordering et barrières mémoire](/42-programmation-bas-niveau/03-memory-ordering.md)
