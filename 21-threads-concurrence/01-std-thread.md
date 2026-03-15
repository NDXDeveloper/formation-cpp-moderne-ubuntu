🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 21.1 std::thread : Création et gestion de threads

## Introduction

`std::thread` est la brique fondamentale de la programmation concurrente en C++. Introduit par C++11, cet objet représente un thread d'exécution du système d'exploitation. Chaque instance de `std::thread` encapsule un thread noyau Linux réel (via pthreads), avec sa propre pile d'appels et son propre flux d'exécution.

La philosophie de `std::thread` est simple : vous lui donnez une fonction (ou tout objet *callable*), il l'exécute dans un thread séparé. Mais cette simplicité apparente masque des subtilités importantes autour du cycle de vie, du passage d'arguments et de la gestion des erreurs, que cette section couvre en détail.

---

## Création d'un thread

### Syntaxe de base

Un `std::thread` se construit en lui passant un *callable* — une fonction, un lambda, un foncteur, ou un pointeur vers une fonction membre — suivi des arguments à lui transmettre :

```cpp
#include <thread>
#include <print>

void say_hello() {
    std::println("Bonjour depuis un thread !");
}

int main() {
    std::thread t(say_hello);  // Le thread démarre immédiatement
    t.join();                  // On attend sa fin
}
```

Le thread commence son exécution **dès la construction** de l'objet `std::thread`. Il n'y a pas de méthode `start()` — la création *est* le démarrage.

### Les différents types de callables

`std::thread` accepte tout ce qui peut être appelé avec les arguments fournis. Voici les formes les plus courantes :

```cpp
#include <thread>
#include <print>
#include <string>

// 1. Fonction libre
void greet(const std::string& name) {
    std::println("Bonjour, {} !", name);
}

// 2. Foncteur (objet avec operator())
struct Worker {
    int id;
    void operator()() const {
        std::println("Worker {} en cours d'exécution", id);
    }
};

// 3. Fonction membre
struct Logger {
    void log(const std::string& msg) const {
        std::println("[LOG] {}", msg);
    }
};

int main() {
    // Fonction libre avec argument
    std::thread t1(greet, "Alice");

    // Lambda (la forme la plus idiomatique en C++ moderne)
    std::thread t2([] {
        std::println("Lambda dans un thread");
    });

    // Foncteur — attention aux parenthèses (voir encadré ci-dessous)
    std::thread t3(Worker{42});

    // Fonction membre : on passe le pointeur de méthode puis l'objet
    Logger logger;
    std::thread t4(&Logger::log, &logger, "Démarrage du service");

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}
```

> ⚠️ **Le piège du "most vexing parse"**  
>  
> Écrire `std::thread t(Worker())` ne crée **pas** un thread exécutant un `Worker` par défaut. Le compilateur interprète cette ligne comme la déclaration d'une *fonction* `t` qui prend en paramètre un pointeur de fonction retournant `Worker` et qui retourne un `std::thread`. C'est le *most vexing parse*, un piège classique du C++.  
>  
> Solutions :  
> ```cpp  
> std::thread t{Worker()};     // Initialisation avec accolades  
> std::thread t(Worker{});     // Construction explicite du foncteur  
> std::thread t((Worker()));   // Parenthèses supplémentaires  
> ```  
> La première forme (accolades) est la plus recommandée en C++ moderne.

---

## Passage d'arguments

### Copie par défaut

`std::thread` **copie** tous les arguments passés au callable. Ce comportement est délibéré : il garantit que le thread possède ses propres données, indépendantes du thread appelant. C'est la stratégie la plus sûre par défaut.

```cpp
#include <thread>
#include <print>
#include <string>

void process(std::string data) {
    std::println("Traitement de : {}", data);
}

int main() {
    std::string message = "Hello";
    std::thread t(process, message);  // 'message' est copié dans le thread
    // 'message' reste utilisable ici, indépendamment du thread
    t.join();
}
```

### Passer une référence avec std::ref

Si vous avez besoin qu'un thread travaille directement sur une variable du thread appelant, vous devez utiliser `std::ref()` (ou `std::cref()` pour une référence constante). Sans cela, même si votre fonction attend une référence, l'argument sera copié.

```cpp
#include <thread>
#include <print>
#include <functional>  // std::ref

void increment(int& value) {
    ++value;
}

int main() {
    int counter = 0;

    // ❌ Ne compile PAS : std::thread tente de copier,
    //    mais int& ne peut pas se lier à un rvalue
    // std::thread t(increment, counter);

    // ✅ Correct : std::ref transmet une vraie référence
    std::thread t(increment, std::ref(counter));
    t.join();

    std::println("counter = {}", counter);  // Affiche 1
}
```

> ⚠️ **Danger** : quand vous utilisez `std::ref`, vous êtes responsable de garantir que la variable référencée **survit** au thread qui l'utilise. Si le thread appelant détruit la variable avant que le thread enfant ait fini, vous obtenez un comportement indéfini (dangling reference). C'est exactement le type de bug que ThreadSanitizer aide à détecter.

### Déplacement d'arguments (move semantics)

Pour les types non-copiables (comme `std::unique_ptr`) ou pour éviter des copies coûteuses, utilisez `std::move()` :

```cpp
#include <thread>
#include <print>
#include <memory>

void consume(std::unique_ptr<int> ptr) {
    std::println("Valeur reçue : {}", *ptr);
}

int main() {
    auto ptr = std::make_unique<int>(42);

    // std::unique_ptr n'est pas copiable, on doit le déplacer
    std::thread t(consume, std::move(ptr));
    // ptr est maintenant nullptr — la propriété a été transférée au thread

    t.join();
}
```

Ce mécanisme s'aligne parfaitement avec la sémantique de propriété exclusive de `std::unique_ptr` (voir chapitre 9). Le thread prend possession de la ressource, et le thread appelant n'y a plus accès.

### Piège courant : conversion implicite et durée de vie

Un piège fréquent survient avec les conversions implicites et les chaînes C-style :

```cpp
#include <thread>
#include <string>

void process(const std::string& data) {
    // Utilise 'data'...
}

void risky() {
    char buffer[] = "Hello";
    // ⚠️ DANGEREUX : le pointeur 'buffer' est copié dans le thread,
    //    mais la conversion en std::string n'a lieu qu'à l'intérieur du thread.
    //    Si 'risky()' retourne avant que la conversion ait lieu,
    //    le thread accède à de la mémoire libérée.
    std::thread t(process, buffer);
    t.detach();  // On n'attend pas le thread → danger !
}

void safe() {
    char buffer[] = "Hello";
    // ✅ SÛR : conversion explicite AVANT la construction du thread
    std::thread t(process, std::string(buffer));
    t.detach();
}
```

**Règle** : convertissez toujours explicitement les arguments *avant* de les passer à `std::thread`, surtout si le thread peut survivre à la portée courante.

---

## Cycle de vie d'un thread

### Les deux états possibles

Un objet `std::thread` est dans l'un de ces deux états :

- **Joinable** : le thread est en cours d'exécution (ou a terminé mais n'a pas encore été rejoint). L'objet `std::thread` est associé à un thread système réel.
- **Non-joinable** : l'objet n'est associé à aucun thread. C'est le cas après un `join()`, un `detach()`, un déplacement, ou pour un `std::thread` construit par défaut.

Vous pouvez vérifier l'état avec `joinable()` :

```cpp
std::thread t;               // Non-joinable (construit par défaut)  
assert(!t.joinable());  

t = std::thread([] { /* ... */ });  
assert(t.joinable());        // Joinable : thread actif  

t.join();  
assert(!t.joinable());       // Non-joinable : rejoint  
```

### join() : attendre la fin du thread

`join()` est l'opération la plus courante. Elle bloque le thread appelant jusqu'à ce que le thread ciblé ait terminé son exécution :

```cpp
#include <thread>
#include <print>
#include <chrono>

int main() {
    std::thread t([] {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::println("Thread terminé");
    });

    std::println("Avant join — le thread tourne en parallèle");
    t.join();  // Bloque jusqu'à la fin du thread
    std::println("Après join — le thread est terminé");
}
```

Sortie :
```
Avant join — le thread tourne en parallèle  
Thread terminé  
Après join — le thread est terminé  
```

Après un `join()`, l'objet `std::thread` passe à l'état non-joinable. Appeler `join()` une seconde fois lance une exception `std::system_error` — vérifiez `joinable()` avant d'appeler `join()` si nécessaire.

### detach() : laisser le thread en autonomie

`detach()` dissocie l'objet `std::thread` du thread système sous-jacent. Le thread continue de s'exécuter indépendamment, et ses ressources seront libérées automatiquement à sa fin par le système d'exploitation. On parle de *daemon thread*.

```cpp
#include <thread>
#include <print>

void background_task() {
    // Ce thread vit sa vie, indépendamment du thread principal
    std::println("Tâche de fond en cours...");
}

int main() {
    std::thread t(background_task);
    t.detach();
    // t n'est plus joinable — on ne peut plus attendre sa fin
    // Le thread peut continuer après la fin de main()...
    // ...mais en pratique, exit() terminera tous les threads.
}
```

> ⚠️ **`detach()` est rarement la bonne solution.** Un thread détaché pose plusieurs problèmes :  
>  
> - Vous ne pouvez plus savoir quand il se termine.  
> - S'il accède à des données du thread principal, vous risquez des accès à de la mémoire libérée.  
> - Si `main()` retourne (appel implicite à `std::exit()`), les threads détachés sont terminés brutalement sans exécuter leurs destructeurs.  
>  
> **Préférez `join()`** dans la grande majorité des cas. Si vous avez besoin de threads de fond durables, utilisez un pool de threads ou `std::jthread` (section 21.7) avec un mécanisme d'arrêt propre.

---

## La règle absolue : joindre ou détacher avant la destruction

C'est la règle la plus critique de `std::thread` : **si un objet `std::thread` joinable est détruit, le programme appelle `std::terminate()`.** Il n'y a pas de comportement par défaut silencieux — c'est un crash volontaire.

```cpp
void dangerous() {
    std::thread t([] {
        // travail...
    });
    // ❌ CRASH : t est détruit ici alors qu'il est encore joinable
    // → std::terminate() est appelé
}
```

Ce choix de design est délibéré. Les alternatives auraient été pires :

- **Join implicite dans le destructeur** : le destructeur pourrait bloquer indéfiniment sans que le programmeur le sache, créant des deadlocks silencieux.
- **Detach implicite** : le thread pourrait continuer à accéder à des ressources détruites, causant des bugs mémoire invisibles.

Le comité C++ a préféré un crash explicite. Si votre programme plante à cause d'un `std::thread` non-joint, c'est un signal clair qu'il y a un bug dans la gestion du cycle de vie.

### Pattern RAII : garantir le join

Pour éviter les oublis (notamment en cas d'exception), encapsulez la gestion du join dans un pattern RAII :

```cpp
#include <thread>

class ThreadGuard {
    std::thread& t_;
public:
    explicit ThreadGuard(std::thread& t) : t_(t) {}

    ~ThreadGuard() {
        if (t_.joinable()) {
            t_.join();
        }
    }

    // Non copiable
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

void safe_function() {
    std::thread t([] {
        // travail qui peut prendre du temps...
    });
    ThreadGuard guard(t);

    // Même si une exception est levée ici,
    // le destructeur de guard appellera t.join()
    do_something_that_might_throw();
}
// guard est détruit → t.join() est appelé automatiquement
```

> 💡 Ce pattern est exactement ce que `std::jthread` (C++20) implémente nativement, avec en prime le support de l'arrêt coopératif. Voir la **section 21.7**.

---

## std::thread est move-only

`std::thread` est **non-copiable mais déplaçable**. Chaque thread système n'a qu'un seul propriétaire à un instant donné. Le transfert de propriété se fait via `std::move()` :

```cpp
#include <thread>
#include <print>

std::thread create_thread() {
    return std::thread([] {
        std::println("Thread créé par une fonction factory");
    });
    // Le return bénéficie du RVO ou du déplacement implicite
}

int main() {
    std::thread t1 = create_thread();   // OK : déplacement

    std::thread t2;                      // Non-joinable
    t2 = std::move(t1);                  // t1 → t2 : transfert de propriété
    // t1 est maintenant non-joinable
    // t2 est joinable

    // ❌ Interdit : copie
    // std::thread t3 = t2;  // Erreur de compilation

    t2.join();
}
```

Cette sémantique move-only est cohérente avec `std::unique_ptr` : un thread est une ressource unique, et son propriétaire est clairement défini. Cela permet de stocker des threads dans des conteneurs :

```cpp
#include <thread>
#include <vector>
#include <print>

int main() {
    std::vector<std::thread> workers;

    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([i] {
            std::println("Worker {} actif", i);
        });
    }

    // Joindre tous les threads
    for (auto& t : workers) {
        t.join();
    }
}
```

> ⚠️ **Attention à l'affectation d'un thread joinable.** Si vous assignez un nouveau thread à un objet `std::thread` déjà joinable, `std::terminate()` est appelé. Vous devez d'abord joindre ou détacher l'ancien thread :  
>  
> ```cpp  
> std::thread t([] { /* ... */ });  
> // t est joinable  
>  
> // ❌ CRASH : t est écrasé alors qu'il est encore joinable  
> // t = std::thread([] { /* ... */ });  
>  
> // ✅ Correct  
> t.join();  
> t = std::thread([] { /* ... */ });  
> t.join();  
> ```

---

## Identification des threads

Chaque thread possède un identifiant unique de type `std::thread::id`. Il est utile pour le débogage et le logging :

```cpp
#include <thread>
#include <print>

int main() {
    std::thread t([] {
        // Obtenir l'ID du thread courant
        auto id = std::this_thread::get_id();
        std::println("Thread enfant, ID : {}", id);
    });

    std::println("Thread principal, ID : {}", std::this_thread::get_id());

    // Obtenir l'ID depuis l'objet thread
    std::println("ID du thread enfant (vu du parent) : {}", t.get_id());

    t.join();
}
```

L'identifiant est comparable (`==`, `!=`, `<`) et hashable, ce qui permet de l'utiliser comme clé dans une `std::unordered_map` ou un `std::set`. En pratique, il correspond souvent au TID (Thread ID) Linux, mais la norme ne le garantit pas.

L'identifiant natif du système est accessible via `native_handle()`, ce qui permet d'utiliser les API pthreads directement si nécessaire :

```cpp
#include <thread>
#include <pthread.h>

void set_thread_name(std::thread& t, const char* name) {
    // API Linux pour nommer un thread (visible dans htop, gdb, perf)
    pthread_setname_np(t.native_handle(), name);
}
```

> 💡 **Nommer ses threads** est une excellente pratique. Un thread nommé "http-worker-3" dans `gdb` ou `htop` est infiniment plus lisible que "Thread 0x7f3a…". Prenez cette habitude dès le début.

---

## Utilitaires de std::this_thread

Le namespace `std::this_thread` fournit des fonctions utilitaires pour le thread courant :

### sleep_for et sleep_until

```cpp
#include <thread>
#include <chrono>

void wait_example() {
    using namespace std::chrono_literals;

    // Dormir pendant une durée relative
    std::this_thread::sleep_for(100ms);
    std::this_thread::sleep_for(2s);

    // Dormir jusqu'à un instant absolu
    auto wake_time = std::chrono::steady_clock::now() + 5s;
    std::this_thread::sleep_until(wake_time);
}
```

`sleep_for` est une attente *au minimum* de la durée spécifiée — le thread peut dormir plus longtemps si le scheduler est occupé. N'utilisez pas `sleep_for` comme mécanisme de synchronisation entre threads ; utilisez des primitives dédiées (mutex, condition variables, atomiques).

### yield

```cpp
void spin_wait_example() {
    // Indique au scheduler qu'on veut céder notre quantum de temps
    std::this_thread::yield();
}
```

`yield()` est un *hint* au système d'exploitation : le thread courant n'a rien d'utile à faire immédiatement et suggère au scheduler de donner la main à un autre thread. C'est utilisé dans les boucles d'attente active (spin loops), mais dans la plupart des cas, une variable de condition ou un mutex sont préférables.

---

## Nombre de threads matériels

`std::thread::hardware_concurrency()` retourne une estimation du nombre de threads matériels disponibles (cœurs physiques × threads par cœur en cas d'hyperthreading) :

```cpp
#include <thread>
#include <print>

int main() {
    unsigned int n = std::thread::hardware_concurrency();
    std::println("Threads matériels disponibles : {}", n);
    // Typique : 8 sur un laptop, 16-128 sur un serveur
}
```

Si la valeur ne peut pas être déterminée, la fonction retourne `0`. Utilisez cette information pour calibrer la taille de vos pools de threads :

```cpp
unsigned int num_workers = std::thread::hardware_concurrency();  
if (num_workers == 0) num_workers = 4;  // Valeur par défaut raisonnable  
```

---

## Gestion des exceptions dans les threads

Une exception non-attrapée dans un thread provoque un appel à `std::terminate()` — le programme entier est terminé. Les exceptions ne se propagent **pas** automatiquement du thread enfant vers le thread parent.

```cpp
#include <thread>
#include <stdexcept>
#include <print>

int main() {
    std::thread t([] {
        throw std::runtime_error("Erreur dans le thread !");
        // ❌ Si l'exception n'est pas attrapée ici,
        //    std::terminate() est appelé.
    });
    t.join();
    // On n'arrive jamais ici si l'exception n'est pas gérée dans le thread
}
```

### Capturer et transmettre les exceptions

Pour propager une exception vers le thread appelant, utilisez `std::exception_ptr` :

```cpp
#include <thread>
#include <stdexcept>
#include <exception>
#include <print>

int main() {
    std::exception_ptr eptr = nullptr;

    std::thread t([&eptr] {
        try {
            // Travail qui peut échouer
            throw std::runtime_error("Échec du traitement");
        } catch (...) {
            eptr = std::current_exception();  // Capture l'exception
        }
    });

    t.join();

    // Relancer l'exception dans le thread principal
    if (eptr) {
        try {
            std::rethrow_exception(eptr);
        } catch (const std::exception& e) {
            std::println("Exception du thread : {}", e.what());
        }
    }
}
```

Ce mécanisme fonctionne, mais il est verbeux. En pratique, `std::async` et `std::future` (section 21.5) automatisent entièrement cette propagation d'exceptions, ce qui en fait souvent une meilleure alternative.

---

## Résumé des bonnes pratiques

1. **Préférez les lambdas** pour définir le travail d'un thread — elles sont concises, capturent le contexte de façon explicite, et évitent le *most vexing parse*.

2. **Joignez toujours vos threads.** Utilisez `join()` systématiquement. Évitez `detach()` sauf cas très spécifiques (et bien documentés).

3. **Convertissez les arguments avant la construction** du thread, surtout les `const char*` en `std::string`.

4. **Utilisez `std::move`** pour transférer la propriété de ressources non-copiables vers un thread.

5. **Utilisez `std::ref` avec précaution** — la variable référencée doit survivre au thread.

6. **Attrapez les exceptions dans le thread** qui les génère, ou utilisez `std::future` pour les propager proprement.

7. **Nommez vos threads** via `pthread_setname_np` pour faciliter le débogage.

8. **Envisagez `std::jthread`** (C++20) qui résout automatiquement le problème du join manquant et ajoute l'arrêt coopératif (section 21.7).

---

> **À suivre** : maintenant que vous savez créer et gérer des threads, la section suivante aborde le problème central de la programmation concurrente : comment **protéger les données partagées** avec les primitives de synchronisation (`std::mutex`, `std::lock_guard`, `std::scoped_lock`).

> 📎 *Voir **section 21.2** — Synchronisation.*

⏭️ [Synchronisation](/21-threads-concurrence/02-synchronisation.md)
