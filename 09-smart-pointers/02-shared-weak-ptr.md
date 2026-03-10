🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 9.2 std::shared_ptr et std::weak_ptr

## Introduction

Dans la section précédente, `std::unique_ptr` répondait à un modèle simple : **un seul propriétaire** à la fois. C'est le cas le plus fréquent, et c'est pourquoi `unique_ptr` devrait toujours être votre premier réflexe.

Mais certaines situations ne rentrent pas dans ce modèle. Parfois, une ressource est légitimement partagée entre plusieurs parties du code, sans qu'un propriétaire unique et évident ne se dégage. Qui doit libérer un nœud dans un graphe quand plusieurs autres nœuds le référencent ? Qui doit détruire un cache partagé entre plusieurs threads ? Qui est responsable d'un objet de configuration utilisé par une dizaine de modules ?

C'est le rôle de `std::shared_ptr` : modéliser une **possession partagée**. La ressource reste vivante tant qu'au moins un `shared_ptr` la référence. Quand le dernier `shared_ptr` est détruit, la ressource est automatiquement libérée.

Et parce que la possession partagée introduit un risque spécifique — les **cycles de références** — le standard fournit un compagnon indispensable : `std::weak_ptr`, un observateur non-possédant qui peut surveiller une ressource partagée sans empêcher sa destruction.

```cpp
#include <memory>

// Deux shared_ptr partagent la même ressource
auto p1 = std::make_shared<std::string>("Bonjour");  
auto p2 = p1;  // ✅ Copie autorisée — possession partagée  

std::print("{}\n", *p1);  // "Bonjour"  
std::print("{}\n", *p2);  // "Bonjour" — même objet  

// p1 et p2 pointent vers le même string
std::print("Même adresse ? {}\n", p1.get() == p2.get());  // true
```

---

## Pourquoi « shared » ?

Le nom `shared_ptr` exprime que la **propriété est partagée**. Contrairement à `unique_ptr`, la copie est non seulement autorisée mais constitue le mécanisme central du smart pointer : chaque copie crée un nouveau co-propriétaire.

```cpp
auto a = std::make_shared<int>(42);  // 1 propriétaire  
auto b = a;                          // 2 propriétaires — copie  
auto c = a;                          // 3 propriétaires — copie  

// Les trois pointent vers le même int(42)
// La ressource sera libérée quand a, b ET c seront tous détruits
```

Pour savoir combien de `shared_ptr` référencent une même ressource, chaque instance maintient un **compteur de références** (*reference count*). Ce compteur est incrémenté à chaque copie, décrémenté à chaque destruction ou réaffectation, et quand il atteint zéro, la ressource est libérée. Le mécanisme complet est détaillé en [section 9.2.1](/09-smart-pointers/02.1-comptage-references.md).

---

## Le coût de la possession partagée

`std::shared_ptr` est plus puissant que `std::unique_ptr`, mais cette puissance a un prix. Il est essentiel de comprendre ce coût pour faire un choix éclairé.

### Surcoût mémoire

Un `shared_ptr` ne contient pas seulement un pointeur vers la ressource. Il pointe aussi vers un **bloc de contrôle** (*control block*) alloué séparément sur le tas, qui contient :

- Le **compteur de références fort** (*strong count*) — nombre de `shared_ptr` actifs.
- Le **compteur de références faible** (*weak count*) — nombre de `weak_ptr` actifs.
- Le **deleter** (éventuellement personnalisé).
- L'**allocateur** utilisé pour le bloc de contrôle.

```
shared_ptr<T>             Control Block              Objet T
┌──────────────┐         ┌──────────────────┐       ┌──────────┐
│ ptr ─────────┼───────────────────────────────────>│          │
│ ctrl_block ──┼──┐      │ strong_count: 2  │       │  données │
└──────────────┘  │      │ weak_count:   1  │       │          │
                  └─────>│ deleter          │       └──────────┘
shared_ptr<T>            │ allocator        │            ▲
┌──────────────┐         └──────────────────┘            │
│ ptr ─────────┼─────────────────────────────────────────┘
│ ctrl_block ──┼──┐            ▲
└──────────────┘  │            │
                  └────────────┘
weak_ptr<T>                    ▲
┌──────────────┐               │
│ ptr ─────────┼───────────────────────────────────>(Objet T)
│ ctrl_block ──┼───────────────┘
└──────────────┘
```

Conséquences concrètes sur la taille :

```cpp
#include <memory>
#include <iostream>

std::print("sizeof(int*)              = {}\n", sizeof(int*));              // 8  
std::print("sizeof(unique_ptr<int>)   = {}\n", sizeof(std::unique_ptr<int>));  // 8  
std::print("sizeof(shared_ptr<int>)   = {}\n", sizeof(std::shared_ptr<int>));  // 16  
```

Un `shared_ptr` fait **le double** d'un `unique_ptr` : il stocke deux pointeurs (un vers la ressource, un vers le bloc de contrôle). Et le bloc de contrôle lui-même consomme de la mémoire supplémentaire sur le tas — typiquement 16 à 32 octets selon l'implémentation.

### Surcoût CPU

Chaque copie, destruction ou réaffectation d'un `shared_ptr` incrémente ou décrémente le compteur de références. Ces opérations sont **atomiques** — elles utilisent des instructions CPU spéciales (`lock xadd` ou équivalent) pour être thread-safe. Même dans du code mono-thread, ce coût atomique est payé systématiquement.

En comparaison, `unique_ptr` ne maintient aucun compteur et ne fait aucune opération atomique. La différence est mesurable dans les boucles serrées ou quand des millions de smart pointers sont copiés.

### Résumé comparatif

| Propriété | `unique_ptr<T>` | `shared_ptr<T>` |
|---|---|---|
| Taille de l'objet | `sizeof(T*)` — 8 octets | `2 × sizeof(T*)` — 16 octets |
| Allocation supplémentaire | Aucune | Bloc de contrôle (~16-32 octets) |
| Coût de copie | Interdit | Incrémentation atomique |
| Coût de destruction | `delete` simple | Décrémentation atomique + `delete` conditionnel |
| Thread-safety du compteur | N/A | Oui (atomique) |

> 💡 Cela ne veut pas dire que `shared_ptr` est « lent ». Il est parfaitement performant pour ses cas d'usage légitimes. Mais utiliser `shared_ptr` là où `unique_ptr` suffirait, c'est payer un surcoût inutile — en mémoire, en CPU, et en lisibilité (la sémantique de propriété devient floue).

---

## Le danger : les cycles de références

Le compteur de références est un mécanisme élégant mais qui a un talon d'Achille : les **cycles**. Si deux objets se référencent mutuellement via des `shared_ptr`, leurs compteurs ne tombent jamais à zéro et la mémoire n'est jamais libérée — c'est un *memory leak* structurel.

```cpp
struct Noeud {
    std::string nom;
    std::shared_ptr<Noeud> voisin;  // ⚠️ Référence partagée

    explicit Noeud(std::string n) : nom(std::move(n)) {}
    ~Noeud() { std::print("Destruction de {}\n", nom); }
};

void creer_cycle() {
    auto a = std::make_shared<Noeud>("A");
    auto b = std::make_shared<Noeud>("B");

    a->voisin = b;  // A → B
    b->voisin = a;  // B → A  → CYCLE !

}   // a et b sortent du scope, MAIS :
    // a->strong_count passe de 2 à 1 (b->voisin tient encore a)
    // b->strong_count passe de 2 à 1 (a->voisin tient encore b)
    // Aucun compteur n'atteint 0 → MEMORY LEAK
    // "Destruction de A" n'est JAMAIS affiché
    // "Destruction de B" n'est JAMAIS affiché
```

Ce problème est **impossible à détecter à la compilation**. Il ne produit ni erreur, ni warning. Le programme fonctionne apparemment normalement, mais accumule de la mémoire non libérée au fil du temps.

C'est précisément pour résoudre ce problème que `std::weak_ptr` existe.

---

## std::weak_ptr : l'observateur non-possédant

Un `std::weak_ptr` est un smart pointer qui **observe** une ressource gérée par `shared_ptr` sans en devenir propriétaire. Il ne contribue pas au compteur de références fort : sa présence n'empêche pas la destruction de la ressource.

Le `weak_ptr` résout les cycles en brisant la symétrie : au lieu que deux objets se possèdent mutuellement, l'un possède l'autre via un `shared_ptr`, et l'autre ne fait qu'observer via un `weak_ptr`.

```cpp
struct Noeud {
    std::string nom;
    std::shared_ptr<Noeud> enfant;   // Possession forte
    std::weak_ptr<Noeud> parent;     // ✅ Observation — pas de cycle

    explicit Noeud(std::string n) : nom(std::move(n)) {}
    ~Noeud() { std::print("Destruction de {}\n", nom); }
};

void creer_arbre() {
    auto racine = std::make_shared<Noeud>("Racine");
    auto feuille = std::make_shared<Noeud>("Feuille");

    racine->enfant = feuille;       // Racine possède Feuille
    feuille->parent = racine;       // Feuille observe Racine (weak_ptr)

}   // feuille sort du scope → strong_count passe de 1 à 0 → détruit
    // racine sort du scope → strong_count passe de 1 à 0 → détruit
    // ✅ "Destruction de Feuille" est affiché
    // ✅ "Destruction de Racine" est affiché
```

### Accéder à la ressource via weak_ptr

Un `weak_ptr` ne permet pas d'accéder directement à la ressource — il n'a pas d'opérateurs `*` ni `->`. C'est volontaire : la ressource pourrait avoir été détruite entre-temps. Pour y accéder, il faut d'abord tenter de le **promouvoir** en `shared_ptr` via la méthode `lock()` :

```cpp
auto shared = std::make_shared<std::string>("Hello");  
std::weak_ptr<std::string> weak = shared;  

// Tentative d'accès
if (auto locked = weak.lock()) {
    // locked est un shared_ptr valide — la ressource existe encore
    std::print("Valeur : {}\n", *locked);
} else {
    // La ressource a été détruite
    std::print("Ressource expirée\n");
}
```

`lock()` retourne un `shared_ptr` non-nul si la ressource existe encore, ou un `shared_ptr` nul si elle a été détruite. Cette opération est **atomique et thread-safe** : il n'y a pas de race condition entre le test et l'accès.

On peut aussi vérifier si la ressource existe encore sans y accéder :

```cpp
if (weak.expired()) {
    std::print("La ressource n'existe plus\n");
}
```

> ⚠️ `expired()` est sujet à des race conditions en multi-thread : la ressource peut être détruite juste après le test. Préférez `lock()` qui combine le test et la promotion en une seule opération atomique.

---

## Quand utiliser shared_ptr vs unique_ptr

Le choix ne devrait jamais être fait par défaut ou par confort. Voici les situations qui justifient réellement un `shared_ptr` :

**Situations légitimes pour shared_ptr :**

- **Caches partagés.** Un objet coûteux à créer (résultat de calcul, image chargée, connexion réseau) est partagé entre plusieurs consommateurs. Le cache ne sait pas quand les consommateurs ont fini — `shared_ptr` libère automatiquement quand le dernier consommateur disparaît.
- **Structures de données à propriété partagée.** Graphes, arbres avec références croisées, systèmes de plugins où l'objet est utilisé par plusieurs modules sans relation hiérarchique.
- **Données partagées entre threads.** Quand un objet est accédé par plusieurs threads et qu'aucun thread unique ne peut être le propriétaire (attention : le compteur de références est thread-safe, mais l'accès à l'objet pointé ne l'est pas — voir [section 21.6](/21-threads-concurrence/06-thread-safety.md)).
- **Observer pattern et callbacks.** Un objet enregistre un callback qui référence une ressource. Le `weak_ptr` permet au callback de vérifier que la ressource existe encore avant de l'utiliser.

**Signaux d'alerte — vous n'avez probablement pas besoin de shared_ptr :**

- Vous utilisez `shared_ptr` parce que vous ne savez pas qui devrait posséder la ressource. C'est un problème de conception, pas une raison d'utiliser `shared_ptr`. Clarifiez la propriété et utilisez `unique_ptr`.
- Vous n'avez jamais plus d'un `shared_ptr` actif sur la même ressource. Alors `unique_ptr` suffit.
- Vous utilisez `shared_ptr` pour éviter les `std::move`. La sémantique de déplacement existe pour une bonne raison — le confort syntaxique ne justifie pas le surcoût d'un compteur atomique.
- Vous utilisez `shared_ptr` « au cas où ». La propriété partagée devrait être une décision architecturale consciente, pas une valeur par défaut.

---

## Quand utiliser weak_ptr

`std::weak_ptr` est un outil plus spécialisé. Ses cas d'usage principaux :

- **Briser les cycles de références** dans les structures bidirectionnelles (arbres parent↔enfant, graphes, listes doublement chaînées avec `shared_ptr`).
- **Caches avec invalidation.** Le cache stocke des `weak_ptr`. Si la ressource a été détruite par ailleurs, le cache sait qu'il doit la recréer.
- **Observer pattern.** L'observable stocke des `weak_ptr` vers les observateurs. Si un observateur est détruit, l'observable le détecte et ne tente pas de le notifier.
- **Prévention des dangling pointers** dans les scénarios asynchrones : un callback stocke un `weak_ptr` et vérifie via `lock()` que l'objet cible existe encore avant de l'utiliser.

---

## Vue d'ensemble des sous-sections

Ce module sur la possession partagée est découpé en deux sous-sections :

**[9.2.1 — Comptage de références](/09-smart-pointers/02.1-comptage-references.md)**
Le mécanisme interne de `shared_ptr` : bloc de contrôle, incrémentation/décrémentation atomique, `use_count()`, et les implications en performance et en multi-threading.

**[9.2.2 — Cycles de références et std::weak_ptr](/09-smart-pointers/02.2-cycles-weak-ptr.md)**
Analyse détaillée des cycles, stratégies pour les détecter et les résoudre, et utilisation complète de `weak_ptr` : `lock()`, `expired()`, patterns courants.

---

## Résumé express

| Propriété | `shared_ptr<T>` | `weak_ptr<T>` |
|---|---|---|
| **Possession** | Partagée — co-propriétaire | Aucune — observateur |
| **Copie** | Autorisée (incrémente le compteur) | Autorisée (incrémente le weak count) |
| **Accès à la ressource** | Direct (`*`, `->`) | Indirect (`lock()` → `shared_ptr`) |
| **Empêche la destruction** | Oui (tant que strong count > 0) | Non |
| **Surcoût** | Bloc de contrôle + compteurs atomiques | Référence au bloc de contrôle existant |
| **Header** | `<memory>` | `<memory>` |
| **Disponible depuis** | C++11 | C++11 |

> **Règle pratique** — La hiérarchie de décision pour la gestion de la mémoire en C++ moderne est : variable locale (stack) > `std::unique_ptr` > `std::shared_ptr`. Chaque niveau vers la droite ajoute de la complexité et du coût. N'avancez dans cette hiérarchie que lorsque le niveau précédent ne suffit pas.

⏭️ [Comptage de références](/09-smart-pointers/02.1-comptage-references.md)
