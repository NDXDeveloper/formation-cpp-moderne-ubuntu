🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 13.5 — std::span : Vue sur données contiguës (zéro allocation)

## Chapitre 13 : Conteneurs Séquentiels

---

## Introduction

`std::span` (C++20) n'est pas un conteneur. C'est une **vue non-propriétaire** (*non-owning view*) sur une séquence contiguë d'éléments en mémoire. Il ne possède pas les données, ne les alloue pas, ne les libère pas. Il se contente de les référencer — exactement comme un pointeur accompagné d'une taille, mais avec une interface type-safe, compatible STL, et sans aucun surcoût.

`std::span` résout un problème omniprésent en C++ : comment écrire une fonction qui accepte indifféremment un `std::vector`, un `std::array`, un tableau C, ou un pointeur brut accompagné d'une taille, sans copier les données et sans écrire quatre surcharges ? La réponse, avant C++20, passait par des conventions fragiles (paire `T* + size_t`) ou des templates verbeux. `std::span` unifie tout cela en un seul type.

---

## Le problème que `std::span` résout

Considérons une fonction qui calcule la somme d'un tableau d'entiers. Avant C++20, on se retrouvait à écrire plusieurs versions de la même logique :

```cpp
#include <vector>
#include <array>
#include <numeric>
#include <print>

// Version 1 : pointeur + taille (API C classique)
int somme_c(const int* data, std::size_t taille) {
    int total = 0;
    for (std::size_t i = 0; i < taille; ++i) {
        total += data[i];
    }
    return total;
}

// Version 2 : std::vector
int somme_vec(const std::vector<int>& v) {
    return std::accumulate(v.begin(), v.end(), 0);
}

// Version 3 : template pour tout conteneur (verbeux)
template <typename Container>  
int somme_generic(const Container& c) {  
    return std::accumulate(c.begin(), c.end(), 0);
}

int main() {
    int tab[] = {1, 2, 3, 4, 5};
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::array<int, 5> arr{1, 2, 3, 4, 5};

    // Chaque appel utilise une interface différente
    std::println("C     : {}", somme_c(tab, 5));
    std::println("vector: {}", somme_vec(vec));
    std::println("array : {}", somme_generic(arr));
}
```

La version pointeur + taille est fragile : rien n'empêche de passer une taille incorrecte. La version `std::vector&` est restrictive : elle refuse un `std::array` ou un tableau C. La version template est générique mais implique une instanciation pour chaque type de conteneur et ne peut pas être compilée séparément (elle doit vivre dans un header).

Avec `std::span`, une seule fonction suffit :

```cpp
#include <span>
#include <vector>
#include <array>
#include <numeric>
#include <print>

// UNE seule fonction, accepte tout conteneur contigu
int somme(std::span<const int> donnees) {
    return std::accumulate(donnees.begin(), donnees.end(), 0);
}

int main() {
    int tab[] = {1, 2, 3, 4, 5};
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::array<int, 5> arr{1, 2, 3, 4, 5};

    // Tous les appels utilisent la même interface
    std::println("C array : {}", somme(tab));
    std::println("vector  : {}", somme(vec));
    std::println("array   : {}", somme(arr));

    // Fonctionne aussi avec un sous-ensemble
    std::println("partiel : {}", somme({vec.data() + 1, 3}));
    // Somme de {2, 3, 4} = 9
}
```

Pas de copie, pas d'allocation, pas de template exposé dans l'API. `std::span` est converti implicitement depuis n'importe quelle source de données contiguës.

---

## Déclaration et inclusion

`std::span` est défini dans l'en-tête `<span>` :

```cpp
#include <span>
```

Son prototype est :

```cpp
template <typename T, std::size_t Extent = std::dynamic_extent>  
class span;  
```

Le paramètre `Extent` représente le nombre d'éléments. Quand il vaut `std::dynamic_extent` (la valeur par défaut), la taille est connue à l'exécution. Quand il est spécifié, la taille est fixée à la compilation. Cette distinction entre span statique et dynamique est le sujet de la section 13.5.1.

---

## Structure interne : un pointeur et une taille

`std::span` est remarquablement léger. En interne, un span dynamique ne contient que deux champs :

```
std::span<int> s(vec.data(), vec.size());

Objet span (stack)            Données (appartenant à vec)
┌──────────────────┐          ┌─────┬─────┬─────┬─────┬─────┐
│ ptr_  ───────────┼─────────▶│  1  │  2  │  3  │  4  │  5  │
│ size_ = 5        │          └─────┴─────┴─────┴─────┴─────┘
└──────────────────┘          mémoire gérée par vec, PAS par span
    16 octets
```

`sizeof(std::span<int>)` vaut **16 octets** sur x86_64 (un pointeur + un `size_t`). Pour un span statique (`std::span<int, 5>`), la taille est encodée dans le type et le compilateur n'a besoin de stocker que le pointeur — `sizeof` vaut alors **8 octets**.

La copie d'un `std::span` est triviale et quasi gratuite : elle copie un pointeur et un entier. C'est pourquoi on le passe **par valeur** et non par référence :

```cpp
// ✅ Par valeur — idiomatique, léger (16 octets)
void traiter(std::span<const int> donnees);

// ❌ Par référence — inutilement indirect
void traiter(const std::span<const int>& donnees);
```

---

## Construction : conversions implicites

La puissance de `std::span` vient de ses conversions implicites depuis les sources de données contiguës les plus courantes :

```cpp
#include <span>
#include <vector>
#include <array>
#include <print>

void afficher(std::span<const int> s) {
    std::print("[");
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (i > 0) std::print(", ");
        std::print("{}", s[i]);
    }
    std::println("]");
}

int main() {
    // Depuis un std::vector
    std::vector<int> vec{10, 20, 30};
    afficher(vec);             // [10, 20, 30]

    // Depuis un std::array
    std::array<int, 4> arr{1, 2, 3, 4};
    afficher(arr);             // [1, 2, 3, 4]

    // Depuis un tableau C
    int tab[] = {100, 200};
    afficher(tab);             // [100, 200]

    // Depuis un pointeur + taille
    afficher({vec.data(), 2}); // [10, 20] — sous-vue des 2 premiers

    // Depuis un autre span
    std::span<const int> s1(vec);
    afficher(s1);              // [10, 20, 30]

    // Construction explicite vide
    afficher({});              // []
}
```

> ⚠️ `std::span` ne se construit **pas** depuis un `std::list` ou un `std::deque` — ces conteneurs ne stockent pas leurs éléments de manière contiguë. Seuls les conteneurs satisfaisant le concept `std::ranges::contiguous_range` sont acceptés.

---

## `const` et mutabilité

La const-correctness de `std::span` suit une logique distincte de celle des conteneurs classiques. Le `const` s'applique à deux niveaux : le span lui-même et les éléments pointés.

```cpp
#include <span>
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30};

    // span<int> : les éléments sont MODIFIABLES
    std::span<int> s_mut(v);
    s_mut[0] = 99;             // ✅ OK — modifie v[0]
    std::println("v[0] = {}", v[0]);  // 99

    // span<const int> : les éléments sont en LECTURE SEULE
    std::span<const int> s_ro(v);
    // s_ro[0] = 42;           // ❌ Erreur de compilation
    std::println("s_ro[0] = {}", s_ro[0]);  // 99

    // const span<int> : le span lui-même est const
    // (ne peut pas être réassigné), mais les éléments restent modifiables
    const std::span<int> s_const(v);
    s_const[1] = 77;           // ✅ OK — const porte sur le span, pas les données
    // s_const = std::span<int>(...);  // ❌ Erreur — le span est const
}
```

La règle est la même que pour les pointeurs : `span<const int>` est l'équivalent de `const int*` (données non modifiables), tandis que `const span<int>` est l'équivalent de `int* const` (pointeur non réassignable, données modifiables).

Pour les paramètres de fonctions, le choix idiomatique est :

```cpp
// Lecture seule — cas le plus courant
void lire(std::span<const int> donnees);

// Lecture/écriture — quand la fonction modifie les données
void modifier(std::span<int> donnees);
```

---

## API de `std::span`

### Accès aux éléments

```cpp
#include <span>
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30, 40, 50};
    std::span<const int> s(v);

    // Accès par index
    std::println("s[2] = {}", s[2]);          // 30

    // front / back
    std::println("front={}, back={}", s.front(), s.back());
    // front=10, back=50

    // Pointeur brut
    const int* p = s.data();
    std::println("data()[3] = {}", p[3]);     // 40
}
```

> 💡 `std::span` ne fournit pas de méthode `at()` avec vérification de bornes. L'accès hors bornes via `operator[]` est un comportement indéfini, comme pour un pointeur brut.

### Taille et état

```cpp
#include <span>
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30};
    std::span<const int> s(v);
    std::span<const int> vide;

    std::println("size = {}", s.size());              // 3
    std::println("size_bytes = {}", s.size_bytes());  // 12 (3 × sizeof(int))
    std::println("empty = {}", s.empty());            // false
    std::println("vide.empty = {}", vide.empty());    // true
}
```

### Sous-vues : `first`, `last`, `subspan`

C'est l'une des fonctionnalités les plus utiles de `std::span` — extraire des sous-vues sans copie et sans allocation :

```cpp
#include <span>
#include <vector>
#include <print>

void afficher(std::span<const int> s) {
    for (auto val : s) std::print("{} ", val);
    std::println("");
}

int main() {
    std::vector<int> v{10, 20, 30, 40, 50, 60, 70};
    std::span<const int> s(v);

    // Les 3 premiers éléments
    afficher(s.first(3));       // 10 20 30

    // Les 2 derniers éléments
    afficher(s.last(2));        // 60 70

    // Sous-vue à partir de l'index 2, longueur 3
    afficher(s.subspan(2, 3));  // 30 40 50

    // Sous-vue à partir de l'index 4 jusqu'à la fin
    afficher(s.subspan(4));     // 50 60 70
}
```

Ces opérations retournent un nouveau `std::span` qui pointe vers la même mémoire — aucune donnée n'est copiée. C'est du *slicing* à coût zéro.

### Itérateurs

```cpp
#include <span>
#include <vector>
#include <algorithm>
#include <print>

int main() {
    std::vector<int> v{50, 30, 10, 40, 20};
    std::span<int> s(v);

    // Tri via itérateurs du span — trie les données du vector sous-jacent
    std::sort(s.begin(), s.end());

    for (auto val : v) std::print("{} ", val);
    // Sortie : 10 20 30 40 50

    // Itérateurs inverses
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        std::print("{} ", *it);
    }
    // Sortie : 50 40 30 20 10
}
```

---

## La responsabilité de la durée de vie

`std::span` ne possède pas les données qu'il référence. C'est à la fois sa force (zéro copie, zéro allocation) et son danger principal : si les données sous-jacentes sont détruites ou déplacées, le span devient un **dangling span** — l'équivalent d'un dangling pointer.

```cpp
#include <span>
#include <vector>
#include <print>

// ❌ DANGER : retourner un span vers des données locales
std::span<const int> creer_span_dangereux() {
    std::vector<int> local{1, 2, 3};
    return local;  // local est détruit à la sortie → span dangling
}

// ❌ DANGER : le vector peut réallouer
void span_invalide_par_reallocation() {
    std::vector<int> v{1, 2, 3};
    std::span<const int> s(v);

    // push_back peut réallouer le vector → s pointe vers mémoire libérée
    for (int i = 0; i < 1000; ++i) {
        v.push_back(i);
    }
    // s est maintenant un dangling span
    // s[0] → COMPORTEMENT INDÉFINI
}

// ✅ CORRECT : le span vit moins longtemps que les données
void utilisation_sure() {
    std::vector<int> v{1, 2, 3, 4, 5};

    // Le span est utilisé localement, v n'est pas modifié pendant ce temps
    std::span<const int> s(v);
    int total = 0;
    for (auto val : s) total += val;
    std::println("Total : {}", total);  // 15
}
```

La règle fondamentale : **le span doit toujours vivre moins longtemps que les données qu'il référence**, et les données ne doivent pas être réallouées pendant que le span est utilisé. C'est exactement la même discipline que pour les pointeurs bruts et les références — `std::span` apporte la sécurité de type, pas la sécurité de durée de vie.

---

## Cas d'usage typiques

### Remplacement de `(T* ptr, size_t count)`

C'est le cas d'usage fondamental. Partout où une API existante prend un pointeur et une taille, `std::span` apporte type-safety et une interface moderne :

```cpp
#include <span>
#include <print>

// Avant (API C-style)
// void traiter(const float* data, int count);

// Après (API moderne)
void traiter(std::span<const float> data) {
    for (auto val : data) {
        std::print("{:.1f} ", val);
    }
    std::println("");
}
```

### Interface unifiée pour fonctions utilitaires

```cpp
#include <span>
#include <vector>
#include <array>
#include <algorithm>
#include <print>

// Fonctionne avec vector, array, tableau C, sous-vues...
bool est_trie(std::span<const int> donnees) {
    return std::is_sorted(donnees.begin(), donnees.end());
}

double moyenne(std::span<const double> donnees) {
    if (donnees.empty()) return 0.0;
    double total = 0.0;
    for (auto val : donnees) total += val;
    return total / static_cast<double>(donnees.size());
}

int main() {
    std::vector<int> v{1, 3, 5, 7};
    std::array<int, 3> a{10, 5, 1};
    int tab[] = {2, 4, 6};

    std::println("v trié   : {}", est_trie(v));    // true
    std::println("a trié   : {}", est_trie(a));    // false
    std::println("tab trié : {}", est_trie(tab));   // true

    std::vector<double> notes{14.5, 16.0, 12.5, 18.0};
    std::println("Moyenne : {:.1f}", moyenne(notes));           // 15.2
    std::println("Top 2   : {:.1f}", moyenne(std::span(notes).last(2)));  // 15.2
}
```

### Travail sur des tranches de données

```cpp
#include <span>
#include <vector>
#include <print>

// Traiter des paquets réseau : header (4 octets) + payload
void analyser_paquet(std::span<const std::uint8_t> paquet) {
    if (paquet.size() < 4) {
        std::println("Paquet trop court");
        return;
    }

    auto header  = paquet.first(4);
    auto payload = paquet.subspan(4);

    std::println("Header  : {} octets", header.size());
    std::println("Payload : {} octets", payload.size());
}

int main() {
    std::vector<std::uint8_t> data{0x01, 0x02, 0x03, 0x04,
                                    0xAA, 0xBB, 0xCC};
    analyser_paquet(data);
    // Header  : 4 octets
    // Payload : 3 octets
}
```

---

## Plan de la section

Les sous-sections suivantes approfondissent `std::span` :

- **13.5.1 — Span statique vs dynamique** : la différence entre `std::span<int>` et `std::span<int, N>`, les garanties à la compilation, et l'impact sur `sizeof`.  
- **13.5.2 — Remplacement des paramètres pointeur+taille** : guide de migration pour les API existantes.  
- **13.5.3 — Interopérabilité avec vector, array et C arrays** : les règles de conversion implicite et les cas limites.

⏭️ [Span statique vs dynamique](/13-conteneurs-sequentiels/05.1-span-statique-dynamique.md)
