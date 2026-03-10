🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.3 std::span (C++20) : Vue sur données contiguës

> 📎 *Cette section est un aperçu introductif de `std::span`. Pour la couverture détaillée — span statique vs dynamique, remplacement des paramètres pointeur+taille, interopérabilité avec les conteneurs — voir **section 13.5** (Conteneurs séquentiels).*

## Le problème : passer des données contiguës à une fonction

Une question revient constamment en C++ : comment écrire une fonction qui accepte un tableau de données contiguës en mémoire, quel que soit le conteneur d'origine ? Avant C++20, il n'existait pas de réponse unifiée, et chaque approche avait ses défauts.

### L'approche C : pointeur + taille

```cpp
// Approche héritée du C
void process(const int* data, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        // utiliser data[i]
    }
}

int arr[] = {1, 2, 3, 4, 5};
process(arr, 5);  // Facile de se tromper sur la taille
```

Cette signature est fragile. Rien ne lie `data` à `size` : on peut passer une taille incorrecte, un pointeur nul accompagné d'une taille non nulle, ou inverser les arguments de deux tableaux distincts. Ces erreurs sont silencieuses et produisent des comportements indéfinis.

### L'approche template : générique mais lourde

```cpp
template <typename Container>
void process(const Container& c) {
    for (const auto& elem : c) {
        // utiliser elem
    }
}
```

Cette approche est sûre, mais elle impose que la fonction soit définie dans un header (puisque c'est un template) et elle est instanciée pour chaque type de conteneur — `vector<int>`, `array<int, 5>`, `deque<int>`, même si le traitement est identique. Pour une fonction interne, c'est acceptable. Pour une API de bibliothèque, c'est une contrainte significative sur les temps de compilation et la taille du binaire.

### L'approche std::vector& : trop spécifique

```cpp
void process(const std::vector<int>& data) {
    // ...
}
```

Simple et sûr, mais cette signature refuse un `std::array`, un tableau C natif, ou tout autre conteneur à mémoire contiguë. On se retrouve à écrire des surcharges ou à forcer des conversions inutiles.

## La solution : std::span

`std::span<T>` est une **vue légère et non-owning** sur une séquence contiguë d'éléments de type `T`. C'est un objet qui encapsule un pointeur et une taille, mais avec une interface sûre et une sémantique claire :

```cpp
#include <span>
#include <vector>
#include <array>
#include <print>

void process(std::span<const int> data) {
    for (int value : data) {
        std::print("{} ", value);
    }
    std::print("\n");
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::array<int, 4> arr = {10, 20, 30, 40};
    int c_arr[] = {100, 200, 300};

    process(vec);      // OK — vector
    process(arr);      // OK — array
    process(c_arr);    // OK — tableau C
    process({vec.data() + 1, 3});  // OK — sous-intervalle manuel
}
```

Une seule signature accepte tous les conteneurs à mémoire contiguë. Pas de template, pas de surcharges multiples, pas de paire pointeur+taille désynchronisée.

## Anatomie de std::span

Un `std::span` est un objet extrêmement léger — il ne contient que deux champs :

```
┌──────────────────────────────────────┐
│           std::span<int>             │
│                                      │
│  ┌──────────┐  ┌──────────────────┐  │
│  │ pointer  │  │ size (count)     │  │
│  │ (int*)   │  │ (std::size_t)    │  │
│  └────┬─────┘  └──────────────────┘  │
│       │                              │
└───────┼──────────────────────────────┘
        │
        ▼
  ┌───┬───┬───┬───┬───┐
  │ 1 │ 2 │ 3 │ 4 │ 5 │   ← données possédées par un autre objet
  └───┴───┴───┴───┴───┘
```

La taille de `std::span` lui-même est typiquement de 16 octets sur une architecture 64 bits (un pointeur + un `size_t`). Le copier est aussi bon marché que copier deux entiers. Il ne possède pas les données qu'il référence — il ne fait que les observer.

## Propriétés fondamentales

### Non-owning : le span ne possède rien

C'est le point le plus important à comprendre. Un `std::span` est un emprunt (*borrow*), pas une possession. Il ne gère pas la durée de vie des données sous-jacentes :

```cpp
std::span<int> make_span_wrong() {
    std::vector<int> local = {1, 2, 3};
    return std::span<int>(local);  // DANGER : local est détruit au retour !
}
// Le span retourné pointe vers de la mémoire libérée → dangling
```

La responsabilité de garantir que les données survivent au span incombe au programmeur. C'est la même relation qu'entre un `std::string_view` et une `std::string`, ou entre une référence et l'objet référencé.

### Zéro coût : pas d'allocation, pas de copie

Créer un `std::span` ne copie jamais les données. C'est une opération O(1) qui se résume à stocker un pointeur et une taille. Passer un span par valeur à une fonction est aussi efficace que passer un pointeur — avec la sécurité en plus.

### Interface de conteneur

Malgré sa légèreté, `std::span` offre une interface familière :

```cpp
#include <span>
#include <vector>
#include <print>

std::vector<int> vec = {10, 20, 30, 40, 50};
std::span<const int> s = vec;

s.size();       // 5
s.empty();      // false
s.front();      // 10
s.back();       // 50
s[2];           // 30
s.data();       // Pointeur brut vers le premier élément

// Itération standard
for (int v : s) {
    std::print("{} ", v);
}

// Sous-vues
auto first_three = s.first(3);    // {10, 20, 30}
auto last_two = s.last(2);        // {40, 50}
auto middle = s.subspan(1, 3);    // {20, 30, 40}
```

Les méthodes `first`, `last` et `subspan` retournent elles-mêmes des `std::span`, ce qui permet de découper et passer des sous-intervalles sans copie ni allocation.

## const correctness

La constance se gère au niveau du type d'élément, pas au niveau du span :

```cpp
void read_only(std::span<const int> data);   // Ne peut pas modifier les éléments
void read_write(std::span<int> data);         // Peut modifier les éléments
```

Un `std::span<const int>` est l'analogue d'un `const int*` + taille : le span lui-même peut être copié ou réaffecté, mais les éléments pointés ne sont pas modifiables. C'est la convention adoptée pour les paramètres de fonctions en lecture seule — l'équivalent d'un `const std::vector<int>&` sans en imposer le type concret.

Un `std::span<int>` permet de modifier les données sous-jacentes :

```cpp
void double_values(std::span<int> data) {
    for (int& v : data) {
        v *= 2;
    }
}

std::vector<int> vec = {1, 2, 3};
double_values(vec);
// vec == {2, 4, 6} — modifié en place
```

## Span statique vs dynamique : aperçu

`std::span` existe en deux variantes selon que la taille est connue à la compilation ou non :

```cpp
std::span<int>      dynamic_span;   // Taille connue à l'exécution seulement
std::span<int, 5>   static_span;    // Taille fixe de 5, connue à la compilation
```

Le span dynamique (sans second paramètre template) est le plus courant. Le span statique encode la taille dans le type, ce qui permet au compilateur d'effectuer des vérifications supplémentaires et d'optimiser. Sa taille en mémoire est réduite à un seul pointeur, puisque la taille est une constante connue à la compilation.

> 📎 *La section 13.5 couvre en détail les différences entre span statique et dynamique, les règles de conversion entre les deux, et les implications sur la performance et la sécurité.*

## Quand utiliser std::span

**En paramètre de fonction pour accepter des données contiguës en lecture :** c'est le cas d'usage principal. `std::span<const T>` remplace avantageusement `const std::vector<T>&` quand la fonction n'a pas besoin de la propriété du vector (resize, push_back, etc.) mais seulement d'un accès séquentiel aux données.

**Pour passer des sous-intervalles sans copie :** `s.subspan(offset, count)` est plus clair et plus sûr qu'un calcul manuel de pointeur.

**Pour interfacer du code C et C++ :** un span convertit naturellement un tableau C ou un couple `(pointeur, taille)` issu d'une API C en une vue C++ sûre.

**Ce que span ne fait pas :** il ne gère pas la mémoire, ne peut pas changer de taille, ne possède pas les données. Ce n'est pas un remplacement de `std::vector` mais un complément — une vue temporaire sur des données possédées par quelqu'un d'autre.

## Analogie avec std::string_view

`std::span<const T>` est aux conteneurs de `T` ce que `std::string_view` est à `std::string`. Les deux sont des vues non-owning, légères, qui unifient l'accès à différentes sources de données :

| Aspect | `std::string_view` | `std::span<const T>` |
|--------|--------------------|-----------------------|
| Vue sur | Caractères contigus | Éléments `T` contigus |
| Sources | `std::string`, `const char*`, littéraux | `vector<T>`, `array<T,N>`, `T[]` |
| Possède les données ? | Non | Non |
| Danger principal | Dangling si la string source est détruite | Dangling si le conteneur source est détruit |
| Introduit en | C++17 | C++20 |

Si `std::string_view` vous est familier, `std::span` suit exactement la même philosophie pour les données typées.

## Bonnes pratiques

**Préférer `std::span<const T>` à `const std::vector<T>&` dans les signatures de fonctions** quand la fonction ne fait que lire les données séquentiellement. Cela découple la fonction du type de conteneur et rend l'API plus générique sans recourir aux templates.

**Ne jamais stocker un span au-delà de la durée de vie des données sources.** Un span est fait pour être créé, utilisé et oublié dans une portée locale ou un appel de fonction — pas pour être stocké comme membre de classe à longue durée de vie.

**Utiliser `std::span<T>` (mutable) quand la fonction modifie les données en place**, comme remplacement de `T*` + `size_t` dans les API de transformation.

**Commencer par le span dynamique** (`std::span<T>` sans taille). Le span statique (`std::span<T, N>`) est un outil d'optimisation plus avancé, couvert en section 13.5.

---

>  
> 📎 [13.5 std::span : Couverture détaillée](/13-conteneurs-sequentiels/05-span.md)

⏭️ [Concepts (C++20) : Contraintes sur les templates](/12-nouveautes-cpp17-26/04-concepts.md)
