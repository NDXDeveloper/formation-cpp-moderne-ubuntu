🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 10.3 Move constructors et move assignment operators

## Introduction

Les sections précédentes ont posé les fondations théoriques : les catégories de valeurs ([10.1](/10-move-semantics/01-lvalues-rvalues.md)) et le rôle de `std::move` comme cast ([10.2](/10-move-semantics/02-std-move.md)). Cette section passe à la pratique : **comment implémenter** le constructeur de déplacement et l'opérateur d'affectation par déplacement pour vos propres classes.

Ces deux opérations spéciales sont les mécanismes concrets qui volent les ressources d'un objet source quand le compilateur détecte qu'il peut le faire. Elles complètent le constructeur de copie et l'opérateur d'affectation par copie pour former, avec le destructeur, les **cinq opérations spéciales** de la Règle des 5 ([section 6.5](/06-classes-encapsulation/05-rule-of-five.md)).

---

## Le constructeur de déplacement

### Signature

```cpp
class T {  
public:  
    T(T&& other) noexcept;
};
```

Le paramètre est une **référence rvalue** (`T&&`). Le compilateur appelle ce constructeur quand l'argument est une rvalue — un temporaire ou le résultat de `std::move`.

### Implémentation : le pattern standard

L'implémentation suit toujours le même schéma en trois temps :

1. **Transférer** les ressources de `other` vers `this` (copie des pointeurs/handles).
2. **Neutraliser** `other` pour que son destructeur ne libère pas les ressources volées.
3. Marquer l'opération **`noexcept`**.

Voici un exemple complet sur une classe qui gère un buffer dynamique :

```cpp
class Buffer {
    char* data_;
    size_t size_;
    size_t capacity_;

public:
    // Constructeur classique
    explicit Buffer(size_t cap)
        : data_(new char[cap]), size_(0), capacity_(cap) {}

    // Destructeur
    ~Buffer() {
        delete[] data_;  // delete[] nullptr est un no-op — sûr
    }

    // Constructeur de copie (pour référence)
    Buffer(const Buffer& other)
        : data_(new char[other.capacity_])
        , size_(other.size_)
        , capacity_(other.capacity_)
    {
        std::memcpy(data_, other.data_, size_);
    }

    // ✅ Constructeur de déplacement
    Buffer(Buffer&& other) noexcept
        : data_(other.data_)            // 1. Voler le pointeur
        , size_(other.size_)            //    Voler la taille
        , capacity_(other.capacity_)    //    Voler la capacité
    {
        other.data_ = nullptr;          // 2. Neutraliser la source
        other.size_ = 0;
        other.capacity_ = 0;
    }
};
```

Après le déplacement, `other` est dans un état cohérent : son destructeur appellera `delete[] nullptr`, ce qui est un no-op. L'objet est vide mais destructible — le contrat minimal est respecté.

### Membres non-primitifs : déplacer récursivement

Quand votre classe contient des membres qui sont eux-mêmes déplaçables (string, vector, unique_ptr…), vous devez utiliser `std::move` sur chaque membre dans la liste d'initialisation :

```cpp
class Session {
    std::string id_;
    std::unique_ptr<Connection> conn_;
    std::vector<Message> historique_;

public:
    Session(Session&& other) noexcept
        : id_(std::move(other.id_))              // string déplacé
        , conn_(std::move(other.conn_))           // unique_ptr déplacé
        , historique_(std::move(other.historique_)) // vector déplacé
    {
        // Rien à neutraliser manuellement :
        // chaque membre s'est occupé de vider other.membre_
    }
};
```

Rappel crucial de la [section 10.1](/10-move-semantics/01-lvalues-rvalues.md) : `other` est une lvalue (il a un nom), même si son type est `Session&&`. Sans `std::move` sur chaque membre, le constructeur de **copie** de chaque membre serait appelé, annulant tout le bénéfice :

```cpp
// ❌ Erreur fréquente : oublier std::move sur les membres
Session(Session&& other) noexcept
    : id_(other.id_)              // COPIE — other.id_ est une lvalue !
    , conn_(other.conn_)          // ❌ Ne compile même pas (unique_ptr non copiable)
    , historique_(other.historique_) // COPIE du vector entier
{}
```

---

## L'opérateur d'affectation par déplacement

### Signature

```cpp
class T {  
public:  
    T& operator=(T&& other) noexcept;
};
```

L'opérateur d'affectation par déplacement est invoqué quand on affecte une rvalue à un objet **déjà construit**. La différence avec le constructeur de déplacement est que `this` possède déjà des ressources qu'il faut **libérer avant** de voler celles de `other`.

### Implémentation : le pattern standard

Le schéma comporte une étape supplémentaire par rapport au constructeur :

1. **Vérifier** l'auto-affectation (optionnel mais recommandé).
2. **Libérer** les ressources actuelles de `this`.
3. **Transférer** les ressources de `other` vers `this`.
4. **Neutraliser** `other`.

```cpp
class Buffer {
    // ... mêmes membres que précédemment ...

    // ✅ Opérateur d'affectation par déplacement
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {               // 1. Garde contre l'auto-affectation
            delete[] data_;                  // 2. Libérer les ressources actuelles

            data_ = other.data_;             // 3. Voler les ressources
            size_ = other.size_;
            capacity_ = other.capacity_;

            other.data_ = nullptr;           // 4. Neutraliser la source
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }
};
```

### La garde contre l'auto-affectation

L'auto-affectation par déplacement (`x = std::move(x)`) est un cas dégénéré qui ne devrait jamais se produire dans du code correct. Cependant, elle peut survenir indirectement — par exemple via un alias ou un échange d'éléments dans un algorithme. La garde `if (this != &other)` protège contre une corruption silencieuse :

```cpp
Buffer buf(1024);  
buf = std::move(buf);  // Sans la garde : delete[] data_ puis data_ = data_  
                        // Le buffer est détruit puis lu → UB
```

Certains développeurs considèrent que cette garde est inutile car `x = std::move(x)` est un bug que le code ne devrait jamais atteindre. C'est un débat légitime. En pratique, le coût de la comparaison de pointeurs est négligeable et la protection qu'elle offre contre les bugs subtils justifie sa présence.

---

## L'idiome copy-and-swap

Il existe une technique élégante qui permet d'écrire un **seul opérateur d'affectation** qui gère à la fois la copie et le déplacement, tout en garantissant l'exception safety forte :

```cpp
class Buffer {
    char* data_;
    size_t size_;
    size_t capacity_;

public:
    // Constructeur de copie
    Buffer(const Buffer& other);

    // Constructeur de déplacement
    Buffer(Buffer&& other) noexcept;

    // Destructeur
    ~Buffer();

    // Fonction swap (noexcept)
    friend void swap(Buffer& a, Buffer& b) noexcept {
        using std::swap;
        swap(a.data_, b.data_);
        swap(a.size_, b.size_);
        swap(a.capacity_, b.capacity_);
    }

    // ✅ Un seul opérateur — gère copie ET déplacement
    Buffer& operator=(Buffer other) noexcept {  // Passage par VALEUR
        swap(*this, other);
        return *this;
    }
    // other est détruit ici → libère les anciennes ressources de this
};
```

Le mécanisme repose sur le passage par valeur du paramètre `other` :

- Si l'argument est une **lvalue** → le constructeur de copie crée `other` → swap → l'ancienne valeur est détruite avec `other`.
- Si l'argument est une **rvalue** → le constructeur de déplacement crée `other` → swap → l'ancienne valeur est détruite avec `other`.

L'auto-affectation est automatiquement gérée (les anciennes ressources finissent dans `other` qui est détruit), et l'exception safety est forte : si la copie échoue (exception dans le constructeur de copie), `this` n'a pas été modifié.

Les avantages sont clairs : un seul opérateur au lieu de deux, exception safety gratuite, auto-affectation gérée. Le compromis est un déplacement supplémentaire (le swap) par rapport à l'implémentation directe — un coût négligeable dans la quasi-totalité des cas.

---

## noexcept : pourquoi c'est non négociable

Marquer le constructeur de déplacement et l'opérateur d'affectation par déplacement `noexcept` n'est pas une optimisation optionnelle. C'est une exigence pratique dont dépend le bon fonctionnement de votre classe avec la STL.

### Le problème concret

Quand un `std::vector` doit réallouer son buffer interne (parce que `push_back` dépasse la capacité), il doit transférer ses éléments de l'ancien buffer vers le nouveau. Deux options :

- **Copier** les éléments : si la copie du N-ième élément échoue (exception), les N-1 déjà copiés sont détruits et l'ancien buffer est intact. L'opération offre la **garantie forte** (l'état est identique à avant l'opération).
- **Déplacer** les éléments : si le déplacement du N-ième élément échoue, les N-1 déjà déplacés sont dans le nouveau buffer, les originaux sont dans un état indéterminé. L'ancien buffer est corrompu. **Aucune garantie d'exception** n'est possible.

Pour se protéger, `std::vector` utilise `std::move_if_noexcept` : il ne déplace que si le constructeur de déplacement est `noexcept`. Sinon, il copie.

```cpp
class Bon {  
public:  
    Bon(Bon&&) noexcept;  // ✅ vector déplacera → O(1) par élément
};

class Mauvais {  
public:  
    Mauvais(Mauvais&&);   // ⚠️ vector copiera → O(n) par élément
};

// Avec 1 million d'éléments dans un vector<Mauvais> :
// Réallocation = 1 million de COPIES au lieu de déplacements
// Différence de performance potentiellement énorme
```

### Les conteneurs concernés

Ce comportement ne se limite pas à `vector`. Tous les conteneurs et algorithmes de la STL qui déplacent des éléments en interne vérifient `noexcept` :

- `std::vector` — réallocations lors de `push_back`, `insert`, `resize`.
- `std::deque` — réorganisation interne.
- `std::sort`, `std::partition`, `std::rotate` — réarrangement d'éléments.
- `std::swap` — utilise le move si `noexcept`.

### Quand peut-on garantir noexcept ?

Un constructeur de déplacement peut être `noexcept` si **toutes** les opérations qu'il effectue sont elles-mêmes `noexcept` :

- Copie de types scalaires (`int`, `double`, pointeurs) — toujours `noexcept`.
- Déplacement de types STL (`std::string`, `std::vector`, `std::unique_ptr`) — tous `noexcept`.
- Mise à `nullptr` / zéro de la source — toujours `noexcept`.

En pratique, si votre classe ne gère que des types scalaires et des types STL, votre constructeur de déplacement **peut et doit** être `noexcept`. Les rares cas où `noexcept` n'est pas possible impliquent généralement des allocations mémoire dans le constructeur de déplacement — un design à reconsidérer.

### Vérifier à la compilation

```cpp
static_assert(std::is_nothrow_move_constructible_v<Buffer>,
              "Buffer doit être nothrow move constructible");

static_assert(std::is_nothrow_move_assignable_v<Buffer>,
              "Buffer doit être nothrow move assignable");
```

Placez ces assertions dans votre code (ou dans les tests) pour garantir que la propriété `noexcept` n'est pas accidentellement perdue lors d'une refactorisation.

---

## Interaction avec la Règle des 5

La Règle des 5 ([section 6.5](/06-classes-encapsulation/05-rule-of-five.md)) stipule que si vous définissez explicitement l'une des cinq opérations spéciales, vous devriez les définir (ou les supprimer) toutes les cinq :

| Opération | Signature |
|---|---|
| Destructeur | `~T()` |
| Constructeur de copie | `T(const T&)` |
| Opérateur d'affectation par copie | `T& operator=(const T&)` |
| Constructeur de déplacement | `T(T&&) noexcept` |
| Opérateur d'affectation par déplacement | `T& operator=(T&&) noexcept` |

### Génération implicite par le compilateur

Le compilateur peut générer automatiquement le constructeur de déplacement et l'opérateur d'affectation par déplacement. Mais les règles sont restrictives — plus restrictives que pour la copie :

Le compilateur **génère** le constructeur de déplacement implicitement si et seulement si la classe ne déclare **aucune** des opérations suivantes :

- Constructeur de copie
- Opérateur d'affectation par copie
- Opérateur d'affectation par déplacement
- Destructeur

La même règle s'applique pour l'opérateur d'affectation par déplacement.

```cpp
// ✅ Le compilateur génère les 5 opérations implicitement
class Simple {
    std::string nom_;
    std::vector<int> data_;
    std::unique_ptr<Logger> logger_;
    // Pas de destructeur, copie, ou move explicites
    // → tout est généré automatiquement
};

// ⚠️ Le destructeur explicite supprime la génération du move
class ProblemeSubtil {
    std::string nom_;
    std::vector<int> data_;

    ~ProblemeSubtil() {
        std::print("Destruction\n");  // Juste un log...
    }
    // Le constructeur de déplacement n'est PAS généré !
    // ProblemeSubtil(ProblemeSubtil&&) n'existe pas
    // Les rvalues seront COPIÉES au lieu d'être déplacées
};
```

Ce second cas est un piège fréquent : un destructeur ajouté « juste pour un log » supprime silencieusement le constructeur de déplacement. L'impact en performance peut être considérable si la classe contient des membres lourds.

### La Règle du 0

La meilleure approche est souvent de ne définir **aucune** des cinq opérations et de laisser le compilateur tout générer. C'est la **Règle du 0** : si votre classe ne gère pas directement de ressources brutes (pas de `new`, pas de `fopen`, pas de handle), elle n'a pas besoin de destructeur personnalisé, et donc pas besoin de copie ou de move personnalisés.

```cpp
// ✅ Règle du 0 — le compilateur fait tout
class UserProfile {
    std::string name_;
    std::string email_;
    std::vector<std::string> roles_;
    std::optional<std::string> avatar_url_;
    // Aucune des 5 opérations déclarée
    // → constructeur de copie, move, destructeur, affectations : tout est généré
    //    et chaque opération fait "la bonne chose" membre par membre
};
```

La Règle du 0 est la forme la plus sûre et la plus maintenable. Réservez la Règle des 5 aux classes RAII qui encapsulent directement des ressources brutes (ce qui, grâce aux smart pointers, devient rare dans le code applicatif).

### = default : demander explicitement la génération

Si vous devez déclarer l'une des cinq opérations (par exemple un destructeur virtuel pour le polymorphisme), utilisez `= default` pour demander au compilateur de générer les autres :

```cpp
class Base {  
public:  
    virtual ~Base() = default;                           // Nécessaire pour le polymorphisme

    Base(const Base&) = default;                         // Re-déclaré explicitement
    Base& operator=(const Base&) = default;

    Base(Base&&) noexcept = default;                     // Re-déclaré explicitement
    Base& operator=(Base&&) noexcept = default;
};
```

Sans les `= default`, le destructeur virtuel aurait supprimé la génération implicite du move. Avec les `= default`, vous retrouvez le comportement par défaut tout en ayant le destructeur virtuel nécessaire.

### = delete : interdire explicitement

Vous pouvez interdire le déplacement (ou la copie) en supprimant les opérations :

```cpp
class NonDeplacable {  
public:  
    NonDeplacable(NonDeplacable&&) = delete;
    NonDeplacable& operator=(NonDeplacable&&) = delete;

    // La copie peut rester autorisée si nécessaire
    NonDeplacable(const NonDeplacable&) = default;
    NonDeplacable& operator=(const NonDeplacable&) = default;
};
```

C'est rare mais justifié dans certains cas : mutex, singletons, objets dont l'adresse doit rester stable (car d'autres parties du code stockent un pointeur brut vers eux).

---

## Exemple complet : une classe ResourceHandle

Voici une classe RAII complète qui illustre la Règle des 5 avec implémentation explicite des cinq opérations :

```cpp
#include <memory>
#include <cstring>
#include <utility>

class ResourceHandle {
    char* data_;
    size_t size_;

public:
    // Constructeur
    explicit ResourceHandle(size_t size)
        : data_(size > 0 ? new char[size]{} : nullptr)
        , size_(size)
    {}

    // 1. Destructeur
    ~ResourceHandle() {
        delete[] data_;
    }

    // 2. Constructeur de copie
    ResourceHandle(const ResourceHandle& other)
        : data_(other.size_ > 0 ? new char[other.size_] : nullptr)
        , size_(other.size_)
    {
        if (data_) {
            std::memcpy(data_, other.data_, size_);
        }
    }

    // 3. Opérateur d'affectation par copie
    ResourceHandle& operator=(const ResourceHandle& other) {
        if (this != &other) {
            ResourceHandle temp(other);   // Copie dans un temporaire
            swap(*this, temp);            // Swap avec this
        }                                 // temp détruit → libère l'ancien buffer
        return *this;
    }

    // 4. Constructeur de déplacement
    ResourceHandle(ResourceHandle&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    // 5. Opérateur d'affectation par déplacement
    ResourceHandle& operator=(ResourceHandle&& other) noexcept {
        if (this != &other) {
            delete[] data_;

            data_ = other.data_;
            size_ = other.size_;

            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    // Swap (ami, noexcept)
    friend void swap(ResourceHandle& a, ResourceHandle& b) noexcept {
        using std::swap;
        swap(a.data_, b.data_);
        swap(a.size_, b.size_);
    }

    // Accesseurs
    const char* data() const noexcept { return data_; }
    size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
};

// Vérifications à la compilation
static_assert(std::is_nothrow_move_constructible_v<ResourceHandle>);  
static_assert(std::is_nothrow_move_assignable_v<ResourceHandle>);  
```

---

## Checklist d'implémentation

Quand vous implémentez les opérations de déplacement pour une classe, vérifiez systématiquement ces points :

| Point de contrôle | Vérifié ? |
|---|---|
| Le constructeur de déplacement est marqué `noexcept` | ☐ |
| L'opérateur d'affectation par déplacement est marqué `noexcept` | ☐ |
| Chaque membre non-scalaire est déplacé avec `std::move` dans la liste d'initialisation | ☐ |
| L'objet source est laissé dans un état destructible (pointeurs à `nullptr`, handles à -1…) | ☐ |
| L'opérateur d'affectation libère les ressources de `this` avant le transfert | ☐ |
| L'auto-affectation est gérée (garde `this != &other` ou idiome copy-and-swap) | ☐ |
| `static_assert` vérifie `is_nothrow_move_constructible` et `is_nothrow_move_assignable` | ☐ |
| Si possible, la Règle du 0 a été préférée à la Règle des 5 | ☐ |

---

## Résumé

| Concept | Détail |
|---|---|
| **Constructeur de déplacement** | `T(T&&) noexcept` — vole les ressources, neutralise la source |
| **Opérateur d'affectation par déplacement** | `T& operator=(T&&) noexcept` — libère l'ancien, vole, neutralise |
| **noexcept** | Obligatoire en pratique — sans lui, la STL revient à la copie |
| **Idiome copy-and-swap** | Un seul opérateur `=` pour copie et move, exception-safe |
| **Règle du 0** | Préférer ne rien déclarer quand les membres se gèrent eux-mêmes |
| **Règle des 5** | Si l'une est déclarée, déclarer (ou `= default` / `= delete`) les cinq |
| **= default** | Demander la génération par le compilateur (ex: destructeur virtuel + move) |
| **Piège** | Un destructeur explicite supprime la génération implicite du move |

> **Règle pratique** — Visez la Règle du 0 : composez vos classes à partir de membres RAII (`unique_ptr`, `vector`, `string`…) et laissez le compilateur générer toutes les opérations. Quand vous devez gérer une ressource brute, implémentez les cinq opérations, marquez les moves `noexcept`, et ajoutez un `static_assert` pour verrouiller cette garantie.

⏭️ [Perfect Forwarding avec std::forward](/10-move-semantics/04-perfect-forwarding.md)
