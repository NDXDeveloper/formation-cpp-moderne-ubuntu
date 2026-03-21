🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 45.2 — Integer overflows et underflows

## Chapitre 45 — Sécurité en C++ ⭐

---

## Introduction

Les dépassements d'entiers — overflows et underflows — sont parmi les bugs les plus insidieux en C++. Contrairement à un buffer overflow qui produit souvent un crash visible, un integer overflow peut passer inaperçu pendant des années avant d'être découvert ou exploité. La valeur calculée est simplement fausse, et le programme continue de s'exécuter avec des données corrompues.

Le danger devient critique lorsque le résultat d'un calcul d'entier sert à déterminer une taille d'allocation, un indice de tableau ou un nombre d'itérations. Dans ces cas, un integer overflow se transforme en buffer overflow, en boucle infinie ou en fuite d'informations — avec des conséquences directes sur la sécurité.

Cette section couvre les mécanismes du dépassement d'entier selon les règles du C++, les scénarios d'exploitation concrets, et les techniques de prévention modernes.

---

## Comportement des entiers en C++ : ce que dit le standard

La compréhension des integer overflows en C++ repose sur une distinction fondamentale que le standard impose entre entiers signés et non signés. Ces deux catégories obéissent à des règles radicalement différentes.

### Entiers non signés : wrap-around défini

Le standard C++ définit explicitement le comportement des entiers non signés en cas de dépassement. L'arithmétique est réalisée modulo 2^N, où N est le nombre de bits du type. Le résultat est parfaitement prédictible et portable :

```cpp
#include <cstdint>
#include <print>
#include <limits>

void unsigned_wrap() {
    uint32_t a = std::numeric_limits<uint32_t>::max();  // 4 294 967 295
    uint32_t b = a + 1;

    std::print("a     = {}\n", a);   // 4294967295
    std::print("a + 1 = {}\n", b);   // 0 — wrap-around à zéro

    uint32_t c = 0;
    uint32_t d = c - 1;

    std::print("0 - 1 = {}\n", d);   // 4294967295 — wrap-around au maximum
}
```

Ce comportement est défini, mais il n'est pas pour autant sûr. Le fait que le compilateur soit obligé de produire ce résultat ne signifie pas que le programme est correct. Un `size_t` qui passe de 0 à `SIZE_MAX` après une soustraction est rarement ce que le développeur attendait.

### Entiers signés : undefined behavior

Pour les entiers signés, le dépassement est un **comportement indéfini** (undefined behavior, UB). Le standard ne dit rien sur ce qui se passe : le compilateur est libre de supposer que ça n'arrive jamais, et il utilise cette liberté pour optimiser le code :

```cpp
#include <cstdint>
#include <limits>

void signed_overflow() {
    int32_t x = std::numeric_limits<int32_t>::max();  // 2 147 483 647
    int32_t y = x + 1;  // ⚠️ UNDEFINED BEHAVIOR

    // Le compilateur peut :
    // - Produire un wrap-around (comportement fréquent en pratique)
    // - Supposer que x + 1 > x est toujours vrai et éliminer du code
    // - Produire n'importe quel résultat
    // - Supprimer la branche entière qui mène à cette ligne
}
```

Le caractère indéfini du signed overflow a des conséquences concrètes sur l'optimisation. Le compilateur exploite l'hypothèse d'absence d'overflow pour transformer le code de manière parfois surprenante :

```cpp
// Le compilateur peut optimiser cette fonction de façon inattendue
bool check(int x) {
    return x + 1 > x;
    // Le compilateur SAIT que pour tout int valide, x + 1 > x est vrai
    // (l'overflow étant UB, le cas x = INT_MAX n'existe pas dans son modèle)
    // Résultat possible : la fonction retourne toujours true, sans calcul
}
```

Ce type d'optimisation est documenté et volontaire. GCC et Clang l'appliquent tous les deux à partir de `-O1`. C'est pourquoi détecter un signed overflow en testant le résultat après coup est fondamentalement non fiable — le compilateur peut avoir éliminé votre vérification.

---

## Scénarios de vulnérabilité

### Calcul de taille d'allocation

Le scénario le plus classique et le plus dangereux : un attaquant contrôle des valeurs qui influencent un calcul de taille. L'overflow produit une valeur petite, le programme alloue un tampon trop petit, puis écrit les données réelles — provoquant un heap buffer overflow :

```cpp
#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>

struct ImageHeader {
    uint32_t width;
    uint32_t height;
    uint32_t bytes_per_pixel;
};

// ❌ Vulnérable : overflow dans le calcul de taille
void decode_image_unsafe(const ImageHeader& header, const uint8_t* pixels) {
    // Si width=65536, height=65536, bpp=4 :
    // 65536 * 65536 * 4 = 17 179 869 184 — dépasse uint32_t
    // En uint32_t : résultat = 0 après wrap-around
    uint32_t total_size = header.width * header.height * header.bytes_per_pixel;

    uint8_t* buffer = new uint8_t[total_size];  // Alloue 0 octets !
    std::memcpy(buffer, pixels, total_size);     // Copie 0 octets — OK ici
    // Mais si total_size est utilisé ensuite pour itérer sur les pixels réels...
    delete[] buffer;
}

// ✅ Défensif : validation avant calcul, utilisation de types larges
void decode_image_safe(const ImageHeader& header, const uint8_t* pixels) {
    // Calcul en 64 bits pour détecter l'overflow
    uint64_t total_size =
        static_cast<uint64_t>(header.width)
        * static_cast<uint64_t>(header.height)
        * static_cast<uint64_t>(header.bytes_per_pixel);

    // Limite raisonnable (ex. : 256 Mo pour une image)
    constexpr uint64_t MAX_IMAGE_SIZE = 256ULL * 1024 * 1024;
    if (total_size > MAX_IMAGE_SIZE) {
        throw std::runtime_error("Image trop volumineuse");
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(total_size));
    std::memcpy(buffer.data(), pixels, buffer.size());
}
```

Ce pattern — multiplication de dimensions contrôlées par l'attaquant → overflow → allocation trop petite → buffer overflow — a été exploité dans de nombreuses bibliothèques de traitement d'images (libpng, libjpeg, libwebp) au fil des années.

### Indices de tableau et boucles

Un integer overflow peut transformer un indice ou un compteur de boucle de manière inattendue :

```cpp
#include <cstdint>
#include <vector>

// ❌ Vulnérable : underflow de size_t
void process_last_n_unsafe(const std::vector<int>& data, size_t n) {
    // Si n > data.size(), la soustraction wrappe
    size_t start = data.size() - n;  // Ex : 5 - 10 = 18446744073709551611
    for (size_t i = start; i < data.size(); ++i) {
        // Boucle potentiellement gigantesque, accès hors bornes
    }
}

// ✅ Défensif : vérification avant soustraction
void process_last_n_safe(const std::vector<int>& data, size_t n) {
    if (n > data.size()) {
        n = data.size();  // Clamp au maximum disponible
    }
    size_t start = data.size() - n;  // Garanti >= 0
    for (size_t i = start; i < data.size(); ++i) {
        // Itération sûre dans les bornes
    }
}
```

La soustraction de deux `size_t` (non signés) est un piège classique. La règle est simple : toujours vérifier que le résultat ne sera pas négatif *avant* de faire la soustraction, car après, il est trop tard — le wrap-around a déjà eu lieu.

### Troncation lors de conversions de types

La conversion d'un type large vers un type plus étroit tronque silencieusement la valeur :

```cpp
#include <cstdint>
#include <vector>
#include <print>

void truncation_danger() {
    uint64_t file_size = 5'000'000'000ULL;  // 5 Go
    uint32_t buffer_size = static_cast<uint32_t>(file_size);
    // buffer_size = 705 032 704 (troncation silencieuse)

    std::print("Taille fichier : {} octets\n", file_size);
    std::print("Taille allouée : {} octets\n", buffer_size);
    // Allocation de ~672 Mo au lieu de 5 Go → overflow à l'écriture
}
```

Ce problème est particulièrement fréquent aux frontières entre le code C++ (qui utilise `size_t`, 64 bits sur x86_64) et les API C ou les protocoles réseau (qui utilisent souvent `uint32_t` ou `int` pour les tailles).

### La conversion signée ↔ non signée

La conversion entre types signés et non signés est une source de bugs subtils car elle est souvent implicite :

```cpp
#include <cstdint>
#include <vector>
#include <print>

void signed_unsigned_mismatch() {
    std::vector<int> v = {10, 20, 30};

    int offset = -1;

    // Comparaison signée/non signée — le compilateur avertit (avec -Wall)
    // offset est converti en size_t : -1 → SIZE_MAX
    if (offset < v.size()) {
        std::print("Dans les bornes\n");  // Jamais atteint !
        // SIZE_MAX n'est PAS < v.size()
    } else {
        std::print("Hors bornes\n");      // Exécuté
    }
}
```

```cpp
#include <cstddef>

void negative_to_unsigned(int user_input) {
    // L'utilisateur fournit -5 via un formulaire ou un protocole
    size_t index = static_cast<size_t>(user_input);
    // index = 18446744073709551611 sur un système 64 bits

    // Utiliser index comme indice → accès mémoire hors bornes
}
```

Le compilateur émet un warning pour les comparaisons signées/non signées (`-Wsign-compare`, inclus dans `-Wall`). Ne jamais ignorer ces warnings.

---

## Techniques de détection

### Warnings du compilateur

La première ligne de défense est gratuite : activer les warnings de conversion :

```bash
# GCC
g++ -Wall -Wextra -Wconversion -Wsign-conversion -Wsign-compare -Werror ...

# Clang
clang++ -Wall -Wextra -Wconversion -Wsign-conversion -Wsign-compare -Werror ...
```

Les flags clés pour les problèmes d'entiers :

| Flag | Détecte |
|---|---|
| `-Wconversion` | Conversions implicites qui changent la valeur (troncation, signe) |
| `-Wsign-conversion` | Conversions implicites entre signés et non signés |
| `-Wsign-compare` | Comparaisons entre types signés et non signés |
| `-Woverflow` | Overflows détectables à la compilation (constantes) |

`-Wconversion` est volontairement exclu de `-Wall` car il produit un grand nombre de warnings dans du code existant. Il est cependant essentiel pour du code orienté sécurité et devrait être activé sur tout nouveau projet.

### UndefinedBehaviorSanitizer (UBSan)

UBSan détecte les signed overflows et d'autres comportements indéfinis à l'exécution, avec un surcoût très faible (typiquement inférieur à 5 %) :

```bash
# Détection spécifique des integer overflows
g++ -fsanitize=undefined -fsanitize=signed-integer-overflow \
    -fsanitize=unsigned-integer-overflow -O1 -g main.cpp -o main

# Exécution : le programme s'arrête avec un diagnostic clair
./main
```

Exemple de sortie UBSan :

```
main.cpp:12:17: runtime error: signed integer overflow:
2147483647 + 1 cannot be represented in type 'int'
```

> **Note importante** : `-fsanitize=unsigned-integer-overflow` détecte les wrap-arounds de types non signés. Bien que ce comportement soit défini par le standard, il est rarement intentionnel et constitue souvent un bug. Ce flag est disponible avec Clang ; GCC ne le propose pas directement, mais UBSan couvre d'autres vérifications utiles via `-fsanitize=undefined`.

UBSan est détaillé en section 29.4.2. C'est l'outil le plus rentable pour détecter les integer overflows : coût quasi nul, intégration triviale, diagnostics précis.

### Analyse statique

`clang-tidy` fournit des checks spécifiques aux problèmes d'entiers :

```yaml
# Extrait de .clang-tidy
Checks: >
  bugprone-narrowing-conversions,
  bugprone-signed-char-misuse,
  bugprone-misplaced-widening-cast,
  bugprone-implicit-widening-of-multiplication-result,
  cert-int09-c,
  cppcoreguidelines-narrowing-conversions
```

Le check `bugprone-implicit-widening-of-multiplication-result` est particulièrement pertinent : il détecte le pattern exact de l'exemple d'allocation d'image vu plus haut, où une multiplication en 32 bits est ensuite assignée à un 64 bits — trop tard pour éviter l'overflow.

---

## Techniques de prévention

### Promouvoir le calcul vers un type plus large

La technique la plus simple pour éviter un overflow dans une multiplication : effectuer le calcul dans un type suffisamment large pour contenir le résultat, puis vérifier que le résultat tient dans le type cible :

```cpp
#include <cstdint>
#include <optional>
#include <limits>

// Multiplication sûre : détection de l'overflow avant qu'il ne se produise
std::optional<uint32_t> safe_multiply_u32(uint32_t a, uint32_t b) {
    uint64_t result = static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
    if (result > std::numeric_limits<uint32_t>::max()) {
        return std::nullopt;  // Overflow détecté
    }
    return static_cast<uint32_t>(result);
}
```

Cette approche fonctionne bien quand un type 2× plus large est disponible (32 → 64 bits). Elle ne fonctionne pas pour les calculs déjà en 64 bits, car le C++ standard ne propose pas de type 128 bits portable (bien que GCC et Clang offrent `__int128` en extension).

### Vérification avant l'opération

Pour les cas où la promotion n'est pas possible, la vérification doit être faite avant l'opération, en utilisant une arithmétique qui ne peut pas elle-même déborder :

```cpp
#include <cstdint>
#include <limits>
#include <optional>

// Addition sûre en uint64_t
std::optional<uint64_t> safe_add_u64(uint64_t a, uint64_t b) {
    if (a > std::numeric_limits<uint64_t>::max() - b) {
        return std::nullopt;
    }
    return a + b;
}

// Multiplication sûre en uint64_t
std::optional<uint64_t> safe_multiply_u64(uint64_t a, uint64_t b) {
    if (a == 0 || b == 0) return 0;
    if (a > std::numeric_limits<uint64_t>::max() / b) {
        return std::nullopt;
    }
    return a * b;
}
```

Le pattern `if (a > MAX - b)` pour l'addition et `if (a > MAX / b)` pour la multiplication est la méthode canonique de détection de l'overflow avant qu'il ne se produise. Ces tests utilisent uniquement des opérations qui ne peuvent pas elles-mêmes déborder.

### Builtins du compilateur

GCC et Clang proposent des builtins qui effectuent une opération arithmétique et indiquent si un overflow s'est produit, en une seule instruction — ce qui est à la fois plus concis et plus performant que les vérifications manuelles :

```cpp
#include <cstdint>
#include <print>

bool safe_allocation_size(uint32_t width, uint32_t height, uint32_t bpp,
                          size_t& out_size) {
    uint64_t temp = 0;

    // __builtin_mul_overflow retourne true si l'overflow se produit
    if (__builtin_mul_overflow(
            static_cast<uint64_t>(width),
            static_cast<uint64_t>(height),
            &temp)) {
        return false;
    }

    if (__builtin_mul_overflow(temp, static_cast<uint64_t>(bpp), &temp)) {
        return false;
    }

    // Vérification que le résultat tient dans un size_t
    if (__builtin_mul_overflow(temp, static_cast<size_t>(1), &out_size)) {
        return false;
    }

    return true;
}

void use_safe_alloc() {
    size_t total = 0;
    if (!safe_allocation_size(65536, 65536, 4, total)) {
        std::print(stderr, "Taille d'image invalide (overflow)\n");
        return;
    }
    std::print("Allocation de {} octets\n", total);
}
```

Les builtins `__builtin_add_overflow`, `__builtin_sub_overflow` et `__builtin_mul_overflow` sont disponibles sur GCC et Clang. Ils compilent vers les instructions matérielles les plus efficaces de l'architecture cible (par exemple, le test du carry flag sur x86).

> **C++26 et au-delà** : des propositions sont en discussion au comité de standardisation pour intégrer une arithmétique vérifiée dans la bibliothèque standard, mais aucune n'est entrée dans C++26. Les builtins restent la solution portable la plus efficace sur GCC/Clang en 2026.

### Utiliser des types adaptés au domaine

Le choix du type est la première forme de prévention. Un type trop étroit pour les valeurs attendues est un bug en attente :

```cpp
#include <cstdint>
#include <cstddef>

// ❌ Taille de fichier en uint32_t → limite à 4 Go
struct FileInfo_bad {
    uint32_t file_size;
};

// ✅ Taille de fichier en uint64_t — couvre les besoins réels
struct FileInfo_good {
    uint64_t file_size;
};

// ❌ Compteur de boucle en int16_t pour un traitement de masse
void process_bad(const std::vector<int>& items) {
    for (int16_t i = 0; i < items.size(); ++i) {  // Overflow si > 32767 éléments
        // ...
    }
}

// ✅ Utiliser size_t pour l'indexation de conteneurs
void process_good(const std::vector<int>& items) {
    for (size_t i = 0; i < items.size(); ++i) {
        // ...
    }
}
```

Règles de choix de type orientées sécurité :

- **Tailles de tampon, indices, compteurs d'éléments** → `size_t` ou `std::size_t`.  
- **Tailles de fichiers** → `uint64_t` ou `std::uintmax_t`.  
- **Données réseau avec format défini** → types à largeur fixe (`uint16_t`, `uint32_t`, etc.) — et valider les bornes après lecture.  
- **Calculs intermédiaires** → promouvoir vers un type suffisamment large avant l'opération.

### Restreindre les plages avec des assertions et des contrats

Plutôt que de vérifier l'absence d'overflow après un calcul complexe, il est souvent plus robuste de valider les entrées en amont :

```cpp
#include <cstdint>
#include <cassert>
#include <vector>
#include <stdexcept>

// Validation en amont : les dimensions sont raisonnables,
// donc le calcul ne peut pas déborder
std::vector<uint8_t> allocate_image(uint32_t width, uint32_t height,
                                     uint32_t bpp) {
    // Limites business (une image de plus de 100 000 pixels de côté
    // n'a pas de sens pour cette application)
    if (width == 0 || width > 100'000) {
        throw std::invalid_argument("Largeur hors limites");
    }
    if (height == 0 || height > 100'000) {
        throw std::invalid_argument("Hauteur hors limites");
    }
    if (bpp == 0 || bpp > 16) {
        throw std::invalid_argument("Bytes per pixel hors limites");
    }

    // Avec ces limites, le produit maximal est :
    // 100 000 * 100 000 * 16 = 160 000 000 000 000
    // Tient dans un uint64_t (max ~1.8e19), mais pas dans un size_t 32 bits.
    // Sur un système 64 bits, size_t est 64 bits : OK.
    size_t total = static_cast<size_t>(width)
                 * static_cast<size_t>(height)
                 * static_cast<size_t>(bpp);

    return std::vector<uint8_t>(total);
}
```

Cette approche est souvent préférable en pratique : les limites métier sont plus faciles à raisonner que les limites arithmétiques, et elles protègent contre une classe de problèmes plus large (déni de service par allocation géante, en plus de l'overflow).

> **C++26** : les contrats (`pre`, `post`, `contract_assert`) fourniront un mécanisme standardisé pour exprimer ces contraintes directement dans la signature de la fonction. Voir section 12.14.1 pour la couverture détaillée des contrats.

---

## Le cas particulier de `size_t` et des boucles descendantes

`size_t` est non signé. C'est le type naturel pour les tailles et les indices, mais il crée un piège classique avec les boucles descendantes :

```cpp
#include <cstddef>
#include <vector>
#include <print>

void reverse_print_broken(const std::vector<int>& v) {
    // ❌ BUG : quand i vaut 0, i-- wrappe à SIZE_MAX
    // La condition i >= 0 est TOUJOURS vraie (size_t est non signé)
    for (size_t i = v.size() - 1; i >= 0; --i) {
        std::print("{} ", v[i]);
    }
    // Boucle infinie, puis accès hors bornes
}

void reverse_print_fixed_v1(const std::vector<int>& v) {
    // ✅ Solution 1 : tester avant le décrément
    if (v.empty()) return;
    for (size_t i = v.size(); i-- > 0; ) {
        std::print("{} ", v[i]);
    }
}

void reverse_print_fixed_v2(const std::vector<int>& v) {
    // ✅ Solution 2 : itérateurs inverses (idiomatique C++)
    for (auto it = v.rbegin(); it != v.rend(); ++it) {
        std::print("{} ", *it);
    }
}

void reverse_print_fixed_v3(const std::vector<int>& v) {
    // ✅ Solution 3 : ranges (C++20, le plus lisible)
    for (int val : v | std::views::reverse) {
        std::print("{} ", val);
    }
}
```

L'idiome `for (size_t i = n; i-- > 0; )` est surnommé l'opérateur "goes to" (`-->`) par les développeurs C++. Le décrément est effectué *après* le test, ce qui évite le wrap-around. Les itérateurs inverses ou les ranges sont cependant préférables en termes de lisibilité et de sécurité.

---

## Résumé des bonnes pratiques

**1. Traiter les signed overflows comme des bugs critiques.** Ce sont des comportements indéfinis que le compilateur exploite pour optimiser — les vérifications après coup ne fonctionnent pas.

**2. Traiter les unsigned wrap-arounds comme des bugs logiques.** Le comportement est défini, mais la valeur résultante est presque toujours incorrecte dans le contexte du programme.

**3. Activer `-Wconversion` et `-Wsign-conversion`.** Ces flags révèlent les conversions implicites dangereuses. Les activer sur tout nouveau projet et traiter chaque warning.

**4. Compiler avec UBSan en phase de test.** `-fsanitize=undefined` (et `-fsanitize=unsigned-integer-overflow` avec Clang) détecte les overflows à l'exécution avec un surcoût négligeable.

**5. Valider les entrées avant le calcul, pas après.** Les limites métier (taille maximale d'image, nombre maximal d'éléments) sont plus faciles à raisonner et à maintenir que les vérifications arithmétiques.

**6. Promouvoir vers un type plus large avant les multiplications.** Calculer `static_cast<uint64_t>(a) * b` plutôt que `a * b` quand `a` et `b` sont des `uint32_t`.

**7. Utiliser les builtins du compilateur pour l'arithmétique vérifiée.** `__builtin_mul_overflow` et ses variantes sont la solution la plus efficace quand la promotion n'est pas possible.

**8. Ne jamais comparer un type signé avec un type non signé sans cast explicite.** Le résultat de la conversion implicite est rarement celui attendu pour les valeurs négatives.

---

## Pour aller plus loin

- **Section 45.1** — Les buffer overflows qui résultent souvent d'un integer overflow dans un calcul de taille.  
- **Section 3.2** — Types primitifs, tailles et représentation mémoire.  
- **Section 3.3** — Conversions de types : cast implicite vs explicite.  
- **Section 29.4.2** — UndefinedBehaviorSanitizer : configuration détaillée et intégration CI.  
- **Section 12.14.1** — Contrats C++26 : exprimer les préconditions sur les plages de valeurs.  
- **Section 32.1** — clang-tidy : configuration des checks de détection de narrowing et de conversions dangereuses.

⏭️ [Use-after-free et temporal safety](/45-securite/03-use-after-free.md)
