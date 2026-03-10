🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 8.5 — Opérateur spaceship `<=>` (C++20)

## Chapitre 8 : Surcharge d'Opérateurs et Conversions · Module 3 : Programmation Orientée Objet

---

## Introduction

Avant C++20, définir les opérateurs de comparaison pour un type utilisateur était un exercice fastidieux et fragile. Pour un type comparable, il fallait implémenter **six opérateurs** (`==`, `!=`, `<`, `>`, `<=`, `>=`), généralement en définissant `==` et `<` puis en dérivant les quatre autres — le tout en espérant ne pas introduire d'incohérence par copier-coller (section 8.1).

C++20 a résolu ce problème de manière élégante avec deux innovations complémentaires :

- L'**opérateur de comparaison tripartite** `<=>`, surnommé *"spaceship operator"* en raison de la ressemblance de `<=>` avec un vaisseau spatial vu de profil. Une seule implémentation de `<=>` permet au compilateur de synthétiser automatiquement `<`, `>`, `<=` et `>=`.
- La **réécriture automatique de `operator==`** : à partir d'un `operator==`, le compilateur génère automatiquement `operator!=`.

Ensemble, ces deux mécanismes permettent de réduire six opérateurs à **une ou deux déclarations**, tout en garantissant la cohérence mathématique entre les opérateurs.

---

## Le concept : comparaison tripartite

La comparaison tripartite (*three-way comparison*) est un concept bien connu en informatique. Au lieu de retourner un booléen (`true`/`false`), elle retourne un résultat à **trois valeurs** : inférieur, égal ou supérieur. C'est l'équivalent de la fonction `strcmp` en C, ou de `compareTo` en Java.

En C++20, `a <=> b` retourne un objet dont le type encode la **catégorie de comparaison** et dont la valeur encode le **résultat** :

```cpp
#include <compare>

int a = 3, b = 5;  
auto resultat = a <=> b;  

if (resultat < 0)  std::println("a < b");     // ✅ — ce cas  
if (resultat == 0) std::println("a == b");  
if (resultat > 0)  std::println("a > b");  
```

Le résultat de `<=>` n'est pas un `int` — c'est un type spécial défini dans `<compare>`. Le type exact dépend de la **catégorie de comparaison** des opérandes.

---

## Les trois catégories de comparaison

C++20 définit trois types de résultat pour `<=>`, correspondant à trois niveaux de rigueur mathématique pour la relation d'ordre :

### `std::strong_ordering`

L'ordre total strict. Deux valeurs sont soit strictement ordonnées, soit **égales** (indiscernables — substituables l'une à l'autre). C'est la catégorie la plus forte.

Valeurs possibles : `strong_ordering::less`, `strong_ordering::equal`, `strong_ordering::greater`.

```cpp
// Les entiers ont un strong_ordering
auto r = 42 <=> 17;   // std::strong_ordering::greater  
static_assert(std::is_same_v<decltype(r), std::strong_ordering>);  
```

Propriété fondamentale : si `a <=> b == 0`, alors `a` et `b` sont **interchangeables** dans tout contexte observable. C'est ce qui distingue `equal` de `equivalent`.

Types de la STL avec `strong_ordering` : `int`, `char`, `long`, `std::string`, `std::vector<T>` (si `T` a un `strong_ordering`).

### `std::weak_ordering`

L'ordre total avec **équivalence**. Deux valeurs peuvent être "équivalentes" sans être "égales" — c'est-à-dire qu'elles sont au même rang dans l'ordre, mais elles peuvent différer sur des aspects non pertinents pour la comparaison.

Valeurs possibles : `weak_ordering::less`, `weak_ordering::equivalent`, `weak_ordering::greater`.

```cpp
class CaseInsensitiveString {
    std::string data_;
public:
    explicit CaseInsensitiveString(std::string s) : data_{std::move(s)} {}

    std::weak_ordering operator<=>(CaseInsensitiveString const& rhs) const {
        // "Hello" et "HELLO" sont équivalents mais pas égaux
        auto cmp = [](char a, char b) {
            return std::tolower(a) <=> std::tolower(b);
        };
        return std::lexicographical_compare_three_way(
            data_.begin(), data_.end(),
            rhs.data_.begin(), rhs.data_.end(), cmp);
    }

    bool operator==(CaseInsensitiveString const& rhs) const {
        // Cohérent avec <=> : comparaison insensible à la casse
        return (*this <=> rhs) == 0;
    }
};
```

`"Hello"` et `"HELLO"` sont `equivalent` (même rang dans le tri) mais pas `equal` (chaînes de caractères différentes). C'est un `weak_ordering`.

### `std::partial_ordering`

L'ordre partiel. Certaines paires de valeurs peuvent être **incomparables** — ni l'une inférieure à l'autre, ni équivalentes.

Valeurs possibles : `partial_ordering::less`, `partial_ordering::equivalent`, `partial_ordering::greater`, `partial_ordering::unordered`.

```cpp
// Les flottants ont un partial_ordering à cause de NaN
double a = 1.0, b = std::numeric_limits<double>::quiet_NaN();  
auto r = a <=> b;   // std::partial_ordering::unordered  

// NaN n'est ni <, ni ==, ni > à quoi que ce soit
```

`NaN` est incomparable avec toute valeur, y compris avec lui-même. C'est pourquoi `double` a un `partial_ordering` et non un `strong_ordering`.

### Hiérarchie des catégories

Les trois catégories forment une hiérarchie. Un type plus fort peut être implicitement converti vers un type plus faible :

```
strong_ordering  →  weak_ordering  →  partial_ordering
 (le plus fort)                       (le plus faible)
```

Si un type a un `strong_ordering`, il satisfait aussi les exigences d'un `weak_ordering` et d'un `partial_ordering`. L'inverse n'est pas vrai.

### Tableau récapitulatif

| Catégorie | `a == b` implique | Incomparabilité possible | Exemple |
|---|---|---|---|
| `strong_ordering` | `a` et `b` interchangeables | Non | `int`, `std::string` |
| `weak_ordering` | `a` et `b` équivalents (même rang) | Non | Tri case-insensitive |
| `partial_ordering` | `a` et `b` équivalents | Oui (`unordered`) | `double` (NaN) |

---

## `= default` : la comparaison automatique

Pour les types dont la comparaison est une **comparaison membre par membre** (lexicographique sur les membres dans leur ordre de déclaration), une seule ligne suffit :

```cpp
#include <compare>
#include <string>

class Etudiant {
    std::string nom_;
    std::string prenom_;
    int annee_naissance_;

public:
    Etudiant(std::string nom, std::string prenom, int annee)
        : nom_{std::move(nom)}, prenom_{std::move(prenom)}, annee_naissance_{annee} {}

    auto operator<=>(Etudiant const&) const = default;
    bool operator==(Etudiant const&) const = default;
};
```

Avec ces deux déclarations, les **six opérateurs** sont disponibles :

```cpp
Etudiant a{"Dupont", "Alice", 2001};  
Etudiant b{"Dupont", "Bob", 1999};  
Etudiant c{"Dupont", "Alice", 2001};  

std::println("{}", a == c);    // true  
std::println("{}", a != b);    // true  
std::println("{}", a < b);     // true  (même nom, "Alice" < "Bob")  
std::println("{}", a >= c);    // true  
```

### Comment fonctionne `= default` ?

Le compilateur génère une comparaison **lexicographique** sur les membres, dans leur **ordre de déclaration** :

1. Compare `nom_` de `*this` avec `nom_` de `rhs`.
2. Si différent, retourne le résultat.
3. Sinon, compare `prenom_`.
4. Si différent, retourne le résultat.
5. Sinon, compare `annee_naissance_`.
6. Retourne le résultat.

C'est exactement ce qu'on écrirait manuellement, mais sans risque d'erreur. Le type de retour de `auto operator<=>(...) const = default` est déterminé par le **type commun le plus fort** parmi les `<=>` de tous les membres. Si tous les membres ont un `strong_ordering`, le résultat est `strong_ordering`. Si un membre a un `partial_ordering` (par exemple un `double`), le résultat est rétrogradé en `partial_ordering`.

### Pourquoi déclarer `operator==` séparément ?

En C++20, `operator<=>` defaulté génère `<`, `>`, `<=`, `>=` — mais **pas** `==` et `!=`. La raison est une question de performance : pour beaucoup de types (comme `std::string` ou `std::vector`), le test d'égalité peut être optimisé (comparaison des tailles d'abord, court-circuit immédiat) alors que `<=>` doit calculer l'ordre complet. Si `==` était déduit de `<=>`, on perdrait cette optimisation.

C'est pourquoi la pratique recommandée est de déclarer **les deux** :

```cpp
auto operator<=>(T const&) const = default;   // génère <, >, <=, >=  
bool operator==(T const&) const = default;    // génère ==, !=  
```

> 💡 Si vous ne déclarez que `operator<=>` defaulté sans `operator==`, le compilateur génère quand même un `operator==` defaulté **implicitement**. Mais déclarer les deux rend l'intention explicite et garantit la clarté du code.

---

## Implémentation personnalisée

Quand `= default` ne suffit pas — par exemple quand certains membres doivent être ignorés, quand l'ordre est personnalisé, ou quand la comparaison nécessite un calcul — on écrit `operator<=>` manuellement :

### Ignorer certains membres

```cpp
class Enregistrement {
    int id_;                 // clé de comparaison
    std::string contenu_;    // clé de comparaison
    uint64_t timestamp_;     // PAS une clé de comparaison (métadonnée)

public:
    Enregistrement(int id, std::string contenu, uint64_t ts)
        : id_{id}, contenu_{std::move(contenu)}, timestamp_{ts} {}

    std::strong_ordering operator<=>(Enregistrement const& rhs) const {
        if (auto cmp = id_ <=> rhs.id_; cmp != 0) return cmp;
        return contenu_ <=> rhs.contenu_;
        // timestamp_ est délibérément ignoré
    }

    bool operator==(Enregistrement const& rhs) const {
        return id_ == rhs.id_ && contenu_ == rhs.contenu_;
        // cohérent avec <=> : timestamp_ ignoré
    }
};
```

Deux enregistrements avec le même `id_` et le même `contenu_` sont considérés égaux, même si leurs timestamps diffèrent. Ce choix fait de `timestamp_` un aspect non observable pour la comparaison — d'où `strong_ordering` (les valeurs "égales" restent substituables pour tout ce qui concerne l'identité de l'enregistrement).

### Pattern d'enchaînement idiomatique

Le pattern pour comparer plusieurs membres séquentiellement utilise le test `cmp != 0` :

```cpp
std::strong_ordering operator<=>(Autre const& rhs) const {
    if (auto cmp = membre1_ <=> rhs.membre1_; cmp != 0) return cmp;
    if (auto cmp = membre2_ <=> rhs.membre2_; cmp != 0) return cmp;
    if (auto cmp = membre3_ <=> rhs.membre3_; cmp != 0) return cmp;
    return membre4_ <=> rhs.membre4_;
}
```

Chaque membre est comparé à tour de rôle. Dès qu'une différence est trouvée, le résultat est retourné. Si tous les membres précédents sont égaux, c'est le dernier qui départage. C'est la comparaison lexicographique — exactement ce que `= default` génère, mais avec un contrôle total sur quels membres sont comparés et dans quel ordre.

### Comparaison avec transformation

Parfois, la comparaison porte sur une **valeur dérivée** plutôt que sur les membres bruts :

```cpp
class Angle {
    double degres_;   // stocké en degrés, peut être > 360

public:
    explicit Angle(double deg) : degres_{deg} {}

    // Normaliser avant de comparer
    std::partial_ordering operator<=>(Angle const& rhs) const {
        auto norm = [](double d) { return std::fmod(d, 360.0); };
        return norm(degres_) <=> norm(rhs.degres_);
    }

    bool operator==(Angle const& rhs) const {
        return (*this <=> rhs) == 0;
    }
};
```

Ici, `Angle{30}` et `Angle{390}` sont considérés équivalents. Le type de retour est `partial_ordering` car `double` a un `partial_ordering` (à cause de NaN).

---

## Comparaison avec des types différents (hétérogène)

`operator<=>` peut comparer un objet avec un type **différent**. C'est courant pour les comparaisons avec des types primitifs ou des string literals :

```cpp
class Version {
    int majeure_, mineure_, patch_;

public:
    Version(int maj, int min, int pat)
        : majeure_{maj}, mineure_{min}, patch_{pat} {}

    // Comparaison Version <=> Version
    std::strong_ordering operator<=>(Version const&) const = default;
    bool operator==(Version const&) const = default;

    // Comparaison Version <=> int (compare uniquement la version majeure)
    std::strong_ordering operator<=>(int majeure) const {
        return majeure_ <=> majeure;
    }

    bool operator==(int majeure) const {
        return majeure_ == majeure;
    }
};
```

```cpp
Version v{2, 1, 3};

std::println("{}", v > Version{1, 9, 9});   // true  (2.1.3 > 1.9.9)  
std::println("{}", v < 3);                   // true  (majeure 2 < 3)  
std::println("{}", 1 < v);                   // true  — réécriture automatique !  
```

### La réécriture automatique

Le dernier appel `1 < v` est remarquable. Nous n'avons pas défini `operator<(int, Version)`. Le compilateur C++20 **réécrit** l'expression :

- `1 < v` est réécrit comme `(v <=> 1) > 0` (en inversant les opérandes et le sens de la comparaison).
- `v <=> 1` appelle `Version::operator<=>(int)`.

Cette réécriture fonctionne pour les six opérateurs (`==`, `!=`, `<`, `>`, `<=`, `>=`). C'est ce qui rend la comparaison hétérogène praticable avec un minimum de code — une seule implémentation de `operator<=>` et `operator==` suffit pour les deux sens.

---

## Interaction entre `<=>` et les opérateurs classiques

### Priorité de résolution

En C++20, quand le compilateur doit résoudre `a < b`, il considère dans l'ordre :

1. Un `operator<(A, B)` classique (défini explicitement).
2. Une réécriture via `(a <=> b) < 0`.
3. Une réécriture via `0 < (b <=> a)` (opérandes inversés).

Si un `operator<` classique existe, il est **prioritaire** sur la réécriture via `<=>`. Cela garantit la rétrocompatibilité avec le code pré-C++20.

### Migration progressive

Cette priorité permet une migration progressive. Vous pouvez ajouter `operator<=>` à un type qui possède déjà des opérateurs classiques. Les opérateurs existants continuent de fonctionner. Vous pouvez ensuite supprimer les opérateurs classiques un par un, en laissant `<=>` prendre le relais.

```cpp
class MonType {
    int valeur_;
public:
    // Ancien code — toujours fonctionnel
    bool operator<(MonType const& rhs) const { return valeur_ < rhs.valeur_; }

    // Nouveau — génère >, <=, >= via <=>
    // < continue d'utiliser l'opérateur classique (prioritaire)
    auto operator<=>(MonType const&) const = default;
    bool operator==(MonType const&) const = default;
};
```

---

## `<=>` et les conteneurs STL

L'opérateur spaceship s'intègre naturellement avec la STL. Les conteneurs ordonnés (`std::map`, `std::set`) nécessitent un `operator<`, que `<=>` génère automatiquement. Les conteneurs non ordonnés (`std::unordered_map`) nécessitent un `operator==` et un `std::hash`, que `<=>` ne fournit pas (le hash n'est pas lié à l'ordre).

```cpp
#include <set>
#include <map>

class Cle {
    int a_, b_;
public:
    Cle(int a, int b) : a_{a}, b_{b} {}

    auto operator<=>(Cle const&) const = default;
    bool operator==(Cle const&) const = default;
};

std::set<Cle> ensemble;                    // ✅ utilise operator< (via <=>)  
std::map<Cle, std::string> dictionnaire;   // ✅ utilise operator< (via <=>)  

ensemble.insert(Cle{1, 2});  
ensemble.insert(Cle{1, 3});  
ensemble.insert(Cle{1, 2});   // doublon — pas inséré  
// ensemble contient {1,2} et {1,3}
```

Pour les `std::flat_map` et `std::flat_set` (C++23, section 12.9), le comportement est identique.

---

## Quand ne pas utiliser `= default`

Le `= default` n'est pas toujours approprié :

**Quand certains membres ne doivent pas participer à la comparaison** — caches internes, compteurs de debug, timestamps de création. Il faut alors écrire `<=>` manuellement et sélectionner les membres pertinents.

**Quand l'ordre des membres dans la classe ne correspond pas à l'ordre de comparaison souhaité.** Le `= default` compare dans l'ordre de déclaration. Si vous voulez comparer par `prenom_` avant `nom_` mais que `nom_` est déclaré en premier, il faut écrire l'opérateur manuellement.

**Quand la comparaison nécessite une normalisation** — angles modulo 360°, chaînes en minuscules, chemins de fichiers canoniques.

**Quand le type contient des pointeurs bruts.** Le `= default` comparerait les **adresses** (comparaison de pointeurs), pas les **valeurs pointées**. C'est rarement le comportement attendu.

**Quand vous avez besoin d'un `weak_ordering` mais que tous les membres ont un `strong_ordering`.** Le `= default` choisira `strong_ordering`, ce qui peut ne pas être sémantiquement correct si deux objets "égaux" selon votre comparaison ne sont pas réellement interchangeables.

---

## Exemple complet : type `Date`

```cpp
#include <compare>
#include <format>
#include <iostream>

class Date {
    int annee_;
    int mois_;    // 1-12
    int jour_;    // 1-31

public:
    Date(int annee, int mois, int jour)
        : annee_{annee}, mois_{mois}, jour_{jour} {}

    // L'ordre de déclaration (annee, mois, jour) correspond
    // exactement à l'ordre lexicographique souhaité → = default suffit
    std::strong_ordering operator<=>(Date const&) const = default;
    bool operator==(Date const&) const = default;

    friend std::ostream& operator<<(std::ostream& os, Date const& d) {
        os << std::format("{:04d}-{:02d}-{:02d}", d.annee_, d.mois_, d.jour_);
        return os;
    }
};
```

```cpp
#include <algorithm>
#include <vector>

int main() {
    Date noel{2026, 12, 25};
    Date nouvel_an{2027, 1, 1};
    Date autre_noel{2026, 12, 25};

    std::cout << std::boolalpha;
    std::cout << noel << " < "  << nouvel_an  << "  → " << (noel < nouvel_an)  << "\n";  // true
    std::cout << noel << " == " << autre_noel  << " → " << (noel == autre_noel) << "\n";  // true
    std::cout << nouvel_an << " >= " << noel   << " → " << (nouvel_an >= noel)  << "\n";  // true
    std::cout << noel << " != " << nouvel_an   << " → " << (noel != nouvel_an)  << "\n";  // true

    // Fonctionne directement avec std::sort
    std::vector<Date> dates = {{2026, 3, 15}, {2025, 12, 1}, {2026, 3, 10}};
    std::sort(dates.begin(), dates.end());
    // dates = {2025-12-01, 2026-03-10, 2026-03-15}
}
```

L'ordre de déclaration des membres (`annee_`, `mois_`, `jour_`) correspond à l'ordre de comparaison naturel d'une date. C'est l'alignement parfait qui rend `= default` approprié. Si les membres étaient déclarés dans un autre ordre (par exemple `jour_`, `mois_`, `annee_`), le `= default` produirait un ordre incorrect et il faudrait écrire l'opérateur manuellement.

---

## Bonnes pratiques

**Utilisez `= default` chaque fois que la comparaison lexicographique par ordre de déclaration est correcte.** C'est le cas le plus fréquent. Déclarez vos membres dans l'ordre qui correspond à la comparaison souhaitée.

**Déclarez toujours `operator==` en plus de `operator<=>`**, même avec `= default`. Cela rend l'intention explicite et garantit que `==` peut être optimisé indépendamment de `<=>`.

**Choisissez la catégorie de retour la plus forte possible.** Utilisez `strong_ordering` quand les objets "égaux" sont interchangeables, `weak_ordering` quand ils sont "au même rang" sans être identiques, et `partial_ordering` uniquement quand l'incomparabilité est possible.

**Rendez `operator<=>` et `operator==` const.** La comparaison ne modifie jamais les opérandes. C'est une convention universelle.

**Vérifiez la cohérence entre `<=>` et `==` personnalisés.** Si vous écrivez les deux manuellement, assurez-vous que `(a <=> b) == 0` si et seulement si `a == b`. Une incohérence entre les deux rendra le comportement des conteneurs STL imprévisible.

**Migrez le code pré-C++20 progressivement.** Commencez par ajouter `operator<=>` et `operator==` defaultés. Les opérateurs classiques existants restent prioritaires et continuent de fonctionner. Supprimez-les ensuite un par un après vérification.

---

## Résumé

| Aspect | Pré-C++20 | C++20 avec `<=>` |
|---|---|---|
| Opérateurs à écrire | 6 (`==`, `!=`, `<`, `>`, `<=`, `>=`) | 2 (`<=>` + `==`) ou 0 (`= default`) |
| Cohérence garantie | Par discipline du développeur | Par le compilateur |
| Comparaison hétérogène | Surcharge pour chaque sens | Réécriture automatique |
| Catégorisation de l'ordre | Implicite (convention) | Explicite (`strong`, `weak`, `partial`) |

| Catégorie | Type | Signification de `cmp == 0` | `unordered` possible |
|---|---|---|---|
| `strong_ordering` | Ordre total strict | Égalité (substituable) | Non |
| `weak_ordering` | Ordre total avec équivalence | Équivalence (même rang) | Non |
| `partial_ordering` | Ordre partiel | Équivalence | Oui |

| Déclaration | Opérateurs générés |
|---|---|
| `auto operator<=>(T const&) const = default;` | `<`, `>`, `<=`, `>=` + `==`, `!=` (implicitement) |
| `bool operator==(T const&) const = default;` | `==`, `!=` |
| `auto operator<=>(T const&) const = default;` + `bool operator==(T const&) const = default;` | Les 6 — **forme recommandée** |

---

> - ↗️ Pour aller plus loin : [12.2 — std::optional, std::variant, std::any](/12-nouveautes-cpp17-26/02-optional-variant-any.md) · [14.1 — std::map et std::multimap](/14-conteneurs-associatifs/01-map-multimap.md)

⏭️ [PARTIE II : C++ MODERNE](/partie-02-cpp-moderne.md)
