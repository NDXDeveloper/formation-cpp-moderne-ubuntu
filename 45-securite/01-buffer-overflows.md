🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 45.1 — Buffer overflows et protection

## Chapitre 45 — Sécurité en C++ ⭐

---

## Introduction

Le buffer overflow — débordement de tampon — est la vulnérabilité la plus emblématique de l'histoire de la sécurité informatique. Le premier ver informatique à grande échelle, le Morris Worm de 1988, exploitait déjà un buffer overflow dans le démon `fingerd` d'Unix. Près de quatre décennies plus tard, cette classe de vulnérabilité reste présente dans les bases de code C et C++ et continue de figurer dans les rapports CVE des projets majeurs.

Le mécanisme est simple à comprendre dans son principe : un programme écrit des données au-delà de la zone mémoire qui leur est réservée, écrasant ainsi des données adjacentes. Les conséquences, en revanche, vont du simple crash à l'exécution de code arbitraire par un attaquant, en passant par la fuite d'informations sensibles.

Cette section explique les différentes formes de buffer overflows, les raisons pour lesquelles le C++ y est exposé, et les techniques concrètes — côté code, compilateur et système — pour s'en prémunir.

---

## Anatomie d'un buffer overflow

### Le mécanisme fondamental

Un buffer overflow se produit lorsqu'une opération d'écriture dépasse les bornes d'un tampon alloué. Le résultat dépend de l'endroit où ce tampon est situé en mémoire.

```cpp
// Exemple volontairement vulnérable — NE PAS reproduire en production
#include <cstring>

void vulnerable(const char* input) {
    char buffer[64];
    std::strcpy(buffer, input);  // Aucune vérification de taille
    // Si input fait plus de 63 caractères + '\0',
    // les octets excédentaires écrasent la mémoire adjacente
}
```

Ce code est un cas d'école que l'on retrouve encore dans du code legacy. La fonction `std::strcpy` copie la chaîne source caractère par caractère jusqu'à rencontrer le terminateur `'\0'`, sans aucune notion de la taille du tampon de destination. Si `input` contient 200 octets, les 136 octets excédentaires écrasent tout ce qui se trouve après `buffer` sur la pile.

### Stack buffer overflow

C'est la forme la plus classique et la plus étudiée. Le tampon est alloué sur la pile (stack), et le débordement écrase les données qui se trouvent au-dessus dans le cadre de pile (stack frame) :

```
Adresses basses                              Adresses hautes
┌───────────┬──────────────┬──────────┬──────────────────┐
│  buffer   │  Canary (*)  │  Saved   │  Return Address  │
│  [64 o]   │  (si activé) │  EBP/RBP │                  │
└───────────┴──────────────┴──────────┴──────────────────┘
     ↑                                        ↑
  Écriture                              Cible de l'attaquant
  normale                               (redirection du flux
                                         d'exécution)
```

Le scénario d'exploitation classique consiste à écraser l'adresse de retour stockée sur la pile. Lorsque la fonction se termine et exécute l'instruction `ret`, le processeur saute à l'adresse écrasée — potentiellement vers du code injecté par l'attaquant (shellcode) ou vers une séquence d'instructions existantes dans le programme (technique ROP — Return-Oriented Programming).

Le canary marqué `(*)` dans le diagramme est une protection compilateur abordée en section 45.4.1 (`-fstack-protector`).

### Heap buffer overflow

Lorsque le tampon est alloué sur le tas (heap), le débordement écrase les métadonnées internes de l'allocateur mémoire ou les données d'objets adjacents :

```cpp
// Heap overflow — code vulnérable
void heap_overflow() {
    char* buf = new char[32];
    // Écriture de 128 octets dans un tampon de 32
    std::memset(buf, 'A', 128);  // Écrase les métadonnées du heap
    delete[] buf;                 // Comportement indéfini
}
```

Les heap overflows sont généralement plus difficiles à exploiter que les stack overflows, car la disposition du tas est moins prévisible. Cependant, des techniques avancées permettent de manipuler l'état de l'allocateur pour obtenir une exécution de code arbitraire (heap spraying, use-after-free chaîné, etc.).

### Off-by-one

Un cas particulier fréquent est le débordement d'un seul octet, souvent dû à une erreur dans le calcul de la taille d'un tampon :

```cpp
void off_by_one() {
    char buffer[256];
    // Erreur : <= au lieu de <, un octet de trop
    for (int i = 0; i <= 256; ++i) {
        buffer[i] = 'A';
    }
}
```

Un seul octet écrasé peut sembler anodin, mais sur la pile, cet octet peut modifier l'octet de poids faible du pointeur de cadre sauvegardé (saved frame pointer), ce qui peut suffire à rediriger le flux d'exécution lors du retour de la fonction appelante. Cette subtilité rend les off-by-one particulièrement insidieux lors des revues de code.

---

## Pourquoi le C++ est exposé

### L'héritage C

Le C++ hérite de la totalité de la bibliothèque standard C, y compris les fonctions intrinsèquement dangereuses qui ne vérifient pas les bornes des tampons :

| Fonction dangereuse | Problème | Alternative sûre |
|---|---|---|
| `strcpy` | Pas de limite de taille | `std::string`, `strncpy` (avec précautions) |
| `strcat` | Pas de limite de taille | `std::string::append` |
| `sprintf` | Pas de limite de taille | `std::format` (C++23), `snprintf` |
| `gets` | Retirée en C11, aucune limite | `std::getline`, `fgets` |
| `scanf("%s", ...)` | Pas de limite de taille | `std::cin` avec `std::string` |
| `memcpy` | Aucune vérification de bornes | `std::copy` avec itérateurs, `std::span` |

Ces fonctions restent disponibles en C++ par compatibilité, et du code legacy ou du code écrit par des développeurs non formés continue de les utiliser.

### L'accès direct à la mémoire

Le C++ permet l'arithmétique des pointeurs et l'indexation brute de tableaux sans vérification de bornes :

```cpp
int arr[10];  
arr[15] = 42;  // Undefined Behavior — aucune erreur à la compilation  
                // Aucune exception à l'exécution (par défaut)
```

Le compilateur ne génère aucun code de vérification de bornes pour les tableaux natifs ou les accès via pointeur. C'est un choix de design du langage en faveur de la performance (principe du zero-overhead abstraction), mais c'est aussi la source fondamentale de cette classe de vulnérabilités.

### La subtilité des tailles et des conversions

Les conversions implicites entre types signés et non signés sont une source classique de débordements inattendus :

```cpp
void risky_copy(const char* src, int len) {
    char buf[256];
    // Si len est négatif, la conversion implicite en size_t
    // produit un très grand nombre positif
    if (len < 256) {
        std::memcpy(buf, src, static_cast<size_t>(len));
        // Potentiellement des gigaoctets copiés
    }
}
```

Ce type de bug est couvert plus en détail en section 45.2 (Integer overflows), mais il est mentionné ici car il constitue un vecteur fréquent de buffer overflow.

---

## Protections côté code : le C++ moderne comme bouclier

La première ligne de défense contre les buffer overflows est le code lui-même. Le C++ moderne fournit des abstractions qui éliminent les classes entières de vulnérabilités, à condition de les utiliser systématiquement.

### Règle fondamentale : ne jamais utiliser de tableaux C bruts

Toute donnée de taille variable doit être gérée par un conteneur STL. Toute donnée de taille fixe connue à la compilation doit utiliser `std::array` :

```cpp
#include <array>
#include <vector>
#include <string>

// ❌ Dangereux : tableau C brut
void legacy_style() {
    char buffer[1024];
    int data[256];
}

// ✅ Sûr : conteneurs STL
void modern_style() {
    std::string buffer;                   // Taille dynamique, gérée automatiquement
    std::vector<int> data(256);           // Vérifiable avec .at(), taille connue
    std::array<int, 256> fixed_data{};    // Taille fixe, vérifiable avec .at()
}
```

### std::vector et std::array : accès vérifié avec .at()

L'opérateur `[]` des conteneurs STL ne vérifie pas les bornes (pour des raisons de performance). La méthode `.at()`, en revanche, lève une exception `std::out_of_range` en cas d'accès hors bornes :

```cpp
#include <vector>
#include <stdexcept>
#include <print>

void safe_access() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // ⚠️ Pas de vérification — undefined behavior si l'indice est invalide
    int a = v[10];

    // ✅ Vérification à l'exécution — lève std::out_of_range
    try {
        int b = v.at(10);
    } catch (const std::out_of_range& e) {
        std::print(stderr, "Accès hors bornes détecté : {}\n", e.what());
    }
}
```

En contexte de sécurité, l'utilisation systématique de `.at()` sur les chemins de code exposés aux entrées utilisateur est une protection simple et efficace. Le surcoût de la vérification de bornes est négligeable face au coût d'une vulnérabilité exploitée.

> **Bonne pratique** : certaines implémentations de la STL proposent un mode de debug qui ajoute des vérifications de bornes à `operator[]` également. GCC offre le flag `-D_GLIBCXX_DEBUG` et Clang/libc++ propose `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG`. Activer ces modes pendant les tests est fortement recommandé.

### std::span : la vue sûre sur les données contiguës (C++20)

`std::span` résout élégamment le pattern classique du passage de pointeur+taille, qui est la source de nombreux bugs :

```cpp
#include <span>
#include <vector>
#include <print>
#include <cstddef>

// ❌ Pattern dangereux : pointeur + taille — rien ne garantit la cohérence
void process_legacy(const int* data, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        // Si size est incorrect, buffer overflow
        std::print("{} ", data[i]);
    }
}

// ✅ Pattern moderne : std::span encapsule pointeur ET taille
void process_modern(std::span<const int> data) {
    for (int value : data) {  // Itération sûre, bornes connues
        std::print("{} ", value);
    }
}

void caller() {
    std::vector<int> v = {1, 2, 3, 4, 5};

    // L'ancien pattern nécessite de passer la taille manuellement
    process_legacy(v.data(), v.size());

    // std::span capture automatiquement les bornes
    process_modern(v);  // Conversion implicite vector → span
}
```

`std::span` ne possède pas les données, il les observe. Il ne fait aucune allocation et n'a aucun coût à l'exécution par rapport au couple pointeur+taille. Mais il porte l'information de taille avec lui, ce qui élimine la possibilité de passer une taille incohérente. Voir la section 13.5 pour une couverture approfondie de `std::span`.

### std::string et std::string_view : éliminer les manipulations de char*

Toute manipulation de chaînes de caractères devrait utiliser `std::string` (données possédées) ou `std::string_view` (vue non possédante) :

```cpp
#include <string>
#include <string_view>
#include <print>

// ❌ Vulnérable aux overflows
void greet_legacy(const char* name) {
    char message[64];
    std::sprintf(message, "Bonjour, %s !", name);
    // Si name fait plus de ~50 caractères → overflow
}

// ✅ Sûr — std::string gère la taille automatiquement
void greet_modern(std::string_view name) {
    std::string message = std::format("Bonjour, {} !", name);
    std::print("{}\n", message);
}
```

`std::format` (C++23) est type-safe et ne peut pas produire de buffer overflow, contrairement à `sprintf` qui écrit aveuglément dans le tampon de destination sans vérifier sa taille.

### Hardening de la STL : le mode renforcé de libc++

Depuis Clang 18 et libc++, un mécanisme de hardening permet d'ajouter des vérifications de bornes à de nombreuses opérations de la STL qui sont normalement non vérifiées, y compris `operator[]` sur les conteneurs, le déréférencement d'itérateurs invalidés, et d'autres préconditions :

```bash
# Mode fast : vérifications peu coûteuses (recommandé en production)
clang++ -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST ...

# Mode extensive : vérifications supplémentaires (recommandé en staging)
clang++ -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE ...

# Mode debug : vérifications maximales (tests et développement)
clang++ -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG ...
```

Le mode `FAST` est conçu pour être activé en production avec un surcoût mesurable mais acceptable. Il transforme de nombreux comportements indéfinis en traps (arrêts immédiats du programme), ce qui est toujours préférable à une exploitation silencieuse.

Pour GCC/libstdc++, l'équivalent est :

```bash
# Mode assertions : vérifications des préconditions (GCC 14+)
g++ -D_GLIBCXX_ASSERTIONS ...

# Mode debug complet (développement uniquement, coût élevé)
g++ -D_GLIBCXX_DEBUG ...
```

---

## Surfaces d'attaque courantes en C++

Au-delà des exemples pédagogiques, les buffer overflows en conditions réelles se produisent sur des surfaces d'entrée identifiables. Les reconnaître permet de concentrer les efforts de sécurisation.

### Parsing de données externes

Tout code qui lit et interprète des données provenant de l'extérieur (réseau, fichier, entrée utilisateur) est une surface d'attaque potentielle. Les parsers de formats complexes (JSON, XML, images, protocoles binaires) sont des cibles privilégiées :

```cpp
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <span>

// ❌ Parser vulnérable — fait confiance à un champ de taille non validé
struct PacketHeader {
    uint32_t magic;
    uint32_t payload_size;
};

void parse_packet_unsafe(const uint8_t* raw, size_t raw_len) {
    if (raw_len < sizeof(PacketHeader)) return;

    PacketHeader header;
    std::memcpy(&header, raw, sizeof(header));

    char payload[512];
    // payload_size vient de l'attaquant — aucune validation
    std::memcpy(payload, raw + sizeof(header), header.payload_size);
    // Overflow si payload_size > 512
}

// ✅ Parser défensif — validation de toutes les tailles
void parse_packet_safe(std::span<const uint8_t> raw) {
    if (raw.size() < sizeof(PacketHeader)) {
        throw std::runtime_error("Paquet trop court pour contenir un header");
    }

    PacketHeader header;
    std::memcpy(&header, raw.data(), sizeof(header));

    // Validation 1 : payload_size ne dépasse pas les données disponibles
    if (header.payload_size > raw.size() - sizeof(PacketHeader)) {
        throw std::runtime_error("Taille de payload incohérente");
    }

    // Validation 2 : limite maximale raisonnable
    constexpr uint32_t MAX_PAYLOAD = 1024 * 1024;  // 1 Mo
    if (header.payload_size > MAX_PAYLOAD) {
        throw std::runtime_error("Payload trop volumineux");
    }

    // Allocation dynamique — pas de tampon fixe
    std::vector<uint8_t> payload(header.payload_size);
    std::memcpy(payload.data(),
                raw.data() + sizeof(header),
                header.payload_size);

    // Traitement sûr de payload...
}
```

Le pattern à retenir : ne jamais faire confiance à un champ de taille lu depuis des données externes. Toujours valider avant d'utiliser.

### Copie de chaînes et concaténation

La construction progressive de chaînes avec des fonctions C (`strcat`, `strncat`) est une source inépuisable de bugs, car le développeur doit manuellement suivre la taille restante dans le tampon :

```cpp
// ❌ Accumulation dangereuse
void build_path_unsafe(const char* dir, const char* file) {
    char path[256];
    std::strcpy(path, dir);
    std::strcat(path, "/");
    std::strcat(path, file);
    // Si dir + "/" + file > 255 → overflow
}

// ✅ std::string élimine le problème
#include <string>
#include <format>

std::string build_path_safe(std::string_view dir, std::string_view file) {
    return std::format("{}/{}", dir, file);
    // La taille est gérée automatiquement, pas de limite fixe
}
```

### Lecture depuis le réseau ou un fichier

Les fonctions de lecture bas niveau (`read`, `recv`) remplissent un tampon sans notion de la taille de la structure de données attendue. C'est au développeur de s'assurer que le tampon est suffisant et que les lectures partielles sont gérées :

```cpp
#include <sys/socket.h>
#include <vector>
#include <cstdint>

// ❌ Tampon fixe arbitraire — inadapté si le message est plus grand
void receive_unsafe(int sockfd) {
    char buf[256];
    ssize_t n = recv(sockfd, buf, 1024, 0);  // Demande 1024 dans un buffer de 256 !
}

// ✅ Tampon correctement dimensionné
void receive_safe(int sockfd) {
    constexpr size_t BUF_SIZE = 4096;
    std::vector<uint8_t> buf(BUF_SIZE);
    ssize_t n = recv(sockfd, buf.data(), buf.size(), 0);
    if (n > 0) {
        buf.resize(static_cast<size_t>(n));  // Ajuster à la taille réellement lue
    }
}
```

---

## Résumé des règles de protection côté code

Les principes suivants, appliqués systématiquement, éliminent la grande majorité des buffer overflows dans du code C++ nouveau :

**1. Utiliser `std::string` au lieu de `char[]`/`char*`.** La gestion automatique de la taille supprime toute possibilité de débordement lors de copies ou de concaténations de chaînes.

**2. Utiliser `std::vector` ou `std::array` au lieu de tableaux C.** Ces conteneurs connaissent leur taille et offrent un accès vérifié via `.at()`.

**3. Utiliser `std::span` pour les paramètres de type tableau.** Il remplace le pattern pointeur+taille et porte l'information de bornes avec les données.

**4. Utiliser `std::format`/`std::print` au lieu de `sprintf`/`printf`.** Le formatage type-safe du C++23 est structurellement immunisé contre les format string vulnerabilities et les buffer overflows.

**5. Valider toute taille provenant de données externes.** Jamais de `memcpy` ou d'allocation dont la taille est directement lue depuis un flux réseau ou un fichier sans validation préalable.

**6. Activer le hardening de la STL.** `-D_GLIBCXX_ASSERTIONS` (GCC) ou `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST` (Clang/libc++) ajoutent des vérifications de bornes à faible coût, y compris en production.

**7. Compiler avec les sanitizers en phase de test.** AddressSanitizer (`-fsanitize=address`) détecte les débordements stack et heap à l'exécution avec un surcoût d'environ 2× (voir section 29.4.1).

---

## Ce qui ne suffit pas

Il est important de comprendre les limites de certaines approches souvent présentées comme des solutions :

**`strncpy` n'est pas une solution fiable.** Cette fonction ne garantit pas la terminaison de la chaîne si la source est plus longue que la taille spécifiée. Elle remplit également le reste du tampon avec des zéros, ce qui a un coût inattendu sur les grands tampons. Préférer `std::string`.

**Les vérifications manuelles de bornes sont fragiles.** Le code `if (len < sizeof(buf))` est correct au moment de l'écriture, mais il suffit d'un refactoring qui change la taille du tampon sans mettre à jour la condition pour réintroduire la vulnérabilité. Les conteneurs STL portent leur taille avec eux et sont donc robustes aux refactorings.

**Le flag `-fstack-protector` ne prévient pas les overflows.** Il les détecte après coup en vérifiant l'intégrité d'un canary sur la pile. C'est une couche de défense précieuse (voir section 45.4.1), mais elle ne remplace pas la prévention au niveau du code source.

---

## Pour aller plus loin

La protection contre les buffer overflows s'articule en plusieurs couches complémentaires, détaillées dans la suite de ce chapitre :

- **Section 45.2** — Les integer overflows qui peuvent mener indirectement à des buffer overflows via des calculs de taille erronés.  
- **Section 45.4** — Les protections à la compilation (`-fstack-protector`, `-D_FORTIFY_SOURCE`) et au niveau du système d'exploitation (ASLR, PIE).  
- **Section 45.5** — Le fuzzing, technique la plus efficace pour découvrir les buffer overflows qui échappent aux tests classiques et aux revues de code.  
- **Section 45.6** — Les initiatives du comité C++ pour rendre le langage plus sûr par défaut (Safety Profiles, hardening standardisé).

⏭️ [Integer overflows et underflows](/45-securite/02-integer-overflows.md)
