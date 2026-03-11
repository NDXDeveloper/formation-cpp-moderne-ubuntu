🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 13.2 — std::array : Tableaux de taille fixe

## Chapitre 13 : Conteneurs Séquentiels

---

## Introduction

`std::array` est un conteneur de **taille fixe** dont la dimension est connue à la compilation. Il encapsule un tableau C classique (`T[N]`) dans une interface conforme à la STL, sans aucun surcoût à l'exécution. Là où un tableau C brut perd sa taille lorsqu'il est passé à une fonction (décroissance en pointeur), `std::array` transporte sa taille avec lui et offre les mêmes garanties de vérification de bornes, d'itérateurs et de compatibilité algorithmique que les autres conteneurs.

En résumé : `std::array` est ce que les tableaux C auraient dû être depuis le début.

---

## Déclaration et inclusion

`std::array` est défini dans l'en-tête `<array>`. Sa taille est un paramètre template non-type, ce qui signifie qu'elle doit être une **constante connue à la compilation** :

```cpp
#include <array>
#include <print>

int main() {
    // Déclaration avec type et taille explicites
    std::array<int, 5> a{10, 20, 30, 40, 50};

    // Déduction de type et de taille (CTAD, C++17)
    std::array b{1.0, 2.0, 3.0};  // déduit std::array<double, 3>

    // Remplissage uniforme
    std::array<int, 4> zeros{};    // {0, 0, 0, 0} — value-initialized

    std::println("a : size={}, b : size={}", a.size(), b.size());
    // Sortie : a : size=5, b : size=3
}
```

Le prototype complet est :

```cpp
template <typename T, std::size_t N>  
struct array;  
```

Notez que `std::array` est une `struct`, pas une `class`. Cela n'a aucune conséquence pratique (en C++, la seule différence est la visibilité par défaut des membres), mais reflète le fait que `std::array` est un **agrégat** — un point que nous détaillerons plus loin.

---

## Pourquoi préférer `std::array` aux tableaux C ?

Les tableaux C bruts souffrent de plusieurs problèmes historiques que `std::array` résout intégralement.

### Le problème de la décroissance en pointeur

Un tableau C perd sa taille dès qu'il est passé à une fonction :

```cpp
#include <print>

// Le tableau C décroît en pointeur — la taille est perdue
void afficher_c(int arr[], int taille) {
    // sizeof(arr) retourne sizeof(int*), PAS la taille du tableau
    for (int i = 0; i < taille; ++i) {
        std::print("{} ", arr[i]);
    }
    std::println("");
}

int main() {
    int tab[] = {10, 20, 30};
    afficher_c(tab, 3);  // il faut passer la taille manuellement
}
```

Avec `std::array`, la taille fait partie du type :

```cpp
#include <array>
#include <print>

// La taille est encodée dans le type — impossible de se tromper
void afficher(const std::array<int, 3>& arr) {
    for (const auto& val : arr) {
        std::print("{} ", val);
    }
    std::println("");
}

int main() {
    std::array<int, 3> tab{10, 20, 30};
    afficher(tab);
}
```

### Pas de copie possible avec les tableaux C

On ne peut pas copier ni affecter un tableau C directement. `std::array` le permet naturellement :

```cpp
#include <array>
#include <print>

int main() {
    std::array<int, 4> a{1, 2, 3, 4};

    // Copie — impossible avec int[4]
    std::array<int, 4> b = a;

    // Affectation — impossible avec int[4]
    std::array<int, 4> c{};
    c = a;

    // Comparaison — impossible avec int[4]
    std::println("a == b : {}", a == b);  // true
}
```

### Compatibilité STL complète

`std::array` fournit des itérateurs, est compatible avec tous les algorithmes de la STL et supporte le range-based `for` :

```cpp
#include <array>
#include <algorithm>
#include <print>

int main() {
    std::array<int, 6> a{30, 10, 50, 20, 40, 60};

    std::sort(a.begin(), a.end());

    for (const auto& val : a) {
        std::print("{} ", val);
    }
    // Sortie : 10 20 30 40 50 60
}
```

---

## Stockage : la stack, pas le heap

La différence fondamentale entre `std::array` et `std::vector` est l'emplacement du stockage. `std::vector` alloue ses données sur le **heap** via `new`. `std::array` stocke ses données directement **dans l'objet lui-même**, typiquement sur la **stack** si l'objet est une variable locale :

```
std::vector<int> v{10, 20, 30};

Stack                          Heap
┌───────────────────┐          ┌─────┬─────┬─────┐
│ begin_ ───────────┼─────────▶│  10 │  20 │  30 │
│ end_              │          └─────┴─────┴─────┘
│ end_cap_          │           allocation dynamique
└───────────────────┘
    24 octets


std::array<int, 3> a{10, 20, 30};

Stack
┌─────┬─────┬──────┐
│  10 │  20 │  30  │   ← pas d'allocation heap, données dans l'objet
└─────┴─────┴──────┘
    12 octets
```

Les conséquences sont importantes :

- **Pas d'allocation dynamique** : la construction et la destruction sont quasi instantanées. Pas de surcoût d'allocateur, pas de fragmentation mémoire.  
- **Localité maximale** : les données sont dans le même espace mémoire que les variables locales environnantes. Si la fonction est "chaude" dans le cache, le tableau l'est aussi.  
- **Taille limitée par la stack** : la stack a une taille limitée (typiquement 1 à 8 Mo sur Linux). Un `std::array<double, 1'000'000>` en variable locale provoquera un stack overflow. Pour les grands tableaux de taille fixe, `std::vector` avec `reserve()` reste le bon choix.  
- **`sizeof` retourne la taille réelle** : `sizeof(std::array<int, 100>)` vaut 400 octets (100 × 4), contre 24 octets pour un `std::vector<int>` quelle que soit sa taille.

---

## Nature d'agrégat

`std::array` est un **agrégat** au sens du standard C++. Cela signifie qu'il n'a pas de constructeur défini par l'utilisateur, pas de membres privés, et que l'initialisation suit les règles de l'**initialisation par agrégat**. En pratique, cela implique quelques subtilités à connaître.

### Initialisation

```cpp
#include <array>
#include <print>

int main() {
    // Initialisation par liste — syntaxe normale
    std::array<int, 4> a{1, 2, 3, 4};

    // Initialisation partielle — les éléments restants sont value-initialized (0)
    std::array<int, 4> b{1, 2};
    // b = {1, 2, 0, 0}

    // Initialisation vide — tous les éléments à 0
    std::array<int, 4> c{};
    // c = {0, 0, 0, 0}

    // ⚠️ ATTENTION : sans accolades, les éléments ne sont PAS initialisés
    std::array<int, 4> d;
    // d contient des valeurs INDÉTERMINÉES (garbage)

    std::println("c[0]={}", c[0]);  // 0 (garanti)
    // std::println("d[0]={}", d[0]);  // valeur indéterminée — pas UB mais imprévisible
}
```

L'absence d'initialisation par défaut (quand on omet les accolades) est héritée du comportement des tableaux C. C'est un piège courant. Prenez l'habitude d'utiliser `{}` systématiquement.

### Pas de construction avec taille + valeur

Contrairement à `std::vector`, il n'existe pas de constructeur `std::array<int, 5>(42)` pour remplir tous les éléments avec la même valeur. On utilise `fill()` après construction :

```cpp
#include <array>
#include <print>

int main() {
    std::array<int, 5> a;
    a.fill(42);
    // a = {42, 42, 42, 42, 42}

    for (auto val : a) std::print("{} ", val);
    // Sortie : 42 42 42 42 42
}
```

---

## `std::to_array` (C++20) : conversion depuis un tableau C

C++20 introduit `std::to_array` pour construire un `std::array` à partir d'un tableau C brut, avec déduction automatique du type et de la taille :

```cpp
#include <array>
#include <print>

int main() {
    // Conversion d'un tableau C en std::array
    int raw[] = {10, 20, 30, 40};
    auto a = std::to_array(raw);
    // a est std::array<int, 4>

    // Fonctionne aussi avec des littéraux
    auto b = std::to_array({1, 2, 3});
    // b est std::array<int, 3>

    // Avec des types move-only
    auto c = std::to_array<std::string>({"hello", "world"});
    // Les chaînes sont déplacées (move), pas copiées

    std::println("a.size()={}, b.size()={}, c.size()={}", a.size(), b.size(), c.size());
    // Sortie : a.size()=4, b.size()=3, c.size()=2
}
```

`std::to_array` est particulièrement utile dans les contextes où CTAD ne suffit pas, ou pour convertir des tableaux C existants dans du code legacy.

---

## API de `std::array`

L'API de `std::array` est volontairement similaire à celle de `std::vector`, mais sans les opérations qui modifient la taille.

### Accès aux éléments

```cpp
#include <array>
#include <print>
#include <stdexcept>

int main() {
    std::array<int, 5> a{10, 20, 30, 40, 50};

    // Accès par index (pas de vérification)
    std::println("a[2] = {}", a[2]);       // 30

    // Accès avec vérification de bornes
    std::println("a.at(2) = {}", a.at(2)); // 30
    try {
        a.at(10);
    } catch (const std::out_of_range& e) {
        std::println("Exception : {}", e.what());
    }

    // Premier et dernier
    std::println("front={}, back={}", a.front(), a.back());
    // Sortie : front=10, back=50

    // Pointeur brut pour interopérabilité C
    int* raw = a.data();
    std::println("data()[3] = {}", raw[3]);  // 40
}
```

### Itérateurs

`std::array` fournit des itérateurs à accès aléatoire, identiques en catégorie à ceux de `std::vector` :

```cpp
#include <array>
#include <print>

int main() {
    std::array<int, 5> a{50, 40, 30, 20, 10};

    // Parcours avant
    for (auto it = a.cbegin(); it != a.cend(); ++it) {
        std::print("{} ", *it);
    }
    std::println("");  // 50 40 30 20 10

    // Parcours inverse
    for (auto it = a.crbegin(); it != a.crend(); ++it) {
        std::print("{} ", *it);
    }
    std::println("");  // 10 20 30 40 50
}
```

### Méthodes utilitaires

```cpp
#include <array>
#include <print>

int main() {
    std::array<int, 5> a{10, 20, 30, 40, 50};

    std::println("size     = {}", a.size());      // 5
    std::println("max_size = {}", a.max_size());   // 5 (toujours == size)
    std::println("empty    = {}", a.empty());      // false

    // fill : remplit tout le tableau avec une valeur
    a.fill(0);
    // a = {0, 0, 0, 0, 0}

    // swap : échange avec un autre array de même type et taille
    std::array<int, 5> b{1, 2, 3, 4, 5};
    a.swap(b);
    // a = {1, 2, 3, 4, 5}, b = {0, 0, 0, 0, 0}

    for (auto val : a) std::print("{} ", val);
    // Sortie : 1 2 3 4 5
}
```

> 💡 `std::array<T, 0>` est valide et représente un tableau vide. `size()` retourne 0, `empty()` retourne `true`, et `data()` peut retourner `nullptr`. C'est un cas limite utile en métaprogrammation.

---

## Invalidation des itérateurs

Les règles d'invalidation sont triviales : puisque `std::array` ne peut ni grandir ni rétrécir, **aucune opération ne provoque de réallocation**. Les itérateurs, pointeurs et références vers les éléments restent valides pendant toute la durée de vie de l'objet.

La seule opération qui "invalide" au sens large est `swap`, mais dans ce cas les itérateurs restent valides — ils pointent toujours vers le même conteneur, dont le contenu a changé. C'est une distinction subtile par rapport à `std::vector::swap`, où les itérateurs suivent le conteneur d'origine.

---

## `constexpr` et `std::array`

`std::array` est entièrement utilisable dans des contextes `constexpr`, ce qui en fait le conteneur de choix pour les calculs à la compilation :

```cpp
#include <array>
#include <algorithm>
#include <print>

constexpr std::array<int, 5> creer_carre() {
    std::array<int, 5> a{};
    for (int i = 0; i < 5; ++i) {
        a[i] = (i + 1) * (i + 1);
    }
    return a;
}

constexpr auto carres = creer_carre();
// carres = {1, 4, 9, 16, 25} — calculé à la COMPILATION

// Recherche à la compilation (C++20 : constexpr std::find)
constexpr bool contient_16 = std::find(carres.begin(), carres.end(), 16) != carres.end();

int main() {
    static_assert(contient_16, "16 devrait être dans la table des carrés");

    for (auto val : carres) std::print("{} ", val);
    // Sortie : 1 4 9 16 25
}
```

`std::vector` a gagné un support `constexpr` partiel en C++20, mais avec des limitations (l'allocation doit être résolue à la compilation). `std::array`, étant entièrement sur la stack et sans allocation, ne souffre d'aucune de ces limitations.

---

## Structured bindings avec `std::array` (C++17)

Les structured bindings permettent de décomposer un `std::array` en variables nommées, ce qui est particulièrement pratique pour les petits tableaux :

```cpp
#include <array>
#include <print>

std::array<double, 3> coordonnees_gps() {
    return {48.8566, 2.3522, 35.0};  // lat, lon, altitude
}

int main() {
    auto [lat, lon, alt] = coordonnees_gps();

    std::println("Latitude  : {}", lat);
    std::println("Longitude : {}", lon);
    std::println("Altitude  : {} m", alt);
}
```

---

## `std::array` vs `std::vector` vs tableau C : guide de choix

| Critère | `int[N]` (C) | `std::array<int, N>` | `std::vector<int>` |  
|---|---|---|---|  
| Taille | Fixe (compil.) | Fixe (compil.) | Dynamique |  
| Stockage | Stack | Stack | Heap |  
| Allocation dynamique | Non | Non | Oui |  
| Décroissance en pointeur | Oui | Non | Non |  
| Copie / affectation | Non | Oui | Oui |  
| Comparaison (`==`, `<`) | Non | Oui | Oui |  
| Itérateurs STL | Non natif | Oui | Oui |  
| Range-based for | Oui | Oui | Oui |  
| `constexpr` complet | Partiel | Oui | Limité (C++20) |  
| `size()` intégré | Non (`sizeof` hack) | Oui | Oui |  
| `data()` pour interop C | Natif | Oui | Oui |  
| Surcoût mémoire | 0 | 0 | 24 octets (3 pointeurs) |

La règle de décision est simple :

- **Taille connue à la compilation et raisonnable pour la stack** → `std::array`  
- **Taille dynamique ou potentiellement grande** → `std::vector`  
- **Tableau C brut** → seulement pour l'interopérabilité avec du code C existant, et même dans ce cas, préférez `std::array` avec `data()` comme passerelle

---

## Bonnes pratiques

**Initialisez toujours avec des accolades.** `std::array<int, 5> a{}` garantit l'initialisation à zéro. Sans accolades, les éléments contiennent des valeurs indéterminées.

**Préférez `std::array` aux tableaux C dans tout code C++ nouveau.** Il n'y a aucun surcoût, et les bénéfices (sécurité, itérateurs, copie, comparaison) sont immédiats.

**Utilisez `std::to_array` (C++20) pour convertir du code legacy.** C'est le moyen le plus propre de transformer un `int[]` en `std::array` sans spécifier manuellement le type et la taille.

**Exploitez `constexpr` pour les tables de lookup.** Les tables calculées à la compilation avec `std::array` n'ont aucun coût à l'exécution et sont garanties correctes par le compilateur.

**Attention à la taille de la stack.** Un `std::array<double, 100'000>` occupe 800 Ko sur la stack. Si vous approchez les limites de la stack (quelques Mo), passez à un `std::vector` ou allouez le `std::array` sur le heap via `std::make_unique<std::array<double, 100'000>>()`.

⏭️ [std::list et std::forward_list : Listes chaînées](/13-conteneurs-sequentiels/03-list-forward-list.md)
