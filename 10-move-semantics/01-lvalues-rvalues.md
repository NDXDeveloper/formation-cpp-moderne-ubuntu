🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 10.1 L-values vs R-values (&&)

## Introduction

Pour comprendre la sémantique de mouvement, il faut d'abord comprendre le système de **catégories de valeurs** du C++. Chaque expression dans un programme C++ appartient à une catégorie qui répond à deux questions fondamentales :

- L'expression désigne-t-elle un objet avec une **identité** (un emplacement mémoire stable, un nom) ?
- L'expression désigne-t-elle un objet dont les ressources peuvent être **volées** (parce qu'il est temporaire ou marqué comme abandonné) ?

Les réponses à ces deux questions déterminent si le compilateur invoquera un constructeur de copie ou un constructeur de déplacement — et c'est cette distinction qui rend la sémantique de mouvement possible.

---

## Les deux catégories essentielles : lvalue et rvalue

Avant d'entrer dans la taxonomie complète du standard, concentrons-nous sur la distinction pratique que vous utiliserez au quotidien.

### Lvalue : « j'ai un nom, je persiste »

Une **lvalue** (*left value*) est une expression qui désigne un objet identifiable et persistant. Vous pouvez prendre son adresse avec `&`, vous pouvez la retrouver plus tard par son nom, et elle survit au-delà de l'expression qui la mentionne.

```cpp
int x = 42;             // x est une lvalue
std::string nom = "Alice";  // nom est une lvalue
int tab[10];            // tab est une lvalue
tab[3] = 7;             // tab[3] est une lvalue

int* ptr = &x;          // On peut prendre l'adresse d'une lvalue ✅
*ptr = 10;              // *ptr est une lvalue (déréférencement)

int& ref = x;           // ref est une lvalue (alias vers x)
```

Le nom « lvalue » vient historiquement de « left value » — ce qui peut apparaître à **gauche** d'une affectation. Mais cette étymologie est trompeuse : toutes les lvalues ne sont pas modifiables (une variable `const` est une lvalue) et certaines rvalues peuvent apparaître à gauche dans des cas spécifiques. Retenez plutôt : **une lvalue a une identité**.

### Rvalue : « je suis temporaire, prenez mes ressources »

Une **rvalue** (*right value*) est une expression qui désigne un objet **temporaire** — un objet sans nom, sans adresse stable, qui va disparaître à la fin de l'expression courante. Ses ressources peuvent être volées sans conséquence puisque personne ne pourra y accéder après.

```cpp
42                          // Littéral entier — rvalue
3.14 + 2.0                  // Résultat d'une opération — rvalue
std::string("Hello")        // Objet temporaire — rvalue
creer_widget()              // Valeur de retour d'une fonction (par valeur) — rvalue
x + y                       // Résultat d'une addition — rvalue

// int* ptr = &42;          // ❌ On ne peut PAS prendre l'adresse d'une rvalue
// int* ptr = &(x + y);    // ❌ Idem
```

Le compilateur sait que ces objets sont éphémères. Il peut donc invoquer un constructeur de déplacement plutôt qu'un constructeur de copie quand il les utilise pour initialiser ou affecter un autre objet.

### Résumé visuel

```
                        Expression
                       /          \
                      /            \
                 lvalue            rvalue
              (identité,          (temporaire,
               persiste)           éphémère)
                  │                    │
                  │                    │
          "peut être               "peut être
           copié"                   déplacé"

Exemples :                    Exemples :
  x                             42
  tab[i]                        x + y
  *ptr                          std::string("Hi")
  ref                           creer_widget()
```

---

## Tester la catégorie d'une expression

Un moyen simple de vérifier si une expression est une lvalue ou une rvalue est de tenter de prendre son adresse :

```cpp
int x = 42;

&x;                     // ✅ Compile → x est une lvalue
&(x + 1);              // ❌ Ne compile pas → x + 1 est une rvalue
&std::string("Hi");    // ❌ Ne compile pas → temporaire, rvalue
&(*(&x));              // ✅ Compile → *(&x) est une lvalue
```

Un second test consiste à vérifier si l'expression peut être à gauche d'une affectation :

```cpp
x = 10;                    // ✅ x est une lvalue modifiable
(x + 1) = 10;             // ❌ x + 1 est une rvalue
std::string("Hi") = "Ho"; // ❌ Temporaire, rvalue
```

> 💡 Ces tests sont des heuristiques utiles, pas des définitions formelles. Le standard C++ définit les catégories de valeurs de manière plus rigoureuse — nous y viendrons dans la section sur la taxonomie complète.

---

## Les références rvalue : T&&

C++11 introduit un nouveau type de référence : la **référence rvalue**, notée `T&&`. Elle se lie exclusivement aux rvalues — les objets temporaires ou les objets explicitement marqués comme déplaçables.

### Rappel : les références lvalue (T&)

Les références classiques, que vous connaissez déjà, sont des références **lvalue**. Elles se lient aux lvalues :

```cpp
int x = 42;
int& ref = x;          // ✅ Référence lvalue → lvalue

// int& ref2 = 42;     // ❌ Ne compile pas : 42 est une rvalue
// int& ref3 = x + 1;  // ❌ Ne compile pas : x + 1 est une rvalue
```

Exception importante : une référence lvalue **constante** (`const T&`) peut se lier à une rvalue. C'est un mécanisme historique du C++ qui existait bien avant C++11 :

```cpp
const int& ref = 42;           // ✅ const lvalue ref peut lier une rvalue
const std::string& r = std::string("Hi");  // ✅ Idem

// Le compilateur crée un temporaire dont la durée de vie est étendue
// à celle de la référence
```

Ce mécanisme est la raison pour laquelle `const T&` est utilisé partout comme type de paramètre en C++ — il accepte à la fois les lvalues et les rvalues.

### Les références rvalue (T&&)

Les références rvalue sont le miroir : elles se lient **uniquement aux rvalues** :

```cpp
int&& rref = 42;                  // ✅ Référence rvalue → rvalue (littéral)
int&& rref2 = x + 1;             // ✅ Référence rvalue → rvalue (temporaire)
std::string&& rref3 = std::string("Hi");  // ✅ Référence rvalue → rvalue

// int&& rref4 = x;              // ❌ Ne compile pas : x est une lvalue
```

La référence rvalue **prolonge la durée de vie** du temporaire auquel elle est liée, exactement comme `const T&`. Mais contrairement à `const T&`, elle autorise la modification du temporaire — ce qui est essentiel pour pouvoir voler ses ressources.

### Le paradoxe : une référence rvalue nommée est une lvalue

C'est le point le plus déroutant de toute la sémantique de mouvement, et il est crucial de le comprendre :

```cpp
void foo(std::string&& s) {
    // s est une référence rvalue...
    // ...mais l'expression "s" est une LVALUE !
    // Parce que s a un nom et un emplacement mémoire stable.

    std::string a = s;              // COPIE — s est une lvalue ici
    std::string b = std::move(s);   // DÉPLACEMENT — std::move convertit en rvalue
}
```

Une référence rvalue (`T&&`) est un **type**. Lvalue et rvalue sont des **catégories d'expression**. Ce sont deux dimensions orthogonales :

- Le paramètre `s` a le **type** `std::string&&` (référence rvalue).
- L'expression `s` dans le corps de la fonction est une **lvalue** (elle a un nom).

C'est pourquoi `std::move` est nécessaire à l'intérieur de la fonction : sans lui, le compilateur traite `s` comme une lvalue et invoque la copie. `std::move` reconvertit explicitement `s` en rvalue pour déclencher le déplacement.

```cpp
void foo(std::string&& s) {
    bar(s);              // bar reçoit une lvalue → copie (ou ref lvalue)
    bar(std::move(s));   // bar reçoit une rvalue → déplacement possible
}
```

Ce comportement est intentionnel et sûr : si `s` était implicitement traité comme une rvalue partout dans la fonction, il serait vidé dès la première utilisation, rendant toute utilisation ultérieure dangereuse.

---

## La surcharge lvalue/rvalue : le mécanisme central

Le véritable pouvoir des références rvalue est de permettre la **surcharge** d'une fonction selon la catégorie de valeur de l'argument. C'est ce mécanisme qui permet au compilateur de choisir automatiquement entre copie et déplacement.

```cpp
class MyString {
    size_t size_;
    char* data_;

public:
    // Surcharge pour les lvalues → copie
    MyString(const MyString& other)
        : size_(other.size_), data_(new char[other.size_])
    {
        std::memcpy(data_, other.data_, size_);
        std::print("[copie] {} octets copiés\n", size_);
    }

    // Surcharge pour les rvalues → déplacement
    MyString(MyString&& other) noexcept
        : size_(other.size_), data_(other.data_)
    {
        other.size_ = 0;
        other.data_ = nullptr;
        std::print("[move] ressources transférées\n");
    }
};
```

Le compilateur choisit la surcharge selon la catégorie de valeur de l'argument :

```cpp
MyString a("Hello");

MyString b = a;                  // a est une lvalue   → constructeur de copie
MyString c = MyString("World");  // temporaire rvalue   → constructeur de déplacement
MyString d = std::move(a);       // std::move → rvalue  → constructeur de déplacement
```

Ce mécanisme s'étend naturellement aux opérateurs d'affectation :

```cpp
class MyString {
    // ...

    // Affectation par copie (lvalue)
    MyString& operator=(const MyString& other);

    // Affectation par déplacement (rvalue)
    MyString& operator=(MyString&& other) noexcept;
};

MyString x("Hello");
MyString y("World");

x = y;                  // y est une lvalue   → affectation par copie
x = MyString("Temp");   // temporaire rvalue   → affectation par déplacement
x = std::move(y);       // std::move → rvalue  → affectation par déplacement
```

### Résolution de surcharge : tableau récapitulatif

Quand le compilateur doit choisir entre les surcharges, il applique ces règles de priorité :

| Argument | `f(T&)` | `f(const T&)` | `f(T&&)` | `f(const T&&)` |
|---|---|---|---|---|
| lvalue modifiable | **✅ Priorité** | ✅ Fallback | ❌ | ❌ |
| lvalue const | ❌ | **✅ Priorité** | ❌ | ❌ |
| rvalue modifiable | ❌ | ✅ Fallback | **✅ Priorité** | ✅ Fallback |
| rvalue const | ❌ | ✅ Fallback | ❌ | **✅ Priorité** |

Le cas le plus important à retenir : si une surcharge `T&&` existe, elle **capture les rvalues en priorité**. Si elle n'existe pas, `const T&` sert de fallback universel — c'est pourquoi du code pré-C++11 (sans constructeur de déplacement) continue de fonctionner : tout passe par la copie.

---

## La taxonomie complète du standard (pour les curieux)

Le standard C++ définit cinq catégories de valeurs organisées en une hiérarchie. Cette taxonomie est plus fine que la distinction lvalue/rvalue, mais les deux catégories que vous utiliserez au quotidien restent lvalue et prvalue.

```
                     expression
                    /          \
                   /            \
              glvalue          rvalue
             /      \         /      \
            /        \       /        \
        lvalue      xvalue        prvalue
```

### glvalue (generalized lvalue)

Une expression qui a une **identité** — elle désigne un objet ou une fonction localisable en mémoire. Regroupe les lvalues et les xvalues.

### prvalue (pure rvalue)

Une expression qui n'a **pas d'identité** et qui produit une valeur temporaire. C'est la rvalue « classique » — le résultat d'un calcul, un littéral, un objet temporaire construit inline.

```cpp
42                        // prvalue — littéral
x + y                     // prvalue — résultat d'opération
std::string("Hello")      // prvalue — temporaire construit
creer_widget()            // prvalue — valeur de retour par valeur
```

### xvalue (expiring value)

Une expression qui a une **identité** mais dont les ressources peuvent être **volées** — c'est un objet « expirant ». C'est le résultat de `std::move` : l'objet a un nom et un emplacement mémoire, mais son propriétaire a signalé qu'il ne s'en sert plus.

```cpp
std::move(x)              // xvalue — x a une identité mais est marqué comme abandonné
std::move(vec[0])         // xvalue — l'élément a une identité mais est marqué
```

Les xvalues sont à la frontière : elles ont une identité (comme les lvalues) mais sont déplaçables (comme les rvalues). C'est pourquoi elles apparaissent des deux côtés de l'arbre — sous glvalue (identité) et sous rvalue (déplaçable).

### Résumé de la taxonomie

| Catégorie | A une identité ? | Ressources volables ? | Exemples |
|---|---|---|---|
| **lvalue** | Oui | Non | `x`, `tab[i]`, `*ptr`, `ref` |
| **xvalue** | Oui | Oui | `std::move(x)`, `static_cast<T&&>(x)` |
| **prvalue** | Non | Oui | `42`, `x + y`, `std::string("Hi")` |

En pratique :

- Si l'expression a un **nom** et que vous n'avez pas écrit `std::move` → c'est une **lvalue** → **copie**.
- Si l'expression est un **temporaire** ou le résultat de `std::move` → c'est une **rvalue** (prvalue ou xvalue) → **déplacement**.

---

## Piège courant : && dans un template ≠ référence rvalue

Il existe un cas où `T&&` ne signifie **pas** « référence rvalue » : quand `T` est un paramètre template déduit. Dans ce contexte, `T&&` est une **forwarding reference** (anciennement appelée *universal reference*), qui peut se lier aussi bien aux lvalues qu'aux rvalues :

```cpp
// Ceci est une FORWARDING REFERENCE, pas une référence rvalue
template <typename T>
void wrapper(T&& arg) {
    // arg peut être une lvalue ou une rvalue selon l'appel
}

int x = 42;
wrapper(x);       // T déduit comme int& → T&& = int& && = int& (lvalue ref)
wrapper(42);      // T déduit comme int  → T&& = int&&             (rvalue ref)
```

Ce mécanisme — les *reference collapsing rules* — est au cœur du *perfect forwarding*, traité en détail en [section 10.4](/10-move-semantics/04-perfect-forwarding.md). Pour l'instant, retenez la règle suivante :

- `T&&` où `T` est un **type concret** (ex: `std::string&&`) → **référence rvalue**.
- `T&&` où `T` est un **paramètre template déduit** → **forwarding reference**.

```cpp
void f(std::string&& s);      // Référence rvalue — n'accepte que les rvalues

template <typename T>
void g(T&& s);                 // Forwarding reference — accepte lvalues ET rvalues
```

De même, `auto&&` est toujours une forwarding reference :

```cpp
auto&& a = x;        // x est une lvalue  → a est int& (lvalue ref)
auto&& b = 42;       // 42 est une rvalue → b est int&& (rvalue ref)

// Utilisation courante dans les range-based for loops
for (auto&& elem : conteneur) {
    // elem se lie à chaque élément avec la catégorie appropriée
}
```

---

## Résumé

| Concept | Détail |
|---|---|
| **Lvalue** | Expression avec identité et nom. Persiste au-delà de l'expression. Ne peut être que copiée. |
| **Rvalue** | Expression temporaire ou marquée comme abandonnée. Peut être déplacée. |
| **`T&`** | Référence lvalue — se lie aux lvalues uniquement |
| **`const T&`** | Référence lvalue const — se lie aux lvalues ET rvalues (fallback universel) |
| **`T&&`** (type concret) | Référence rvalue — se lie aux rvalues uniquement |
| **`T&&`** (template déduit) | Forwarding reference — se lie aux deux catégories |
| **`std::move(x)`** | Convertit la lvalue `x` en xvalue (rvalue) — donne la permission de déplacer |
| **Surcharge** | `f(const T&)` attrape les lvalues, `f(T&&)` attrape les rvalues en priorité |

> **Règle pratique** — Si l'expression a un nom, c'est une lvalue. Si elle n'en a pas (temporaire, littéral, résultat de calcul), c'est une rvalue. Si vous voulez qu'une lvalue soit traitée comme une rvalue, utilisez `std::move`. C'est aussi simple que ça pour 99% du code que vous écrirez.

⏭️ [std::move : Transfert de propriété sans copie](/10-move-semantics/02-std-move.md)
