🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 16.3 — Spécialisation partielle et totale

## Chapitre 16 : Templates et Métaprogrammation · Module 5 : La STL

---

## Introduction

Les sections précédentes ont montré comment écrire du code générique qui fonctionne uniformément pour tout type `T`. Mais il arrive fréquemment qu'un type particulier nécessite un traitement différent : une implémentation plus efficace, un comportement adapté, ou simplement une logique incompatible avec la version générique.

La **spécialisation** permet de fournir une implémentation **alternative** d'un template pour un type spécifique (spécialisation **totale**) ou pour une famille de types partageant une caractéristique commune (spécialisation **partielle**). Le compilateur sélectionne automatiquement la version la plus spécifique applicable.

C'est un mécanisme fondamental : la STL l'utilise abondamment, par exemple pour optimiser `std::vector<bool>` (un cas célèbre — et controversé) ou pour adapter le comportement de `std::hash` à chaque type.

---

## Le besoin : quand le générique ne suffit pas

Reprenons notre template `Serializer` qui convertit une valeur en chaîne de caractères :

```cpp
#include <string>
#include <format>

template <typename T>  
class Serializer {  
public:  
    static std::string to_string(const T& valeur) {
        return std::format("{}", valeur);
    }
};
```

Ce template fonctionne pour `int`, `double`, `std::string` et tout type compatible avec `std::format`. Mais certains cas posent problème :

- **`bool`** : `std::format("{}", true)` produit `"true"`, mais on pourrait vouloir `"1"` ou `"oui"` dans un contexte spécifique.
- **`const char*`** : on veut peut-être l'encadrer de guillemets pour distinguer une chaîne vide d'une absence de valeur.
- **Pointeurs** : afficher l'adresse brute n'est probablement pas le comportement souhaité pour la sérialisation.

La spécialisation permet de traiter chaque cas sans modifier le template principal.

---

## Spécialisation totale (explicite)

Une **spécialisation totale** (*full specialization* ou *explicit specialization*) fournit une implémentation dédiée pour une combinaison **complète et précise** de paramètres template. Tous les paramètres sont fixés ; il ne reste aucun paramètre libre.

### Syntaxe

On écrit `template <>` (liste de paramètres vide) suivi de la définition de la classe avec les types concrets :

```cpp
// Template principal (version générique)
template <typename T>  
class Serializer {  
public:  
    static std::string to_string(const T& valeur) {
        return std::format("{}", valeur);
    }
};

// Spécialisation totale pour bool
template <>  
class Serializer<bool> {  
public:  
    static std::string to_string(bool valeur) {
        return valeur ? "true" : "false";
    }
};

// Spécialisation totale pour const char*
template <>  
class Serializer<const char*> {  
public:  
    static std::string to_string(const char* valeur) {
        if (valeur == nullptr) {
            return "null";
        }
        return std::format("\"{}\"", valeur);
    }
};
```

```cpp
std::print("{}\n", Serializer<int>::to_string(42));           // "42"  
std::print("{}\n", Serializer<bool>::to_string(true));        // "true"  
std::print("{}\n", Serializer<const char*>::to_string("hi")); // "\"hi\""  
std::print("{}\n", Serializer<const char*>::to_string(nullptr)); // "null"  
```

### Points essentiels

La spécialisation totale est une **définition complètement indépendante**. Elle ne partage rien avec le template principal : ni les membres, ni les méthodes, ni la disposition mémoire. Vous réécrivez la classe entièrement. Cela signifie que si le template principal possède dix méthodes, la spécialisation doit redéfinir chacune de celles qu'elle souhaite exposer — il n'y a pas d'héritage automatique entre le template principal et ses spécialisations.

Le préfixe `template <>` est distinctif : il signale une spécialisation où tous les paramètres sont fixés. Le compilateur n'a rien à déduire.

---

## Spécialisation totale de fonctions template

Les fonctions template supportent la spécialisation totale (mais **pas** la spécialisation partielle — nous y reviendrons). La syntaxe est similaire :

```cpp
// Template principal
template <typename T>  
std::string convertir(const T& valeur) {  
    return std::format("{}", valeur);
}

// Spécialisation totale pour bool
template <>  
std::string convertir<bool>(const bool& valeur) {  
    return valeur ? "vrai" : "faux";
}
```

```cpp
std::print("{}\n", convertir(42));    // "42" — template principal  
std::print("{}\n", convertir(true));  // "vrai" — spécialisation pour bool  
```

### Préférer la surcharge à la spécialisation de fonctions

En pratique, la spécialisation totale de fonctions est **rarement la bonne approche**. La surcharge classique est presque toujours préférable, car elle interagit mieux avec les règles de résolution de surcharge :

```cpp
// Surcharge (recommandée) — participe normalement à l'overload resolution
std::string convertir(bool valeur) {
    return valeur ? "vrai" : "faux";
}
```

La raison profonde est que la spécialisation de fonctions et la surcharge obéissent à des règles de sélection différentes. Le compilateur choisit d'abord parmi les surcharges (templates et non-templates), puis seulement après, parmi les spécialisations du template sélectionné. Cela peut mener à des résultats surprenants lorsque les deux mécanismes sont combinés.

Considérons cet exemple piégeux :

```cpp
// (1) Template principal
template <typename T>  
void traiter(T val) {  
    std::print("template general: {}\n", val);
}

// (2) Surcharge template pour les pointeurs
template <typename T>  
void traiter(T* val) {  
    std::print("template pointeur: {}\n", *val);
}

// (3) Spécialisation totale du template (1) pour int*
template <>  
void traiter<int*>(int* val) {  
    std::print("specialisation int*: {}\n", *val);
}
```

```cpp
int x = 42;  
traiter(&x);  // Appelle (2), PAS (3) !  
```

La spécialisation (3) est rattachée au template (1). Or, lors de la résolution de surcharge, le compilateur compare (1) et (2) en tant que templates et choisit (2) comme meilleure correspondance pour un pointeur. Les spécialisations de (1) ne sont même pas considérées. La spécialisation (3) est silencieusement ignorée.

> **Règle pratique** — Pour les templates de fonctions, privilégiez systématiquement la **surcharge** plutôt que la spécialisation totale. Réservez la spécialisation aux templates de classes, où elle est le mécanisme naturel et sans ambiguïté.

---

## Spécialisation partielle

La **spécialisation partielle** (*partial specialization*) permet de fournir une implémentation alternative pour une **famille de types** partageant une caractéristique structurelle commune, tout en conservant un ou plusieurs paramètres template libres.

La spécialisation partielle n'est disponible que pour les **templates de classes** (et les templates de variables depuis C++14). Elle n'existe pas pour les templates de fonctions.

### Syntaxe

On écrit `template <...>` avec les paramètres encore libres, et on fixe partiellement les paramètres dans la liste d'arguments de la classe :

```cpp
// Template principal : conteneur générique avec sérialisation
template <typename T>  
class Serializer {  
public:  
    static std::string to_string(const T& valeur) {
        return std::format("{}", valeur);
    }
};

// Spécialisation partielle pour tous les types pointeur
template <typename T>  
class Serializer<T*> {  
public:  
    static std::string to_string(T* ptr) {
        if (ptr == nullptr) {
            return "null";
        }
        // Sérialise la valeur pointée, pas l'adresse
        return std::format("*{}", *ptr);
    }
};
```

```cpp
int x = 42;  
double y = 3.14;  

std::print("{}\n", Serializer<int>::to_string(x));       // "42" — template principal  
std::print("{}\n", Serializer<int*>::to_string(&x));     // "*42" — spécialisation T*  
std::print("{}\n", Serializer<double*>::to_string(&y));  // "*3.14" — spécialisation T*  
std::print("{}\n", Serializer<double*>::to_string(nullptr)); // "null"  
```

Le paramètre `T` reste libre dans la spécialisation : `Serializer<T*>` s'applique à `int*`, `double*`, `std::string*`, etc. Le compilateur décompose le type fourni et déduit `T` automatiquement.

### Formes courantes de spécialisation partielle

La spécialisation partielle peut matcher différentes structures de types :

```cpp
// Spécialisation pour les références
template <typename T>  
class Serializer<T&> {  
    // ...
};

// Spécialisation pour les tableaux C de taille connue
template <typename T, std::size_t N>  
class Serializer<T[N]> {  
    // ...
};

// Spécialisation pour les paires
template <typename K, typename V>  
class Serializer<std::pair<K, V>> {  
public:  
    static std::string to_string(const std::pair<K, V>& p) {
        return std::format("({}, {})",
            Serializer<K>::to_string(p.first),
            Serializer<V>::to_string(p.second));
    }
};

// Spécialisation pour les vecteurs
template <typename T>  
class Serializer<std::vector<T>> {  
public:  
    static std::string to_string(const std::vector<T>& vec) {
        std::string result = "[";
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) result += ", ";
            result += Serializer<T>::to_string(vec[i]);
        }
        return result + "]";
    }
};
```

```cpp
std::pair<std::string, int> p{"Alice", 30};  
std::print("{}\n", Serializer<decltype(p)>::to_string(p));  
// "(Alice, 30)"

std::vector<int> v{1, 2, 3};  
std::print("{}\n", Serializer<decltype(v)>::to_string(v));  
// "[1, 2, 3]"
```

Notez que la spécialisation pour `std::vector<T>` appelle récursivement `Serializer<T>` pour chaque élément. Si `T` est `int*`, c'est la spécialisation `Serializer<T*>` qui sera utilisée pour chaque élément. Le compilateur compose automatiquement les spécialisations.

---

## Spécialisation partielle avec paramètres non-type

La spécialisation partielle peut également fixer un paramètre non-type tout en laissant le type libre (ou l'inverse) :

```cpp
// Template principal
template <typename T, std::size_t N>  
class FixedBuffer {  
public:  
    void info() const {
        std::print("FixedBuffer<T, {}> generique\n", N);
    }
private:
    T data_[N];
};

// Spécialisation partielle : buffer de taille 1 (cas dégénéré, optimisable)
template <typename T>  
class FixedBuffer<T, 1> {  
public:  
    void info() const {
        std::print("FixedBuffer<T, 1> optimise (element unique)\n");
    }

    void set(const T& val) { element_ = val; }
    const T& get() const { return element_; }

private:
    T element_;  // Pas besoin d'un tableau pour un seul élément
};

// Spécialisation partielle : tout buffer de bool (représentation compacte)
template <std::size_t N>  
class FixedBuffer<bool, N> {  
public:  
    void info() const {
        std::print("FixedBuffer<bool, {}> compact (bitset)\n", N);
    }
private:
    std::bitset<N> bits_;  // 1 bit par bool au lieu de 1 octet
};
```

```cpp
FixedBuffer<int, 100> a;    // Template principal  
a.info();                    // "FixedBuffer<T, 100> generique"  

FixedBuffer<double, 1> b;   // Spécialisation T, 1  
b.info();                    // "FixedBuffer<T, 1> optimise (element unique)"  

FixedBuffer<bool, 64> c;    // Spécialisation bool, N  
c.info();                    // "FixedBuffer<bool, 64> compact (bitset)"  
```

### Ambiguïté de spécialisation

Que se passe-t-il pour `FixedBuffer<bool, 1>` ? Deux spécialisations partielles correspondent : `FixedBuffer<T, 1>` et `FixedBuffer<bool, N>`. Aucune n'est plus spécifique que l'autre. Le compilateur signale une **ambiguïté** :

```cpp
FixedBuffer<bool, 1> d;  // ERREUR : ambiguïté entre <T, 1> et <bool, N>
```

Pour résoudre cette ambiguïté, il faut fournir une spécialisation qui couvre exactement ce cas :

```cpp
// Spécialisation totale : résout l'ambiguïté <bool, 1>
template <>  
class FixedBuffer<bool, 1> {  
public:  
    void info() const {
        std::print("FixedBuffer<bool, 1> cas special\n");
    }
private:
    bool flag_;
};
```

La spécialisation totale est toujours plus spécifique que toute spécialisation partielle. L'ambiguïté disparaît.

---

## Règles de sélection du compilateur

Lorsque le compilateur rencontre une instanciation comme `Serializer<int*>`, il suit un processus de sélection précis :

**Étape 1 — Identifier les candidats.** Le compilateur considère le template principal et toutes ses spécialisations (totales et partielles).

**Étape 2 — Éliminer les non-correspondances.** Chaque candidat est testé : les arguments template fournis sont-ils compatibles avec le pattern de la spécialisation ? `Serializer<T*>` correspond à `int*` (avec `T = int`), mais `Serializer<std::pair<K,V>>` ne correspond pas.

**Étape 3 — Sélectionner le plus spécifique.** Parmi les candidats restants, le compilateur choisit celui dont le pattern est le **plus spécifique** (*most specialized*). L'ordre de spécificité est :

1. **Spécialisation totale** (`template <>`) — toujours préférée si elle correspond.
2. **Spécialisation partielle** — préférée au template principal si elle correspond.
3. **Template principal** — utilisé si aucune spécialisation ne correspond.

Si deux spécialisations partielles sont également applicables et qu'aucune n'est plus spécifique que l'autre, le compilateur signale une erreur d'ambiguïté.

Illustrons avec un tableau de résolution pour notre `Serializer` :

| Type instancié | Template principal | `Serializer<T*>` | `Serializer<bool>` | Sélectionné |
|---|---|---|---|---|
| `int` | ✓ | ✗ | ✗ | **Principal** |
| `double*` | ✓ | ✓ (T=double) | ✗ | **`T*`** (plus spécifique) |
| `bool` | ✓ | ✗ | ✓ | **`bool`** (total > principal) |
| `bool*` | ✓ | ✓ (T=bool) | ✗ | **`T*`** |

Le cas `bool*` est intéressant : la spécialisation totale `Serializer<bool>` ne correspond pas (le type est `bool*`, pas `bool`), donc c'est la spécialisation partielle `Serializer<T*>` qui est choisie.

---

## Application : traits de type

L'un des usages les plus puissants de la spécialisation est la création de **traits de type** (*type traits*) — des templates qui extraient des informations sur un type à la compilation. La STL en fournit une vaste collection dans `<type_traits>`, et toutes reposent sur la spécialisation.

Voici comment implémenter un trait `is_pointer` simplifié :

```cpp
// Template principal : par défaut, T n'est pas un pointeur
template <typename T>  
struct is_pointer {  
    static constexpr bool value = false;
};

// Spécialisation partielle : tout type T* est un pointeur
template <typename T>  
struct is_pointer<T*> {  
    static constexpr bool value = true;
};

// Alias pratique (convention de la STL)
template <typename T>  
constexpr bool is_pointer_v = is_pointer<T>::value;  
```

```cpp
static_assert(!is_pointer_v<int>);        // int n'est pas un pointeur  
static_assert(is_pointer_v<int*>);        // int* est un pointeur  
static_assert(is_pointer_v<double*>);     // double* est un pointeur  
static_assert(!is_pointer_v<int&>);       // int& n'est pas un pointeur  
```

Les `static_assert` vérifient les résultats **à la compilation**. Aucun code n'est exécuté ; tout est résolu par le compilateur grâce au mécanisme de spécialisation.

### Un trait plus élaboré : `remove_const`

Le trait `std::remove_const` retire le qualificateur `const` d'un type. Son implémentation repose sur une spécialisation partielle minimale :

```cpp
// Template principal : T n'a pas de const à retirer, on renvoie T tel quel
template <typename T>  
struct remove_const {  
    using type = T;
};

// Spécialisation partielle : si T est const U, on renvoie U
template <typename T>  
struct remove_const<const T> {  
    using type = T;
};

// Alias pratique
template <typename T>  
using remove_const_t = typename remove_const<T>::type;  
```

```cpp
static_assert(std::is_same_v<remove_const_t<const int>, int>);  
static_assert(std::is_same_v<remove_const_t<int>, int>);  
static_assert(std::is_same_v<remove_const_t<const double*>, const double*>);  
// Attention : const double* est un pointeur vers const double, le const
// qualifie double, pas le pointeur lui-même. Le pointeur n'est pas const.
```

Le dernier cas est un piège classique. `const double*` est `pointer to const double`, pas `const pointer to double`. Le `const` ne qualifie pas le type de premier niveau (le pointeur), donc `remove_const` n'a rien à retirer.

---

## Spécialisation et la Rule of Five

Lorsque vous spécialisez un template de classe, la spécialisation est une classe entièrement indépendante. Elle peut avoir une disposition mémoire, des membres et des invariants totalement différents du template principal. Cela signifie que la **Rule of Five** (section 6.5) doit être évaluée indépendamment pour chaque spécialisation.

Si le template principal gère sa mémoire avec des smart pointers et peut se contenter des opérations par défaut (`= default`), une spécialisation qui utilise un pointeur brut pour des raisons de performance devra probablement définir explicitement les cinq opérations spéciales.

---

## Le cas `std::vector<bool>` : leçon d'une spécialisation controversée

La STL contient une spécialisation partielle célèbre : `std::vector<bool>`. Plutôt que de stocker un `bool` par octet comme le ferait le template principal, cette spécialisation compacte les booléens en bits individuels pour économiser la mémoire.

Cette optimisation a un coût : `std::vector<bool>` ne se comporte pas comme un vrai conteneur. L'opérateur `[]` ne retourne pas une référence (`bool&`) mais un **objet proxy** qui simule une référence. Ce comportement casse de nombreuses attentes :

```cpp
std::vector<bool> flags{true, false, true};

auto& ref = flags[0];   // ERREUR ou comportement inattendu :
                         // flags[0] retourne un proxy temporaire, pas un bool&

// Comparaison avec un vector<int> normal
std::vector<int> nums{1, 2, 3};  
auto& r = nums[0];      // OK : retourne int&  
```

Cette spécialisation est largement considérée comme une erreur de conception dans la STL. Elle illustre un risque important de la spécialisation : si une spécialisation altère le **contrat** attendu par les utilisateurs du template principal, elle peut provoquer des bugs subtils et détruire la généricité.

> **Leçon** — Une spécialisation devrait fournir la même interface et respecter le même contrat que le template principal, même si l'implémentation interne diffère radicalement. Si le contrat doit changer, il est préférable de créer un type séparé (comme `std::bitset` ou `boost::dynamic_bitset`) plutôt qu'une spécialisation.

---

## Spécialisation de membres individuels

Plutôt que de spécialiser la classe entière (et de devoir réécrire tous ses membres), il est possible de spécialiser une **méthode individuelle** du template. Cela permet de ne modifier que le comportement spécifique tout en conservant le reste de l'implémentation générique :

```cpp
template <typename T>  
class Formatter {  
public:  
    void format(const T& valeur) const {
        std::print("[generic] {}\n", valeur);
    }

    void banner() const {
        std::print("=== Formatter ===\n");
    }
};

// Spécialisation de la méthode format() uniquement, pour bool
template <>  
void Formatter<bool>::format(const bool& valeur) const {  
    std::print("[bool] {}\n", valeur ? "OUI" : "NON");
}
```

```cpp
Formatter<int> fi;  
fi.banner();           // "=== Formatter ===" — méthode générique  
fi.format(42);         // "[generic] 42" — méthode générique  

Formatter<bool> fb;  
fb.banner();           // "=== Formatter ===" — méthode générique conservée  
fb.format(true);       // "[bool] OUI" — méthode spécialisée  
```

Seule la méthode `format` est redéfinie pour `bool`. La méthode `banner` reste celle du template principal. C'est un avantage considérable par rapport à la spécialisation de la classe entière, qui aurait exigé de redéfinir les deux méthodes.

Cette technique n'est possible que pour la **spécialisation totale** d'une méthode. La spécialisation partielle de membres individuels n'existe pas.

---

## Organisation du code

La spécialisation suit les mêmes règles de visibilité que les templates : les définitions doivent être visibles au point d'instanciation. En pratique, cela signifie que les spécialisations sont placées dans le même header que le template principal, ou dans un header inclus juste après.

Un pattern d'organisation courant :

```
include/
  serializer.h            ← Template principal
  serializer_bool.h       ← Spécialisation totale pour bool
  serializer_pointers.h   ← Spécialisation partielle pour T*
  serializer_containers.h ← Spécialisations pour vector<T>, pair<K,V>, etc.
```

Le header principal inclut les spécialisations à la fin :

```cpp
// serializer.h
#pragma once

// Template principal
template <typename T>  
class Serializer { /* ... */ };  

// Inclure les spécialisations
#include "serializer_bool.h"
#include "serializer_pointers.h"
#include "serializer_containers.h"
```

Ce découpage évite un header monolithique tout en garantissant que toutes les spécialisations sont visibles partout où le template principal est utilisé.

> **Attention** — Si une spécialisation n'est pas visible au point d'instanciation, le compilateur utilise silencieusement le template principal. Il n'y a ni erreur ni avertissement. C'est un bug subtil qui peut passer inaperçu longtemps.

---

## Spécialisation vs `if constexpr` : deux philosophies

Depuis C++17, `if constexpr` offre une alternative à la spécialisation pour adapter le comportement en fonction du type :

```cpp
// Approche par spécialisation
template <typename T>  
class Serializer { /* version générique */ };  

template <>  
class Serializer<bool> { /* version bool */ };  

template <typename T>  
class Serializer<T*> { /* version pointeur */ };  


// Approche par if constexpr (tout dans le template principal)
template <typename T>  
class Serializer {  
public:  
    static std::string to_string(const T& valeur) {
        if constexpr (std::is_same_v<T, bool>) {
            return valeur ? "true" : "false";
        } else if constexpr (std::is_pointer_v<T>) {
            if (valeur == nullptr) return "null";
            return std::format("*{}", *valeur);
        } else {
            return std::format("{}", valeur);
        }
    }
};
```

Chaque approche a ses mérites :

| Critère | Spécialisation | `if constexpr` |
|---|---|---|
| **Lisibilité** | Claire pour peu de cas | Plus compacte, tout en un endroit |
| **Extensibilité** | Ouverte : on ajoute une spécialisation sans modifier l'existant | Fermée : il faut modifier la fonction |
| **Données membres** | Chaque spécialisation peut avoir des membres différents | Les membres sont communs |
| **Complexité** | Peut devenir verbeuse (réécriture de la classe entière) | Devient confuse si trop de branches |

En règle générale, `if constexpr` convient pour des **variations de comportement mineures** au sein d'une même structure. La spécialisation est préférable quand les types nécessitent des **structures de données ou des interfaces fondamentalement différentes**.

---

## Bonnes pratiques

**Ne spécialisez que lorsque c'est nécessaire.** Le template principal devrait couvrir le cas le plus courant. La spécialisation est un mécanisme d'exception, pas la norme.

**Respectez le contrat du template principal.** Une spécialisation qui change l'interface publique ou les invariants brise la généricité et crée des bugs subtils pour les utilisateurs.

**Préférez la surcharge à la spécialisation pour les fonctions.** Pour les templates de fonctions, la surcharge est plus prévisible et interagit mieux avec la résolution de surcharge.

**Résolvez les ambiguïtés explicitement.** Si deux spécialisations partielles se chevauchent, ajoutez une spécialisation totale ou reformulez les patterns pour éliminer le recouvrement.

**Assurez la visibilité des spécialisations.** Incluez toujours les spécialisations depuis le même header que le template principal. Une spécialisation invisible est un bug silencieux.

**Envisagez les Concepts (C++20) comme alternative.** Les Concepts (section 16.6) permettent souvent d'exprimer des contraintes et des dispatches de manière plus lisible que la spécialisation partielle combinée à SFINAE. Pour du code nouveau en C++20, c'est l'approche à privilégier.

---

## En résumé

La spécialisation est le mécanisme qui permet d'adapter un template à des types ou des familles de types spécifiques. La **spécialisation totale** fixe tous les paramètres et fournit une implémentation entièrement dédiée. La **spécialisation partielle** — disponible uniquement pour les classes — fixe une partie des paramètres ou contraint leur forme structurelle (pointeur, référence, conteneur, etc.).

Le compilateur sélectionne toujours la version la plus spécifique applicable : spécialisation totale d'abord, partielle ensuite, template principal en dernier recours. Ce mécanisme est le fondement des traits de type de la STL et de nombreux patterns de métaprogrammation.

La section suivante (16.4) explore **SFINAE**, un mécanisme plus subtil qui exploite les échecs de substitution pour activer ou désactiver des surcharges et des spécialisations de manière conditionnelle.

⏭️ [SFINAE (Substitution Failure Is Not An Error)](/16-templates-metaprogrammation/04-sfinae.md)
