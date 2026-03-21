🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 45.3 — Use-after-free et temporal safety

## Chapitre 45 — Sécurité en C++ ⭐

---

## Introduction

Le use-after-free (UAF) est aujourd'hui la classe de vulnérabilité la plus exploitée dans les logiciels écrits en C et C++. Les rapports de sécurité de Google sur Chromium montrent que les UAF représentent une proportion dominante des vulnérabilités de sécurité mémoire sévères du projet. Microsoft rapporte des proportions comparables dans ses produits.

Le mécanisme est conceptuellement simple : un programme accède à un objet après que celui-ci a été détruit. Les données lues sont soit des résidus de l'objet précédent, soit le contenu d'un nouvel objet alloué au même emplacement. Dans les deux cas, le comportement du programme est corrompu. Lorsqu'un attaquant peut contrôler ce qui est alloué à la place de l'objet détruit, il peut rediriger le flux d'exécution du programme — typiquement en plaçant un faux vtable pointer pour détourner un appel de méthode virtuelle.

Ce qui rend les UAF particulièrement redoutables, c'est qu'ils sont difficiles à détecter par revue de code. La destruction de l'objet et l'accès invalide peuvent être séparés par des milliers de lignes, dans des fichiers différents, avec des chemins d'exécution complexes. Contrairement à un buffer overflow qui se produit sur un seul site, un UAF implique au minimum deux sites distants dans le code — l'endroit de la libération et l'endroit de l'utilisation — et souvent un troisième : l'endroit où le pointeur est copié sans que la propriété soit clairement transférée.

Cette section explore les différentes formes de use-after-free, les raisons structurelles pour lesquelles le C++ y est exposé, et les techniques modernes pour les prévenir et les détecter.

---

## Anatomie d'un use-after-free

### Le mécanisme fondamental

Un use-after-free se décompose en trois étapes :

1. **Allocation** — un objet est créé sur le heap (via `new`, `malloc`, ou un conteneur).
2. **Libération** — l'objet est détruit (`delete`, `free`, ou destruction du conteneur), mais un ou plusieurs pointeurs vers l'objet subsistent.
3. **Utilisation** — le programme accède à l'objet via l'un de ces pointeurs résiduels (dangling pointers).

```cpp
#include <print>

struct Widget {
    int id;
    virtual void render() {
        std::print("Widget #{}\n", id);
    }
};

// ❌ Use-after-free élémentaire
void basic_uaf() {
    Widget* w = new Widget{42};
    w->render();     // OK — l'objet existe

    delete w;        // L'objet est détruit, la mémoire est rendue à l'allocateur

    w->render();     // ⚠️ USE-AFTER-FREE — comportement indéfini
                     // La mémoire peut contenir n'importe quoi
                     // L'appel virtuel déréférence le vtable pointer,
                     // qui peut avoir été écrasé
}
```

L'appel `w->render()` après le `delete` est un comportement indéfini. En pratique, si aucune autre allocation n'a eu lieu entre-temps, la mémoire contient encore les anciennes données et l'appel semble fonctionner. C'est précisément ce qui rend le bug invisible pendant les tests et exploitable en production : le comportement dépend de l'état de l'allocateur au moment de l'accès.

### Dangling pointer après `delete`

La forme la plus directe : un pointeur n'est pas mis à `nullptr` après la destruction de l'objet, et il est réutilisé ensuite — parfois dans une branche d'exécution rarement empruntée :

```cpp
class ResourceManager {
    Connection* conn_ = nullptr;

public:
    void connect(const std::string& url) {
        conn_ = new Connection(url);
    }

    void disconnect() {
        delete conn_;
        // ❌ conn_ conserve l'ancienne adresse (dangling pointer)
    }

    void send(const std::string& data) {
        if (conn_) {              // Ce test passe — conn_ n'est pas nullptr !
            conn_->write(data);   // ⚠️ USE-AFTER-FREE
        }
    }
};
```

Le bug est structurel : après `disconnect()`, `conn_` pointe toujours vers l'adresse de l'objet détruit. Le test `if (conn_)` ne protège rien car il teste l'adresse, pas la validité de l'objet.

Mettre `conn_` à `nullptr` après `delete` est un palliatif :

```cpp
void disconnect() {
    delete conn_;
    conn_ = nullptr;  // Palliatif — mais ne résout pas le problème
                       // si d'autres pointeurs vers conn_ existent ailleurs
}
```

Ce palliatif protège contre la réutilisation locale du même membre, mais il ne protège pas contre les pointeurs copiés :

```cpp
void fragile_code(ResourceManager& mgr) {
    Connection* local_ptr = mgr.get_connection();  // Copie du pointeur
    mgr.disconnect();                               // L'objet est détruit
    local_ptr->write("data");                       // ⚠️ USE-AFTER-FREE
    // Mettre conn_ à nullptr dans disconnect() n'a aucun effet ici
}
```

C'est ce problème fondamental — la prolifération non contrôlée des pointeurs bruts — que les smart pointers résolvent.

### Dangling reference sur la stack

Les use-after-free ne concernent pas uniquement le heap. Une référence ou un pointeur vers un objet local crée un dangling reference dès que la portée de l'objet se termine :

```cpp
#include <string_view>
#include <string>

// ❌ Retourne une référence vers un objet local détruit
std::string_view make_greeting(const std::string& name) {
    std::string result = "Bonjour, " + name + " !";
    return result;  // result est détruit en sortie de fonction
                    // Le string_view pointe vers de la mémoire libérée
}

void caller() {
    auto greeting = make_greeting("Alice");
    // greeting est un dangling string_view
    std::print("{}\n", greeting);  // ⚠️ USE-AFTER-FREE
}
```

Ce pattern est fréquent avec `std::string_view` et `std::span`, qui sont des vues non possédantes. Elles ne prolongent pas la durée de vie de l'objet observé. Quand l'objet meurt, la vue devient un dangling reference.

```cpp
#include <span>
#include <vector>

// ❌ Le vector temporaire est détruit, le span est invalide
std::span<const int> get_data() {
    std::vector<int> temp = {1, 2, 3, 4, 5};
    return temp;  // Conversion implicite vector → span
                  // temp détruit à la sortie → span dangling
}
```

> **Bonne pratique** : ne jamais retourner un `std::string_view` ou un `std::span` qui observe un objet local. Retourner l'objet lui-même (`std::string`, `std::vector`) — le compilateur appliquera le RVO (Return Value Optimization) pour éviter la copie (voir section 10.5).

### Invalidation d'itérateurs et de références

Les conteneurs STL invalident les itérateurs et les références vers leurs éléments lors de certaines opérations. Utiliser un itérateur invalidé est une forme de use-after-free :

```cpp
#include <vector>
#include <print>

void iterator_invalidation() {
    std::vector<int> v = {1, 2, 3, 4, 5};

    auto it = v.begin() + 2;  // Pointe vers l'élément 3

    v.push_back(6);  // ⚠️ Si le vector réalloue, TOUS les itérateurs
                      // et TOUTES les références sont invalidés

    std::print("{}\n", *it);  // ⚠️ USE-AFTER-FREE potentiel
                               // it pointe vers l'ancien buffer, libéré
}
```

Le `push_back` peut déclencher une réallocation du tampon interne du `vector`. L'ancien tampon est libéré, et tous les itérateurs, pointeurs et références vers les éléments deviennent dangling. Le même problème existe avec `insert`, `erase`, `resize` et toute opération qui modifie la taille d'un `vector`.

La suppression d'éléments pendant l'itération est un autre classique :

```cpp
#include <vector>

// ❌ Suppression pendant itération — itérateur invalidé
void remove_evens_broken(std::vector<int>& v) {
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (*it % 2 == 0) {
            v.erase(it);  // Invalide it ET tous les itérateurs après it
            // ++it dans la boucle → comportement indéfini
        }
    }
}

// ✅ Pattern correct : erase retourne l'itérateur suivant valide
void remove_evens_correct(std::vector<int>& v) {
    for (auto it = v.begin(); it != v.end(); ) {
        if (*it % 2 == 0) {
            it = v.erase(it);  // erase retourne l'itérateur valide suivant
        } else {
            ++it;
        }
    }
}

// ✅ Encore mieux : erase-remove idiom
#include <algorithm>

void remove_evens_idiomatic(std::vector<int>& v) {
    std::erase_if(v, [](int x) { return x % 2 == 0; });  // C++20
}
```

Voir section 13.1.3 pour une couverture détaillée des règles d'invalidation d'itérateurs par conteneur.

---

## Pourquoi les UAF sont si dangereux

### Exploitabilité élevée

Un buffer overflow écrase de la mémoire adjacente de manière relativement prévisible. Un use-after-free, en revanche, donne à l'attaquant un contrôle plus fin : il peut influencer *ce qui est alloué* à l'emplacement de l'objet libéré.

Le scénario d'exploitation typique sur un objet polymorphique :

1. L'attaquant déclenche la destruction d'un objet qui possède un vtable pointer.
2. L'attaquant provoque une allocation de même taille qui réutilise le même emplacement mémoire (le heap est un recycleur).
3. Les données de la nouvelle allocation écrasent le vtable pointer de l'ancien objet.
4. Le programme appelle une méthode virtuelle via le dangling pointer → le processeur saute à l'adresse choisie par l'attaquant.

Ce type d'exploitation est particulièrement pertinent en C++ à cause du polymorphisme dynamique : chaque objet avec des méthodes virtuelles commence par un pointeur vers sa vtable, ce qui constitue une cible de premier choix.

### Détection tardive

Contrairement à un buffer overflow qui provoque souvent un crash immédiat (violation d'accès, canary corrompu), un use-after-free peut rester silencieux pendant longtemps. Si la mémoire n'a pas été réutilisée, les anciennes données sont encore en place et le programme semble fonctionner normalement. Le bug ne se manifeste que sous des conditions de charge spécifiques ou de timing particulier — précisément les conditions que les tests unitaires ne reproduisent pas.

### Reproductibilité difficile

Le comportement d'un UAF dépend de l'état de l'allocateur mémoire au moment de l'accès invalide, ce qui dépend lui-même de l'historique complet des allocations et désallocations du programme. Deux exécutions du même programme avec les mêmes entrées peuvent produire des comportements différents. Le bug peut se manifester en production mais être impossible à reproduire en débogage — un phénomène que les développeurs systèmes appellent un heisenbug.

---

## Prévention par le code moderne

### Smart pointers : la solution structurelle

Les smart pointers (chapitre 9) résolvent le problème à la racine en éliminant la gestion manuelle de la durée de vie :

```cpp
#include <memory>
#include <print>

struct Widget {
    int id;
    virtual void render() { std::print("Widget #{}\n", id); }
    ~Widget() { std::print("Widget #{} détruit\n", id); }
};

// ✅ Possession exclusive : le pointeur unique contrôle la durée de vie
void ownership_unique() {
    auto w = std::make_unique<Widget>(42);
    w->render();  // OK

    // Pas de delete manuel — le destructeur est appelé automatiquement
    // quand w sort de portée.
    // Aucun dangling pointer possible car unique_ptr est non copiable.
}
```

**`std::unique_ptr`** — possession exclusive. L'objet est détruit quand le `unique_ptr` sort de portée. Le pointeur ne peut pas être copié, seulement déplacé, ce qui élimine structurellement la prolifération de pointeurs :

```cpp
void transfer_ownership() {
    auto w = std::make_unique<Widget>(1);

    // auto copy = w;           // ❌ Ne compile pas — non copiable
    auto moved = std::move(w);  // ✅ Transfert de propriété
    // w est maintenant nullptr, moved possède l'objet

    moved->render();  // OK
}
```

**`std::shared_ptr`** — possession partagée. L'objet reste vivant tant qu'au moins un `shared_ptr` le référence. Le comptage de références garantit que la destruction ne se produit que lorsque le dernier possesseur disparaît :

```cpp
#include <memory>
#include <vector>

class Scene {
    std::vector<std::shared_ptr<Widget>> widgets_;

public:
    void add(std::shared_ptr<Widget> w) {
        widgets_.push_back(std::move(w));
    }

    std::shared_ptr<Widget> get(size_t index) {
        return widgets_.at(index);
        // L'appelant reçoit un shared_ptr → l'objet ne sera pas détruit
        // tant que l'appelant conserve sa copie
    }
};

void shared_usage() {
    auto w = std::make_shared<Widget>(99);
    // Compteur de références = 1

    {
        Scene scene;
        scene.add(w);  // Compteur = 2
        // w et scene partagent la propriété
    }
    // scene détruite, compteur retombe à 1 — l'objet survit

    w->render();  // ✅ OK — l'objet est toujours vivant
}
// w détruit ici, compteur = 0 → Widget détruit
```

**`std::weak_ptr`** — observation sans possession. Un `weak_ptr` observe un objet géré par `shared_ptr` sans empêcher sa destruction. Il résout le problème des cycles de références et fournit un mécanisme sûr pour vérifier si l'objet existe encore :

```cpp
#include <memory>
#include <print>

void observer_pattern() {
    std::weak_ptr<Widget> observer;

    {
        auto w = std::make_shared<Widget>(7);
        observer = w;  // Observe sans posséder

        if (auto locked = observer.lock()) {  // Tentative de promotion
            locked->render();  // ✅ OK — l'objet est garanti vivant
        }
    }
    // w détruit ici — l'objet n'existe plus

    if (auto locked = observer.lock()) {
        locked->render();  // Jamais atteint
    } else {
        std::print("L'objet a été détruit\n");  // ✅ Détection sûre
    }
}
```

Le pattern `weak_ptr::lock()` est la réponse du C++ moderne au problème du dangling pointer : au lieu de vérifier si un pointeur est non-null (ce qui ne prouve rien sur la validité de l'objet), on tente de promouvoir le `weak_ptr` en `shared_ptr`. Si l'objet a été détruit, `lock()` retourne un `shared_ptr` null.

### Le principe : pointer sans posséder → `weak_ptr` ou référence à durée de vie contrôlée

La règle de design qui prévient les UAF se résume ainsi :

| Relation avec l'objet | Mécanisme |
|---|---|
| Je possède l'objet exclusivement | `std::unique_ptr` |
| Je possède l'objet conjointement | `std::shared_ptr` |
| J'observe un objet que quelqu'un d'autre possède | `std::weak_ptr` (heap) ou référence `&` (stack, durée de vie englobante garantie) |
| Je ne devrais jamais utiliser | `new`/`delete` manuels, pointeur brut possédant |

Les pointeurs bruts (`T*`) restent utiles en C++ moderne, mais uniquement comme pointeurs **non possédants** vers des objets dont la durée de vie est gérée par un autre mécanisme. Un pointeur brut ne devrait jamais être responsable de la destruction d'un objet.

### RAII et scoping : prévenir les dangling references

Pour les objets sur la stack, la protection repose sur le scoping — s'assurer que les vues (`string_view`, `span`) et les références ne survivent pas à l'objet qu'elles observent :

```cpp
#include <string>
#include <string_view>
#include <print>

// ❌ Dangereux : string_view qui survit à l'objet observé
void dangerous() {
    std::string_view sv;
    {
        std::string s = "Hello, World!";
        sv = s;  // sv observe s
    }
    // s est détruit — sv est un dangling view
    std::print("{}\n", sv);  // ⚠️ USE-AFTER-FREE
}

// ✅ Sûr : la vue ne survit pas à l'objet
void safe() {
    std::string s = "Hello, World!";
    std::string_view sv = s;  // sv observe s
    std::print("{}\n", sv);   // OK — s est toujours vivant
    // s et sv détruits ensemble en sortie de portée
}

// ✅ Sûr : retourner l'objet possédant, pas la vue
std::string make_greeting_safe(const std::string& name) {
    return "Bonjour, " + name + " !";  // Retourne un std::string (possédant)
    // RVO évite la copie
}
```

La règle est structurelle : une vue non possédante (`string_view`, `span`, référence, pointeur brut) doit toujours avoir une durée de vie inférieure ou égale à celle de l'objet qu'elle observe. Le compilateur ne vérifie pas cette contrainte en C++ (contrairement à Rust avec le borrow checker), c'est donc au développeur de la respecter.

> **Aide du compilateur** : GCC et Clang émettent des warnings pour les cas les plus évidents de dangling (`-Wdangling-gsl`, `-Wreturn-stack-address`, `-Wdangling`). Ces warnings ne couvrent pas tous les cas, mais ils attrapent les patterns les plus courants comme le retour d'une `string_view` vers un objet local.

---

## Détection des use-after-free

### AddressSanitizer (ASan)

AddressSanitizer est l'outil le plus efficace pour détecter les use-after-free à l'exécution. Il fonctionne en maintenant une zone de quarantaine pour la mémoire libérée : au lieu de rendre immédiatement la mémoire à l'allocateur, ASan la marque comme inaccessible pendant un certain temps. Tout accès à cette mémoire quarantinée déclenche une erreur immédiate :

```bash
g++ -fsanitize=address -g -O1 main.cpp -o main
./main
```

Sortie typique pour un use-after-free :

```
=================================================================
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x602000000010
READ of size 4 at 0x602000000010 thread T0
    #0 0x4011a3 in main main.cpp:15
    #1 0x7f... in __libc_start_main

0x602000000010 is located 0 bytes inside of 16-byte region [0x602000000010,0x602000000020)
freed by thread T0 here:
    #0 0x7f... in operator delete(void*)
    #1 0x401165 in main main.cpp:12

previously allocated by thread T0 here:
    #0 0x7f... in operator new(unsigned long)
    #1 0x401132 in main main.cpp:10
```

ASan fournit trois informations essentielles : l'endroit de l'accès invalide, l'endroit de la libération, et l'endroit de l'allocation originale. Cette triple trace est ce qui rend les UAF déboguables — sans elle, remonter du crash au bug est souvent un travail de plusieurs jours.

Le surcoût d'ASan est d'environ 2× en temps d'exécution et 2-3× en mémoire. Il n'est pas destiné à la production, mais il devrait être activé systématiquement dans les suites de tests et le fuzzing. Voir section 29.4.1 pour la configuration détaillée.

### Hardening de l'allocateur en production

Pour la détection en production, des allocateurs renforcés peuvent être utilisés. Ils rendent la réutilisation d'adresses mémoire moins immédiate, ce qui réduit l'exploitabilité des UAF sans les détecter formellement :

- **Scudo** (allocateur hardened de LLVM) — randomise la réutilisation des adresses et détecte certains patterns de double-free. C'est l'allocateur par défaut d'Android depuis Android 11.  
- **jemalloc** / **tcmalloc** en mode debug — offrent des fonctionnalités de quarantaine similaires à ASan mais avec un surcoût moindre.

```bash
# Utilisation de Scudo comme allocateur
clang++ -fsanitize=scudo main.cpp -o main
```

### MTE (Memory Tagging Extension) — ARM

Sur les architectures ARM v8.5+ (serveurs Graviton 3+, mobiles récents), l'extension MTE (Memory Tagging Extension) fournit une détection matérielle des use-after-free avec un surcoût de quelques pourcents seulement. Chaque allocation reçoit un tag aléatoire de 4 bits, stocké à la fois dans le pointeur et dans la mémoire physique. Tout accès dont le tag du pointeur ne correspond pas au tag de la mémoire déclenche une exception.

MTE est la technologie la plus prometteuse pour la détection de UAF en production, mais elle est encore en cours de déploiement en 2026 et limitée aux architectures ARM.

---

## Le problème structurel du C++ et la comparaison avec Rust

Le C++ ne dispose pas de mécanisme intégré au langage pour vérifier la validité temporelle des pointeurs à la compilation. Le compilateur peut vérifier les types, les constantes, les templates — mais il ne peut pas prouver qu'un pointeur est valide au moment où il est déréférencé.

C'est exactement le problème que Rust résout avec son borrow checker : le compilateur Rust vérifie statiquement qu'aucune référence ne survit à l'objet qu'elle observe. Le coût est une complexité accrue du modèle de propriété, mais le bénéfice est l'élimination de la classe entière des use-after-free à la compilation.

En C++, la prévention des UAF repose sur une combinaison de :

- **Discipline de design** — smart pointers, RAII, propriété claire.  
- **Outils dynamiques** — ASan en test, allocateurs renforcés en production.  
- **Analyse statique** — clang-tidy peut détecter certains patterns évidents de dangling, mais pas les cas complexes.  
- **Hardening matériel** — MTE sur ARM pour la détection en production.

Cette combinaison n'offre pas les mêmes garanties que la vérification statique de Rust, mais elle permet d'atteindre un niveau de sécurité élevé dans la pratique. La section 45.6 aborde les initiatives du comité C++ pour réduire cet écart, notamment les Safety Profiles et la stratégie d'interopérabilité avec Rust (section 45.6.4).

---

## Patterns à risque et comment les éviter

### Pattern 1 : retour de pointeur brut depuis un conteneur

```cpp
#include <vector>
#include <memory>

class EntityManager {
    std::vector<std::unique_ptr<Entity>> entities_;

public:
    // ❌ Dangereux : le pointeur brut peut devenir dangling
    // si entities_ est modifié (push_back → réallocation)
    // ou si l'Entity est supprimée de entities_
    Entity* get_entity(size_t id) {
        return entities_.at(id).get();
    }

    void remove_entity(size_t id) {
        entities_.erase(entities_.begin() + id);
        // Tous les pointeurs bruts retournés par get_entity()
        // pour des indices >= id sont maintenant invalides
    }
};
```

La solution dépend du contexte. Si l'appelant a besoin d'une garantie que l'objet reste vivant, utiliser `std::shared_ptr` :

```cpp
class EntityManager_safe {
    std::vector<std::shared_ptr<Entity>> entities_;

public:
    // ✅ L'appelant partage la propriété — l'objet ne sera pas détruit
    // tant que l'appelant conserve le shared_ptr
    std::shared_ptr<Entity> get_entity(size_t id) {
        return entities_.at(id);
    }
};
```

Si l'appelant n'a besoin que d'un accès temporaire dans un scope bien délimité, un pointeur brut ou une référence est acceptable — à condition de documenter le contrat :

```cpp
class EntityManager_documented {
    std::vector<std::unique_ptr<Entity>> entities_;

public:
    // Le pointeur retourné est valide uniquement tant que
    // le EntityManager n'est pas modifié. Ne pas stocker.
    Entity* get_entity_temp(size_t id) {
        return entities_.at(id).get();
    }
};
```

### Pattern 2 : callbacks et lambdas capturant `this`

Les callbacks qui capturent `this` sont une source prolifique de UAF, en particulier dans le code asynchrone :

```cpp
#include <functional>
#include <memory>

class Session {
    int data_ = 42;

public:
    std::function<void()> make_callback() {
        // ❌ La lambda capture this par pointeur brut
        // Si le Session est détruit avant que le callback ne soit appelé → UAF
        return [this]() {
            process(data_);  // ⚠️ this peut être dangling
        };
    }
};

void dangerous_async() {
    std::function<void()> cb;
    {
        Session session;
        cb = session.make_callback();
    }
    // session détruite ici
    cb();  // ⚠️ USE-AFTER-FREE — this pointe vers un objet détruit
}
```

La solution : utiliser `std::shared_ptr` et `shared_from_this` pour garantir que l'objet survit au callback :

```cpp
#include <functional>
#include <memory>

class Session : public std::enable_shared_from_this<Session> {
    int data_ = 42;

public:
    std::function<void()> make_callback() {
        // ✅ Capture un shared_ptr → l'objet reste vivant
        // tant que le callback existe
        auto self = shared_from_this();
        return [self]() {
            self->process(self->data_);  // Garanti valide
        };
    }

private:
    void process(int d) { /* ... */ }
};

void safe_async() {
    std::function<void()> cb;
    {
        auto session = std::make_shared<Session>();
        cb = session->make_callback();
    }
    // Le shared_ptr dans session est détruit, mais le callback
    // détient sa propre copie → l'objet survit
    cb();  // ✅ OK
}
```

### Pattern 3 : stocker une référence ou un itérateur pour usage différé

```cpp
#include <string>
#include <vector>

class Config {
    std::vector<std::string> entries_;
    // ❌ Dangereux : stocke une référence vers un élément du vector
    // Toute modification du vector peut invalider cette référence
    const std::string& cached_entry_;

public:
    Config(const std::string& first_entry)
        : entries_{first_entry}
        , cached_entry_{entries_[0]}  // Référence valide ici
    {}

    void add_entry(const std::string& e) {
        entries_.push_back(e);
        // ⚠️ Si push_back provoque une réallocation,
        // cached_entry_ est maintenant un dangling reference
    }
};
```

La solution : ne jamais stocker de référence ou de pointeur vers l'intérieur d'un conteneur qui peut être modifié. Stocker un indice à la place, ou recalculer la référence à chaque accès :

```cpp
class Config_safe {
    std::vector<std::string> entries_;
    size_t cached_index_ = 0;  // ✅ Un indice ne devient pas dangling

public:
    const std::string& cached_entry() const {
        return entries_.at(cached_index_);  // Accès vérifié à chaque appel
    }
};
```

---

## Résumé des bonnes pratiques

**1. Utiliser `std::unique_ptr` pour la possession exclusive.** C'est le remplacement direct de `new`/`delete` et le défaut à privilégier dans la grande majorité des cas.

**2. Utiliser `std::shared_ptr` quand la durée de vie est partagée.** Notamment pour les callbacks, les patterns observer, et les graphes d'objets sans hiérarchie de propriété claire.

**3. Utiliser `std::weak_ptr` pour observer sans posséder.** Et toujours vérifier la validité via `lock()` avant d'accéder à l'objet.

**4. Ne jamais retourner une vue (`string_view`, `span`) vers un objet local.** Retourner l'objet possédant ; le RVO éliminera la copie.

**5. Ne jamais stocker une référence ou un itérateur vers l'intérieur d'un conteneur modifiable.** Stocker un indice ou recalculer à chaque accès.

**6. Capturer `shared_from_this()` dans les lambdas asynchrones.** Jamais `this` brut quand le callback peut survivre à l'objet.

**7. Activer AddressSanitizer dans tous les tests.** `-fsanitize=address` est la méthode la plus efficace pour détecter les UAF avant la production.

**8. Utiliser les warnings de dangling.** `-Wdangling` (Clang) et `-Wreturn-local-addr` (GCC) attrapent les cas les plus évidents.

---

## Pour aller plus loin

- **Chapitre 9** — Smart pointers : couverture complète de `unique_ptr`, `shared_ptr`, `weak_ptr`.  
- **Section 6.3** — RAII : le principe qui sous-tend la gestion sûre de la durée de vie.  
- **Section 10.2** — `std::move` et le transfert de propriété.  
- **Section 13.1.3** — Invalidation des itérateurs : règles par conteneur.  
- **Section 29.4.1** — AddressSanitizer : configuration détaillée et intégration CI.  
- **Section 45.6** — Sécurité mémoire en 2026 : Safety Profiles, interopérabilité Rust, et le bilan de la sûreté en C++.  
- **Section 43.3** — C++ et Rust : comment les deux langages peuvent coexister dans un même projet, et quand envisager une migration.

⏭️ [Compilation avec protections](/45-securite/04-compilation-protections.md)
