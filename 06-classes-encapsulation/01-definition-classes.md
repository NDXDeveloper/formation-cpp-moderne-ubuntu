🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 6.1 — Définition de classes : Membres et méthodes

## Chapitre 6 : Classes et Encapsulation

---

## Ce que vous allez apprendre

- Déclarer une classe et instancier des objets.  
- Distinguer les **données membres** (attributs) et les **fonctions membres** (méthodes).  
- Comprendre la différence entre `struct` et `class` en C++.  
- Utiliser le pointeur implicite `this`.  
- Séparer déclaration (`.h`) et définition (`.cpp`).  
- Manipuler les qualificateurs `const` et `static` sur les membres.

---

## De la structure C à la classe C++

Si vous venez du C, vous connaissez les structures : un regroupement nommé de données.

```cpp
// En C — données et fonctions séparées
struct Sensor {
    int id;
    double value;
};

void sensor_init(struct Sensor* s, int id) {
    s->id = id;
    s->value = 0.0;
}

void sensor_read(struct Sensor* s, double new_value) {
    s->value = new_value;
}
```

Ce code fonctionne, mais il pose trois problèmes. Premièrement, rien ne lie `sensor_init` à `Sensor` — le compilateur ne vous empêchera pas d'utiliser un `Sensor` non initialisé. Deuxièmement, n'importe quel code peut modifier `id` ou `value` directement, sans passer par les fonctions prévues. Troisièmement, si un jour vous renommez un champ, vous devez traquer manuellement toutes les fonctions qui le manipulent.

En C++, la classe résout ces trois problèmes d'un coup :

```cpp
// En C++ — données et fonctions réunies
class Sensor {  
public:  
    void init(int id) {
        id_ = id;
        value_ = 0.0;
    }

    void read(double new_value) {
        value_ = new_value;
    }

    double value() const {
        return value_;
    }

private:
    int id_;
    double value_;
};
```

Les fonctions sont désormais **à l'intérieur** de la classe. Elles opèrent directement sur les données de l'objet, sans avoir besoin d'un pointeur explicite en paramètre. Les données sont marquées `private` : seul le code de la classe peut y accéder. Et le compilateur sait que `init`, `read` et `value` appartiennent à `Sensor` — le lien est explicite et vérifiable.

> 💡 Cet exemple utilise une méthode `init` pour l'initialisation. En pratique, on utilise un **constructeur** (section 6.2) qui garantit que l'objet est initialisé dès sa création. Nous y viendrons très vite.

---

## Anatomie d'une classe

Une classe se compose de deux catégories d'éléments : les **données membres** (aussi appelées attributs ou champs) et les **fonctions membres** (aussi appelées méthodes).

```cpp
class Rectangle {  
public:  
    // --- Fonctions membres (méthodes) ---
    double area() const {
        return width_ * height_;
    }

    double perimeter() const {
        return 2.0 * (width_ + height_);
    }

    void resize(double w, double h) {
        width_ = w;
        height_ = h;
    }

private:
    // --- Données membres (attributs) ---
    double width_ = 0.0;    // Initialisation en place (C++11)
    double height_ = 0.0;
};
```

### Données membres

Les données membres définissent l'**état** de l'objet. Chaque instance de `Rectangle` possède sa propre copie de `width_` et `height_`. Depuis C++11, vous pouvez leur donner une **valeur par défaut directement dans la déclaration** — c'est ce qu'on appelle un *default member initializer* (ou *in-class initializer*). Si aucun constructeur ne fournit de valeur explicite, c'est cette valeur par défaut qui sera utilisée.

```cpp
class Config {  
private:  
    int max_retries_ = 3;              // Entier avec valeur par défaut
    std::string host_ = "localhost";   // String avec valeur par défaut
    bool verbose_ = false;             // Booléen avec valeur par défaut
};
```

Cette pratique est recommandée par les *C++ Core Guidelines* (règle C.45) : elle centralise les valeurs par défaut et réduit le risque d'oublier d'initialiser un membre dans l'un des constructeurs.

### Fonctions membres

Les fonctions membres définissent le **comportement** de l'objet. Elles ont accès à toutes les données membres de l'instance sur laquelle elles sont appelées, y compris les membres privés.

```cpp
Rectangle r;  
r.resize(4.0, 3.0);           // Appel de méthode sur l'objet r  
std::cout << r.area() << "\n"; // Affiche 12  
```

Quand vous écrivez `r.area()`, le compilateur sait que `width_` et `height_` dans le corps de `area()` font référence aux données de `r`. Ce mécanisme repose sur le pointeur `this`, que nous verrons plus loin dans cette section.

---

## `struct` vs `class` : une seule différence

En C++, `struct` et `class` sont **quasiment identiques**. La seule différence tient à la **visibilité par défaut** :

- Dans une `struct`, les membres sont `public` par défaut.  
- Dans une `class`, les membres sont `private` par défaut.

```cpp
struct Point {
    double x;   // public par défaut
    double y;   // public par défaut
};

class Point2 {
    double x;   // private par défaut
    double y;   // private par défaut
};
```

Ces deux types sont strictement équivalents en termes de fonctionnalités : une `struct` peut avoir des constructeurs, des destructeurs, de l'héritage, des méthodes virtuelles — tout ce qu'une `class` peut faire.

La convention largement adoptée dans l'industrie est la suivante :

- **`struct`** pour les agrégats simples : des types dont tous les membres sont publics, sans invariant à protéger. Un `Point`, une `Color`, un `Config` de type POD (*Plain Old Data*).  
- **`class`** pour les types avec un invariant : dès qu'il y a des données privées, des constructeurs non triviaux ou un comportement à encapsuler.

```cpp
// struct — agrégat simple, pas d'invariant
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// class — invariant à protéger (la capacité doit être >= size)
class DynArray {  
public:  
    explicit DynArray(std::size_t size);
    // ...
private:
    int* data_;
    std::size_t size_;
};
```

> 💡 Ne choisissez pas `struct` ou `class` au hasard. La distinction est sémantique : elle signale au lecteur du code si le type a un invariant ou non.

---

## Instanciation d'objets

Une classe est un **plan**. Un objet est une **instance** concrète de ce plan, avec sa propre zone mémoire pour ses données membres.

```cpp
Rectangle a;                  // Objet sur la stack, valeurs par défaut (0.0, 0.0)  
a.resize(5.0, 2.0);  

Rectangle b;                  // Un autre objet, indépendant de a  
b.resize(10.0, 7.0);  

std::cout << a.area() << "\n"; // 10  
std::cout << b.area() << "\n"; // 70  
```

Chaque objet possède sa propre copie des données membres. Modifier `a` n'affecte pas `b`. En mémoire, `a` et `b` occupent chacun `sizeof(Rectangle)` octets sur la pile.

On peut aussi créer des objets sur le heap (nous verrons en section 6.3 pourquoi les smart pointers sont préférables) :

```cpp
Rectangle* p = new Rectangle;    // Allocation dynamique  
p->resize(3.0, 4.0);             // Accès via ->  
std::cout << p->area() << "\n";  // 12  
delete p;                         // Libération manuelle obligatoire  
```

Notez la syntaxe : l'opérateur `.` pour les objets, l'opérateur `->` pour les pointeurs vers des objets. Le `->` est un raccourci pour `(*p).area()`.

---

## Le pointeur `this`

À l'intérieur d'une fonction membre, le mot-clé `this` est un **pointeur vers l'objet courant**. Le compilateur le passe implicitement à chaque appel de méthode.

```cpp
class Counter {  
public:  
    void increment() {
        this->count_++;   // Explicite — rarement nécessaire
        count_++;          // Équivalent — forme usuelle
    }

    int count() const {
        return count_;
    }

private:
    int count_ = 0;
};
```

La plupart du temps, vous n'avez pas besoin d'écrire `this->` explicitement. Le compilateur résout les noms des membres automatiquement. Cependant, `this` devient utile dans trois situations concrètes.

### Lever une ambiguïté de nom

Si un paramètre de fonction porte le même nom qu'un attribut (ce qui arrive quand on n'utilise pas la convention du suffixe `_`), `this` permet de désambiguïser :

```cpp
class Widget {  
public:  
    void set_name(const std::string& name) {
        this->name = name;   // this->name = attribut, name = paramètre
    }
private:
    std::string name;
};
```

En pratique, la convention du suffixe underscore (`name_`) élimine ce problème. Si vous la suivez — et nous le recommandons — ce cas n'apparaît presque jamais.

### Retourner l'objet courant (chaînage de méthodes)

Le **method chaining** (ou interface fluide) consiste à retourner `*this` pour enchaîner les appels :

```cpp
class QueryBuilder {  
public:  
    QueryBuilder& select(const std::string& field) {
        query_ += "SELECT " + field + " ";
        return *this;
    }

    QueryBuilder& from(const std::string& table) {
        query_ += "FROM " + table + " ";
        return *this;
    }

    QueryBuilder& where(const std::string& condition) {
        query_ += "WHERE " + condition;
        return *this;
    }

    std::string build() const { return query_; }

private:
    std::string query_;
};

// Utilisation — les appels s'enchaînent naturellement
auto sql = QueryBuilder()
    .select("name")
    .from("users")
    .where("age > 18")
    .build();
```

Le type de retour est `QueryBuilder&` (référence sur l'objet courant). On retourne `*this`, c'est-à-dire l'objet déréférencé. Ce pattern est fréquent dans les builders, les configurateurs, et les bibliothèques comme les streams de la STL.

### Passer l'objet courant en argument

Parfois, une méthode doit passer l'objet lui-même à une fonction externe :

```cpp
class Task {  
public:  
    void register_in(Scheduler& scheduler) {
        scheduler.add(this);   // Passe un pointeur vers l'objet courant
    }
};
```

---

## Méthodes `const` : garantir la lecture seule

Quand une méthode ne modifie pas l'état de l'objet, elle doit être marquée `const` :

```cpp
class Circle {  
public:  
    explicit Circle(double radius) : radius_(radius) {}

    // Méthode const — ne modifie pas l'objet
    double area() const {
        return 3.14159265358979 * radius_ * radius_;
    }

    // Méthode non-const — modifie l'objet
    void scale(double factor) {
        radius_ *= factor;
    }

private:
    double radius_;
};
```

Le `const` après la liste de paramètres signifie : *cette méthode promet de ne pas modifier les données membres de l'objet*. Le compilateur vérifie cette promesse. Si vous essayez de modifier un attribut dans une méthode `const`, la compilation échoue.

Cette distinction a une conséquence directe sur l'utilisabilité de la classe :

```cpp
void print_info(const Circle& c) {
    std::cout << c.area() << "\n";    // OK — area() est const
    // c.scale(2.0);                  // ERREUR — scale() n'est pas const
}
```

Quand vous recevez un objet par `const&` (ce qui est la norme pour les paramètres en lecture seule, comme vu au chapitre 4), seules les méthodes `const` sont accessibles. Si `area()` n'était pas marquée `const`, vous ne pourriez pas l'appeler sur une référence constante — ce qui serait absurde pour une méthode de lecture.

**Règle pratique** : marquez `const` toute méthode qui ne modifie pas l'état observable de l'objet. C'est une habitude à prendre dès le début. Les *C++ Core Guidelines* (règle Con.2) sont catégoriques sur ce point.

---

## Membres `static` : données et méthodes de classe

Un membre `static` n'appartient pas à une instance particulière — il appartient à la **classe elle-même**. Il existe en un seul exemplaire, partagé par toutes les instances.

### Données membres statiques

```cpp
class Connection {  
public:  
    Connection() { ++active_count_; }
    ~Connection() { --active_count_; }

    static int active_count() { return active_count_; }

private:
    static int active_count_;   // Déclaration
};

// Définition obligatoire dans un .cpp (avant C++17)
int Connection::active_count_ = 0;
```

Ici, `active_count_` est un compteur unique partagé entre toutes les instances de `Connection`. Chaque construction incrémente le compteur, chaque destruction le décrémente. Peu importe le nombre d'objets `Connection` en vie — il n'y a qu'un seul `active_count_` en mémoire.

Depuis **C++17**, vous pouvez utiliser le mot-clé `inline` pour définir une donnée statique directement dans le header, sans fichier `.cpp` séparé :

```cpp
class Connection {
    // ...
private:
    inline static int active_count_ = 0;  // C++17 — déclaration + définition
};
```

Cette approche est plus concise et évite les problèmes classiques de définition multiple dans les projets multi-fichiers.

### Fonctions membres statiques

Une méthode `static` n'a pas de pointeur `this` — elle ne peut pas accéder aux membres non-statiques :

```cpp
class MathUtils {  
public:  
    static double clamp(double val, double lo, double hi) {
        if (val < lo) return lo;
        if (val > hi) return hi;
        return val;
    }
};

// Appel sans instance — via le nom de la classe
double result = MathUtils::clamp(15.0, 0.0, 10.0);  // 10.0
```

On appelle une méthode statique avec l'opérateur de résolution de portée `::`. Aucune instance n'est nécessaire.

---

## Séparation déclaration / définition

Pour les projets de taille réelle, la bonne pratique est de **déclarer** la classe dans un fichier header (`.h` ou `.hpp`) et de **définir** les corps des méthodes dans un fichier source (`.cpp`).

**`sensor.h`** — Déclaration (l'interface) :

```cpp
#pragma once

#include <string>

class Sensor {  
public:  
    Sensor(int id, const std::string& name);

    void read(double new_value);
    double value() const;
    std::string to_string() const;

private:
    int id_;
    std::string name_;
    double value_ = 0.0;
};
```

**`sensor.cpp`** — Définition (l'implémentation) :

```cpp
#include "sensor.h"

#include <sstream>

Sensor::Sensor(int id, const std::string& name)
    : id_(id), name_(name) {}

void Sensor::read(double new_value) {
    value_ = new_value;
}

double Sensor::value() const {
    return value_;
}

std::string Sensor::to_string() const {
    std::ostringstream oss;
    oss << name_ << " (#" << id_ << "): " << value_;
    return oss.str();
}
```

Notez la syntaxe `Sensor::` devant chaque définition de méthode — elle indique au compilateur que la fonction appartient à la classe `Sensor`.

**Pourquoi cette séparation ?**

Elle offre trois avantages. D'abord, la **compilation incrémentale** : modifier le corps d'une méthode dans le `.cpp` ne force la recompilation que de ce fichier, pas de tous les fichiers qui incluent le header. Ensuite, l'**encapsulation du code source** : vous pouvez distribuer le `.h` (l'interface) et un fichier objet `.o` compilé, sans révéler votre implémentation. Enfin, la **lisibilité** : le header donne une vue d'ensemble de l'API en un coup d'œil, sans être noyé dans les détails d'implémentation.

### Méthodes inline dans le header

Les méthodes courtes (accesseurs, fonctions triviales) peuvent être définies directement dans la déclaration de classe. Le compilateur les traite implicitement comme `inline` :

```cpp
class Circle {  
public:  
    explicit Circle(double r) : radius_(r) {}

    double radius() const { return radius_; }   // Implicitement inline
    double area() const;                         // Définie dans le .cpp

private:
    double radius_;
};
```

La règle d'usage est simple : les **one-liners** (accesseurs, prédicats simples) restent dans le header. Les méthodes plus longues ou complexes vont dans le `.cpp`.

---

## Fil conducteur : première version de `DynArray`

Appliquons ce que nous venons de voir à notre classe `DynArray`. Pour l'instant, nous nous contentons de la structure de base — les constructeurs, le destructeur et la Règle des 5 viendront dans les sections suivantes.

**`dynarray.h`** :

```cpp
#pragma once

#include <cstddef>

class DynArray {  
public:  
    // Constructeur — nous le détaillerons en 6.2
    explicit DynArray(std::size_t size);

    // Destructeur — nous le détaillerons en 6.3
    ~DynArray();

    // Accesseurs
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    // Accès aux éléments
    int& operator[](std::size_t index);
    const int& operator[](std::size_t index) const;

private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};
```

**`dynarray.cpp`** :

```cpp
#include "dynarray.h"

#include <stdexcept>
#include <algorithm>

DynArray::DynArray(std::size_t size)
    : data_(new int[size]{}), size_(size) {}
    //       ^^^^^^^^^^^^ allocation sur le heap, initialisation à zéro

DynArray::~DynArray() {
    delete[] data_;   // Libération du tableau alloué
}

int& DynArray::operator[](std::size_t index) {
    if (index >= size_) {
        throw std::out_of_range("DynArray: index out of range");
    }
    return data_[index];
}

const int& DynArray::operator[](std::size_t index) const {
    if (index >= size_) {
        throw std::out_of_range("DynArray: index out of range");
    }
    return data_[index];
}
```

**Quelques points à noter :**

Le mot-clé **`explicit`** devant le constructeur empêche les conversions implicites. Sans lui, le compilateur autoriserait `DynArray arr = 5;`, ce qui est trompeur (on ne veut pas convertir un entier en tableau). La règle est simple : tout constructeur prenant un seul argument devrait être `explicit`, sauf si la conversion implicite a un sens sémantique clair.

L'opérateur `[]` existe en **deux versions** : une version non-`const` qui retourne `int&` (permettant la modification), et une version `const` qui retourne `const int&` (lecture seule). C'est un pattern courant que vous retrouverez dans `std::vector` et tous les conteneurs de la STL.

Les données membres sont initialisées avec des **valeurs par défaut** (`nullptr`, `0`) directement dans la déclaration. Même si le constructeur les écrase, ces valeurs servent de filet de sécurité — si un jour vous ajoutez un second constructeur et oubliez d'initialiser un membre, le comportement restera défini.

> ⚠️ **Attention** — Cette classe est volontairement incomplète. En l'état, copier un `DynArray` provoque un *double free* (deux objets partagent le même pointeur, et le premier détruit libère la mémoire sous les pieds du second). Nous corrigerons ce problème dans les sections 6.2 et 6.5 avec le constructeur de copie et la Règle des 5.

---

## Points clés à retenir

- Une **classe** regroupe données (état) et fonctions (comportement) dans une même entité. L'encapsulation protège l'invariant de la classe.  
- `struct` et `class` ne diffèrent que par la visibilité par défaut (`public` vs `private`). La convention est d'utiliser `struct` pour les agrégats sans invariant, `class` sinon.  
- Le pointeur `this` est implicite dans toute méthode non-statique. Il est rarement écrit explicitement, sauf pour le chaînage (`return *this`) ou la désambiguïsation.  
- Marquez `const` toute méthode qui ne modifie pas l'objet. C'est une garantie pour l'appelant et une vérification du compilateur.  
- Les membres `static` appartiennent à la classe, pas aux instances. Depuis C++17, `inline static` simplifie leur définition.  
- Séparez déclaration (`.h`) et définition (`.cpp`) pour la compilation incrémentale, l'encapsulation et la lisibilité. Les accesseurs triviaux peuvent rester dans le header.  
- Utilisez les *default member initializers* (C++11) pour donner des valeurs par défaut aux attributs directement dans la déclaration de classe.

---


⏭️ [Constructeurs](/06-classes-encapsulation/02-constructeurs.md)
