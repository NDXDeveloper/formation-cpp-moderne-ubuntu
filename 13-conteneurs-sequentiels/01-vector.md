🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 13.1 — std::vector : Le conteneur par défaut

## Chapitre 13 : Conteneurs Séquentiels

---

## Introduction

`std::vector` est le conteneur le plus utilisé en C++ — et à raison. Il s'agit d'un **tableau dynamique** qui stocke ses éléments de manière **contiguë en mémoire**, se redimensionne automatiquement lorsqu'il est plein, et offre un accès par index en temps constant. C'est le conteneur que le comité de standardisation, les auteurs de référence et les C++ Core Guidelines recommandent comme **choix par défaut**.

La citation attribuée à Bjarne Stroustrup résume la philosophie : si vous ne savez pas quel conteneur choisir, prenez un `std::vector`. Dans la majorité des cas réels, il surpasse les alternatives — y compris celles qui semblent théoriquement supérieures — grâce à l'exploitation optimale du cache CPU.

---

## Déclaration et inclusion

`std::vector` est défini dans l'en-tête `<vector>` :

```cpp
#include <vector>
#include <string>
#include <print>    // C++23

int main() {
    std::vector<int> nombres;                    // vector vide d'entiers
    std::vector<std::string> mots{"hello", "world"};  // initialisé avec 2 éléments
    std::vector<double> valeurs(100, 0.0);       // 100 éléments initialisés à 0.0

    std::println("mots contient {} éléments", mots.size());
    // Sortie : mots contient 2 éléments
}
```

`std::vector` est un template à un paramètre de type obligatoire (le type des éléments) et un paramètre optionnel (l'allocateur, que l'on modifie rarement). Son prototype complet est :

```cpp
template <typename T, typename Allocator = std::allocator<T>>  
class vector;  
```

---

## Pourquoi "le conteneur par défaut" ?

Quatre propriétés fondamentales font de `std::vector` le choix par défaut en C++ moderne.

**Contiguïté mémoire.** Les éléments sont stockés dans un bloc unique et continu sur le heap. Cela signifie que le parcours d'un `std::vector` exploite au maximum les lignes de cache du processeur. Sur les architectures modernes, cette propriété domine presque tous les autres critères de performance. Itérer sur un million d'entiers dans un `std::vector` est typiquement 10 à 50 fois plus rapide que le même parcours dans une `std::list`, malgré une complexité théorique identique O(n).

**Accès aléatoire en O(1).** L'opérateur `[]` et la méthode `at()` permettent d'accéder à n'importe quel élément en temps constant. L'adresse de l'élément `i` se calcule par simple arithmétique de pointeur : `base + i * sizeof(T)`.

**Taille dynamique avec amortissement.** Contrairement à `std::array` dont la taille est fixée à la compilation, un `std::vector` grandit automatiquement. Lorsqu'il atteint sa capacité, il alloue un nouveau bloc (généralement le double de la taille précédente), copie ou déplace les éléments, puis libère l'ancien bloc. Ce mécanisme garantit que l'insertion en fin de vecteur est en **O(1) amorti** — le coût des réallocations ponctuelles est dilué sur l'ensemble des insertions.

**Compatibilité universelle.** `std::vector` fournit des itérateurs à accès aléatoire (*random access iterators*), la catégorie la plus puissante. Il est donc compatible avec l'intégralité des algorithmes de la STL et des Ranges (C++20). De plus, la méthode `data()` retourne un pointeur brut vers le bloc interne, ce qui rend `std::vector` directement interopérable avec les API C et les appels système POSIX.

---

## Constructions courantes

`std::vector` offre de nombreuses façons de construire une instance, adaptées à différents scénarios :

```cpp
#include <vector>
#include <print>

int main() {
    // 1. Construction vide
    std::vector<int> v1;                // vide, size=0, capacity=0

    // 2. Construction avec taille et valeur par défaut
    std::vector<int> v2(5);             // {0, 0, 0, 0, 0}
    std::vector<int> v3(5, 42);         // {42, 42, 42, 42, 42}

    // 3. Construction par liste d'initialisation (C++11)
    std::vector<int> v4{1, 2, 3, 4, 5};

    // 4. Construction par copie
    std::vector<int> v5 = v4;           // copie profonde

    // 5. Construction par déplacement (C++11)
    std::vector<int> v6 = std::move(v4);
    // v4 est maintenant dans un état valide mais indéterminé (typiquement vide)

    // 6. Construction à partir d'une paire d'itérateurs
    std::vector<int> v7(v6.begin(), v6.begin() + 3);  // {1, 2, 3}

    // 7. Déduction de type (CTAD, C++17)
    std::vector v8{10, 20, 30};         // déduit std::vector<int>

    std::println("v6 : size={}, v4 : size={}", v6.size(), v4.size());
    // Sortie : v6 : size=5, v4 : size=0
}
```

Un point mérite attention : la différence entre parenthèses et accolades lors de la construction. `std::vector<int> v(5)` crée un vecteur de 5 éléments initialisés à 0, tandis que `std::vector<int> v{5}` crée un vecteur contenant un seul élément de valeur 5. Cette distinction est une source d'erreurs classique.

---

## Opérations fondamentales

Voici un aperçu des opérations les plus courantes sur un `std::vector`. Les sections suivantes les détailleront en profondeur.

### Ajout d'éléments

```cpp
#include <vector>
#include <string>
#include <print>

int main() {
    std::vector<std::string> logs;

    // push_back : ajoute une copie ou déplace en fin
    logs.push_back("Démarrage du service");

    // emplace_back : construit l'élément directement en place (pas de copie)
    logs.emplace_back("Connexion établie");

    // Depuis C++17, emplace_back retourne une référence vers l'élément créé
    auto& dernier = logs.emplace_back("Traitement en cours");
    std::println("Dernier log : {}", dernier);

    std::println("Nombre de logs : {}", logs.size());
    // Sortie : Nombre de logs : 3
}
```

`emplace_back` est généralement préféré à `push_back` en C++ moderne car il évite la construction d'un temporaire suivi d'un déplacement. La différence est surtout significative pour les types coûteux à construire.

### Accès aux éléments

```cpp
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30, 40, 50};

    // Accès par index — pas de vérification de bornes
    std::println("v[2] = {}", v[2]);       // 30

    // Accès avec vérification — lance std::out_of_range si hors bornes
    std::println("v.at(2) = {}", v.at(2)); // 30

    // Premier et dernier éléments
    std::println("front = {}", v.front()); // 10
    std::println("back = {}", v.back());   // 50

    // Pointeur brut vers les données (interopérabilité C)
    int* raw = v.data();
    std::println("*(raw+3) = {}", *(raw + 3)); // 40
}
```

La méthode `data()` est particulièrement utile pour interfacer un `std::vector` avec des API C, des appels système ou des bibliothèques comme OpenGL qui attendent un pointeur brut :

```cpp
// Exemple : écriture dans un fichier via API POSIX
#include <vector>
#include <unistd.h>  // write()
#include <fcntl.h>   // open()

void ecrire_donnees(int fd, const std::vector<char>& buffer) {
    write(fd, buffer.data(), buffer.size());
}
```

### Suppression d'éléments

```cpp
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30, 40, 50};

    // Supprimer le dernier élément
    v.pop_back();                          // {10, 20, 30, 40}

    // Supprimer un élément par itérateur
    v.erase(v.begin() + 1);               // {10, 30, 40}

    // Supprimer une plage
    v.erase(v.begin(), v.begin() + 2);    // {40}

    // Vider complètement
    v.clear();                             // size=0, capacity inchangée

    std::println("size={}, empty={}", v.size(), v.empty());
    // Sortie : size=0, empty=true
}
```

### Parcours

```cpp
#include <vector>
#include <print>

int main() {
    std::vector<int> v{10, 20, 30, 40, 50};

    // Range-based for loop (C++11) — méthode préférée
    for (const auto& val : v) {
        std::print("{} ", val);
    }
    std::println("");  // 10 20 30 40 50

    // Par index — quand l'index est nécessaire
    for (std::size_t i = 0; i < v.size(); ++i) {
        std::print("[{}]={} ", i, v[i]);
    }
    std::println("");  // [0]=10 [1]=20 [2]=30 [3]=40 [4]=50

    // Par itérateurs — utile pour les algorithmes
    for (auto it = v.cbegin(); it != v.cend(); ++it) {
        std::print("{} ", *it);
    }
    std::println("");  // 10 20 30 40 50
}
```

Le `range-based for` avec `const auto&` est la manière idiomatique de parcourir un conteneur en C++ moderne. Utilisez `auto&` (sans `const`) uniquement si vous devez modifier les éléments.

---

## Size vs Capacity : une distinction fondamentale

`std::vector` maintient deux métriques distinctes qu'il est essentiel de ne pas confondre :

- **`size()`** : le nombre d'éléments effectivement présents dans le vecteur.  
- **`capacity()`** : le nombre d'éléments que le bloc mémoire alloué peut contenir *avant* qu'une réallocation soit nécessaire.

```cpp
#include <vector>
#include <print>

int main() {
    std::vector<int> v;

    std::println("Départ : size={}, capacity={}", v.size(), v.capacity());

    for (int i = 0; i < 10; ++i) {
        v.push_back(i);
        std::println("Après push_back({}) : size={}, capacity={}",
                      i, v.size(), v.capacity());
    }
}
```

Un résultat typique sur GCC/libstdc++ :

```
Départ : size=0, capacity=0  
Après push_back(0) : size=1, capacity=1  
Après push_back(1) : size=2, capacity=2  
Après push_back(2) : size=3, capacity=4  
Après push_back(3) : size=4, capacity=4  
Après push_back(4) : size=5, capacity=8  
Après push_back(5) : size=6, capacity=8  
Après push_back(6) : size=7, capacity=8  
Après push_back(7) : size=8, capacity=8  
Après push_back(8) : size=9, capacity=16  
Après push_back(9) : size=10, capacity=16  
```

On observe que la capacité double à chaque réallocation (1 → 2 → 4 → 8 → 16). Ce facteur de croissance (2 pour GCC, 1.5 pour MSVC) garantit le **coût amorti O(1)** des insertions en fin. La section 13.1.1 détaillera ce mécanisme en profondeur.

---

## `std::vector<bool>` : la spécialisation problématique

Il existe une spécialisation de `std::vector` pour le type `bool` qui mérite une mise en garde explicite. `std::vector<bool>` ne se comporte **pas** comme un vecteur classique : il compacte les booléens à raison d'un bit par élément pour économiser de la mémoire, ce qui rompt plusieurs garanties fondamentales.

```cpp
#include <vector>

int main() {
    std::vector<bool> flags{true, false, true};

    // Ceci ne compile PAS ou produit un comportement surprenant :
    // bool& ref = flags[0];  // ERREUR : operator[] ne retourne pas bool&

    // Il retourne un proxy object (std::vector<bool>::reference)
    auto ref = flags[0];      // ref est un proxy, pas un bool&
}
```

Le problème fondamental est que `operator[]` retourne un objet proxy au lieu d'une vraie référence. Cela signifie que `std::vector<bool>` n'est pas un vrai conteneur au sens du standard : il viole le contrat de `Container`. Les conséquences pratiques sont nombreuses : impossible de prendre l'adresse d'un élément, comportement inattendu avec `auto&`, incompatibilité avec certains algorithmes STL.

Les alternatives recommandées sont `std::vector<char>` ou `std::vector<std::uint8_t>` si vous avez besoin d'un vrai conteneur de booléens, ou `std::bitset` si la taille est connue à la compilation et que vous souhaitez réellement la compaction en bits.

---

## Quand `std::vector` n'est pas le bon choix

Malgré son statut de conteneur par défaut, `std::vector` n'est pas adapté à tous les scénarios. Voici les situations où un autre choix s'impose :

**Taille connue à la compilation** → `std::array`. Si le nombre d'éléments ne change jamais, `std::array` élimine toute allocation dynamique et vit sur la stack. C'est plus léger et plus rapide pour les petites collections de taille fixe.

**Insertions fréquentes en tête** → `std::deque`. Insérer en début de `std::vector` est O(n) car tous les éléments doivent être déplacés. `std::deque` offre une insertion O(1) aux deux extrémités.

**Stabilité des itérateurs et des adresses requise** → `std::list`. Toute réallocation d'un `std::vector` invalide l'ensemble de ses itérateurs et pointeurs. Si votre code dépend de la stabilité des adresses mémoire des éléments (par exemple, des pointeurs vers des éléments stockés ailleurs), une `std::list` garantit que les adresses restent valides après insertion ou suppression.

**Vue sur des données existantes sans copie** → `std::span` (C++20). Si vous ne souhaitez pas posséder les données mais simplement les observer, `std::span` fournit une interface propre et sans allocation.

Chacune de ces alternatives fait l'objet d'une section dédiée dans la suite de ce chapitre.

---

## Plan de la section

Les sous-sections suivantes explorent `std::vector` en profondeur :

- **13.1.1 — Fonctionnement interne et capacité** : le mécanisme de croissance, `reserve()`, `shrink_to_fit()`, et l'amortissement des réallocations.  
- **13.1.2 — Méthodes essentielles** : référence complète des opérations courantes avec exemples.  
- **13.1.3 — Invalidation des itérateurs** : les règles précises d'invalidation et comment écrire du code robuste face aux réallocations.

⏭️ [Fonctionnement interne et capacité](/13-conteneurs-sequentiels/01.1-fonctionnement-interne.md)
