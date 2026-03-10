🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 7.5 — Coût du polymorphisme dynamique en performance

## Chapitre 7 : Héritage et Polymorphisme · Module 3 : Programmation Orientée Objet

---

## Introduction

Les sections précédentes ont présenté le polymorphisme dynamique comme un outil de conception puissant : il permet de manipuler des objets de types différents à travers une interface commune, en déléguant la résolution des appels à l'exécution. Mais en C++, tout a un coût, et la philosophie du langage — *"you don't pay for what you don't use"* — impose de comprendre ce coût avant de s'engager dans un design polymorphique.

Le polymorphisme dynamique introduit un surcoût à trois niveaux : **mémoire** (le vptr dans chaque objet), **exécution** (l'indirection via la vtable) et **optimisation manquée** (l'impossibilité pour le compilateur d'inliner les appels virtuels). Aucun de ces surcoûts n'est rédhibitoire en soi, mais leur impact cumulé peut devenir significatif dans des contextes à haute performance — boucles critiques, traitement de flux massifs, moteurs de calcul.

Cette section quantifie ces coûts, identifie les situations où ils importent réellement et présente les alternatives que le C++ moderne met à disposition pour atteindre le polymorphisme sans payer le prix du dispatch dynamique.

---

## Les trois sources de surcoût

### 1. Surcoût mémoire : le vptr

Chaque objet d'une classe contenant au moins une fonction virtuelle porte un **vptr** — un pointeur vers la vtable de sa classe concrète. Sur une architecture 64 bits, cela représente 8 octets supplémentaires par objet.

Pour un objet volumineux (centaines d'octets de données membres), ces 8 octets sont négligeables. Mais pour un petit objet, l'impact relatif peut être important :

```cpp
class Point3D {
    float x_, y_, z_;   // 12 octets
};

class Point3DVirtual {
    float x_, y_, z_;   // 12 octets
public:
    virtual ~Point3DVirtual() = default;
};

static_assert(sizeof(Point3D) == 12);  
static_assert(sizeof(Point3DVirtual) == 24);   // 8 (vptr) + 12 + 4 (padding)  
```

Le `Point3DVirtual` est **deux fois plus grand** que `Point3D`. Pour un million de points dans un `std::vector`, cela représente 12 Mo contre 24 Mo — soit le double de mémoire consommée et le double de données à charger dans le cache CPU pour itérer sur la collection.

### 2. Surcoût d'appel : la double indirection

Un appel non virtuel est une instruction `call` directe vers une adresse connue à la compilation. Un appel virtuel passe par une double indirection :

```
Appel direct (non-virtual) :
    call  Cercle::dessiner        →  1 instruction

Appel virtuel :
    mov   rax, [rdi]              →  lecture du vptr (accès mémoire)
    call  [rax + offset]          →  lecture de l'adresse dans la vtable + appel
```

Le surcoût brut est de l'ordre de **quelques nanosecondes** par appel sur un processeur moderne. Isolément, c'est invisible. Mais dans une boucle appelée des millions de fois par seconde, ces nanosecondes s'accumulent.

### 3. Surcoût d'optimisation manquée : l'impossibilité d'inlining

C'est souvent le coût le **plus significatif** en pratique, et paradoxalement le moins visible. Quand le compilateur connaît la cible d'un appel à la compilation, il peut **inliner** la fonction — c'est-à-dire remplacer l'appel par le corps de la fonction directement dans le code appelant. L'inlining permet ensuite d'autres optimisations en cascade : élimination de variables temporaires, propagation de constantes, vectorisation, etc.

Un appel virtuel empêche l'inlining car le compilateur ne connaît pas la cible au moment de la compilation. Il doit générer un appel indirect, ce qui coupe la chaîne d'optimisations.

Considérons un exemple concret :

```cpp
class Filtre {  
public:  
    virtual double appliquer(double valeur) const = 0;
    virtual ~Filtre() = default;
};

class FiltreGain final : public Filtre {
    double gain_;
public:
    explicit FiltreGain(double g) : gain_{g} {}
    double appliquer(double valeur) const override {
        return valeur * gain_;
    }
};
```

La méthode `appliquer()` ne fait qu'une multiplication. Inlinée, elle se réduit à une seule instruction assembleur (`mulsd`). Via un appel virtuel, elle nécessite le chargement du vptr, l'indexation dans la vtable, un `call` indirect, le prologue/épilogue de la fonction, puis le `mulsd`. Le ratio de surcoût peut atteindre **5× à 20×** pour des fonctions aussi courtes.

---

## Quantifier le coût : approche par micro-benchmark

Le meilleur moyen de comprendre le coût réel est de le mesurer. Voici un scénario typique : appliquer une opération à chaque élément d'un grand vecteur, en comparant un appel direct, un appel virtuel et un appel virtuel dévirtualisé.

### Scénario de mesure

```cpp
#include <vector>
#include <chrono>
#include <print>
#include <memory>

class Operation {  
public:  
    virtual double executer(double x) const = 0;
    virtual ~Operation() = default;
};

class Doubler final : public Operation {  
public:  
    double executer(double x) const override { return x * 2.0; }
};

// Version sans virtual — appel direct
double traiter_direct(std::vector<double> const& data) {
    double somme = 0.0;
    for (auto x : data) {
        somme += x * 2.0;   // opération inlinée
    }
    return somme;
}

// Version avec virtual — dispatch dynamique
double traiter_virtuel(std::vector<double> const& data, Operation const& op) {
    double somme = 0.0;
    for (auto x : data) {
        somme += op.executer(x);   // appel virtuel à chaque itération
    }
    return somme;
}
```

### Résultats typiques (indicatifs)

Sur un processeur x86-64 moderne (compilation GCC 15, `-O2`), avec un vecteur de 10 millions d'éléments :

| Méthode | Temps | Ratio |  
|---|---|---|  
| Appel direct (inliné + vectorisé) | ~3 ms | 1× (référence) |  
| Appel virtuel (dispatch dynamique) | ~18 ms | ~6× |  
| Appel virtuel sur classe `final` (dévirtualisé) | ~3 ms | ~1× |

Le ratio varie considérablement selon la complexité de la fonction appelée. Pour une multiplication, le surcoût relatif est élevé car le travail utile est minimal. Pour une fonction qui effectue des centaines d'opérations, le surcoût du dispatch devient négligeable en proportion.

> ⚠️ Ces chiffres sont **indicatifs** et dépendent fortement de l'architecture, du compilateur, du niveau d'optimisation et du pattern d'accès mémoire. Utilisez Google Benchmark (section 35.1) pour mesurer dans votre contexte réel. Les micro-benchmarks isolés surestiment souvent le surcoût car ils amplifient l'impact de l'indirection en éliminant les effets de pipeline et de prédiction de branchement qui, dans du code réel, atténuent le coût.

---

## L'impact cache : le coût invisible

Au-delà de l'indirection elle-même, le polymorphisme dynamique a un impact sur le **cache CPU** qui est souvent le facteur dominant en performance réelle.

### Fragmentation mémoire des collections polymorphiques

Une collection polymorphique typique est un `std::vector<std::unique_ptr<Base>>`. Chaque `unique_ptr` pointe vers un objet alloué individuellement sur le heap. Itérer sur cette collection implique de suivre des pointeurs vers des emplacements mémoire potentiellement dispersés :

```
vector (contigu en mémoire) :
┌─────┬─────┬─────┬─────┬─────┐
│ ptr │ ptr │ ptr │ ptr │ ptr │
└──┬──┴──┬──┴──┬──┴──┬──┴──┬──┘
   │     │     │     │     │
   ▼     ▼     ▼     ▼     ▼
  obj   obj   obj   obj   obj    ← éparpillés sur le heap
 @0x10 @0x8F @0x42 @0xAB @0x67
```

Chaque déréférencement de pointeur est un **accès mémoire potentiellement non-local**. Si l'objet pointé n'est pas dans le cache L1 (32-64 Ko), il faut le charger depuis le L2 (~4-10 cycles) ou le L3 (~30-50 cycles), voire depuis la mémoire principale (~100-300 cycles). Pour un vecteur de milliers d'objets, les cache misses dominent largement le coût de l'indirection vtable.

### Comparaison avec un vecteur de valeurs

Un `std::vector<Cercle>` (type concret, par valeur) stocke tous les objets de manière **contiguë en mémoire**. L'itération est séquentielle, le prefetcher matériel anticipe les accès, et le cache est utilisé de manière optimale :

```
vector<Cercle> (contigu en mémoire) :
┌─────────┬─────────┬─────────┬─────────┬─────────┐
│ Cercle  │ Cercle  │ Cercle  │ Cercle  │ Cercle  │
│ rayon=5 │ rayon=3 │ rayon=8 │ rayon=1 │ rayon=4 │
└─────────┴─────────┴─────────┴─────────┴─────────┘
 accès séquentiel → prefetcher efficace → cache optimal
```

Mais ce layout n'est possible que si tous les éléments ont le même type. Dès qu'on veut mélanger des `Cercle` et des `Rectangle` dans la même collection, il faut des pointeurs — et on perd la contiguïté.

### Atténuation : pool allocators

Pour les cas où la performance cache est critique et le polymorphisme dynamique nécessaire, on peut utiliser un **pool allocator** qui alloue les objets dans des blocs contigus plutôt que via `new` individuel. Cela restaure partiellement la localité spatiale tout en conservant le dispatch dynamique. Ce sujet avancé relève de l'optimisation mémoire (chapitre 41).

---

## La dévirtualisation : quand le compilateur compense

Les compilateurs modernes sont capables de **dévirtualiser** des appels virtuels — c'est-à-dire de remplacer un dispatch dynamique par un appel direct — quand ils peuvent prouver le type concret de l'objet. C'est une optimisation transparente qui élimine entièrement le surcoût dans les cas favorables.

### Cas dévirtualisés par le compilateur

**Variable locale de type concret :**

```cpp
Cercle c{5.0};  
c.dessiner();   // dévirtualisé : le compilateur sait que c est un Cercle  
```

**Objet construit localement via `make_unique` :**

```cpp
auto ptr = std::make_unique<Cercle>(5.0);  
ptr->dessiner();   // souvent dévirtualisé (dépend du compilateur et du niveau -O)  
```

**Classe ou méthode marquée `final` :**

```cpp
class Cercle final : public Forme { /* ... */ };

void rendu(Cercle const& c) {
    c.dessiner();   // dévirtualisé grâce à final
}
```

**Propagation interprocédurale (LTO) :**

Avec Link-Time Optimization (`-flto`, section 41.5), le compilateur peut dévirtualiser des appels même à travers les frontières de fichiers objets, car il dispose de la vue globale du programme au moment du link.

### Cas qui résistent à la dévirtualisation

**Référence ou pointeur vers le type de base dans un contexte générique :**

```cpp
void rendu(Forme const& f) {
    f.dessiner();   // PAS dévirtualisé — f pourrait être n'importe quelle Forme
}
```

**Objets dans une collection polymorphique :**

```cpp
for (auto const& forme : formes) {     // vector<unique_ptr<Forme>>
    forme->dessiner();   // PAS dévirtualisé — type concret inconnu
}
```

**Objet reçu depuis une bibliothèque externe ou un plugin :**

```cpp
auto forme = plugin->creer_forme();   // type concret inconnu au compilateur  
forme->dessiner();                     // PAS dévirtualisé  
```

### Vérifier la dévirtualisation

On peut vérifier si un appel a été dévirtualisé en inspectant l'assembleur généré :

```bash
g++ -std=c++23 -O2 -S -o output.s source.cpp
```

Un appel dévirtualisé apparaît comme un `call` direct vers un symbole nommé (`call Cercle::dessiner()`). Un appel virtuel apparaît comme un `call` indirect via un registre (`call [rax]` ou `call *(%rax)`).

Compiler Explorer ([godbolt.org](https://godbolt.org)) est un outil précieux pour ce type d'inspection sans quitter le navigateur.

---

## Alternatives au polymorphisme dynamique

Quand le surcoût du dispatch dynamique est inacceptable — ou simplement inutile parce que les types sont connus à la compilation — C++ offre plusieurs alternatives qui fournissent une forme de polymorphisme à **coût zéro à l'exécution**.

### 1. CRTP — Curiously Recurring Template Pattern

Le CRTP est un idiome qui simule le polymorphisme via les templates. La classe de base est un template paramétré par la classe dérivée elle-même :

```cpp
template <typename Derived>  
class FormeBase {  
public:  
    void dessiner() const {
        // Appel statique — résolu à la compilation, inlinable
        static_cast<Derived const*>(this)->dessiner_impl();
    }

    double aire() const {
        return static_cast<Derived const*>(this)->aire_impl();
    }
};

class Cercle : public FormeBase<Cercle> {
    double rayon_;
public:
    explicit Cercle(double r) : rayon_{r} {}

    void dessiner_impl() const {
        std::println("○ (rayon = {})", rayon_);
    }

    double aire_impl() const {
        return std::numbers::pi * rayon_ * rayon_;
    }
};
```

L'appel `forme.dessiner()` est résolu à la compilation via le `static_cast`. Il n'y a ni vtable, ni vptr, ni indirection. Le compilateur inline directement `dessiner_impl()`.

Le CRTP sera couvert en détail en section 44.3.

**Limitation :** chaque `FormeBase<Cercle>` et `FormeBase<Rectangle>` sont des types **différents**. On ne peut pas stocker des `Cercle` et des `Rectangle` dans la même collection sans recourir à un type erasure ou à un `std::variant`.

### 2. `std::variant` + `std::visit`

Si l'ensemble des types concrets est connu à la compilation et fermé (pas d'extension par des classes dérivées), `std::variant` offre un polymorphisme basé sur les types et la visite :

```cpp
#include <variant>
#include <vector>
#include <print>
#include <numbers>

class Cercle {
    double rayon_;
public:
    explicit Cercle(double r) : rayon_{r} {}
    double aire() const { return std::numbers::pi * rayon_ * rayon_; }
    void dessiner() const { std::println("○ (rayon = {})", rayon_); }
};

class Rectangle {
    double largeur_, hauteur_;
public:
    Rectangle(double l, double h) : largeur_{l}, hauteur_{h} {}
    double aire() const { return largeur_ * hauteur_; }
    void dessiner() const { std::println("□ ({}x{})", largeur_, hauteur_); }
};

using Forme = std::variant<Cercle, Rectangle>;

void dessiner_forme(Forme const& f) {
    std::visit([](auto const& forme) { forme.dessiner(); }, f);
}

double aire_forme(Forme const& f) {
    return std::visit([](auto const& forme) { return forme.aire(); }, f);
}

int main() {
    std::vector<Forme> formes;
    formes.emplace_back(Cercle{5.0});
    formes.emplace_back(Rectangle{3.0, 4.0});

    for (auto const& f : formes) {
        dessiner_forme(f);
        std::println("  Aire = {:.2f}", aire_forme(f));
    }
}
```

Les avantages sont multiples. Les objets sont stockés **par valeur** dans le `std::vector<Forme>` (pas de pointeur, pas d'allocation heap individuelle), ce qui assure une contiguïté mémoire optimale. Le dispatch est résolu à la compilation via le mécanisme de visite — pas de vtable. Le compilateur peut inliner les appels et vectoriser les boucles.

**Limitation :** l'ensemble de types est **fermé**. Ajouter un `Triangle` exige de modifier la définition du `variant` et de recompiler tout le code qui l'utilise. C'est l'exact opposé du polymorphisme dynamique, où ajouter un type dérivé ne nécessite aucune modification du code existant.

### 3. Concepts C++20

Les Concepts (section 16.6) permettent de contraindre des templates pour qu'ils n'acceptent que des types satisfaisant certaines exigences — une forme de "contrat d'interface" vérifié à la compilation :

```cpp
#include <concepts>

template <typename T>  
concept Dessinable = requires(T const& t) {  
    { t.dessiner() } -> std::same_as<void>;
    { t.aire() } -> std::convertible_to<double>;
};

template <Dessinable T>  
void afficher(T const& forme) {  
    forme.dessiner();   // appel direct, inlinable
    std::println("  Aire = {:.2f}", forme.aire());
}
```

`afficher()` accepte tout type qui satisfait le concept `Dessinable`, sans héritage ni vtable. Chaque instanciation du template est un appel direct vers le type concret.

**Limitation :** comme le CRTP, cela ne permet pas de mélanger des types différents dans une même collection sans passer par un `std::variant` ou un type erasure.

### Tableau comparatif

| Critère | `virtual` (dynamique) | CRTP (statique) | `std::variant` | Concepts C++20 |  
|---|---|---|---|---|  
| Résolution | Exécution | Compilation | Compilation | Compilation |  
| Surcoût d'appel | Indirection vtable | Aucun | Switch/jump table | Aucun |  
| Inlining possible | Non (sauf dévirtualisation) | Oui | Oui | Oui |  
| Stockage par valeur | Non (pointeurs) | Oui | Oui | Oui |  
| Ensemble de types ouvert | ✅ Oui | ✅ Oui (via templates) | ❌ Non (fermé) | ✅ Oui (via templates) |  
| Collection hétérogène | ✅ `vector<unique_ptr<Base>>` | ❌ Difficile | ✅ `vector<variant>` | ❌ Difficile |  
| Ajout de type sans recompilation | ✅ Oui | ❌ Non | ❌ Non | ❌ Non |  
| Complexité de mise en œuvre | Faible | Moyenne | Faible | Faible |

---

## Quand le surcoût importe-t-il réellement ?

Le polymorphisme dynamique est souvent critiqué pour ses performances, mais cette critique est **surdimensionnée** dans la majorité des contextes applicatifs. Voici un guide pragmatique :

### Le surcoût est négligeable quand…

- La fonction virtuelle effectue un **travail substantiel** (I/O disque, appel réseau, calcul complexe). Le coût de l'indirection vtable est noyé dans le temps de traitement.  
- Le nombre d'appels virtuels par seconde se compte en **milliers ou dizaines de milliers**. À cette échelle, quelques nanosecondes par appel représentent moins d'une microseconde au total.  
- L'architecture du programme prime sur les micro-performances : **plugins**, **injection de dépendances**, **systèmes de callbacks**. La flexibilité offerte par le polymorphisme dynamique justifie largement son coût.

### Le surcoût devient significatif quand…

- La fonction virtuelle est **triviale** (une multiplication, un accès à un champ) et appelée dans une boucle critique des millions de fois par seconde.  
- Les objets sont **petits** (quelques octets) et stockés en grand nombre. Le vptr double ou triple la taille de chaque objet, dégradant l'utilisation du cache.  
- Le profiling (section 31) montre que le temps passé dans les appels virtuels représente un **pourcentage mesurable** du temps total. Sans cette preuve, l'optimisation est prématurée.

### Règle d'or

> **Mesurez avant d'optimiser.** Utilisez `perf` (section 31.1) ou Google Benchmark (section 35.1) pour identifier les goulots d'étranglement réels. Si le profiling ne montre pas de hotspot lié aux appels virtuels, le polymorphisme dynamique est le bon choix — sa clarté architecturale et sa maintenabilité l'emportent sur un surcoût théorique invisible en pratique.

---

## Stratégie de conception : choisir le bon mécanisme

Le choix entre polymorphisme dynamique et statique n'est pas binaire. Dans un même projet, différentes parties du code ont des contraintes différentes :

```
                        Les types concrets sont-ils
                        connus à la compilation ?
                               │
                    ┌──── OUI ─┴─ NON ─────┐
                    │                      │
            L'ensemble de types            │
            est-il fermé ?                 │
                    │                  Polymorphisme
             ┌─ OUI ┴ NON ──┐          dynamique
             │              │         (virtual)
        std::variant    CRTP ou
        + std::visit    Concepts
```

Et au sein d'une même hiérarchie, on peut combiner les approches. Par exemple, un système de rendu pourrait utiliser le polymorphisme dynamique pour la collection principale de formes (ensemble ouvert, extensible par plugins), mais un `std::variant` pour les primitives les plus fréquentes dans la boucle de rendu critique.

---

## Bonnes pratiques

**Ne sacrifiez pas la clarté du design pour des micro-optimisations.** Le polymorphisme dynamique est l'outil le plus naturel et le plus lisible pour les hiérarchies de types extensibles. Remplacez-le par des alternatives statiques uniquement quand le profiling le justifie.

**Marquez `final` les classes et méthodes terminales.** C'est le moyen le plus simple d'aider le compilateur à dévirtualiser sans changer l'architecture du code. Le gain peut être significatif pour un effort minimal.

**Activez LTO (`-flto`) pour les builds de production.** L'optimisation au link permet au compilateur de dévirtualiser des appels à travers les frontières de fichiers, élargissant considérablement les opportunités d'optimisation.

**Préférez `std::variant` quand l'ensemble de types est petit et fermé.** Pour deux à cinq types concrets connus à la compilation, `std::variant` + `std::visit` offre un meilleur profil de performance qu'une hiérarchie polymorphique, avec un stockage par valeur et un dispatch inlinable.

**Profilez avec `perf stat` pour repérer les branch misses.** Les appels virtuels génèrent des branchements indirects. Un taux élevé de *branch mispredictions* dans `perf stat` peut indiquer que le dispatch dynamique sur des objets de types variés est un goulot d'étranglement. Inversement, si les objets sont majoritairement d'un même type, le prédicteur de branchement s'en sort très bien et le surcoût réel est minime.

**Considérez le data-oriented design pour les cas extrêmes.** Pour du traitement de masse sur des millions d'objets homogènes, le data-oriented design (section 41.1.3) — qui sépare les données des comportements et les organise en *Structure of Arrays* (SoA) — surpasse systématiquement les approches orientées objet, polymorphiques ou non.

---

## Résumé

| Source de surcoût | Impact | Atténuation |  
|---|---|---|  
| **vptr** (8 octets/objet) | Significatif pour les petits objets en grand nombre | Éviter les hiérarchies virtuelles pour les types à forte volumétrie |  
| **Indirection vtable** | ~quelques ns par appel | `final`, LTO, dévirtualisation du compilateur |  
| **Impossibilité d'inlining** | Dominant pour les fonctions triviales en boucle critique | CRTP, `std::variant`, Concepts, `final` |  
| **Fragmentation mémoire** (pointeurs) | Cache misses sur les collections `unique_ptr<Base>` | Pool allocators, `std::variant` pour le stockage par valeur |  
| **Branch mispredictions** | Variable selon la diversité des types concrets | Trier les collections par type, réduire la diversité dans les hot paths |

Le polymorphisme dynamique reste l'**outil par défaut** pour les hiérarchies de types extensibles en C++. Son surcoût est le prix d'une flexibilité architecturale considérable. Les alternatives statiques (CRTP, `std::variant`, Concepts) sont des outils complémentaires pour les chemins critiques où chaque nanoseconde compte — pas des remplacements systématiques.

---


⏭️ [Surcharge d'Opérateurs et Conversions](/08-surcharge-operateurs/README.md)
