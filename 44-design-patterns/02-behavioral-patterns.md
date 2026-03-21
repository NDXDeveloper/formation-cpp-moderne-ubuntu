🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 44.2 — Observer, Strategy, Command

## Patterns Comportementaux en C++ Moderne

---

## Introduction

Les patterns comportementaux (*behavioral patterns*) définissent la manière dont les objets **communiquent** et **collaborent** entre eux. Là où les patterns de création (section 44.1) portent sur l'instanciation, les patterns comportementaux portent sur le flux de contrôle et la distribution des responsabilités.

Trois patterns comportementaux restent omniprésents dans le code C++ contemporain — système, DevOps, backend haute performance :

- **Observer** : notifier automatiquement un ensemble d'objets lorsqu'un état change.  
- **Strategy** : rendre un algorithme interchangeable à l'exécution (ou à la compilation).  
- **Command** : encapsuler une action en tant qu'objet, permettant l'annulation, la file d'attente et la journalisation.

Le C++ moderne a profondément transformé leur implémentation. Les hiérarchies de classes lourdes héritées du GoF cèdent la place à des lambdas, `std::function`, templates et concepts. Dans certains cas, le pattern tout entier se réduit à un type de la bibliothèque standard.

---

## Observer : réagir aux changements d'état

### Le problème

Un objet (le *sujet*) change d'état, et plusieurs autres objets (les *observateurs*) doivent être notifiés de ce changement sans que le sujet ne les connaisse individuellement. C'est le mécanisme derrière les systèmes d'événements, les signaux/slots, les callbacks de monitoring, les watchers de fichiers et les flux de métriques.

### Approche classique (GoF)

L'implémentation GoF repose sur une interface `Observer` avec une méthode `update()` et un sujet qui maintient une liste de pointeurs vers cette interface :

```cpp
// ⚠️ Approche historique — fragile et verbeuse
class Observer {  
public:  
    virtual ~Observer() = default;
    virtual void update(float temperature, float humidity) = 0;
};

class Subject {
    std::vector<Observer*> observers_;  // Pointeurs bruts — danger !
public:
    void attach(Observer* obs)  { observers_.push_back(obs); }
    void detach(Observer* obs)  {
        std::erase(observers_, obs);
    }
    void notify(float temp, float hum) {
        for (auto* obs : observers_) {
            obs->update(temp, hum);  // L'observateur est-il encore vivant ?
        }
    }
};
```

Ce design souffre de **trois défauts majeurs**. Les pointeurs bruts créent un risque de *dangling pointer* si un observateur est détruit sans se désinscrire. L'interface `Observer` impose à chaque observateur d'hériter d'une classe abstraite et d'implémenter exactement la bonne signature — couplage fort. Et les paramètres de `update()` sont figés dans l'interface : tout changement de signature propage une modification dans toute la hiérarchie.

### Approche moderne : `std::function` et tokens de désinscription

En C++ moderne, un observateur n'est pas un objet héritant d'une interface : c'est n'importe quel **callable** — une lambda, une fonction libre, un objet avec `operator()`. Le sujet stocke des `std::function` et retourne un token de désinscription :

```cpp
#include <functional>
#include <vector>
#include <cstdint>
#include <algorithm>

template<typename... Args>  
class Signal {  
public:  
    using Slot     = std::function<void(Args...)>;
    using SlotId   = uint64_t;

    // Inscription — retourne un token pour se désinscrire
    SlotId connect(Slot slot) {
        auto id = next_id_++;
        slots_.push_back({id, std::move(slot)});
        return id;
    }

    // Désinscription par token
    void disconnect(SlotId id) {
        std::erase_if(slots_, [id](const auto& entry) {
            return entry.id == id;
        });
    }

    // Notification de tous les observateurs
    void emit(Args... args) const {
        for (const auto& [id, slot] : slots_) {
            slot(args...);
        }
    }

    // Nombre d'observateurs connectés
    std::size_t size() const { return slots_.size(); }

private:
    struct Entry {
        SlotId id;
        Slot   slot;
    };

    std::vector<Entry> slots_;
    SlotId next_id_ = 0;
};
```

Utilisation :

```cpp
class TemperatureSensor {  
public:  
    Signal<float> on_temperature_changed;  // Signal public

    void set_temperature(float temp) {
        if (temp != temperature_) {
            temperature_ = temp;
            on_temperature_changed.emit(temp);
        }
    }

private:
    float temperature_ = 0.0f;
};

// --- Côté client ---

TemperatureSensor sensor;

// Lambda comme observateur — aucune interface à implémenter
auto id1 = sensor.on_temperature_changed.connect([](float temp) {
    std::print("Affichage : {:.1f}°C\n", temp);
});

auto id2 = sensor.on_temperature_changed.connect([](float temp) {
    if (temp > 40.0f) {
        std::print(stderr, "ALERTE : température critique !\n");
    }
});

sensor.set_temperature(25.0f);   // Notifie les deux observateurs  
sensor.set_temperature(42.0f);   // Déclenche l'alerte  

sensor.on_temperature_changed.disconnect(id1);  // Désinscription ciblée  
sensor.set_temperature(10.0f);   // Seul l'observateur d'alerte est notifié  
```

### Désinscription automatique avec RAII

Le token brut (`SlotId`) laisse au client la responsabilité de se désinscrire. Un oubli produit un appel vers un contexte invalide si la lambda capture des références vers un objet détruit. La solution idiomatique en C++ est un **guard RAII** qui se désinscrit automatiquement à sa destruction :

```cpp
template<typename... Args>  
class ScopedConnection {  
public:  
    ScopedConnection() = default;

    ScopedConnection(Signal<Args...>& signal, typename Signal<Args...>::SlotId id)
        : signal_(&signal), id_(id) {}

    // Non copiable
    ScopedConnection(const ScopedConnection&)            = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    // Déplaçable
    ScopedConnection(ScopedConnection&& other) noexcept
        : signal_(std::exchange(other.signal_, nullptr))
        , id_(other.id_) {}

    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if (this != &other) {
            disconnect();
            signal_ = std::exchange(other.signal_, nullptr);
            id_     = other.id_;
        }
        return *this;
    }

    ~ScopedConnection() { disconnect(); }

    void disconnect() {
        if (signal_) {
            signal_->disconnect(id_);
            signal_ = nullptr;
        }
    }

private:
    Signal<Args...>*                      signal_ = nullptr;
    typename Signal<Args...>::SlotId      id_     = 0;
};
```

Utilisation :

```cpp
{
    ScopedConnection<float> conn(
        sensor.on_temperature_changed,
        sensor.on_temperature_changed.connect([](float t) {
            std::print("Temp : {:.1f}°C\n", t);
        })
    );

    sensor.set_temperature(30.0f);  // L'observateur est notifié

}   // conn est détruit → désinscription automatique

sensor.set_temperature(35.0f);      // Plus aucun observateur — rien ne se passe
```

Le `ScopedConnection` suit le même principe que `std::lock_guard` pour les mutex ou `std::unique_ptr` pour la mémoire : la durée de vie de la connexion est liée au scope. Aucun risque de *dangling callback*.

### Observer et `std::weak_ptr` : survie incertaine

Lorsque l'observateur est un objet à durée de vie gérée par `std::shared_ptr`, le signal peut stocker un `std::weak_ptr` et vérifier la validité avant chaque notification :

```cpp
template<typename... Args>  
class WeakSignal {  
public:  
    using Callback = std::function<void(Args...)>;

    template<typename T>
    void connect(std::shared_ptr<T> observer, Callback cb) {
        slots_.push_back({
            std::weak_ptr<void>(observer),
            std::move(cb)
        });
    }

    void emit(Args... args) {
        // Nettoyer les observateurs morts et notifier les vivants
        std::erase_if(slots_, [&](auto& entry) {
            if (entry.weak.expired()) return true;   // Nettoyage
            entry.callback(args...);                  // Notification
            return false;
        });
    }

private:
    struct Entry {
        std::weak_ptr<void> weak;
        Callback            callback;
    };
    std::vector<Entry> slots_;
};
```

Cette approche élimine le besoin de désinscription explicite : quand le `shared_ptr` de l'observateur est détruit, le `weak_ptr` expire et l'entrée est nettoyée au prochain `emit()`.

### Observer thread-safe

Dans un contexte multi-thread (monitoring, métriques, logging), le signal doit protéger ses structures internes. Le point délicat est la notification : prendre un lock pendant l'appel aux callbacks risque un deadlock si un callback tente de se désinscrire.

La solution classique est de **copier la liste** des slots sous le lock, puis de notifier hors du lock :

```cpp
void emit(Args... args) const {
    std::vector<Entry> snapshot;
    {
        std::shared_lock lock(mtx_);   // Lecture seule
        snapshot = slots_;              // Copie sous le lock
    }
    // Notification hors du lock — pas de deadlock
    for (const auto& [id, slot] : snapshot) {
        slot(args...);
    }
}
```

Le coût est une copie de `std::vector<Entry>` à chaque émission. Si les émissions sont fréquentes et les observateurs nombreux, une approche lock-free ou une structure immutable (copy-on-write) peut être préférable, mais ces optimisations relèvent du chapitre 42 (programmation bas niveau).

---

## Strategy : algorithmes interchangeables

### Le problème

Un objet doit pouvoir **changer de comportement** à l'exécution sans modifier son code. Typiquement : un module de compression qui peut utiliser gzip, lz4 ou zstd ; un système de logging qui peut écrire sur stdout, dans un fichier ou vers syslog ; un routeur qui peut appliquer différentes stratégies de load balancing.

### Approche classique (GoF) : hiérarchie de classes

```cpp
// ⚠️ Approche historique — verbose
class SortStrategy {  
public:  
    virtual ~SortStrategy() = default;
    virtual void sort(std::vector<int>& data) = 0;
};

class QuickSort : public SortStrategy {  
public:  
    void sort(std::vector<int>& data) override { /* ... */ }
};

class MergeSort : public SortStrategy {  
public:  
    void sort(std::vector<int>& data) override { /* ... */ }
};

class Sorter {
    std::unique_ptr<SortStrategy> strategy_;
public:
    void set_strategy(std::unique_ptr<SortStrategy> s) {
        strategy_ = std::move(s);
    }
    void sort(std::vector<int>& data) {
        strategy_->sort(data);
    }
};
```

Pour chaque nouvelle stratégie : une nouvelle classe, un nouveau fichier, un nouveau `#include`. Le ratio signal/boilerplate est défavorable.

### Approche moderne 1 : `std::function` (Strategy runtime)

En C++ moderne, une stratégie est un **callable**. `std::function` absorbe lambdas, fonctions libres, functors et méthodes liées :

```cpp
class Sorter {  
public:  
    using Strategy = std::function<void(std::vector<int>&)>;

    explicit Sorter(Strategy strategy)
        : strategy_(std::move(strategy)) {}

    void set_strategy(Strategy strategy) {
        strategy_ = std::move(strategy);
    }

    void sort(std::vector<int>& data) {
        strategy_(data);
    }

private:
    Strategy strategy_;
};
```

Utilisation :

```cpp
// Lambda comme stratégie
auto sorter = Sorter([](std::vector<int>& data) {
    std::ranges::sort(data);  // Quick sort (implémentation standard)
});

sorter.sort(my_data);

// Changement de stratégie à l'exécution
sorter.set_strategy([](std::vector<int>& data) {
    std::ranges::stable_sort(data);  // Merge sort stable
});

sorter.sort(my_data);
```

Zéro hiérarchie, zéro allocation supplémentaire (les petites lambdas sont stockées inline dans `std::function` via la *small buffer optimization*). Le code est concis et lisible.

### Un exemple réaliste : stratégie de retry

En contexte DevOps, les stratégies de retry sont un cas d'usage naturel du pattern :

```cpp
using RetryStrategy = std::function<std::chrono::milliseconds(int attempt)>;

// Stratégies prédéfinies
namespace retry {

    inline RetryStrategy constant(std::chrono::milliseconds delay) {
        return [delay](int) { return delay; };
    }

    inline RetryStrategy linear(std::chrono::milliseconds base) {
        return [base](int attempt) {
            return base * attempt;
        };
    }

    inline RetryStrategy exponential(
        std::chrono::milliseconds base,
        std::chrono::milliseconds max_delay = std::chrono::seconds(60))
    {
        return [base, max_delay](int attempt) {
            auto delay = base * (1 << std::min(attempt, 20));
            return std::min(delay, max_delay);
        };
    }

}  // namespace retry
```

```cpp
class HttpClient {  
public:  
    explicit HttpClient(RetryStrategy strategy = retry::exponential(
                            std::chrono::milliseconds(100)))
        : retry_strategy_(std::move(strategy)) {}

    Response get(std::string_view url) {
        for (int attempt = 0; attempt < max_retries_; ++attempt) {
            auto response = do_request(url);
            if (response.status < 500) return response;

            auto delay = retry_strategy_(attempt);
            std::this_thread::sleep_for(delay);
        }
        throw std::runtime_error("Max retries exceeded");
    }

    void set_retry_strategy(RetryStrategy strategy) {
        retry_strategy_ = std::move(strategy);
    }

private:
    Response do_request(std::string_view url);
    RetryStrategy retry_strategy_;
    int           max_retries_ = 5;
};
```

```cpp
// Utilisation avec différentes stratégies
auto client = HttpClient(retry::exponential(std::chrono::milliseconds(200)));

// Changement à l'exécution pour un contexte différent
client.set_retry_strategy(retry::constant(std::chrono::seconds(1)));

// Stratégie personnalisée via lambda
client.set_retry_strategy([](int attempt) {
    // Jitter aléatoire pour éviter le thundering herd
    auto base = std::chrono::milliseconds(100 * (1 << attempt));
    auto jitter = std::chrono::milliseconds(rand() % 100);
    return base + jitter;
});
```

### Approche moderne 2 : Policy-Based Design (Strategy compile-time)

Quand la stratégie est connue à la compilation, un **template parameter** élimine totalement le coût runtime de `std::function` (qui implique une indirection et potentiellement une allocation heap pour les gros callables) :

```cpp
// La stratégie est un paramètre template — zéro overhead
template<typename RetryPolicy>  
class HttpClient {  
public:  
    explicit HttpClient(RetryPolicy policy = {})
        : policy_(std::move(policy)) {}

    Response get(std::string_view url) {
        for (int attempt = 0; attempt < max_retries_; ++attempt) {
            auto response = do_request(url);
            if (response.status < 500) return response;

            auto delay = policy_(attempt);
            std::this_thread::sleep_for(delay);
        }
        throw std::runtime_error("Max retries exceeded");
    }

private:
    Response do_request(std::string_view url);
    RetryPolicy policy_;
    int         max_retries_ = 5;
};
```

Avec un concept pour contraindre le paramètre :

```cpp
template<typename T>  
concept RetryPolicyLike = requires(T policy, int attempt) {  
    { policy(attempt) } -> std::convertible_to<std::chrono::milliseconds>;
};

template<RetryPolicyLike RetryPolicy>  
class HttpClient {  
    // ...
};
```

Utilisation :

```cpp
// Functor comme policy
struct ExponentialBackoff {
    std::chrono::milliseconds base{100};

    std::chrono::milliseconds operator()(int attempt) const {
        return base * (1 << std::min(attempt, 20));
    }
};

auto client = HttpClient<ExponentialBackoff>(ExponentialBackoff{.base = 200ms});

// Lambda directement
auto client2 = HttpClient([](int attempt) {
    return std::chrono::milliseconds(100 * attempt);
});
```

### `std::function` vs template : le bon compromis

| Critère | `std::function` (runtime) | Template parameter (compile-time) |
|---|---|---|
| Changement de stratégie à l'exécution | ✅ Oui | ❌ Non (type fixé) |
| Overhead | Indirection + potentielle allocation | Zéro (inlining complet) |
| Collections hétérogènes de stratégies | ✅ Oui | ❌ Non (types différents) |
| Temps de compilation | Rapide | Plus long (instanciations templates) |
| Code dans les headers | Non requis | Oui (templates) |
| Cas d'usage | Stratégie configurable, plugins | Code performance-critique, bibliothèques |

En pratique, `std::function` est le choix par défaut. Le policy-based design (template) est réservé aux cas où le profiling montre que l'indirection de `std::function` est un goulot d'étranglement — ce qui est rare en dehors des boucles très chaudes.

---

## Command : encapsuler des actions

### Le problème

Une action doit être traitée comme une **donnée** : stockée dans une file d'attente, différée, rejouée, annulée, journalisée ou sérialisée. C'est le cœur des systèmes de undo/redo, des job queues, des pipelines de commandes et des systèmes d'audit.

### Approche classique (GoF) : une classe par commande

```cpp
// ⚠️ Approche historique — une classe par action
class Command {  
public:  
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

class InsertTextCommand : public Command {
    Document& doc_;
    std::string text_;
    size_t position_;
public:
    InsertTextCommand(Document& doc, std::string text, size_t pos)
        : doc_(doc), text_(std::move(text)), position_(pos) {}

    void execute() override { doc_.insert(position_, text_); }
    void undo()    override { doc_.erase(position_, text_.size()); }
};
```

Pour chaque nouvelle action : une nouvelle classe. Le boilerplate explose.

### Approche moderne : `std::function` pour les commandes simples

Quand une commande n'a pas besoin de `undo()`, elle se réduit à un callable :

```cpp
using Command = std::function<void()>;

class CommandQueue {  
public:  
    void push(Command cmd) {
        queue_.push(std::move(cmd));
    }

    void execute_all() {
        while (!queue_.empty()) {
            queue_.front()();
            queue_.pop();
        }
    }

    std::size_t pending() const { return queue_.size(); }

private:
    std::queue<Command> queue_;
};
```

Utilisation :

```cpp
CommandQueue queue;

// Les lambdas capturent tout le contexte nécessaire
queue.push([&server] { server.start(); });  
queue.push([&db]     { db.run_migrations(); });  
queue.push([&cache]  { cache.warm_up(); });  

// Exécution séquentielle
queue.execute_all();
```

La lambda capture par référence ou par valeur selon les besoins. Aucune hiérarchie, aucune classe dédiée. Le pattern Command est devenu **un alias de type sur `std::function<void()>`**.

### Command avec undo/redo

Quand l'annulation est nécessaire, le pattern nécessite un peu plus de structure. Mais une paire de `std::function` remplace toujours la hiérarchie de classes :

```cpp
struct UndoableCommand {
    std::function<void()> execute;
    std::function<void()> undo;
    std::string           description;  // Pour le logging/debugging
};

class CommandHistory {  
public:  
    void execute(UndoableCommand cmd) {
        cmd.execute();
        undo_stack_.push(std::move(cmd));
        // Vider le redo stack après une nouvelle action
        redo_stack_ = {};
    }

    bool can_undo() const { return !undo_stack_.empty(); }
    bool can_redo() const { return !redo_stack_.empty(); }

    void undo() {
        if (!can_undo()) return;
        auto cmd = std::move(undo_stack_.top());
        undo_stack_.pop();
        cmd.undo();
        redo_stack_.push(std::move(cmd));
    }

    void redo() {
        if (!can_redo()) return;
        auto cmd = std::move(redo_stack_.top());
        redo_stack_.pop();
        cmd.execute();
        undo_stack_.push(std::move(cmd));
    }

private:
    std::stack<UndoableCommand> undo_stack_;
    std::stack<UndoableCommand> redo_stack_;
};
```

Création de commandes avec des lambdas capturantes :

```cpp
// Factory de commandes — chaque lambda capture son contexte
UndoableCommand make_insert_cmd(Document& doc, size_t pos, std::string text) {
    return {
        .execute     = [&doc, pos, text] { doc.insert(pos, text); },
        .undo        = [&doc, pos, len = text.size()] { doc.erase(pos, len); },
        .description = std::format("Insert '{}' at {}", text, pos)
    };
}

UndoableCommand make_delete_cmd(Document& doc, size_t pos, size_t len) {
    // Capturer le texte supprimé pour pouvoir le restaurer
    auto deleted = doc.substr(pos, len);
    return {
        .execute     = [&doc, pos, len]  { doc.erase(pos, len); },
        .undo        = [&doc, pos, deleted] { doc.insert(pos, deleted); },
        .description = std::format("Delete {} chars at {}", len, pos)
    };
}
```

```cpp
CommandHistory history;

history.execute(make_insert_cmd(doc, 0, "Hello "));  
history.execute(make_insert_cmd(doc, 6, "World"));  
// doc contient "Hello World"

history.undo();
// doc contient "Hello "

history.redo();
// doc contient "Hello World"
```

### Command asynchrone : la job queue

En contexte serveur ou DevOps, les commandes sont souvent exécutées de manière **asynchrone** dans un pool de threads. Le Command pattern se combine naturellement avec une file thread-safe :

```cpp
class JobQueue {  
public:  
    using Job = std::function<void()>;

    explicit JobQueue(int num_workers) {
        for (int i = 0; i < num_workers; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ~JobQueue() {
        {
            std::lock_guard lock(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& w : workers_) {
            w.join();
        }
    }

    void submit(Job job) {
        {
            std::lock_guard lock(mtx_);
            jobs_.push(std::move(job));
        }
        cv_.notify_one();
    }

    // Non copiable, non déplaçable
    JobQueue(const JobQueue&)            = delete;
    JobQueue& operator=(const JobQueue&) = delete;

private:
    void worker_loop() {
        while (true) {
            Job job;
            {
                std::unique_lock lock(mtx_);
                cv_.wait(lock, [this] { return stop_ || !jobs_.empty(); });
                if (stop_ && jobs_.empty()) return;
                job = std::move(jobs_.front());
                jobs_.pop();
            }
            job();  // Exécution hors du lock
        }
    }

    std::queue<Job>            jobs_;
    std::vector<std::jthread>  workers_;
    std::mutex                 mtx_;
    std::condition_variable    cv_;
    bool                       stop_ = false;
};
```

```cpp
JobQueue queue(4);  // 4 worker threads

queue.submit([&] { deploy_service("auth-service"); });  
queue.submit([&] { deploy_service("api-gateway"); });  
queue.submit([&] { run_health_checks(); });  
queue.submit([&] { notify_slack("Deployment started"); });  
```

Chaque lambda est une commande autonome. La `JobQueue` ne connaît rien des actions qu'elle exécute — découplage total entre la soumission et l'exécution.

### Command typée : au-delà de `std::function<void()>`

Quand les commandes ont des types de retour ou des paramètres hétérogènes, `std::function<void()>` ne suffit plus. Deux approches se complètent.

**`std::packaged_task` pour les commandes avec résultat :**

```cpp
class AsyncExecutor {  
public:  
    template<typename F>
    auto submit(F&& task) -> std::future<std::invoke_result_t<F>> {
        using R = std::invoke_result_t<F>;

        auto packaged = std::packaged_task<R()>(std::forward<F>(task));
        auto future   = packaged.get_future();

        {
            std::lock_guard lock(mtx_);
            tasks_.push([t = std::move(packaged)]() mutable { t(); });
        }
        cv_.notify_one();

        return future;
    }

    // ... worker_loop similaire à JobQueue
};
```

```cpp
AsyncExecutor executor;

auto result = executor.submit([] {
    return compute_heavy_metric();
});

// ... faire autre chose ...

auto metric = result.get();  // Bloque jusqu'au résultat
```

**`std::variant` pour les commandes hétérogènes dans une même file :**

```cpp
struct StartService  { std::string name; };  
struct StopService   { std::string name; };  
struct ScaleService  { std::string name; int replicas; };  

using DeployCommand = std::variant<StartService, StopService, ScaleService>;

void dispatch(const DeployCommand& cmd) {
    std::visit([](const auto& c) {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, StartService>) {
            std::print("Starting {}\n", c.name);
        } else if constexpr (std::is_same_v<T, StopService>) {
            std::print("Stopping {}\n", c.name);
        } else if constexpr (std::is_same_v<T, ScaleService>) {
            std::print("Scaling {} to {} replicas\n", c.name, c.replicas);
        }
    }, cmd);
}
```

Cette approche est type-safe, exhaustive (le compilateur vérifie que tous les cas sont traités), sérialisable et inspectable — contrairement à un `std::function` opaque.

---

## Combinaison des patterns : un système d'événements complet

En pratique, Observer, Strategy et Command se combinent fréquemment. Un système de monitoring réaliste illustre cette synergie :

```cpp
// Strategy : comment formater une alerte
using AlertFormatter = std::function<std::string(std::string_view metric,
                                                  double value,
                                                  double threshold)>;

// Command : action à exécuter quand une alerte est déclenchée
using AlertAction = std::function<void(const std::string& formatted_alert)>;

class MetricMonitor {  
public:  
    struct AlertRule {
        std::string     metric_name;
        double          threshold;
        AlertFormatter  formatter;
        AlertAction     action;
    };

    // Observer : signal émis quand une métrique dépasse un seuil
    Signal<std::string, double> on_metric_received;

    void add_rule(AlertRule rule) {
        auto metric = rule.metric_name;
        on_metric_received.connect(
            [rule = std::move(rule)](const std::string& name, double value) {
                if (name == rule.metric_name && value > rule.threshold) {
                    auto message = rule.formatter(name, value, rule.threshold);
                    rule.action(message);  // Exécution du Command
                }
            }
        );
    }

    void record(const std::string& metric, double value) {
        on_metric_received.emit(metric, value);
    }
};
```

```cpp
MetricMonitor monitor;

// Formatters (Strategy)
auto simple_fmt = [](std::string_view m, double v, double t) {
    return std::format("[ALERT] {} = {:.2f} (seuil: {:.2f})", m, v, t);
};

// Actions (Command)
auto log_action = [](const std::string& msg) {
    std::print(stderr, "{}\n", msg);
};

auto slack_action = [&slack_client](const std::string& msg) {
    slack_client.post("#alerts", msg);
};

// Composition
monitor.add_rule({
    .metric_name = "cpu_usage",
    .threshold   = 90.0,
    .formatter   = simple_fmt,
    .action      = log_action
});

monitor.add_rule({
    .metric_name = "memory_usage",
    .threshold   = 85.0,
    .formatter   = simple_fmt,
    .action      = slack_action
});

// Flux de métriques
monitor.record("cpu_usage", 95.2);     // Déclenche l'alerte CPU → log  
monitor.record("memory_usage", 72.0);  // Sous le seuil → rien  
monitor.record("memory_usage", 88.5);  // Déclenche l'alerte mém → Slack  
```

L'Observer (`Signal`) distribue les événements. La Strategy (`AlertFormatter`) détermine comment les formater. Le Command (`AlertAction`) détermine quoi en faire. Les trois patterns coopèrent, chacun étant réduit à un `std::function` — aucune hiérarchie de classes, aucun couplage.

---

## Anti-patterns communs aux trois patterns

### Capturer `this` par référence dans une lambda stockée

```cpp
// ❌ Danger : si l'objet est détruit, la lambda pointe vers rien
signal.connect([this](float t) {
    this->update_display(t);  // Dangling si *this est détruit
});
```

Utiliser un `ScopedConnection` (présenté plus haut) ou un `std::weak_ptr` pour les objets à durée de vie incertaine.

### `std::function` pour une stratégie appelée des millions de fois

Dans une boucle critique appelant la stratégie à chaque itération, l'indirection de `std::function` (appel virtuel interne) peut être mesurable. Profiler d'abord (cf. chapitre 31), puis basculer vers un template parameter si nécessaire.

### Command sans limite de taille de l'historique

Un undo/redo qui ne limite pas la taille de la pile consomme de la mémoire indéfiniment. Toujours définir une capacité maximale :

```cpp
void execute(UndoableCommand cmd) {
    cmd.execute();
    undo_stack_.push(std::move(cmd));
    if (undo_stack_.size() > max_history_) {
        // Supprimer la commande la plus ancienne
        // (nécessite un std::deque au lieu d'un std::stack)
    }
}
```

---

## Points clés à retenir

- **Observer** : la classe `Signal<Args...>` avec `std::function` et tokens de désinscription remplace les hiérarchies GoF. Le `ScopedConnection` RAII élimine les *dangling callbacks*. Pour le multi-thread, copier la liste des slots sous le lock avant de notifier hors du lock.  
- **Strategy** : `std::function` est le choix par défaut pour les stratégies interchangeables à l'exécution. Le policy-based design (template parameter + concept) offre un zéro-overhead quand la stratégie est connue à la compilation. Profiler avant de choisir la variante template.  
- **Command** : `std::function<void()>` couvre la majorité des cas. Le undo/redo nécessite une paire `execute`/`undo` dans une struct, toujours implémentable avec des lambdas. Les commandes typées (retour de valeur, hétérogénéité) utilisent `std::packaged_task` ou `std::variant`.  
- Les trois patterns se **combinent** naturellement : l'Observer distribue, la Strategy décide comment traiter, le Command encapsule l'action.  
- En C++ moderne, **`std::function` + lambdas** ont absorbé l'essentiel des patterns comportementaux GoF. Une hiérarchie de classes n'est justifiée que pour les stratégies/commandes nécessitant un polymorphisme binaire (plugins, bibliothèques dynamiques).

⏭️ [CRTP (Curiously Recurring Template Pattern)](/44-design-patterns/03-crtp.md)
