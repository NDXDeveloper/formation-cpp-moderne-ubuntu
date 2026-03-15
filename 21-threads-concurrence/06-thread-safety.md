🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 21.6 Thread-safety et data races

## Penser concurrent

Les sections précédentes ont présenté les outils — threads, mutex, condition variables, atomiques, futures. Cette section prend du recul pour aborder la question centrale : **comment concevoir du code qui est correct en environnement concurrent ?**

Les outils ne suffisent pas. Un programme peut utiliser des mutex partout et contenir des data races. Un autre peut n'avoir aucun mutex et être parfaitement thread-safe (s'il ne partage aucune donnée mutable). La thread-safety est une propriété de **conception**, pas une conséquence mécanique de l'utilisation de primitives.

---

## Définitions précises

### Data race (au sens du standard C++)

Une **data race** se produit lorsque :

1. Deux threads accèdent à la **même zone mémoire** ;
2. Au moins un des accès est une **écriture** ;
3. Les accès ne sont pas ordonnés par une relation **happens-before** (c'est-à-dire qu'il n'y a aucune synchronisation entre eux).

Une data race est un **comportement indéfini** selon le standard. Le compilateur est en droit de supposer qu'il n'y en a pas et d'optimiser en conséquence — ce qui peut transformer un bug latent en comportement catastrophique.

```cpp
// Data race : deux threads, une écriture, aucune synchronisation
int counter = 0;

// Thread 1            // Thread 2
counter++;             counter++;
// → Comportement indéfini
```

### Race condition (au sens logique)

Une **race condition** est un bug où le résultat du programme dépend de l'ordre d'exécution relatif des threads. Contrairement à une data race, une race condition peut exister même si toutes les données sont correctement protégées par des mutex :

```cpp
std::mutex mtx;  
int balance = 100;  

// Thread 1 : retrait                    // Thread 2 : retrait
{                                        {
    std::lock_guard lock(mtx);               std::lock_guard lock(mtx);
    if (balance >= 80) {                     if (balance >= 80) {
        balance -= 80;                           balance -= 80;
    }                                        }
}                                        }
// Pas de data race (mutex protège balance)
// Mais race condition : les deux threads peuvent voir balance >= 80
// et retirer chacun 80, laissant balance = -60.
```

Ici, chaque accès à `balance` est protégé, mais la logique « vérifier puis agir » (*check-then-act*) n'est pas atomique dans son ensemble. La vérification et l'action doivent être dans la **même** section critique :

```cpp
// ✅ Correct : vérification et action dans la même section critique
std::lock_guard lock(mtx);  
if (balance >= 80) {  
    balance -= 80;
}
```

### Distinction fondamentale

| | Data race | Race condition |
|---|-----------|----------------|
| Définition | Accès concurrent non-synchronisé dont au moins une écriture | Résultat dépendant de l'ordonnancement des threads |
| Selon le standard | Comportement indéfini | Bug logique (comportement défini mais incorrect) |
| Détectable par TSan | Oui | Non (logique métier) |
| Solution | Synchronisation (mutex, atomic) | Conception correcte des sections critiques |

---

## Niveaux de thread-safety

En C++, on distingue classiquement trois niveaux de thread-safety pour une classe ou une fonction :

### Thread-safe (totalement)

Toutes les opérations peuvent être appelées simultanément par plusieurs threads sans synchronisation externe. La classe gère sa propre synchronisation en interne.

```cpp
// Exemple : AtomicCounter est totalement thread-safe
class AtomicCounter {
    std::atomic<int> count_{0};
public:
    void increment() { count_.fetch_add(1); }
    int get() const { return count_.load(); }
};
```

Les conteneurs thread-safe ont un coût : chaque opération acquiert un verrou, même quand un seul thread les utilise. C'est pourquoi la STL a fait un autre choix.

### Thread-compatible (la convention STL)

Les accès en **lecture seule** simultanés sont sûrs. Les accès en **écriture** nécessitent une synchronisation externe. C'est le niveau de thread-safety de la quasi-totalité des conteneurs et classes de la bibliothèque standard.

```cpp
std::vector<int> vec = {1, 2, 3};

// ✅ OK : lectures simultanées
// Thread 1: vec.size();    Thread 2: vec[0];

// ❌ Data race : une écriture + une lecture sans synchronisation
// Thread 1: vec.push_back(4);    Thread 2: vec.size();

// ✅ OK avec synchronisation externe
std::mutex mtx;
// Thread 1: { std::lock_guard l(mtx); vec.push_back(4); }
// Thread 2: { std::lock_guard l(mtx); vec.size(); }
```

Ce choix est délibéré : imposer un verrou interne à chaque opération du `vector` pénaliserait le code mono-thread (l'immense majorité des accès), et les utilisateurs qui ont besoin de thread-safety peuvent ajouter la synchronisation adaptée à leur cas.

### Thread-hostile

L'objet ne peut pas être utilisé en contexte concurrent, même avec une synchronisation externe. C'est rare dans le code moderne mais peut se produire avec des objets qui dépendent d'un état global mutable (variables statiques non-protégées, caches globaux, etc.).

---

## Garanties de la bibliothèque standard

Le standard C++ fournit des garanties précises sur la thread-safety de ses composants :

### Règle générale

- Les **fonctions const** (lectures) sur des objets distincts OU sur le même objet sont thread-safe sans synchronisation.
- Les **fonctions non-const** (écritures) sur le même objet nécessitent une synchronisation externe.
- Les **fonctions statiques** de la bibliothèque standard sont thread-safe.

### Cas particuliers notables

**`std::cout` et les flux de sortie** : les opérations individuelles sur `std::cout` sont thread-safe (pas de data race), mais les sorties de différents threads peuvent s'entrelacer au niveau des caractères. Chaque `operator<<` est atomique, mais une séquence `std::cout << a << b << c` n'est pas indivisible :

```cpp
// Thread 1 : std::cout << "Hello " << "World\n";
// Thread 2 : std::cout << "Bonjour " << "Monde\n";
// Sortie possible : "Hello Bonjour " << "World\nMonde\n"

// Solution : std::print (C++23) garantit une sortie atomique par appel
// Ou protéger avec un mutex, ou écrire dans un std::ostringstream puis flush
```

**`std::shared_ptr`** : le compteur de références est atomique (les copies/destructions concurrentes sont safe). Mais l'accès au pointé n'est pas protégé — si deux threads modifient l'objet pointé, il faut synchroniser.

```cpp
auto ptr = std::make_shared<std::vector<int>>();

// ✅ OK : copier/détruire le shared_ptr depuis plusieurs threads
// auto ptr2 = ptr;  // Atomique sur le compteur de références

// ❌ Data race : modifier l'objet pointé sans synchronisation
// Thread 1: ptr->push_back(1);
// Thread 2: ptr->push_back(2);
```

**`const` et thread-safety** : depuis C++11, `const` implique thread-safe pour les types de la bibliothèque standard. Si vous écrivez vos propres classes, une méthode `const` devrait être safe à appeler depuis plusieurs threads simultanément. Si elle modifie un état interne (cache, compteur), cet état doit être protégé (typiquement avec un `mutable std::mutex`).

---

## Patterns de conception thread-safe

### Pattern 1 : Données immutables

La stratégie la plus simple et la plus robuste : si une donnée ne change jamais après sa construction, elle est automatiquement thread-safe. Aucune synchronisation nécessaire.

```cpp
// Immutable par construction — thread-safe sans aucun verrou
class Config {
    const std::string host_;
    const int port_;
    const std::chrono::seconds timeout_;

public:
    Config(std::string host, int port, std::chrono::seconds timeout)
        : host_(std::move(host)), port_(port), timeout_(timeout) {}

    const std::string& host() const { return host_; }
    int port() const { return port_; }
    std::chrono::seconds timeout() const { return timeout_; }
};

// Partagé librement entre threads sans synchronisation
auto config = std::make_shared<const Config>("localhost", 8080, 30s);
```

Pour modifier la configuration, on crée un nouvel objet et on remplace atomiquement le pointeur partagé :

```cpp
std::atomic<std::shared_ptr<const Config>> current_config;

void update_config(std::shared_ptr<const Config> new_config) {
    current_config.store(std::move(new_config));  // C++20 : atomic<shared_ptr>
}
```

### Pattern 2 : Confinement au thread (thread-local)

Si chaque thread possède sa propre copie d'une donnée, il n'y a pas de partage et donc pas de data race. C++ offre le mot-clé `thread_local` :

```cpp
// Chaque thread a son propre compteur — aucune synchronisation nécessaire
thread_local int request_count = 0;

void handle_request() {
    ++request_count;  // Accès local au thread, pas de data race
    // ...
}

// Pour obtenir le total, on agrège les compteurs de tous les threads
// (via un mécanisme de collecte, par exemple un atomic global incrémenté à la fin)
```

`thread_local` est idéal pour les caches, les allocateurs par thread, les générateurs de nombres aléatoires, ou toute donnée qui n'a pas besoin d'être visible par d'autres threads.

> ⚠️ Les variables `thread_local` ont un coût d'accès légèrement supérieur aux variables globales classiques (indirection via le TLS — Thread-Local Storage). Cela reste bien moins cher qu'un mutex, mais ce n'est pas zéro.

### Pattern 3 : Passage de messages

Plutôt que de partager des données protégées par des mutex, les threads communiquent en s'envoyant des **messages** via des queues thread-safe. Chaque thread est propriétaire exclusif de ses données — le partage se fait uniquement via la queue.

```cpp
// Chaque thread possède ses données — pas de partage direct
// La communication se fait via la BlockingQueue (section 21.3)

void worker(BlockingQueue<Task>& inbox, BlockingQueue<Result>& outbox) {
    Task task;
    while (inbox.pop(task)) {
        // Traitement local — pas de donnée partagée
        Result result = process(task);
        outbox.push(std::move(result));
    }
}
```

Ce modèle, inspiré du paradigme acteur (Erlang, Akka), simplifie considérablement le raisonnement sur la concurrence : au lieu de se demander « quels accès à quelles données sont protégés ? », on se demande « quels messages circulent entre quels threads ? ».

### Pattern 4 : Le moniteur (mutex + condition variable)

Le pattern classique de la programmation concurrente orientée objet : une classe encapsule ses données avec un mutex et expose des méthodes synchronisées. La `BlockingQueue` de la section 21.3 en est un exemple parfait :

```cpp
template <typename T>  
class Monitor {  
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    T data_;

public:
    template <typename F>
    auto apply(F&& func) -> decltype(func(data_)) {
        std::unique_lock lock(mtx_);
        return func(data_);
    }

    template <typename Pred, typename F>
    auto wait_and_apply(Pred pred, F&& func) -> decltype(func(data_)) {
        std::unique_lock lock(mtx_);
        cv_.wait(lock, [&] { return pred(data_); });
        auto result = func(data_);
        lock.unlock();
        cv_.notify_all();
        return result;
    }
};
```

### Pattern 5 : Read-Copy-Update (RCU)

Pour les structures lues fréquemment et rarement modifiées (tables de routage, caches de configuration), le pattern RCU évite tout verrouillage côté lecteur :

1. Les **lecteurs** accèdent à la version courante via un pointeur atomique — aucun verrou.
2. L'**écrivain** crée une nouvelle copie de la structure, la modifie, puis remplace atomiquement le pointeur.
3. L'ancienne version est libérée quand plus aucun lecteur ne la référence (`std::shared_ptr` gère cela automatiquement).

```cpp
#include <atomic>
#include <memory>
#include <map>
#include <shared_mutex>

class RoutingTable {
    // Lecteurs accèdent sans verrou via le shared_ptr
    std::atomic<std::shared_ptr<const std::map<std::string, std::string>>> table_;

public:
    RoutingTable()
        : table_(std::make_shared<const std::map<std::string, std::string>>()) {}

    // Lecture — zéro contention
    std::string lookup(const std::string& key) const {
        auto snapshot = table_.load();  // Copie du shared_ptr (atomique)
        auto it = snapshot->find(key);
        return (it != snapshot->end()) ? it->second : "";
    }

    // Écriture — crée une nouvelle copie
    void update(const std::string& key, const std::string& value) {
        auto old = table_.load();
        auto updated = std::make_shared<std::map<std::string, std::string>>(*old);
        (*updated)[key] = value;
        table_.store(std::move(updated));
        // L'ancienne version est libérée quand le dernier lecteur la relâche
    }
};
```

> 💡 Ce pattern suppose que les écritures sont rares et que la copie de la structure est acceptable. Pour une `std::map` de quelques milliers d'entrées, c'est parfaitement viable. Pour une table de millions d'entrées, il faut envisager des structures persistantes (*persistent data structures*) ou un reader-writer lock (`std::shared_mutex`).

---

## Composer des opérations thread-safe

Un piège récurrent est de croire que si chaque opération individuelle est thread-safe, une séquence d'opérations l'est aussi. Ce n'est **presque jamais** le cas :

```cpp
// Chaque méthode de ThreadSafeMap est thread-safe individuellement
ThreadSafeMap<std::string, int> map;

// ❌ Race condition : la séquence n'est pas atomique
if (!map.contains("key")) {       // Un autre thread peut insérer "key"
    map.insert("key", 42);         // entre ces deux lignes
}

// ✅ La map doit exposer une opération combinée
map.insert_if_absent("key", 42);  // Atomique en une seule section critique
```

C'est la raison pour laquelle les conteneurs de la STL ne sont pas thread-safe en interne : même s'ils l'étaient, les patterns d'utilisation courants (vérifier-puis-agir, itérer-puis-modifier) nécessitent un verrouillage externe qui englobe plusieurs opérations.

**Règle** : concevez vos interfaces thread-safe autour d'**opérations atomiques composites** qui correspondent aux besoins métier, plutôt que de rendre chaque getter/setter individuellement synchronisé.

---

## Outils de détection

### ThreadSanitizer (TSan)

L'outil le plus efficace pour détecter les data races à l'exécution. Il instrumente le code à la compilation et détecte les accès concurrents non-synchronisés avec un très faible taux de faux positifs :

```bash
# Compilation avec TSan
g++ -std=c++23 -fsanitize=thread -g -O1 main.cpp -o main_tsan  
clang++ -std=c++23 -fsanitize=thread -g -O1 main.cpp -o main_tsan  

# Exécution — TSan rapporte les data races détectées
./main_tsan
```

Un rapport typique :

```
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 4 at 0x7f... by thread T1:
    #0 increment() main.cpp:8
  Previous write of size 4 at 0x7f... by thread T2:
    #0 increment() main.cpp:8
  Location is global 'counter' of size 4 at 0x7f...
```

TSan ralentit l'exécution d'un facteur 5-15x et augmente la consommation mémoire de 5-10x. Ce n'est pas un outil de production, mais il doit faire partie de votre pipeline CI pour tous les tests touchant à la concurrence.

> 📎 *Pour une couverture complète de ThreadSanitizer, voir la **section 29.4.3**.*

### Helgrind (Valgrind)

Alternative à TSan, intégrée à la suite Valgrind. Détecte les data races, les mauvais usages de mutex (double lock, unlock par un thread non-propriétaire), et les potentiels deadlocks :

```bash
valgrind --tool=helgrind ./main
```

Helgrind est significativement plus lent que TSan (20-50x) mais détecte des catégories de bugs supplémentaires (comme les ordres de verrouillage incohérents).

### Analyse statique

`clang-tidy` propose des checks liés à la concurrence :

- `bugprone-use-after-move` : détecte l'utilisation d'un objet après `std::move`.
- `clang-analyzer-core.NullDereference` : peut détecter des problèmes liés aux pointeurs partagés.
- `cppcoreguidelines-avoid-non-const-global-variables` : les globales mutables sont les premières victimes des data races.

Ces outils ne remplacent pas TSan (l'analyse statique ne peut pas raisonner sur l'ordonnancement dynamique des threads), mais ils attrapent des catégories de bugs complémentaires.

---

## Checklist de conception

Avant de partager une donnée entre threads, parcourez cette checklist :

1. **La donnée a-t-elle besoin d'être partagée ?** Souvent, chaque thread peut travailler sur sa propre copie et les résultats sont agrégés à la fin. C'est la solution la plus simple et la plus performante.

2. **La donnée peut-elle être immutable ?** Si oui, aucune synchronisation n'est nécessaire. Construisez-la une fois, partagez-la librement.

3. **`thread_local` suffit-il ?** Si chaque thread a besoin de sa propre version, le confinement au thread élimine le problème.

4. **Un `std::atomic` suffit-il ?** Si la donnée est un type primitif et que les opérations sont individuellement atomiques, pas besoin de mutex.

5. **Un passage de messages est-il plus adapté ?** Si les threads interagissent selon un pattern producteur/consommateur, une queue thread-safe est souvent plus claire qu'un partage de données protégé.

6. **Si un mutex est nécessaire, quelle est la granularité correcte ?** Protéger exactement ce qui doit l'être, pas plus. Garder les sections critiques courtes.

7. **Les opérations composites sont-elles atomiques ?** Vérifier-puis-agir, lire-modifier-écrire, itérer-puis-modifier — ces séquences doivent être dans la même section critique.

8. **ThreadSanitizer a-t-il été exécuté ?** Compilez vos tests avec `-fsanitize=thread` systématiquement. Un test qui passe sans TSan ne prouve rien sur la thread-safety.

---

## Résumé

| Concept | Détail |
|---------|--------|
| Data race | Accès concurrent non-synchronisé avec au moins une écriture → UB |
| Race condition | Bug logique dépendant de l'ordonnancement → résultat incorrect |
| Thread-safe | L'objet gère sa synchronisation en interne |
| Thread-compatible | Lectures concurrentes OK, écritures nécessitent synchronisation externe (convention STL) |
| Pattern le plus sûr | Données immutables — zéro synchronisation |
| Pattern le plus courant | Moniteur (mutex + CV encapsulés dans une classe) |
| Pattern haute performance | RCU (lecteurs sans verrou, écrivain copie-et-remplace) |
| Outil de détection | ThreadSanitizer (`-fsanitize=thread`) — intégrer au CI |
| Piège principal | Opérations individuellement thread-safe ≠ séquence thread-safe |

> **À suivre** : la section 21.7 présente `std::jthread` (C++20), qui résout deux problèmes récurrents de `std::thread` — le join manquant et l'arrêt propre des threads — grâce à un destructeur automatiquement joignant et au mécanisme de `std::stop_token`.

⏭️ [std::jthread (C++20) : Threads auto-stoppables](/21-threads-concurrence/07-jthread.md)
