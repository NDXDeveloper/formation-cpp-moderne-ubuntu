🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 9.3 std::make_unique et std::make_shared

## Introduction

Tout au long des sections précédentes, nous avons utilisé `std::make_unique` et `std::make_shared` sans justifier en profondeur pourquoi ces fonctions sont préférables à la construction directe avec `new`. Cette section comble cette lacune : vous allez comprendre les **trois raisons fondamentales** — sécurité face aux exceptions, performance, et lisibilité — qui font de ces fonctions factory le standard incontesté du C++ moderne.

La règle est simple : **utilisez toujours `make_unique` et `make_shared` sauf quand c'est impossible**. Les cas d'impossibilité existent, mais ils sont rares et bien identifiés.

---

## Rappel de la syntaxe

Les deux fonctions ont une interface identique : elles prennent le type comme paramètre template et les arguments du constructeur comme paramètres de fonction. Elles allouent l'objet sur le tas et retournent le smart pointer approprié.

```cpp
#include <memory>

// make_unique — retourne un std::unique_ptr<T>
auto u1 = std::make_unique<int>(42);
auto u2 = std::make_unique<std::string>("Hello");
auto u3 = std::make_unique<std::vector<int>>(10, 0);  // vector de 10 zéros

// make_shared — retourne un std::shared_ptr<T>
auto s1 = std::make_shared<int>(42);
auto s2 = std::make_shared<std::string>("Hello");
auto s3 = std::make_shared<std::vector<int>>(10, 0);

// Tableaux (make_unique uniquement, C++14 / C++20)
auto arr = std::make_unique<int[]>(100);  // 100 entiers initialisés à 0
```

Les arguments sont transmis au constructeur de `T` par *perfect forwarding*. Le mot-clé `new` n'apparaît nulle part.

> 💡 `std::make_unique` a été introduit en **C++14**. `std::make_shared` existe depuis **C++11**. Si vous êtes contraint au C++11, vous pouvez écrire votre propre `make_unique` en quelques lignes — mais en 2026, cette situation ne devrait plus se présenter.

---

## Raison n°1 : sécurité face aux exceptions (*exception safety*)

C'est la raison historique et la plus critique. Avant C++17, la construction directe d'un smart pointer avec `new` dans un appel de fonction pouvait provoquer un *memory leak* en cas d'exception.

### Le problème (pré-C++17)

Considérons cet appel de fonction :

```cpp
void traiter(std::shared_ptr<Widget> w, int priorite);

// Construction directe avec new
traiter(std::shared_ptr<Widget>(new Widget()), calculer_priorite());
```

Avant C++17, le compilateur était libre d'évaluer les arguments dans n'importe quel ordre et de les entrelacer. Une séquence d'évaluation possible était :

1. `new Widget()` — allocation sur le tas ✅
2. `calculer_priorite()` — **lève une exception** 💥
3. `std::shared_ptr<Widget>(...)` — **jamais exécuté**

Le `Widget` est alloué à l'étape 1, mais le `shared_ptr` n'est jamais construit à l'étape 3. Le pointeur brut est perdu — *memory leak*.

### La solution avec make_shared

```cpp
// ✅ Aucune fuite possible, quel que soit l'ordre d'évaluation
traiter(std::make_shared<Widget>(), calculer_priorite());
```

Avec `make_shared`, l'allocation et la construction du smart pointer sont une **opération atomique** : soit les deux réussissent, soit aucune des deux ne se produit. Il n'y a pas de pointeur brut intermédiaire exposé.

### C++17 a-t-il corrigé le problème ?

C++17 a modifié les règles d'évaluation des arguments : chaque argument est désormais entièrement évalué avant que l'évaluation du suivant ne commence. Le scénario d'entrelacement décrit ci-dessus **n'est plus possible** en C++17 et au-delà.

Cependant, `make_unique` et `make_shared` restent recommandés pour plusieurs raisons :

- Votre code peut être compilé un jour avec un standard plus ancien (code partagé, rétrocompatibilité).
- Les deux autres raisons (performance et lisibilité) sont indépendantes de cette correction.
- La règle « ne jamais écrire `new` » est plus simple à appliquer et à vérifier qu'une règle conditionnelle.

---

## Raison n°2 : performance (make_shared uniquement)

`std::make_shared` offre un avantage de performance significatif sur la construction directe, grâce à une optimisation d'allocation.

### Le problème de la double allocation

Quand vous construisez un `shared_ptr` avec `new`, **deux allocations** ont lieu :

```cpp
// Deux allocations mémoire distinctes
auto p = std::shared_ptr<Widget>(new Widget());
```

1. `new Widget()` — alloue l'objet `Widget` sur le tas.
2. Le constructeur de `shared_ptr` — alloue le **bloc de contrôle** (compteurs, deleter) sur le tas.

Deux appels à l'allocateur, deux blocs mémoire séparés, deux zones potentiellement éloignées en mémoire.

### L'optimisation de make_shared : allocation unique

`std::make_shared` fusionne les deux allocations en une seule :

```cpp
// Une seule allocation mémoire
auto p = std::make_shared<Widget>();
```

L'objet `Widget` et le bloc de contrôle sont placés dans un **bloc mémoire contigu**. Une seule allocation, une seule zone mémoire.

```
Construction directe (shared_ptr + new) :

  Allocation 1              Allocation 2
  ┌────────────────┐       ┌──────────┐
  │ Control Block  │       │  Widget  │
  │ strong_count   │       │  données │
  │ weak_count     │       │          │
  │ deleter        │       └──────────┘
  └────────────────┘
  (quelque part             (ailleurs
   sur le tas)               sur le tas)


make_shared — allocation unique et contiguë :

  Allocation unique
  ┌────────────────────────────────┐
  │ Control Block  │    Widget     │
  │ strong_count   │    données    │
  │ weak_count     │               │
  │ deleter        │               │
  └────────────────────────────────┘
  (un seul bloc contigu sur le tas)
```

### Bénéfices concrets

**Moins d'appels à l'allocateur.** Chaque appel à `new` (ou `malloc` en interne) a un coût non négligeable : recherche d'un bloc libre, mise à jour des métadonnées de l'allocateur, synchronisation éventuelle en multi-thread. Une allocation au lieu de deux divise ce coût par deux.

**Meilleure localité mémoire.** L'objet et son bloc de contrôle sont adjacents en mémoire. Quand le CPU charge le bloc de contrôle dans le cache (ce qui arrive à chaque copie/destruction du `shared_ptr`), l'objet a de bonnes chances d'être dans la même cache line. Cela réduit les *cache misses* lors de l'accès séquentiel.

**Moins de fragmentation mémoire.** Un gros bloc est plus facile à gérer pour l'allocateur que deux petits blocs séparés.

### Le compromis : durée de vie du bloc de contrôle

L'allocation unique a une contrepartie subtile. Quand l'objet et le bloc de contrôle partagent le même bloc mémoire, la mémoire de l'objet ne peut pas être libérée indépendamment du bloc de contrôle. Or, le bloc de contrôle survit à l'objet tant que des `weak_ptr` existent ([section 9.2.1](/09-smart-pointers/02.1-comptage-references.md)).

Avec la construction directe (`shared_ptr(new T)`), les deux allocations sont indépendantes : l'objet est libéré quand le strong count atteint 0, et le bloc de contrôle est libéré quand le weak count atteint 0.

Avec `make_shared`, le bloc mémoire unique n'est libéré que quand **les deux compteurs** sont à zéro. Si des `weak_ptr` persistent longtemps après la destruction logique de l'objet, la mémoire de l'objet reste occupée (même si le destructeur a été appelé).

```cpp
auto shared = std::make_shared<GrosBuffer>();  // 1 Mo + control block = 1 bloc
std::weak_ptr<GrosBuffer> weak = shared;

shared.reset();
// Le destructeur de GrosBuffer est appelé → l'objet est "détruit"
// MAIS le bloc mémoire (1 Mo + control block) n'est PAS libéré
// car weak maintient le bloc de contrôle en vie

// La mémoire de 1 Mo reste occupée jusqu'à :
weak.reset();  // Maintenant le bloc entier est libéré
```

En pratique, ce compromis est rarement un problème. Il ne devient significatif que dans deux cas :

- L'objet est **très volumineux** (mégaoctets) ET des `weak_ptr` persistent longtemps après la destruction de l'objet.
- Vous avez des contraintes mémoire très serrées (embedded, temps réel).

Dans ces situations spécifiques, la construction directe `shared_ptr(new T)` est justifiée.

---

## Raison n°3 : lisibilité et cohérence

Au-delà de la sécurité et de la performance, `make_unique` et `make_shared` produisent un code plus lisible et plus cohérent.

### Pas de new, pas de delete

L'utilisation systématique des fonctions factory élimine toute occurrence de `new` dans le code applicatif. Cette absence est un signal fort : quand un reviewer voit `new` dans du code moderne, il sait immédiatement qu'il y a une raison particulière (custom deleter, API C…) ou un problème à corriger.

```cpp
// ⚠️ new visible — le reviewer doit vérifier que la propriété est correcte
auto p = std::shared_ptr<Config>(new Config("prod.yaml"));

// ✅ Pas de new — la propriété est correcte par construction
auto p = std::make_shared<Config>("prod.yaml");
```

### Pas de répétition du type

La construction directe répète le type deux fois — une fois pour le smart pointer, une fois pour `new` :

```cpp
// Type répété : shared_ptr<MonTypeTresLong> et new MonTypeTresLong
std::shared_ptr<MonTypeTresLong> p(new MonTypeTresLong(args));

// Type mentionné une seule fois avec auto + make_shared
auto p = std::make_shared<MonTypeTresLong>(args);
```

### Symétrie unique/shared

`make_unique` et `make_shared` ont exactement la même syntaxe. Passer d'un modèle de propriété à l'autre se fait en changeant un seul mot :

```cpp
// Possession exclusive
auto config = std::make_unique<Config>("prod.yaml");

// → Besoin de possession partagée ? Un mot à changer :
auto config = std::make_shared<Config>("prod.yaml");
```

---

## Quand make_unique / make_shared ne fonctionnent PAS

Malgré tous leurs avantages, ces fonctions factory ont des limitations. Voici les cas où la construction directe est nécessaire.

### Custom deleters

`make_unique` et `make_shared` utilisent toujours `delete` (ou `delete[]`) comme deleter. Si vous avez besoin d'un custom deleter, vous devez construire le smart pointer directement :

```cpp
// ❌ Impossible avec make_unique — pas de paramètre deleter
// auto f = std::make_unique<FILE, FCloser>(...);

// ✅ Construction directe obligatoire
auto f = std::unique_ptr<FILE, FCloser>(fopen("data.txt", "r"));
```

C'est le cas d'usage le plus courant de la construction directe, et il est parfaitement légitime. Voir [section 9.1.3](/09-smart-pointers/01.3-custom-deleters.md).

### Constructeurs privés ou protégés

`make_unique` et `make_shared` appellent le constructeur via une fonction template interne. Si le constructeur est privé ou protégé, l'appel échoue même si la classe qui fait l'appel est amie (*friend*) :

```cpp
class Singleton {
    Singleton() = default;  // Constructeur privé
    friend class Factory;

public:
    static std::unique_ptr<Singleton> creer() {
        // ❌ make_unique ne peut pas accéder au constructeur privé
        // return std::make_unique<Singleton>();

        // ✅ Construction directe — le contexte a accès au constructeur
        return std::unique_ptr<Singleton>(new Singleton());
    }
};
```

> 💡 Une solution alternative consiste à utiliser le *passkey idiom* : un type clé dont le constructeur est privé et ami de la classe cible, passé en paramètre du constructeur public.

### Initialisation avec accolades (braced-init-list)

`make_unique` et `make_shared` transmettent les arguments avec des parenthèses `()`, pas avec des accolades `{}`. Cela signifie que l'initialisation par liste d'initialisation ne fonctionne pas directement :

```cpp
// Parenthèses : appelle vector(size_t count, int value)
auto v1 = std::make_shared<std::vector<int>>(10, 0);  // 10 zéros ✅

// Accolades : on voudrait vector{1, 2, 3, 4, 5}
// auto v2 = std::make_shared<std::vector<int>>({1, 2, 3, 4, 5});  // ❌
```

La solution idiomatique est de passer par un `std::initializer_list` explicite :

```cpp
auto init = std::initializer_list<int>{1, 2, 3, 4, 5};
auto v = std::make_shared<std::vector<int>>(init);  // ✅
```

Ou, si cette syntaxe est trop lourde, de construire directement :

```cpp
auto v = std::shared_ptr<std::vector<int>>(
    new std::vector<int>{1, 2, 3, 4, 5}
);
```

### Objets très volumineux avec des weak_ptr longue durée

Comme expliqué dans la section sur le compromis de `make_shared` : si l'objet fait plusieurs mégaoctets et que des `weak_ptr` persistent longtemps après sa destruction logique, la construction directe permet de libérer la mémoire de l'objet indépendamment du bloc de contrôle.

```cpp
// Objet de 10 Mo — on veut libérer la mémoire dès que possible
auto p = std::shared_ptr<HugeBuffer>(new HugeBuffer());
std::weak_ptr<HugeBuffer> w = p;

p.reset();
// Avec la construction directe : les 10 Mo sont libérés immédiatement ✅
// Avec make_shared : les 10 Mo resteraient occupés tant que w existe ⚠️
```

---

## Résumé des cas d'utilisation

| Situation | Recommandation |
|---|---|
| Cas général | `make_unique` / `make_shared` — **toujours** |
| Custom deleter | Construction directe — obligatoire |
| Constructeur privé/protégé | Construction directe — obligatoire |
| Initialisation par `{}` | `initializer_list` explicite ou construction directe |
| Objet volumineux + `weak_ptr` longue durée | Construction directe — justifié |
| Tous les autres cas | `make_unique` / `make_shared` |

---

## make_unique vs make_shared : récapitulatif croisé

| Propriété | `std::make_unique` | `std::make_shared` |
|---|---|---|
| **Disponible depuis** | C++14 | C++11 |
| **Retourne** | `std::unique_ptr<T>` | `std::shared_ptr<T>` |
| **Exception safety** | ✅ Pas de pointeur brut exposé | ✅ Pas de pointeur brut exposé |
| **Allocation unique** | N/A (pas de bloc de contrôle) | ✅ Objet + bloc de contrôle fusionnés |
| **Support `T[]`** | ✅ `make_unique<int[]>(n)` | ✅ `make_shared<int[]>(n)` (C++20) |
| **Custom deleter** | ❌ Impossible | ❌ Impossible |
| **Constructeur privé** | ❌ Impossible | ❌ Impossible |

> **Règle finale** — Écrivez `make_unique` ou `make_shared` par défaut. Si le compilateur refuse, vérifiez si l'un des cas d'exception ci-dessus s'applique. Si oui, utilisez la construction directe en documentant la raison. Si non, c'est probablement un bug dans votre code.

⏭️ [Ne jamais utiliser new/delete dans du code moderne](/09-smart-pointers/04-jamais-new-delete.md)
