🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 18.1 — `assert` et `static_assert`

## Introduction

Les assertions sont un mécanisme de **programmation défensive** : elles expriment des hypothèses que le développeur considère comme toujours vraies à un point donné du programme. Si une assertion échoue, c'est qu'un **bug** existe dans le code — pas qu'une situation d'erreur attendue s'est produite.

C++ offre deux mécanismes d'assertion complémentaires :

- **`assert`** (hérité du C) : vérifie une condition **à l'exécution** (*runtime*).  
- **`static_assert`** (C++11, amélioré en C++17) : vérifie une condition **à la compilation** (*compile-time*).

Comprendre quand et comment utiliser chacun est essentiel pour écrire du code robuste et détectable précocement en cas de régression.

---

## `assert` : Vérification à l'exécution

### Principe

La macro `assert` est définie dans l'en-tête `<cassert>` (ou `<assert.h>` en C). Elle évalue une expression booléenne à l'exécution. Si l'expression est **fausse** (évaluée à `0`), le programme est **immédiatement interrompu** via un appel à `std::abort()`, avec un message diagnostique affiché sur `stderr`.

```cpp
#include <cassert>

double divide(double numerator, double denominator) {
    assert(denominator != 0.0 && "Le dénominateur ne doit jamais être zéro");
    return numerator / denominator;
}
```

En cas d'échec, la sortie ressemble typiquement à :

```
a.out: src/math_utils.cpp:4: double divide(double, double): Assertion `denominator != 0.0 && "Le dénominateur ne doit jamais être zéro"' failed.  
Aborted (core dumped)  
```

### Anatomie du message d'erreur

Le message affiché par `assert` contient quatre informations clés :

1. Le nom de l'exécutable.
2. Le fichier source et le numéro de ligne.
3. La signature de la fonction (sous GCC/Clang).
4. L'expression exacte qui a échoué.

### L'astuce du message personnalisé

`assert` n'accepte pas nativement de second argument pour un message. L'idiome courant consiste à utiliser l'opérateur `&&` avec une chaîne littérale. Puisqu'un pointeur vers une chaîne littérale est toujours *truthy*, l'expression conserve la valeur booléenne de la condition réelle, tout en incluant le message dans la sortie diagnostique :

```cpp
assert(index < size && "Index hors limites du tableau");
```

Si `index < size` est vrai, l'expression complète est vraie. Si `index < size` est faux, `assert` échoue et le texte `"Index hors limites du tableau"` apparaît dans le message.

### Désactivation avec `NDEBUG`

Le comportement le plus important de `assert` est qu'il peut être **complètement désactivé** à la compilation. Si la macro `NDEBUG` est définie avant l'inclusion de `<cassert>`, toutes les assertions sont remplacées par des expressions vides :

```cpp
#define NDEBUG    // Désactive toutes les assertions
#include <cassert>

void process(int* ptr) {
    assert(ptr != nullptr);  // Cette ligne ne génère AUCUN code
    // ...
}
```

En pratique, on ne définit pas `NDEBUG` manuellement dans le code source. Ce sont les options de compilation et le build system qui s'en chargent :

```bash
# Compilation en mode Debug : assertions actives (comportement par défaut)
g++ -std=c++23 -g -O0 main.cpp -o app_debug

# Compilation en mode Release : assertions désactivées
g++ -std=c++23 -O2 -DNDEBUG main.cpp -o app_release
```

Avec CMake, le mode de build contrôle automatiquement `NDEBUG` :

```cmake
# Debug : NDEBUG n'est PAS défini → assertions actives
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release : NDEBUG EST défini → assertions désactivées
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Ce qu'il faut mettre — et ne pas mettre — dans un `assert`

Les assertions servent à vérifier des **invariants internes** du programme, c'est-à-dire des conditions qui ne devraient jamais être fausses si le code est correct.

**Bons usages :**

```cpp
// Précondition d'une fonction interne (non exposée à l'utilisateur)
void set_pixel(int x, int y, Color c) {
    assert(x >= 0 && x < width_);
    assert(y >= 0 && y < height_);
    buffer_[y * width_ + x] = c;
}

// Vérification d'un invariant de classe
void CircularBuffer::push(int value) {
    assert(count_ <= capacity_ && "Invariant violé : count dépasse capacity");
    // ...
}

// Branche théoriquement impossible
switch (state) {
    case State::Running:  /* ... */ break;
    case State::Stopped:  /* ... */ break;
    case State::Paused:   /* ... */ break;
    default:
        assert(false && "État inconnu — bug dans la machine à états");
}
```

**Mauvais usages — ne jamais faire ceci :**

```cpp
// ERREUR : l'assert contient un effet de bord.
// En mode Release, allocate() ne sera jamais appelé !
assert(allocate(buffer) && "Échec d'allocation");

// ERREUR : une entrée utilisateur invalide n'est pas un bug.
// Il faut gérer cette situation avec une vraie logique d'erreur.
assert(user_input > 0 && "L'entrée doit être positive");

// ERREUR : un fichier absent est une condition d'erreur attendue,
// pas un invariant du programme.
assert(file.is_open() && "Impossible d'ouvrir le fichier");
```

La règle fondamentale est simple : **ne jamais placer d'expression avec effet de bord dans un `assert`**, car cette expression disparaît entièrement en mode Release. De même, `assert` ne remplace jamais une gestion d'erreur destinée à traiter des situations prévisibles (entrée utilisateur invalide, fichier introuvable, connexion réseau perdue, etc.).

---

## `static_assert` : Vérification à la compilation

### Principe

Introduit en C++11 et amélioré en C++17, `static_assert` permet de vérifier une condition **au moment de la compilation**. Si la condition est fausse, la compilation échoue avec un message d'erreur explicite. Contrairement à `assert`, `static_assert` ne génère **aucun code exécutable** et n'a **aucun coût à l'exécution**.

```cpp
// Syntaxe C++11 : message obligatoire
static_assert(sizeof(int) == 4, "Ce code requiert des entiers 32 bits");

// Syntaxe C++17 : message optionnel
static_assert(sizeof(int) == 4);
```

Si la vérification échoue, le compilateur produit une erreur semblable à :

```
error: static assertion failed: Ce code requiert des entiers 32 bits
    8 | static_assert(sizeof(int) == 4, "Ce code requiert des entiers 32 bits");
      |               ~~~~~~~~~~~~~~~~
```

### Contrainte : expression constante

L'expression passée à `static_assert` doit être évaluable à la compilation. Cela inclut les littéraux, `sizeof`, `alignof`, les valeurs `constexpr`, et les *type traits* de `<type_traits>`.

```cpp
#include <type_traits>

// Vérification de taille et d'alignement
static_assert(sizeof(void*) == 8, "Architecture 64 bits requise");  
static_assert(alignof(double) == 8);  

// Utilisation avec constexpr
constexpr int VERSION_MAJOR = 3;  
static_assert(VERSION_MAJOR >= 2, "Version minimale requise : 2.x");  
```

### Cas d'usage courants

#### 1. Contraintes sur les types dans les templates

C'est l'un des usages les plus répandus de `static_assert`. Avant les Concepts (C++20), `static_assert` était le moyen principal de contraindre les paramètres de template avec des messages clairs :

```cpp
#include <type_traits>

template <typename T>  
T add(T a, T b) {  
    static_assert(std::is_arithmetic_v<T>,
                  "add() requiert un type arithmétique (entier ou flottant)");
    return a + b;
}

add(3, 4);         // OK  
add(1.5, 2.3);     // OK  
// add("a", "b");  // Erreur de compilation avec message explicite
```

> 📎 *Depuis C++20, les **Concepts** (section 16.6) offrent une alternative plus élégante pour contraindre les templates. `static_assert` reste néanmoins pertinent pour des vérifications ponctuelles et des messages diagnostiques précis à l'intérieur des corps de fonctions.*

#### 2. Validation de la plateforme et des hypothèses d'architecture

```cpp
// En tête d'un fichier de sérialisation binaire
static_assert(sizeof(float) == 4,  "IEEE 754 single precision attendu");  
static_assert(sizeof(double) == 8, "IEEE 754 double precision attendu");  

// Endianness (C++20 avec std::endian)
#include <bit>
static_assert(std::endian::native == std::endian::little,
              "Ce code suppose une architecture little-endian");
```

#### 3. Vérification de la cohérence des structures

Lorsqu'on échange des données avec des protocoles réseau, des fichiers binaires ou du matériel, les tailles et l'agencement mémoire des structures doivent être garantis :

```cpp
struct [[gnu::packed]] NetworkPacketHeader {
    uint8_t  version;
    uint8_t  type;
    uint16_t length;
    uint32_t sequence;
};

static_assert(sizeof(NetworkPacketHeader) == 8,
              "Le header réseau doit faire exactement 8 octets");

static_assert(std::is_trivially_copyable_v<NetworkPacketHeader>,
              "Le header doit être trivialement copiable pour memcpy/send");
```

#### 4. Vérification des propriétés de types avec `<type_traits>`

La bibliothèque `<type_traits>` fournit un riche ensemble de prédicats que `static_assert` peut exploiter :

```cpp
#include <type_traits>

class MyResource {
    int* data_;
public:
    MyResource(const MyResource&) = delete;
    MyResource(MyResource&& other) noexcept;
    // ...
};

// Vérifications après la définition de la classe
static_assert(!std::is_copy_constructible_v<MyResource>,
              "MyResource ne doit pas être copiable");
static_assert(std::is_nothrow_move_constructible_v<MyResource>,
              "MyResource doit être déplaçable sans exception");
```

---

## Comparaison `assert` vs `static_assert`

| Critère | `assert` | `static_assert` |  
|---|---|---|  
| **Moment de vérification** | Exécution (*runtime*) | Compilation (*compile-time*) |  
| **En-tête requis** | `<cassert>` | Aucun (mot-clé du langage) |  
| **Désactivable** | Oui, via `NDEBUG` | Non, toujours actif |  
| **Coût à l'exécution** | Faible (mais existant si actif) | Aucun |  
| **Type d'expression** | Toute expression booléenne | Expression constante uniquement |  
| **Message personnalisé** | Via l'astuce `&& "msg"` | Paramètre natif (optionnel en C++17) |  
| **Conséquence d'un échec** | `std::abort()` à l'exécution | Erreur de compilation |

La règle de choix est directe : **si la condition peut être évaluée à la compilation, préférer `static_assert`**. Détecter un problème avant même de lancer le programme est toujours préférable.

---

## Bonnes pratiques

**Préférer `static_assert` chaque fois que possible.** Une erreur détectée à la compilation est infiniment plus facile à corriger qu'un crash à l'exécution sur une machine de production.

**Toujours fournir un message explicite.** Même si C++17 rend le message optionnel pour `static_assert`, un message clair accélère considérablement le diagnostic. Pour `assert`, utiliser systématiquement l'idiome `&& "message"`.

**Ne jamais utiliser `assert` pour la validation d'entrées externes.** Les données provenant de l'utilisateur, du réseau ou du système de fichiers doivent être validées par une logique d'erreur robuste (exceptions, `std::expected`, codes de retour). Les assertions ne servent qu'à vérifier des invariants internes.

**Ne jamais placer d'effets de bord dans un `assert`.** C'est le piège classique. L'expression entière disparaît en mode Release, et le programme se comporte alors différemment selon le mode de compilation — un bug particulièrement difficile à diagnostiquer.

**Utiliser `assert` généreusement pendant le développement.** Les assertions coûtent peu en mode Debug et permettent de localiser les bugs au plus près de leur origine. Un `assert` qui échoue pointe directement vers le fichier, la ligne et la condition violée.

**Vérifier les invariants de classe avec `assert`.** Après toute opération qui modifie l'état interne d'un objet, une assertion sur les invariants de classe aide à détecter les régressions immédiatement :

```cpp
void CircularBuffer::push(int value) {
    // ... logique d'insertion ...
    assert(count_ <= capacity_);  // Invariant de classe
}
```

---

## Vers C++26 : les contrats

Les assertions `assert` et `static_assert` resteront des outils fondamentaux, mais C++26 introduit un mécanisme bien plus puissant avec les **contrats** (*Contracts*). Les contrats permettent d'exprimer des préconditions (`pre`), des postconditions (`post`) et des assertions (`contract_assert`) directement dans la signature des fonctions, avec un contrôle fin de leur comportement en cas de violation.

```cpp
// Aperçu C++26 — Contrats
int safe_divide(int a, int b)
    pre(b != 0)
    post(r : r * b + (a % b) == a)  // Propriété de la division entière
{
    contract_assert(a != 0 || b != 0);  // Assertion contractuelle dans le corps
    return a / b;
}
```

> 📎 *Les contrats C++26 sont couverts en détail dans les sections 12.14.1 et 17.6.*

---

## Résumé

`assert` et `static_assert` sont les deux piliers de la programmation défensive en C++. Le premier attrape les bugs à l'exécution pendant le développement et les tests ; le second empêche la compilation si une hypothèse structurelle du code n'est pas satisfaite. Utilisés ensemble et de manière disciplinée, ils forment un filet de sécurité efficace qui détecte les problèmes au plus tôt, là où ils sont les moins coûteux à corriger.

⏭️ [Compilation conditionnelle (#ifdef DEBUG)](/18-assertions-debug/02-compilation-conditionnelle.md)
