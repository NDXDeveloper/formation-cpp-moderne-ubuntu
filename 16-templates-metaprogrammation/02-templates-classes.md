🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 16.2 — Templates de classes

## Chapitre 16 : Templates et Métaprogrammation · Module 5 : La STL

---

## Introduction

La section précédente a montré comment les templates de fonctions permettent d'écrire des algorithmes génériques. Les templates de classes étendent ce principe aux **structures de données** : on définit une classe une seule fois, en laissant un ou plusieurs types en paramètres, et le compilateur génère une version concrète pour chaque combinaison de types utilisée.

C'est exactement ainsi que fonctionnent les conteneurs de la bibliothèque standard. `std::vector<int>`, `std::vector<std::string>` et `std::vector<double>` sont trois classes distinctes, toutes générées automatiquement à partir d'un unique template `std::vector<T>`. Comprendre les templates de classes, c'est comprendre la mécanique interne de la STL — et être capable de concevoir ses propres structures de données génériques.

---

## Syntaxe de base

La déclaration d'un template de classe suit la même logique que celle d'un template de fonction : on préfixe la définition avec `template <typename T>`.

Prenons un exemple concret : une pile (*stack*) simplifiée qui stocke ses éléments dans un `std::vector` interne :

```cpp
#pragma once
#include <vector>
#include <stdexcept>
#include <print>

template <typename T>  
class Stack {  
public:  
    void push(const T& valeur);
    void pop();
    const T& top() const;
    bool empty() const { return elements_.empty(); }
    std::size_t size() const { return elements_.size(); }

private:
    std::vector<T> elements_;
};
```

Plusieurs points méritent attention :

- `T` est utilisé comme type des éléments stockés dans le vecteur interne (`std::vector<T>`).
- Les méthodes `push` et `top` utilisent `const T&` pour éviter les copies inutiles.
- La méthode `empty` est définie **inline** directement dans le corps de la classe. C'est la forme la plus concise.
- Les méthodes `push`, `pop` et `top` sont seulement **déclarées** dans le corps de la classe. Leur définition se trouve à l'extérieur.

---

## Définition des méthodes hors du corps de la classe

Lorsqu'une méthode d'un template de classe est définie en dehors du corps de la classe, il faut répéter le préambule `template <typename T>` et qualifier le nom de la méthode avec le nom de la classe suivi des paramètres template :

```cpp
template <typename T>  
void Stack<T>::push(const T& valeur) {  
    elements_.push_back(valeur);
}

template <typename T>  
void Stack<T>::pop() {  
    if (elements_.empty()) {
        throw std::runtime_error("pop() sur une pile vide");
    }
    elements_.pop_back();
}

template <typename T>  
const T& Stack<T>::top() const {  
    if (elements_.empty()) {
        throw std::runtime_error("top() sur une pile vide");
    }
    return elements_.back();
}
```

La notation `Stack<T>::` indique au compilateur que ces fonctions appartiennent à l'instanciation de `Stack` pour le type `T`. Sans cette qualification, le compilateur ne sait pas associer la définition au template.

### Tout dans le header

Comme pour les templates de fonctions (section 16.1), le code complet — déclaration de la classe **et** définition de toutes ses méthodes — doit résider dans le fichier d'en-tête. Le compilateur a besoin de la définition complète pour instancier la classe à chaque point d'utilisation. Les mêmes stratégies d'atténuation s'appliquent : ccache (section 2.3), instanciation explicite et modules C++20 (section 12.13).

---

## Utilisation

L'utilisation d'un template de classe nécessite de spécifier explicitement le ou les types template entre chevrons :

```cpp
#include "Stack.h"
#include <print>

int main() {
    Stack<int> pile_entiers;
    pile_entiers.push(10);
    pile_entiers.push(20);
    pile_entiers.push(30);

    std::print("Sommet : {}\n", pile_entiers.top());  // 30
    std::print("Taille : {}\n", pile_entiers.size());  // 3

    pile_entiers.pop();
    std::print("Sommet après pop : {}\n", pile_entiers.top());  // 20

    Stack<std::string> pile_mots;
    pile_mots.push("hello");
    pile_mots.push("world");
    std::print("Mot au sommet : {}\n", pile_mots.top());  // "world"
}
```

`Stack<int>` et `Stack<std::string>` sont deux classes complètement distinctes, générées indépendamment par le compilateur. Elles ne partagent aucun code à l'exécution. Tenter de passer un `Stack<int>` là où un `Stack<std::string>` est attendu produit une erreur de type, exactement comme entre deux classes sans lien.

---

## Class Template Argument Deduction (CTAD) — C++17

Avant C++17, il fallait toujours spécifier les types template lors de la construction d'un objet. Depuis C++17, le compilateur peut **déduire** les paramètres template à partir des arguments du constructeur. Ce mécanisme s'appelle **CTAD** (*Class Template Argument Deduction*) :

```cpp
std::vector v{1, 2, 3, 4};          // Déduit std::vector<int>  
std::pair p{3.14, "hello"s};        // Déduit std::pair<double, std::string>  
std::tuple t{42, 3.14, "text"s};    // Déduit std::tuple<int, double, std::string>  
```

### CTAD sur notre classe Stack

Pour que CTAD fonctionne avec notre classe `Stack`, il faut soit un constructeur qui permet la déduction, soit un **deduction guide** explicite.

Ajoutons un constructeur qui accepte une `std::initializer_list` :

```cpp
template <typename T>  
class Stack {  
public:  
    Stack() = default;

    Stack(std::initializer_list<T> init)
        : elements_{init} {}

    // ... autres méthodes inchangées

private:
    std::vector<T> elements_;
};
```

Le compilateur peut désormais déduire `T` à partir du contenu de la liste d'initialisation :

```cpp
Stack pile{1, 2, 3, 4, 5};       // CTAD → Stack<int>  
Stack mots{"alpha"s, "beta"s};   // CTAD → Stack<std::string>  
```

### Deduction guides

Dans certains cas, la déduction automatique ne suffit pas ou produit un résultat inattendu. On peut alors fournir un **deduction guide** explicite :

```cpp
// Guide de déduction : un Stack construit depuis un vector<T> produit un Stack<T>
template <typename T>  
Stack(std::vector<T>) -> Stack<T>;  
```

```cpp
std::vector<double> donnees{1.1, 2.2, 3.3};  
Stack pile(donnees);  // CTAD via le guide → Stack<double>  
```

Le deduction guide est une déclaration hors de la classe, placée dans le même header. Il indique au compilateur comment mapper les types des arguments du constructeur vers les paramètres template de la classe.

### Limites de CTAD

CTAD ne fonctionne qu'à la **construction**. Il ne s'applique pas aux déclarations de variables sans initialisation, ni aux paramètres de fonctions :

```cpp
Stack pile;                     // ERREUR : T ne peut pas être déduit sans arguments  
void traiter(Stack s);          // ERREUR : T manquant  
void traiter(Stack<int> s);     // OK  
```

De plus, CTAD peut produire des résultats surprenants avec les littéraux de chaînes :

```cpp
Stack mots{"hello", "world"};   // Déduit Stack<const char*>, pas Stack<std::string> !
```

Pour obtenir `Stack<std::string>`, il faut utiliser le suffixe `s` (`"hello"s`) ou écrire un deduction guide approprié.

---

## Paramètres template multiples

Un template de classe peut accepter plusieurs paramètres de type, exactement comme un template de fonction :

```cpp
template <typename K, typename V>  
class Pair {  
public:  
    Pair(const K& cle, const V& valeur)
        : cle_{cle}, valeur_{valeur} {}

    const K& cle() const { return cle_; }
    const V& valeur() const { return valeur_; }

private:
    K cle_;
    V valeur_;
};
```

```cpp
Pair<std::string, int> score{"Alice", 95};  
std::print("{} : {}\n", score.cle(), score.valeur());  

Pair p2{"Bob"s, 87};  // CTAD → Pair<std::string, int>
```

La STL utilise abondamment les paramètres multiples : `std::pair<K, V>`, `std::map<Key, Value>`, `std::tuple<Types...>`.

---

## Paramètres template non-type dans les classes

Comme pour les fonctions, les classes template acceptent des **paramètres non-type** — des valeurs constantes connues à la compilation. L'exemple le plus connu de la STL est `std::array` :

```cpp
template <typename T, std::size_t N>  
class FixedBuffer {  
public:  
    void set(std::size_t index, const T& valeur) {
        if (index >= N) {
            throw std::out_of_range("Index hors limites");
        }
        data_[index] = valeur;
    }

    const T& get(std::size_t index) const {
        if (index >= N) {
            throw std::out_of_range("Index hors limites");
        }
        return data_[index];
    }

    constexpr std::size_t capacity() const { return N; }

private:
    T data_[N];  // Tableau C de taille fixe, alloué sur la stack
};
```

```cpp
FixedBuffer<int, 128> buffer;         // Buffer de 128 int sur la stack  
FixedBuffer<double, 1024> mesures;    // Buffer de 1024 double sur la stack  

static_assert(buffer.capacity() == 128);  // Vérification compile-time
```

`N` étant une constante à la compilation, la taille du tableau interne est connue par le compilateur. Aucune allocation dynamique n'est nécessaire : tout réside sur la pile. C'est exactement le fonctionnement de `std::array<T, N>`.

Chaque combinaison de paramètres produit un type distinct : `FixedBuffer<int, 128>` et `FixedBuffer<int, 256>` sont deux classes **totalement différentes** et incompatibles entre elles.

---

## Valeurs par défaut des paramètres template

Les paramètres template d'une classe peuvent avoir des **valeurs par défaut**, ce qui simplifie l'utilisation dans les cas courants :

```cpp
template <typename T, typename Allocator = std::allocator<T>>  
class SimpleVector {  
    // ...
};
```

```cpp
SimpleVector<int> v1;                                // Allocator = std::allocator<int>  
SimpleVector<int, MyCustomAllocator<int>> v2;        // Allocator personnalisé  
```

C'est le pattern exact de `std::vector<T, Allocator>` : la grande majorité des utilisateurs ne spécifient jamais le second paramètre et profitent de l'allocateur par défaut.

Les valeurs par défaut s'appliquent de droite à gauche. Si un paramètre a une valeur par défaut, tous les paramètres qui le suivent doivent également en avoir une :

```cpp
template <typename T = int, std::size_t N = 10>  
class Grid {  
    T data_[N][N];
public:
    constexpr std::size_t dimension() const { return N; }
};
```

```cpp
Grid<> g1;               // T = int, N = 10 (tous les défauts)  
Grid<double> g2;         // T = double, N = 10  
Grid<float, 20> g3;      // T = float, N = 20  
```

Notez les chevrons vides `<>` dans `Grid<> g1` : ils sont nécessaires pour indiquer au compilateur qu'on instancie un template, même si tous les paramètres utilisent leurs valeurs par défaut. Sans eux, le compilateur chercherait une classe nommée `Grid` (non-template).

---

## Membres statiques dans les templates de classes

Les membres statiques d'un template de classe sont **propres à chaque instanciation**. Chaque type concret possède sa propre copie indépendante :

```cpp
template <typename T>  
class Compteur {  
public:  
    Compteur() { ++instances_; }
    ~Compteur() { --instances_; }

    static int instances() { return instances_; }

private:
    static inline int instances_ = 0;  // inline depuis C++17
};
```

```cpp
{
    Compteur<int> a, b, c;
    Compteur<double> x, y;

    std::print("Instances int    : {}\n", Compteur<int>::instances());     // 3
    std::print("Instances double : {}\n", Compteur<double>::instances());  // 2
}
// Tous les destructeurs sont appelés, les compteurs reviennent à 0
```

`Compteur<int>` et `Compteur<double>` maintiennent chacun leur propre variable `instances_`. Il n'y a aucun partage entre les instanciations : ce sont des classes indépendantes.

---

## Types membres et alias de type

Un template de classe peut définir des **alias de type** internes, ce qui est une pratique fondamentale de la STL. Ces alias permettent au code utilisateur et aux algorithmes génériques d'interroger les types associés à un conteneur :

```cpp
template <typename T>  
class DynamicArray {  
public:  
    using value_type      = T;
    using size_type       = std::size_t;
    using reference       = T&;
    using const_reference = const T&;
    using iterator        = T*;
    using const_iterator  = const T*;

    // ...

    iterator begin() { return data_; }
    iterator end()   { return data_ + size_; }

private:
    T* data_     = nullptr;
    size_type size_     = 0;
    size_type capacity_ = 0;
};
```

Le code utilisateur peut ensuite accéder à ces types :

```cpp
DynamicArray<double>::value_type val = 3.14;      // double  
DynamicArray<double>::size_type  idx = 0;          // std::size_t  
```

Les algorithmes de la STL et les concepts (section 16.6) s'appuient sur ces conventions de nommage pour fonctionner avec n'importe quel conteneur conforme.

### Le mot-clé `typename` pour les types dépendants

Lorsqu'on accède à un type membre d'un template à l'intérieur d'un autre contexte template, le compilateur ne sait pas a priori si le nom désigne un type ou une valeur. Il faut lever l'ambiguïté avec `typename` :

```cpp
template <typename Container>  
void afficher_premier(const Container& c) {  
    // typename est obligatoire ici : le compilateur ne sait pas
    // que Container::value_type est un type avant l'instanciation
    typename Container::value_type premier = *c.begin();
    std::print("{}\n", premier);
}
```

Sans `typename`, le compilateur suppose que `Container::value_type` est une valeur (un membre statique, par exemple) et produit une erreur de syntaxe. Cette règle s'applique chaque fois qu'un nom qualifié dépend d'un paramètre template.

> **Note** — C++20 a assoupli cette contrainte dans un certain nombre de contextes où le compilateur peut déterminer sans ambiguïté qu'un type est attendu (type de retour, paramètre de fonction, etc.). Dans le doute, écrire `typename` reste toujours valide et rend le code compatible avec les standards antérieurs.

---

## Templates de classes imbriqués

Un template de classe peut contenir un autre template en tant que membre interne. C'est courant pour les **itérateurs** ou les **nœuds** internes d'une structure de données :

```cpp
template <typename T>  
class LinkedList {  
private:  
    struct Node {
        T data;
        Node* next = nullptr;

        Node(const T& val, Node* n = nullptr)
            : data{val}, next{n} {}
    };

    Node* head_ = nullptr;
    std::size_t size_ = 0;

public:
    ~LinkedList() {
        while (head_) {
            Node* tmp = head_;
            head_ = head_->next;
            delete tmp;
        }
    }

    void push_front(const T& valeur) {
        head_ = new Node(valeur, head_);
        ++size_;
    }

    // Classe itérateur imbriquée
    class Iterator {
    public:
        explicit Iterator(Node* ptr) : current_{ptr} {}

        const T& operator*() const { return current_->data; }

        Iterator& operator++() {
            current_ = current_->next;
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return current_ != other.current_;
        }

    private:
        Node* current_;
    };

    Iterator begin() const { return Iterator(head_); }
    Iterator end()   const { return Iterator(nullptr); }
};
```

```cpp
LinkedList<int> liste;  
liste.push_front(30);  
liste.push_front(20);  
liste.push_front(10);  

for (auto val : liste) {
    std::print("{} ", val);  // 10 20 30
}
```

La structure `Node` et la classe `Iterator` héritent automatiquement du paramètre `T` du template englobant. Il n'est pas nécessaire de les rendre elles-mêmes template (bien que ce soit possible si elles doivent être paramétrées indépendamment).

> **Note** — Cet exemple utilise `new`/`delete` à des fins pédagogiques pour illustrer la structure interne. En code de production, un smart pointer comme `std::unique_ptr` serait préféré pour respecter le principe RAII (section 6.3). Voir également la section 9.4 sur l'abandon de `new`/`delete` en code moderne.

---

## Héritage et templates

Les templates de classes interagissent naturellement avec l'héritage. Plusieurs configurations sont possibles.

### Une classe template hérite d'une autre classe template

```cpp
template <typename T>  
class Base {  
protected:  
    T valeur_;
public:
    explicit Base(const T& v) : valeur_{v} {}
    const T& get() const { return valeur_; }
};

template <typename T>  
class Derived : public Base<T> {  
public:  
    explicit Derived(const T& v) : Base<T>(v) {}

    void doubler() {
        this->valeur_ *= 2;  // 'this->' nécessaire pour accéder au membre de la base
    }
};
```

Le `this->` devant `valeur_` n'est pas optionnel ici. Dans une classe dérivée d'un template, les noms hérités de la base ne sont pas automatiquement visibles car la classe de base dépend du paramètre `T`. Le préfixe `this->` ou la qualification `Base<T>::valeur_` lèvent l'ambiguïté.

### Une classe non-template hérite d'un template instancié

```cpp
class IntStack : public Stack<int> {  
public:  
    int somme() const {
        int total = 0;
        // Accès aux éléments via l'interface de Stack<int>
        // ...
        return total;
    }
};
```

Ici, `IntStack` est une classe ordinaire (non-template) qui hérite de `Stack<int>` — une instanciation concrète du template. C'est une technique parfois utilisée pour fournir une interface spécialisée tout en réutilisant l'implémentation générique.

### Un template hérite d'une classe non-template

```cpp
class Loggable {  
public:  
    void log(const std::string& message) const {
        std::print("[LOG] {}\n", message);
    }
};

template <typename T>  
class LoggedStack : public Loggable, public Stack<T> {  
public:  
    void push(const T& valeur) {
        this->log("push");
        Stack<T>::push(valeur);
    }
};
```

Ce pattern de **mixin** permet d'ajouter des fonctionnalités transversales (logging, comptage, etc.) à une classe template sans modifier l'implémentation originale.

---

## Amitié (friendship) et templates

Le mot-clé `friend` fonctionne avec les templates, mais sa syntaxe demande de l'attention.

### Déclarer un opérateur ami

Un cas fréquent est la surcharge de `operator<<` pour l'affichage. Pour un template de classe, la déclaration d'amitié doit mentionner que l'ami est lui-même un template :

```cpp
template <typename T>  
class Wrapper {  
public:  
    explicit Wrapper(const T& val) : valeur_{val} {}

    // Déclaration d'amitié pour une instanciation spécifique
    template <typename U>
    friend std::ostream& operator<<(std::ostream&, const Wrapper<U>&);

private:
    T valeur_;
};

template <typename T>  
std::ostream& operator<<(std::ostream& os, const Wrapper<T>& w) {  
    return os << "Wrapper(" << w.valeur_ << ")";
}
```

```cpp
Wrapper w{42};  
std::cout << w << "\n";  // Wrapper(42)  
```

Il existe une forme plus restreinte qui n'autorise l'amitié que pour l'instanciation correspondante (par exemple, seul `operator<< <int>` est ami de `Wrapper<int>`) :

```cpp
// Forward declarations nécessaires
template <typename T> class Wrapper;  
template <typename T>  
std::ostream& operator<<(std::ostream&, const Wrapper<T>&);  

template <typename T>  
class Wrapper {  
    // Amitié restreinte : seul operator<< <T> est ami de Wrapper<T>
    friend std::ostream& operator<< <T>(std::ostream&, const Wrapper<T>&);
    // ...
};
```

La distinction est subtile mais importante pour la sécurité : dans la première forme (avec `template <typename U> friend`), `operator<< <double>` peut accéder aux membres privés de `Wrapper<int>`. Dans la forme restreinte (avec `friend ... operator<< <T>`), chaque instanciation de l'opérateur ne voit que l'instanciation correspondante du Wrapper.

---

## Alias de templates avec `using` (C++11)

Les **alias de templates** permettent de créer des noms plus concis pour des instanciations partielles de templates complexes :

```cpp
// Alias : un StringMap est une map dont la clé est toujours std::string
template <typename V>  
using StringMap = std::map<std::string, V>;  
```

```cpp
StringMap<int> ages;           // Équivalent à std::map<std::string, int>  
ages["Alice"] = 30;  
ages["Bob"]   = 25;  

StringMap<double> scores;      // Équivalent à std::map<std::string, double>
```

Les alias de templates sont purement syntaxiques : ils ne créent pas de nouveau type, seulement un raccourci. Le compilateur les traite exactement comme le template original.

Quelques alias courants dans la pratique :

```cpp
// Vecteur 2D
template <typename T>  
using Matrix = std::vector<std::vector<T>>;  

// Pointeur partagé raccourci
template <typename T>  
using Ptr = std::shared_ptr<T>;  

// Callback générique
template <typename Ret, typename... Args>  
using Callback = std::function<Ret(Args...)>;  
```

> **Note** — Avant C++11, on utilisait des `typedef` imbriqués dans des structures pour obtenir un effet similaire, mais la syntaxe était nettement plus lourde. Les alias `using` sont la forme moderne recommandée.

---

## Instanciation paresseuse des méthodes

Un aspect important des templates de classes est que le compilateur ne génère le code d'une méthode **que si elle est effectivement appelée**. Cela a une conséquence pratique significative : un template de classe peut être instancié avec un type qui ne supporte pas toutes ses opérations, tant que les méthodes incompatibles ne sont jamais invoquées.

```cpp
template <typename T>  
class Boite {  
public:  
    explicit Boite(const T& val) : valeur_{val} {}

    T get() const { return valeur_; }

    void afficher() const {
        std::print("{}\n", valeur_);  // Requiert que T soit formatable
    }

    bool est_positif() const {
        return valeur_ > T{0};  // Requiert que T supporte > et T{0}
    }

private:
    T valeur_;
};
```

```cpp
struct Point { int x, y; };

Boite<Point> b{Point{3, 4}};  
auto p = b.get();            // OK : get() ne requiert rien de spécial  

// b.afficher();             // ERREUR si décommentée : Point n'est pas formatable
// b.est_positif();          // ERREUR si décommentée : pas de operator> pour Point
```

`Boite<Point>` compile parfaitement tant qu'on n'appelle que `get()`. Les méthodes `afficher()` et `est_positif()` ne sont jamais instanciées, donc leurs contraintes sur `T` ne sont pas vérifiées.

Ce comportement est à la fois un atout (flexibilité) et un risque (erreurs découvertes tardivement). Les **Concepts** (section 16.6) permettent de rendre ces contraintes explicites et de produire des erreurs claires dès l'instanciation de la classe, pas seulement à l'appel d'une méthode spécifique.

---

## Bonnes pratiques

**Définir des alias de type internes.** Exposez `value_type`, `size_type`, `iterator`, etc., dans vos classes template. C'est la convention de la STL et c'est indispensable pour l'interopérabilité avec les algorithmes et les concepts standard.

**Utiliser CTAD avec discernement.** La déduction automatique est pratique, mais elle peut surprendre (déduction en `const char*` au lieu de `std::string`, par exemple). Fournissez des deduction guides explicites si le comportement par défaut est ambigu.

**Préférer `this->` pour les noms hérités.** Dans une classe template dérivée d'un template, qualifiez toujours les accès aux membres de la base avec `this->` ou `Base<T>::`. C'est une source d'erreurs fréquente, surtout pour les développeurs venant d'autres langages.

**Penser à l'instanciation paresseuse.** Ne considérez pas que toutes les méthodes seront compilées pour tous les types. Documentez les contraintes attendues sur `T` (ou, mieux, exprimez-les avec des Concepts).

**Favoriser les alias `using` sur les `typedef`.** La syntaxe `using` est plus lisible, supporte les templates et s'intègre mieux dans le code moderne.

**Garder les classes template cohérentes et ciblées.** Une classe template qui accumule des dizaines de méthodes avec des contraintes disparates sur `T` est un signe de conception fragile. Envisagez de séparer les responsabilités ou d'utiliser la spécialisation (section 16.3).

---

## En résumé

Les templates de classes permettent de définir des structures de données génériques, réutilisables avec n'importe quel type, sans sacrifier la sûreté du typage ni les performances. Le compilateur génère une classe concrète et distincte pour chaque combinaison de paramètres template utilisée. Les paramètres peuvent être des types (`typename T`), des valeurs constantes (`std::size_t N`) ou une combinaison des deux, avec des valeurs par défaut optionnelles.

CTAD (C++17) simplifie l'utilisation en déduisant les types depuis les arguments du constructeur. Les alias `using` permettent de créer des raccourcis pour des instanciations partielles. L'instanciation paresseuse des méthodes offre une flexibilité importante, mais les Concepts (section 16.6) apportent un cadre bien plus robuste pour exprimer les contraintes sur les types.

La section suivante (16.3) explore la **spécialisation**, qui permet de fournir des implémentations alternatives pour des types ou des familles de types spécifiques.

⏭️ [Spécialisation partielle et totale](/16-templates-metaprogrammation/03-specialisation.md)
