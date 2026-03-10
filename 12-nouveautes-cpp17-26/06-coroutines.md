🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 12.6 Coroutines (C++20) : Programmation asynchrone

## Des fonctions qui savent s'arrêter et reprendre

Une fonction classique en C++ s'exécute du début à la fin en une seule traite. Elle est appelée, elle fait son travail, elle retourne — et tout son état local disparaît. C'est un modèle simple et efficace, mais il atteint ses limites dans deux grandes familles de situations : la production paresseuse de séquences de valeurs, et la programmation asynchrone où une opération doit « attendre » un résultat sans bloquer le thread courant.

Les coroutines de C++20 introduisent un nouveau type de fonction capable de **suspendre** son exécution à des points définis, de **rendre le contrôle** à l'appelant, puis de **reprendre** exactement là où elle s'était arrêtée, avec l'intégralité de son état local préservé. Ce mécanisme ouvre la porte à des générateurs paresseux, des pipelines asynchrones, et des modèles de concurrence coopérative — le tout intégré au langage.

C'est aussi, il faut le dire, l'une des fonctionnalités les plus complexes à comprendre de C++20. La raison : le standard définit un **mécanisme bas niveau** extrêmement flexible, mais ne fournit pas (avant C++23) de types prêts à l'emploi pour les cas d'usage courants. C++20 donne les briques ; C++23 et C++26 construisent les maisons.

> 📎 *Cette section explique les fondamentaux des coroutines. La section 12.11 couvre `std::generator` (C++23), qui simplifie radicalement l'écriture de générateurs. La section 12.14.4 présente `std::execution` (C++26), le modèle Senders/Receivers pour l'asynchronisme standardisé.*

## Coroutine vs fonction : la différence fondamentale

Une fonction classique possède un seul point d'entrée (le début) et un seul point de sortie (le `return`). Une coroutine possède un point d'entrée, mais **plusieurs points de suspension** et peut être reprise autant de fois que nécessaire :

```
Fonction classique :
  appel → exécution complète → retour

Coroutine :
  appel → exécution → suspension → ... → reprise → exécution → suspension → ... → fin
                         ↑                    ↑
                    (co_yield /           (l'appelant
                     co_await)            décide de reprendre)
```

Entre chaque suspension et reprise, l'état de la coroutine — variables locales, position dans le code, état des boucles — est entièrement préservé. La coroutine reprend exactement à l'instruction suivant le point de suspension, comme si elle ne s'était jamais arrêtée.

## Les trois mots-clés

C++20 introduit trois mots-clés qui transforment une fonction en coroutine. La simple présence de l'un de ces mots-clés dans le corps d'une fonction suffit à en faire une coroutine :

### co_yield : produire une valeur et suspendre

`co_yield` produit une valeur vers l'appelant et suspend la coroutine. C'est le mécanisme des **générateurs** — des fonctions qui produisent une séquence de valeurs à la demande :

```cpp
// Pseudo-code conceptuel (le type Generator est défini plus bas)
Generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;             // Produit la valeur courante et suspend
        auto next = a + b;
        a = b;
        b = next;
    }
}

// Côté appelant : on consomme la séquence paresseusement
auto fib = fibonacci();
// Chaque itération reprend la coroutine jusqu'au prochain co_yield
// → 0, 1, 1, 2, 3, 5, 8, 13, 21, ...
```

La boucle `while (true)` ne pose aucun problème : la coroutine suspend à chaque `co_yield` et ne continue que lorsque l'appelant demande la valeur suivante. La séquence est potentiellement infinie, mais seules les valeurs effectivement consommées sont calculées.

### co_await : attendre un résultat asynchrone

`co_await` suspend la coroutine en attendant qu'une opération asynchrone soit terminée. C'est le mécanisme de la **programmation asynchrone** :

```cpp
// Pseudo-code conceptuel
Task<std::string> fetch_data(std::string url) {
    auto response = co_await http_get(url);      // Suspend jusqu'à réception
    auto body = co_await response.read_body();    // Suspend jusqu'à lecture complète
    co_return body;                                // Retourne le résultat final
}
```

Pendant que la coroutine est suspendue en attente du résultat HTTP, le thread n'est pas bloqué — il peut exécuter d'autres tâches. Quand la réponse arrive, la coroutine est reprise automatiquement. C'est le même modèle que `async`/`await` en C#, JavaScript ou Python, mais avec un contrôle bas niveau sur le mécanisme de reprise.

### co_return : terminer la coroutine

`co_return` termine la coroutine et (optionnellement) fournit une valeur finale. C'est l'équivalent de `return` pour les coroutines :

```cpp
Task<int> compute() {
    int result = co_await expensive_calculation();
    co_return result * 2;    // Termine la coroutine avec une valeur
}
```

Une coroutine peut aussi se terminer implicitement en atteignant la fin de son corps (sans `co_return`), auquel cas elle se termine sans produire de valeur finale.

## Anatomie d'une coroutine : le modèle complet

Le mécanisme des coroutines C++20 repose sur trois composants interconnectés. Comprendre cette architecture est essentiel pour utiliser les coroutines correctement, même si en pratique on utilise des types fournis par des bibliothèques.

### 1. Le type de retour de la coroutine

Le type de retour d'une coroutine n'est pas la valeur produite — c'est un **handle** qui permet à l'appelant d'interagir avec la coroutine (la reprendre, lire les valeurs produites, vérifier si elle est terminée) :

```cpp
Generator<int> my_coroutine() {   // Generator<int> est le type de retour
    co_yield 1;
    co_yield 2;
    co_yield 3;
}
```

### 2. Le promise type

Chaque type de retour de coroutine doit contenir un type interne appelé `promise_type`. Ce type contrôle le comportement de la coroutine : que faire au démarrage ? que faire à chaque `co_yield` ? que faire à la fin ? C'est le « cerveau » de la coroutine :

```cpp
struct Generator<T>::promise_type {
    T current_value;

    Generator get_return_object();          // Crée l'objet retourné à l'appelant
    std::suspend_always initial_suspend();  // Suspendre au démarrage ?
    std::suspend_always final_suspend();    // Suspendre à la fin ?
    std::suspend_always yield_value(T v);   // Que faire à co_yield
    void return_void();                     // Que faire à co_return (sans valeur)
    void unhandled_exception();             // Que faire si une exception s'échappe
};
```

### 3. Le coroutine handle

`std::coroutine_handle<promise_type>` est un pointeur bas niveau vers l'état de la coroutine. Il permet de la reprendre (`resume()`), de vérifier si elle est terminée (`done()`), et d'accéder au promise :

```cpp
std::coroutine_handle<promise_type> handle;

handle.resume();    // Reprend l'exécution jusqu'au prochain point de suspension
handle.done();      // true si la coroutine est terminée
handle.destroy();   // Libère la mémoire de la coroutine
```

### Vue d'ensemble du cycle de vie

```
Appelant                           Coroutine
   │                                  │
   │── appel ────────────────────────▶│ Création du coroutine frame (heap)
   │                                  │ Construction du promise
   │◀── get_return_object() ──────────│ Retour du handle à l'appelant
   │                                  │ initial_suspend() → suspendu
   │                                  │
   │── handle.resume() ──────────────▶│ Exécution du corps
   │                                  │ ...
   │◀── co_yield value ───────────────│ yield_value(v) → suspendu
   │  (lire current_value)            │
   │                                  │
   │── handle.resume() ──────────────▶│ Reprise après co_yield
   │                                  │ ...
   │◀── co_return ────────────────────│ return_void/return_value → fin
   │                                  │ final_suspend() → suspendu (ou terminé)
   │── handle.destroy() ─────────────▶│ Destruction du frame
```

## Implémenter un générateur minimal

Pour comprendre concrètement le mécanisme, voici une implémentation minimale d'un type `Generator<T>` capable de supporter `co_yield` :

```cpp
#include <coroutine>
#include <optional>
#include <utility>

template <typename T>
class Generator {
public:
    struct promise_type {
        std::optional<T> current_value;

        Generator get_return_object() {
            return Generator{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(T value) {
            current_value = std::move(value);
            return {};   // Toujours suspendre après co_yield
        }

        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    // --- Interface publique ---

    explicit Generator(std::coroutine_handle<promise_type> h) : handle_(h) {}
    ~Generator() { if (handle_) handle_.destroy(); }

    // Non copiable, déplaçable
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)) {}
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    // Avancer au prochain co_yield
    bool next() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
            return !handle_.done();
        }
        return false;
    }

    // Lire la valeur courante
    const T& value() const {
        return *handle_.promise().current_value;
    }

private:
    std::coroutine_handle<promise_type> handle_;
};
```

Utilisation :

```cpp
#include <print>

Generator<int> range(int start, int end) {
    for (int i = start; i < end; ++i) {
        co_yield i;
    }
}

int main() {
    auto gen = range(1, 6);
    while (gen.next()) {
        std::print("{} ", gen.value());
    }
    // Sortie : 1 2 3 4 5
}
```

Ce code fonctionne, mais représente environ 50 lignes de boilerplate pour un simple générateur. C'est précisément le problème que C++23 résout avec `std::generator`.

## std::generator (C++23) : la solution prête à l'emploi

L'implémentation manuelle ci-dessus illustre le mécanisme, mais personne ne devrait avoir à l'écrire en production. C++23 fournit `std::generator<T>` dans `<generator>`, qui fait exactement cela — avec support complet des itérateurs, compatibilité avec les range-based for loops et les pipelines Ranges :

```cpp
#include <generator>
#include <print>

std::generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}

int main() {
    // std::generator est un range — compatible avec for et les pipelines
    for (int n : fibonacci() | std::views::take(10)) {
        std::print("{} ", n);
    }
    // Sortie : 0 1 1 2 3 5 8 13 21 34
}
```

Plus besoin de `promise_type`, de `coroutine_handle`, de destructeur personnalisé. Le type standard encapsule toute la machinerie et s'intègre naturellement dans l'écosystème des Ranges.

> 📎 *La section 12.11 couvre `std::generator` en détail : fonctionnalités avancées, générateurs récursifs avec `co_yield std::ranges::elements_of`, et comparaison avec les implémentations manuelles.*

## co_await : le mécanisme d'attente asynchrone

Si `co_yield` est relativement intuitif (produire une valeur et suspendre), `co_await` est le mécanisme le plus puissant — et le plus subtil — des coroutines C++20.

### Le concept Awaitable

Quand le compilateur rencontre `co_await expr`, il s'attend à ce que `expr` soit un objet *awaitable* — un type qui expose trois méthodes :

```cpp
struct MyAwaitable {
    bool await_ready();                        // L'opération est-elle déjà terminée ?
    void await_suspend(std::coroutine_handle<> h);  // Que faire à la suspension
    T await_resume();                          // Récupérer le résultat à la reprise
};
```

Le protocole est le suivant :

1. **`await_ready()`** — Si retourne `true`, la coroutine ne suspend pas et passe directement à `await_resume()`. Cela optimise le cas où le résultat est déjà disponible (lecture en cache, opération immédiate).

2. **`await_suspend(handle)`** — Appelée si `await_ready()` a retourné `false`. C'est ici qu'on programme la reprise de la coroutine : enregistrer le handle dans une file d'attente I/O, le passer à un thread pool, planifier un timer, etc.

3. **`await_resume()`** — Appelée quand la coroutine est reprise. Retourne le résultat de l'opération asynchrone.

### Les awaitables standard

C++20 fournit deux awaitables triviaux :

```cpp
// Ne suspend jamais — await_ready() retourne true
std::suspend_never{};

// Suspend toujours — await_ready() retourne false
std::suspend_always{};
```

Ce sont les briques de base utilisées dans les `promise_type` pour contrôler le comportement à `initial_suspend()` et `final_suspend()`.

### Exemple : un timer awaitable simplifié

Pour illustrer le mécanisme `co_await` avec un cas concret, voici un timer simplifié qui suspend la coroutine pendant une durée donnée :

```cpp
#include <coroutine>
#include <chrono>
#include <thread>

struct TimerAwaitable {
    std::chrono::milliseconds duration;

    bool await_ready() const {
        return duration <= std::chrono::milliseconds(0);  // Pas d'attente si durée <= 0
    }

    void await_suspend(std::coroutine_handle<> handle) const {
        // Dans un vrai système, on enregistrerait le handle dans un event loop.
        // Ici, on utilise un thread détaché pour simplifier.
        std::thread([handle, d = duration]() {
            std::this_thread::sleep_for(d);
            handle.resume();   // Reprend la coroutine après le délai
        }).detach();
    }

    void await_resume() const {}   // Pas de valeur de retour
};

// Helper pour créer l'awaitable
TimerAwaitable async_sleep(std::chrono::milliseconds ms) {
    return TimerAwaitable{ms};
}
```

Ce timer pourrait être utilisé dans une coroutine :

```cpp
Task<void> delayed_greeting() {
    std::print("Bonjour...\n");
    co_await async_sleep(std::chrono::seconds(2));
    std::print("...monde !\n");   // Exécuté 2 secondes plus tard
}
```

La coroutine s'interrompt à `co_await`, le thread est libre de faire autre chose, et la coroutine est reprise automatiquement après le délai. C'est le pattern fondamental de toute la programmation asynchrone par coroutines.

> ⚠️ *Cet exemple est pédagogique. En production, on utilise une event loop (Asio, libuv) ou un scheduler plutôt qu'un thread détaché par timer. L'intégration avec un scheduler est exactement ce que `std::execution` (C++26) standardise — voir section 12.14.4.*

## Coroutine frame et allocation mémoire

Quand une coroutine est appelée, le compilateur alloue un **coroutine frame** sur le heap. Ce frame contient les variables locales de la coroutine, le promise, et l'état nécessaire pour savoir où reprendre l'exécution. Cette allocation heap est le principal coût des coroutines par rapport aux fonctions classiques.

```
┌─────────────────────────────────────┐
│         Coroutine Frame             │
│                                     │
│  ┌──────────────────────────────┐   │
│  │  Promise object              │   │
│  ├──────────────────────────────┤   │
│  │  Variables locales           │   │
│  │  (a, b, next, ...)           │   │
│  ├──────────────────────────────┤   │
│  │  Point de reprise            │   │
│  │  (resume point index)        │   │
│  ├──────────────────────────────┤   │
│  │  Paramètres de la coroutine  │   │
│  └──────────────────────────────┘   │
└─────────────────────────────────────┘
          ↑
    Alloué sur le heap
    (operator new du promise, ou global)
```

### Heap Allocation Elision (HARE)

Le standard autorise le compilateur à éliminer l'allocation heap si la durée de vie du coroutine frame est entièrement contenue dans celle de l'appelant — c'est-à-dire si le compilateur peut prouver que la coroutine est créée, utilisée et détruite dans le même scope. On parle de *Heap Allocation Elision* (HARE).

En pratique, cette optimisation est appliquée par les compilateurs modernes dans de nombreux cas, notamment pour les générateurs simples consommés dans une boucle locale. Mais elle n'est pas garantie par le standard, et les coroutines de longue durée ou celles dont le handle est stocké dans une structure de données échappent à cette optimisation.

### Custom allocators

Le `promise_type` peut surcharger `operator new` et `operator delete` pour contrôler l'allocation du coroutine frame :

```cpp
struct promise_type {
    // Allocation personnalisée (arena, pool, stack allocator...)
    void* operator new(std::size_t size) {
        return my_pool_allocate(size);
    }
    void operator delete(void* ptr, std::size_t size) {
        my_pool_deallocate(ptr, size);
    }
    // ...
};
```

C'est un levier important pour les applications temps réel ou à haute fréquence où les allocations heap sont inacceptables.

## Cas d'usage des coroutines

### 1. Générateurs de séquences

Le cas d'usage le plus immédiatement utile. Les générateurs permettent de produire des séquences paresseuses, potentiellement infinies, avec un code qui ressemble à une simple boucle :

```cpp
std::generator<int> iota(int start = 0) {
    while (true) {
        co_yield start++;
    }
}

std::generator<std::string> read_lines(std::ifstream& file) {
    std::string line;
    while (std::getline(file, line)) {
        co_yield std::move(line);
    }
}
```

### 2. Pipelines asynchrones

Les coroutines permettent d'écrire du code asynchrone de manière séquentielle, sans callbacks imbriqués (*callback hell*) :

```cpp
// Sans coroutines : callback hell
void fetch_and_process(const std::string& url) {
    http_get(url, [](Response resp) {
        parse_json(resp.body(), [](Json data) {
            save_to_db(data, [](bool success) {
                if (success) log("Done");
            });
        });
    });
}

// Avec coroutines : séquentiel et lisible
Task<void> fetch_and_process(const std::string& url) {
    auto resp = co_await http_get(url);
    auto data = co_await parse_json(resp.body());
    bool success = co_await save_to_db(data);
    if (success) log("Done");
}
```

Les deux versions font la même chose — trois opérations asynchrones enchaînées. La version coroutine se lit comme du code synchrone, avec la gestion d'erreurs naturelle du C++ (try/catch, RAII).

### 3. Concurrence coopérative

Les coroutines permettent d'implémenter des modèles de concurrence coopérative (comme les *green threads* ou les *fibers*) où les tâches cèdent explicitement le contrôle plutôt que d'être interrompues par un scheduler préemptif.

### 4. State machines

Une coroutine est essentiellement une machine à états dont le compilateur génère la logique de transition. Des protocoles complexes (parsers, protocoles réseau) peuvent être exprimés comme du code séquentiel avec des points de suspension aux endroits d'attente de données.

## L'écosystème en 2026

Le standard C++20 fournit le mécanisme (`<coroutine>`) mais pas les abstractions de haut niveau. Voici l'état de l'écosystème en 2026 :

**Standard C++23** — `std::generator<T>` est le premier type coroutine standard prêt à l'emploi. Il couvre le cas des générateurs, mais pas l'asynchronisme.

**Standard C++26** — `std::execution` (Senders/Receivers) fournit un cadre complet pour l'asynchronisme, incluant l'intégration avec les coroutines. C'est le futur standardisé de la programmation asynchrone en C++. Voir section 12.14.4.

**Bibliothèques tierces** — En attendant la pleine maturité de `std::execution` dans les compilateurs :

- **cppcoro** — Bibliothèque de référence historique fournissant `task<T>`, `generator<T>`, `async_generator<T>`, et des primitives de synchronisation. Maintenance réduite depuis l'arrivée de `std::generator`.
- **Asio** — Supporte les coroutines C++20 via `asio::awaitable<T>` et `co_spawn`. C'est la solution la plus mature pour le networking asynchrone par coroutines en production aujourd'hui. Voir section 22.4.
- **folly::coro** (Meta) — Bibliothèque de coroutines utilisée en production à grande échelle.
- **libunifex** — Implémentation de référence du modèle Senders/Receivers, précurseur de `std::execution`.

## Limites et pièges

**Pas de coroutines pour les constructeurs et destructeurs.** Un constructeur ou un destructeur ne peut pas être une coroutine. C'est une restriction du standard.

**Pas de `co_await` dans les blocs `catch`.** On ne peut pas suspendre une coroutine pendant le traitement d'une exception. Le `co_await` est interdit dans un bloc `catch`.

**Les lambdas coroutines existent mais attention aux captures.** Une lambda peut être une coroutine, mais si elle capture des variables par référence et que la coroutine survit au scope de la lambda, les références deviennent invalides :

```cpp
auto make_coro() {
    int local = 42;
    auto lambda = [&local]() -> Generator<int> {
        co_yield local;   // DANGER : local sera détruit quand make_coro() retourne
    };
    return lambda();   // La coroutine survit à local → dangling reference
}
```

**Le debugging est plus complexe.** Le point d'exécution d'une coroutine suspendue n'apparaît pas dans la pile d'appels classique. Les outils de debugging s'améliorent (GDB, LLDB, Visual Studio ont un support croissant), mais l'expérience n'est pas encore au niveau des fonctions classiques.

**L'allocation heap peut être un problème.** Pour du code à haute performance ou temps réel, l'allocation du coroutine frame est un coût à considérer. Les custom allocators et le HARE atténuent ce problème, mais ne l'éliminent pas dans tous les cas.

## Bonnes pratiques

**Utiliser `std::generator` (C++23) pour les générateurs.** Ne pas réimplémenter manuellement le boilerplate `promise_type` + `coroutine_handle` quand le standard fournit une solution testée et optimisée.

**Utiliser une bibliothèque éprouvée pour l'asynchronisme.** Asio, folly::coro, ou (quand le support compilateur le permettra pleinement) `std::execution`. L'écriture manuelle de types `Task<T>` avec `co_await` est un exercice formateur mais rarement justifié en production.

**Préférer `co_yield` à `co_await` pour débuter.** Les générateurs sont conceptuellement plus simples que les tâches asynchrones. Ils constituent un excellent point d'entrée pour se familiariser avec le modèle de suspension/reprise.

**Capturer par valeur dans les lambdas coroutines.** Puisqu'une coroutine peut survivre au scope de sa création, les captures par référence sont dangereuses. Préférer systématiquement les captures par valeur (`[=]` ou captures explicites par copie).

**Marquer les move constructors `noexcept`.** Les types stockés dans les variables locales d'une coroutine bénéficient de move constructors `noexcept` pour les opérations internes de la machinerie coroutine. C'est une bonne pratique générale (section 10.3) qui prend une importance particulière ici.

**Penser en termes de flux de données, pas de flux de contrôle.** Les coroutines sont plus naturelles quand on les conçoit comme des producteurs/consommateurs de données (générateurs, streams) ou des étapes dans un pipeline asynchrone, plutôt que comme des threads légers à gérer manuellement.

---

>  
> 📎 [12.11 std::generator (C++23) : Coroutines simplifiées](/12-nouveautes-cpp17-26/11-generator.md)  
>  
> 📎 [12.14.4 std::execution (C++26) : Asynchronisme standardisé](/12-nouveautes-cpp17-26/14.4-std-execution.md)

⏭️ [std::print et std::format (C++23) : Formatage moderne](/12-nouveautes-cpp17-26/07-std-print-format.md)
