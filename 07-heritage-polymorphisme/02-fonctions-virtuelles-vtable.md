🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 7.2 — Fonctions virtuelles et mécanisme de vtable

## Chapitre 7 : Héritage et Polymorphisme · Module 3 : Programmation Orientée Objet

---

## Introduction

La section 7.1.1 a montré que, sans le mot-clé `virtual`, la redéfinition d'une méthode dans une classe dérivée relève du **name hiding** : l'appel est résolu à la compilation en fonction du type statique de la variable. Ce mécanisme est insuffisant pour le polymorphisme — la capacité d'un même appel à exécuter un code différent selon le type réel de l'objet manipulé.

Le mot-clé `virtual` change fondamentalement la donne. Quand une méthode est déclarée `virtual` dans la classe de base, l'appel n'est plus résolu à la compilation mais **à l'exécution**, en fonction du type dynamique de l'objet. Ce mécanisme porte le nom de **dispatch dynamique** (*dynamic dispatch* ou *late binding*).

Mais comment le programme sait-il, à l'exécution, quelle version de la méthode appeler ? La réponse réside dans un mécanisme d'implémentation que le standard C++ ne prescrit pas mais que tous les compilateurs majeurs (GCC, Clang, MSVC) implémentent de manière quasi identique : la **vtable** (*virtual method table*) et le **vptr** (*virtual table pointer*).

Cette section explore les deux faces de la question : la sémantique du langage (ce que `virtual` signifie pour le développeur) et le mécanisme d'implémentation (ce que le compilateur génère réellement). Comprendre ce second aspect est ce qui vous permettra de raisonner sur les performances, la disposition mémoire et les contraintes liées au polymorphisme dynamique.

---

## Le problème : résolution statique vs dynamique

Reprenons un exemple simple pour poser le problème :

```cpp
#include <print>

class Forme {  
public:  
    void dessiner() const {
        std::println("Forme::dessiner()");
    }
};

class Cercle : public Forme {  
public:  
    void dessiner() const {
        std::println("Cercle::dessiner()");
    }
};

class Rectangle : public Forme {  
public:  
    void dessiner() const {
        std::println("Rectangle::dessiner()");
    }
};

void afficher(Forme const& f) {
    f.dessiner();   // Quelle version est appelée ?
}

int main() {
    Cercle c;
    Rectangle r;
    afficher(c);
    afficher(r);
}
```

```
Forme::dessiner()  
Forme::dessiner()  
```

Le résultat est décevant. `afficher()` reçoit une `Forme const&`, donc le compilateur résout l'appel `f.dessiner()` vers `Forme::dessiner()` à la compilation. Le type réel de l'objet (`Cercle` ou `Rectangle`) est ignoré. C'est la **résolution statique** — le comportement par défaut en C++, cohérent avec la philosophie "you don't pay for what you don't use".

---

## La solution : le mot-clé `virtual`

Pour obtenir un comportement polymorphique, il suffit de déclarer `dessiner()` comme `virtual` dans la classe de base :

```cpp
class Forme {  
public:  
    virtual void dessiner() const {
        std::println("Forme::dessiner()");
    }
    virtual ~Forme() = default;   // destructeur virtuel — expliqué plus bas
};

class Cercle : public Forme {  
public:  
    void dessiner() const override {
        std::println("Cercle::dessiner()");
    }
};

class Rectangle : public Forme {  
public:  
    void dessiner() const override {
        std::println("Rectangle::dessiner()");
    }
};
```

Avec le même code d'appel :

```cpp
void afficher(Forme const& f) {
    f.dessiner();   // dispatch dynamique
}

int main() {
    Cercle c;
    Rectangle r;
    afficher(c);
    afficher(r);
}
```

```
Cercle::dessiner()  
Rectangle::dessiner()  
```

Cette fois, l'appel `f.dessiner()` est résolu **à l'exécution** en fonction du type réel de l'objet référencé par `f`. C'est le **dispatch dynamique**. Le mot-clé `override` dans les classes dérivées est traité en détail dans la section 7.3 — pour l'instant, concentrons-nous sur le mécanisme sous-jacent.

---

## Conditions du dispatch dynamique

Le dispatch dynamique ne se produit que lorsque **trois conditions** sont réunies simultanément :

1. La méthode appelée est déclarée `virtual` (dans la classe de base ou un de ses ancêtres).
2. L'appel est effectué via un **pointeur** ou une **référence** vers la classe de base.
3. L'appel n'est pas qualifié avec l'opérateur de portée `::`.

Si l'une de ces conditions n'est pas remplie, l'appel est résolu statiquement :

```cpp
Cercle c;  
Forme& ref = c;  
Forme* ptr = &c;  
Forme  val = c;   // slicing !  

ref.dessiner();           // ✅ Dispatch dynamique → Cercle::dessiner()  
ptr->dessiner();          // ✅ Dispatch dynamique → Cercle::dessiner()  
val.dessiner();           // ❌ Résolution statique → Forme::dessiner() (slicing)  
ref.Forme::dessiner();    // ❌ Résolution statique → Forme::dessiner() (qualifié)  
```

Le cas `val.dessiner()` illustre encore une fois le danger du slicing (section 7.1.1) : l'objet a été copié par valeur dans une variable de type `Forme`, la partie `Cercle` est perdue, et le dispatch dynamique n'a plus de raison de se produire puisque `val` **est** un `Forme`, pas un `Cercle`.

---

## Le mécanisme de vtable : ce que le compilateur génère

Le standard C++ ne prescrit aucune implémentation particulière pour le dispatch dynamique. En pratique, tous les compilateurs majeurs sur toutes les plateformes utilisent le même mécanisme : les **vtables** et les **vptrs**. Sur Linux, l'implémentation est formalisée par l'**Itanium C++ ABI**, suivie par GCC et Clang.

### La vtable (Virtual Method Table)

Pour chaque classe qui contient au moins une fonction virtuelle (directement ou par héritage), le compilateur génère une **table statique en mémoire** contenant les **adresses** des implémentations concrètes de chaque fonction virtuelle pour cette classe. Cette table est la vtable.

Pour notre hiérarchie `Forme`/`Cercle`/`Rectangle`, le compilateur génère trois vtables :

```
vtable de Forme :
┌─────────────────────────────────────┐
│ [0]  → Forme::dessiner()            │
│ [1]  → Forme::~Forme()              │
└─────────────────────────────────────┘

vtable de Cercle :
┌─────────────────────────────────────┐
│ [0]  → Cercle::dessiner()           │  ← redéfinie
│ [1]  → Forme::~Forme()              │  ← héritée (si ~Cercle pas défini)
└─────────────────────────────────────┘

vtable de Rectangle :
┌─────────────────────────────────────┐
│ [0]  → Rectangle::dessiner()        │  ← redéfinie
│ [1]  → Forme::~Forme()              │  ← héritée
└─────────────────────────────────────┘
```

Chaque vtable a la **même structure** (même nombre d'entrées, au même index pour chaque méthode), mais les adresses qu'elle contient diffèrent selon la classe. C'est cette uniformité de structure qui rend le dispatch possible sans connaître le type concret.

### Le vptr (Virtual Table Pointer)

Chaque **objet** d'une classe contenant des fonctions virtuelles possède un pointeur caché — le vptr — qui pointe vers la vtable de sa classe concrète. Ce pointeur est initialisé par le constructeur et occupe typiquement 8 octets sur une architecture 64 bits.

```
Objet Cercle (c)                          Mémoire statique
┌──────────────────────┐
│ vptr ──────────────────────────────►  vtable de Cercle
│ (membres de Forme)   │                ┌─────────────────────┐
│ (membres de Cercle)  │                │ → Cercle::dessiner()│
└──────────────────────┘                │ → ~Forme()          │
                                        └─────────────────────┘

Objet Rectangle (r)
┌──────────────────────┐
│ vptr ──────────────────────────────►  vtable de Rectangle
│ (membres de Forme)   │                ┌──────────────────────┐
│ (membres de Rect.)   │                │ → Rect.::dessiner()  │
└──────────────────────┘                │ → ~Forme()           │
                                        └──────────────────────┘
```

Le vptr est placé **au début** de l'objet (dans l'Itanium ABI), avant les données membres. Cela signifie que l'ajout d'une seule fonction virtuelle à une classe modifie sa disposition mémoire — un point important quand on raisonne sur la compatibilité binaire (ABI).

### Déroulement d'un appel virtuel

Quand le compilateur rencontre `f.dessiner()` où `f` est une référence vers `Forme` et `dessiner()` est `virtual`, il génère le pseudo-code suivant :

```
1. Lire le vptr de l'objet f              →  adresse de la vtable
2. Indexer dans la vtable à l'offset 0    →  adresse de la fonction dessiner()
3. Appeler cette adresse
```

En assembleur x86-64 (simplifié), cela ressemble à :

```asm
mov  rax, [rdi]          ; rdi = adresse de l'objet, [rdi] = vptr  
call [rax]               ; [rax + 0] = première entrée de la vtable = dessiner()  
```

C'est une **double indirection** : d'abord lire le vptr dans l'objet, puis lire l'adresse de la fonction dans la vtable, puis appeler cette adresse. C'est ce surcoût — modeste mais réel — qui distingue un appel virtuel d'un appel direct.

---

## Impact du vptr sur `sizeof`

L'ajout d'un vptr modifie la taille des objets. On peut le constater facilement :

```cpp
class SansVirtual {
    int x_;
};

class AvecVirtual {
    int x_;
public:
    virtual void f() {}
};

// Sur architecture 64 bits :
static_assert(sizeof(SansVirtual) == 4);    // juste un int  
static_assert(sizeof(AvecVirtual) == 16);   // vptr (8) + int (4) + padding (4)  
```

La classe `AvecVirtual` est **quatre fois** plus grande que `SansVirtual` à cause du vptr et de l'alignement. Pour un seul objet, c'est insignifiant. Pour un `std::vector` de millions d'éléments, cela peut impacter l'empreinte mémoire et la performance du cache (section 7.5).

---

## Initialisation du vptr pendant la construction

Le vptr est initialisé par le constructeur, et c'est un processus **progressif** : à chaque niveau de la hiérarchie de construction, le vptr est mis à jour pour pointer vers la vtable de la classe en cours de construction.

```cpp
#include <print>

class Base {  
public:  
    Base() {
        std::println("Base::Base() — type dynamique : Base");
        qui_suis_je();   // appel virtuel dans le constructeur
    }
    virtual void qui_suis_je() const {
        std::println("  → Je suis Base");
    }
    virtual ~Base() = default;
};

class Derivee : public Base {  
public:  
    Derivee() {
        std::println("Derivee::Derivee() — type dynamique : Derivee");
        qui_suis_je();
    }
    void qui_suis_je() const override {
        std::println("  → Je suis Derivee");
    }
};

int main() {
    Derivee d;
}
```

```
Base::Base() — type dynamique : Base
  → Je suis Base
Derivee::Derivee() — type dynamique : Derivee
  → Je suis Derivee
```

Le résultat est contre-intuitif pour les développeurs venant de Java ou Python. Pendant l'exécution du constructeur de `Base`, le vptr pointe vers la **vtable de `Base`**, même si l'objet en cours de construction est un `Derivee`. L'appel virtuel `qui_suis_je()` dans le constructeur de `Base` appelle donc `Base::qui_suis_je()`, pas `Derivee::qui_suis_je()`.

### Pourquoi ce comportement ?

C'est une décision de conception délibérée du langage. Pendant la construction de `Base`, les membres propres à `Derivee` ne sont **pas encore initialisés**. Appeler `Derivee::qui_suis_je()` à ce stade risquerait d'accéder à des données membres non initialisées — un comportement indéfini. En pointant le vptr vers la vtable de `Base` pendant la construction de `Base`, le langage garantit que seules les méthodes de `Base` sont accessibles, ce qui est cohérent avec l'état partiellement construit de l'objet.

Le même mécanisme s'applique **en miroir** pendant la destruction : quand le destructeur de `Base` s'exécute, le vptr a déjà été ramené vers la vtable de `Base`, car les membres de `Derivee` ont déjà été détruits.

> ⚠️ **Règle d'or : n'appelez jamais de fonction virtuelle depuis un constructeur ou un destructeur dans l'intention d'obtenir un dispatch vers une classe dérivée.** Le dispatch se fera toujours vers la classe en cours de construction/destruction. C'est l'un des pièges les plus classiques du C++.

---

## Le destructeur virtuel : une nécessité

Un destructeur virtuel est **indispensable** dès qu'une classe est destinée à servir de base dans une hiérarchie polymorphique. Sans lui, la suppression d'un objet dérivé via un pointeur de base est un **comportement indéfini** :

```cpp
class Forme {  
public:  
    // PAS de destructeur virtuel
    ~Forme() { std::println("~Forme()"); }
};

class Cercle : public Forme {
    std::vector<double> points_;
public:
    ~Cercle() { std::println("~Cercle()"); }
};

int main() {
    Forme* f = new Cercle{};
    delete f;   // ⚠️ COMPORTEMENT INDÉFINI — ~Cercle() n'est PAS appelé
}
```

```
~Forme()       ← ~Cercle() n'est jamais appelé — fuite de points_
```

Le destructeur de `Forme` n'étant pas virtuel, `delete f` appelle directement `~Forme()` sans dispatch dynamique. Le destructeur de `Cercle` n'est jamais exécuté, et le `std::vector<double>` qu'il contient n'est jamais détruit — c'est une fuite mémoire.

La correction est simple :

```cpp
class Forme {  
public:  
    virtual ~Forme() = default;   // ✅ Destructeur virtuel
};
```

Avec un destructeur virtuel, `delete f` effectue un dispatch dynamique vers `~Cercle()`, qui détruit les membres propres à `Cercle`, puis appelle automatiquement `~Forme()`.

### Quelle forme donner au destructeur virtuel ?

Trois approches, selon l'intention :

```cpp
// 1. La classe est une base polymorphique concrète (peut être instanciée)
class Base {  
public:  
    virtual ~Base() = default;
};

// 2. La classe est une interface pure (ne peut pas être instanciée)
class Interface {  
public:  
    virtual void methode() = 0;
    virtual ~Interface() = default;
};

// 3. La classe n'est PAS destinée à l'héritage polymorphique
class Finale {
    // Pas de destructeur virtuel
    // Éventuellement : class Finale final { ... };
};
```

> 💡 La C++ Core Guideline **C.35** stipule : *"A base class destructor should be either public and virtual, or protected and non-virtual."* Si le destructeur est `protected` et non virtuel, la classe ne peut pas être détruite via un pointeur de base, ce qui élimine le risque de comportement indéfini.

---

## Inspection des vtables avec les outils

### GCC : `-fdump-lang-class`

GCC peut générer un rapport détaillé des hiérarchies de classes et de leurs vtables :

```bash
g++ -std=c++23 -fdump-lang-class -c formes.cpp
```

Cela produit un fichier `formes.cpp.001l.class` contenant, pour chaque classe, la liste des fonctions virtuelles et leurs offsets dans la vtable. Pour notre hiérarchie `Forme`/`Cercle` :

```
Vtable for Forme  
Forme::_ZTV5Forme: 4 entries  
0     (int (*)(...))0
8     (int (*)(...))(& _ZTI5Forme)
16    (int (*)(...))Forme::dessiner
24    (int (*)(...))Forme::~Forme

Vtable for Cercle  
Cercle::_ZTV6Cercle: 4 entries  
0     (int (*)(...))0
8     (int (*)(...))(& _ZTI6Cercle)
16    (int (*)(...))Cercle::dessiner      ← remplace Forme::dessiner
24    (int (*)(...))Cercle::~Cercle       ← remplace Forme::~Forme
```

Les deux premières entrées (offset 0 et 8) sont des métadonnées internes de l'ABI (offset-to-top et pointeur RTTI). Les entrées utiles commencent à l'offset 16.

### Clang : `-fdump-record-layouts` et `-fdump-vtable-layouts`

```bash
clang++ -std=c++23 -Xclang -fdump-vtable-layouts -c formes.cpp
```

Clang produit une sortie plus lisible, montrant directement les index et les fonctions associées dans chaque vtable.

### objdump et nm : observer les vtables dans le binaire

Les vtables sont des symboles globaux dans le binaire. On peut les retrouver avec `nm` :

```bash
nm -C programme | grep "vtable"
```

```
0000000000403d50 V vtable for Forme
0000000000403d80 V vtable for Cercle
0000000000403db0 V vtable for Rectangle
```

Et `objdump` permet de voir le contenu brut de ces tables :

```bash
objdump -d -C -j .rodata programme | grep -A 10 "vtable for Cercle"
```

Les vtables sont stockées dans la section `.rodata` (read-only data) du binaire ELF — elles ne sont jamais modifiées à l'exécution.

---

## Fonctions virtuelles et RTTI

Le mécanisme de vtable est intimement lié au **RTTI** (*Run-Time Type Information*). Chaque vtable contient un pointeur vers une structure `std::type_info` qui décrit le type de la classe. C'est ce mécanisme qui permet à `dynamic_cast` et `typeid` de fonctionner :

```cpp
#include <typeinfo>
#include <print>

void identifier(Forme const& f) {
    std::println("Type réel : {}", typeid(f).name());
}

int main() {
    Cercle c;
    Rectangle r;
    identifier(c);   // affiche le nom mangé de Cercle
    identifier(r);   // affiche le nom mangé de Rectangle
}
```

`typeid` accède au pointeur RTTI stocké dans la vtable pointée par le vptr de l'objet. Sans fonctions virtuelles, `typeid` retourne le type **statique**, pas le type dynamique.

Le RTTI peut être désactivé avec l'option `-fno-rtti` (GCC/Clang), ce qui réduit légèrement la taille du binaire et des vtables mais rend `dynamic_cast` et `typeid` inutilisables. Certains projets à forte contrainte de taille (embarqué, jeux vidéo) font ce choix.

---

## Appel virtuel vs appel direct : le compilateur optimise

Le dispatch dynamique a un coût (double indirection, impossibilité d'inlining), mais le compilateur est souvent capable de **dévirtualiser** un appel quand il peut prouver le type concret de l'objet à la compilation :

```cpp
Cercle c;  
c.dessiner();          // le compilateur SAIT que c est un Cercle  
                       // → appel direct, pas de dispatch dynamique
```

Ici, `c` est une variable locale de type `Cercle`, pas un pointeur ni une référence vers `Forme`. Le compilateur génère un appel direct à `Cercle::dessiner()`, exactement comme si la méthode n'était pas virtuelle. C'est la **dévirtualisation**.

Les compilateurs modernes (GCC 15, Clang 20) sont de plus en plus agressifs dans cette optimisation. Avec l'option `-O2` ou supérieure, ils peuvent dévirtualiser des cas plus subtils :

```cpp
auto ptr = std::make_unique<Cercle>();  
ptr->dessiner();   // le type concret est connu → dévirtualisation possible  
```

Le mot-clé `final` (section 7.3) aide considérablement le compilateur dans cette optimisation en lui garantissant qu'aucune classe ne dérivera plus de la classe ou ne redéfinira la méthode.

---

## Synthèse du mécanisme

Voici le tableau complet de ce qui se passe pour un appel `f.dessiner()` selon les conditions :

| `dessiner()` est `virtual` ? | Appel via pointeur/référence ? | Qualifié (`Base::`) ? | Type de dispatch |  
|---|---|---|---|  
| Non | — | — | **Statique** (name hiding si redéfini) |  
| Oui | Non (variable locale) | — | **Statique** (dévirtualisé par le compilateur) |  
| Oui | Oui | Non | **Dynamique** (vtable) |  
| Oui | Oui | Oui | **Statique** (appel forcé à la version de Base) |

---

## Bonnes pratiques

**Déclarez `virtual` uniquement dans la classe de base.** Une fois qu'une méthode est `virtual`, elle l'est dans toute la hiérarchie. Il n'est pas nécessaire (ni recommandé) de répéter `virtual` dans les classes dérivées — utilisez `override` à la place (section 7.3).

**Rendez le destructeur `virtual` dans toute classe de base polymorphique.** C'est la règle la plus importante de ce chapitre. Un destructeur non virtuel dans une classe de base est une bombe à retardement.

**N'appelez pas de fonctions virtuelles dans les constructeurs et destructeurs.** Le dispatch ne se fait pas vers la classe dérivée à ces moments-là. Si vous avez besoin d'un comportement personnalisé à la construction, utilisez une méthode d'initialisation séparée appelée après la construction complète.

**Activez les warnings pour détecter les oublis.** L'option `-Wnon-virtual-dtor` (GCC/Clang) avertit quand une classe avec des fonctions virtuelles n'a pas de destructeur virtuel. Combinée avec `-Werror`, elle transforme cet oubli en erreur de compilation.

**Ne désactivez le RTTI (`-fno-rtti`) que si vous en comprenez les conséquences.** `dynamic_cast` et `typeid` cesseront de fonctionner, et certaines bibliothèques tierces qui en dépendent pourront échouer à la compilation ou au link.

---

## Résumé

| Concept | Description |  
|---|---|  
| **`virtual`** | Déclare qu'une méthode doit être dispatchée dynamiquement |  
| **vtable** | Table statique par classe contenant les adresses des implémentations virtuelles |  
| **vptr** | Pointeur caché dans chaque objet, initialisé par le constructeur, pointe vers la vtable de la classe concrète |  
| **Dispatch dynamique** | Double indirection : objet → vptr → vtable → adresse de la fonction |  
| **Dévirtualisation** | Optimisation du compilateur qui remplace un appel virtuel par un appel direct quand le type est connu |  
| **Destructeur virtuel** | Obligatoire dans toute classe de base polymorphique pour éviter un comportement indéfini |  
| **Appel virtuel dans un constructeur** | Dispatch vers la classe en cours de construction, pas vers la dérivée |  
| **RTTI** | Information de type stockée dans la vtable, utilisée par `dynamic_cast` et `typeid` |  
| **Surcoût mémoire** | +8 octets par objet (vptr) sur architecture 64 bits |  
| **Surcoût d'appel** | Une à deux indirections supplémentaires par appel virtuel |

---


⏭️ [Polymorphisme dynamique : virtual, override, final](/07-heritage-polymorphisme/03-polymorphisme-dynamique.md)
