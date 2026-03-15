🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 23.3 — Shared memory et mmap

## Chapitre 23 : Processus et IPC

---

## Introduction

Les pipes (section 23.2) sont élégants mais ils ont une limitation fondamentale : les données sont **copiées**. L'écrivain copie ses données dans le buffer noyau (`write`), puis le lecteur copie les données du buffer noyau dans son propre espace d'adressage (`read`). Pour des messages courts, ce double copie est négligeable. Pour des volumes importants — images, matrices, bases de données en mémoire, buffers audio/vidéo — il devient un goulot d'étranglement.

La **mémoire partagée** élimine ce problème en permettant à deux processus de mapper la **même zone de mémoire physique** dans leurs espaces d'adressage respectifs. Les données ne sont jamais copiées — les deux processus lisent et écrivent directement dans les mêmes pages mémoire. C'est le mécanisme IPC le plus rapide qui existe, et de loin.

```
Pipe (section 23.2) :

  Processus A              Noyau               Processus B
  ┌──────────┐         ┌──────────┐          ┌──────────┐
  │ données  │──copy──►│  buffer  │──copy───►│ données  │
  └──────────┘  write  └──────────┘  read    └──────────┘
                   2 copies, passage par le noyau

Mémoire partagée :

  Processus A                                 Processus B
  ┌──────────┐                               ┌──────────┐
  │ pointeur │────────►┌──────────┐◄─────────│ pointeur │
  └──────────┘         │  mémoire │          └──────────┘
                       │ physique │
                       │ PARTAGÉE │
                       └──────────┘
                   0 copies, accès direct
```

Le prix de cette performance est la **complexité de synchronisation**. Avec un pipe, le noyau gère tout — l'ordre, le blocage, la cohérence. Avec la mémoire partagée, c'est votre code qui doit garantir que deux processus ne corrompent pas les données en écrivant simultanément. Les outils sont les mêmes que pour les threads (chapitre 21) — mutexes, sémaphores, atomiques — mais configurés pour fonctionner **inter-processus**.

---

## `mmap()` — L'appel système fondamental

### Signature

```cpp
#include <sys/mman.h>

void* mmap(void* addr, size_t length, int prot, int flags,
           int fd, off_t offset);

int munmap(void* addr, size_t length);
```

`mmap` est l'un des appels système les plus polyvalents de Linux. Il crée un **mapping** — une association entre une plage d'adresses virtuelles du processus et une source de données (fichier, mémoire anonyme, ou périphérique). Selon les flags, ce mapping peut être privé ou partagé entre processus.

### Les paramètres

| Paramètre | Rôle |
|-----------|------|
| `addr` | Adresse souhaitée (ou `nullptr` pour laisser le noyau choisir — recommandé) |
| `length` | Taille du mapping en octets |
| `prot` | Protection mémoire : `PROT_READ`, `PROT_WRITE`, `PROT_EXEC`, `PROT_NONE` (combinables par OR) |
| `flags` | Type de mapping (voir ci-dessous) |
| `fd` | File descriptor du fichier à mapper (ou `-1` pour la mémoire anonyme) |
| `offset` | Offset dans le fichier (doit être un multiple de la taille de page) |

### Les flags importants

| Flag | Signification |
|------|---------------|
| `MAP_SHARED` | Les modifications sont visibles par tous les processus qui partagent le mapping, et sont propagées au fichier sous-jacent (si applicable). **C'est le flag pour l'IPC.** |
| `MAP_PRIVATE` | Le processus obtient une copie privée (copy-on-write). Les modifications ne sont pas visibles par les autres processus ni propagées au fichier. |
| `MAP_ANONYMOUS` | Pas de fichier sous-jacent — mémoire initialisée à zéro. `fd` doit être `-1`. |
| `MAP_FIXED` | Utiliser exactement l'adresse `addr` (dangereux — peut écraser un mapping existant). |

Les deux combinaisons essentielles pour l'IPC :

- `MAP_SHARED | MAP_ANONYMOUS` — Mémoire partagée anonyme entre parent et enfants (via `fork`).
- `MAP_SHARED` avec un fd — Fichier mappé en mémoire, partageable entre processus indépendants.

---

## Mémoire partagée anonyme (parent ↔ enfant)

Le cas le plus simple : partager de la mémoire entre un parent et ses enfants via `fork`. Comme `fork` préserve les mappings, une zone `MAP_SHARED | MAP_ANONYMOUS` créée avant le fork est directement accessible par les deux processus.

```cpp
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <print>
#include <cstring>

int main() {
    // Créer une zone de mémoire partagée anonyme
    size_t size = 4096;
    void* shared = mmap(nullptr, size,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS,
                        -1, 0);

    if (shared == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "mmap()");
    }

    // Initialiser
    std::memset(shared, 0, size);

    pid_t pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork()");
    }

    if (pid == 0) {
        // Enfant : écrire dans la mémoire partagée
        const char* msg = "Hello depuis l'enfant!";
        std::memcpy(shared, msg, std::strlen(msg) + 1);
        std::println("[Enfant] Écrit: {}", msg);
        _exit(0);
    }

    // Parent : attendre l'enfant puis lire
    waitpid(pid, nullptr, 0);

    auto* data = static_cast<const char*>(shared);
    std::println("[Parent] Lu: {}", data);

    // Libérer
    munmap(shared, size);
}
```

```
[Enfant] Écrit: Hello depuis l'enfant!
[Parent] Lu: Hello depuis l'enfant!
```

C'est la forme la plus simple de mémoire partagée — pas de fichier, pas de nom, pas de configuration. Mais elle ne fonctionne qu'entre processus issus du même `fork`, car les processus indépendants n'ont aucun moyen de retrouver ce mapping anonyme.

---

## Mémoire partagée POSIX (processus indépendants)

### Le mécanisme : `shm_open` + `mmap`

Pour partager de la mémoire entre processus **sans lien de parenté**, il faut un mécanisme de nommage. POSIX shared memory fournit exactement cela : un espace de noms (similaire au filesystem) où les segments de mémoire partagée sont identifiés par un nom.

```cpp
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

// Créer ou ouvrir un segment de mémoire partagée
int shm_open(const char* name, int oflag, mode_t mode);

// Supprimer un segment (le nom disparaît, la mémoire persiste
// jusqu'à ce que tous les processus l'aient unmappé)
int shm_unlink(const char* name);
```

`shm_open` retourne un file descriptor, comme `open` pour un fichier. Ce fd est ensuite passé à `mmap` pour obtenir un pointeur vers la mémoire partagée. Le nom doit commencer par `/` (convention POSIX) et ne doit pas contenir d'autre `/`.

Sur Linux, les segments de mémoire partagée POSIX vivent dans le pseudo-filesystem `/dev/shm` :

```bash
$ ls -la /dev/shm/
# Les segments de mémoire partagée apparaissent ici comme des fichiers
```

### Workflow complet

```
1. shm_open("/mon_segment", O_CREAT | O_RDWR, 0666)
   → Crée le segment et retourne un fd

2. ftruncate(fd, taille)
   → Dimensionne le segment (obligatoire après création)

3. mmap(nullptr, taille, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)
   → Mappe le segment dans l'espace d'adressage
   → Retourne un pointeur utilisable

4. close(fd)
   → Le fd n'est plus nécessaire (le mapping reste)

5. ... utiliser le pointeur ...

6. munmap(ptr, taille)
   → Démappe la mémoire

7. shm_unlink("/mon_segment")
   → Supprime le nom (un seul processus le fait, typiquement le dernier)
```

### Processus écrivain

```cpp
// shm_writer.cpp
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <cstring>
#include <atomic>

struct SharedData {
    int counter;
    char message[256];
    std::atomic<bool> ready;
};

int main() {
    const char* name = "/demo_shm";
    const size_t size = sizeof(SharedData);

    // Créer le segment
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category(), "shm_open()");
    }

    // Dimensionner
    if (ftruncate(fd, static_cast<off_t>(size)) == -1) {
        throw std::system_error(errno, std::system_category(), "ftruncate()");
    }

    // Mapper
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "mmap()");
    }
    close(fd);  // Le fd n'est plus nécessaire

    // Écrire des données
    auto* data = static_cast<SharedData*>(ptr);
    data->counter = 42;
    std::strcpy(data->message, "Hello depuis la mémoire partagée!");
    data->ready.store(true, std::memory_order_release);

    std::println("Écrivain: données écrites, en attente du lecteur...");
    std::println("  counter = {}", data->counter);
    std::println("  message = {}", data->message);

    // Attendre que l'utilisateur appuie sur Entrée
    // (pour laisser le temps au lecteur de lire)
    std::println("Appuyez sur Entrée pour nettoyer...");
    std::string line;
    std::getline(std::cin, line);

    // Nettoyer
    munmap(ptr, size);
    shm_unlink(name);
    std::println("Segment supprimé");
}
```

### Processus lecteur

```cpp
// shm_reader.cpp
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <atomic>

struct SharedData {
    int counter;
    char message[256];
    std::atomic<bool> ready;
};

int main() {
    const char* name = "/demo_shm";
    const size_t size = sizeof(SharedData);

    // Ouvrir le segment existant (pas O_CREAT)
    int fd = shm_open(name, O_RDONLY, 0);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category(),
                                "shm_open() — l'écrivain doit être lancé d'abord");
    }

    // Mapper en lecture seule
    void* ptr = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "mmap()");
    }
    close(fd);

    // Lire les données
    auto* data = static_cast<const SharedData*>(ptr);

    if (data->ready.load(std::memory_order_acquire)) {
        std::println("Lecteur:");
        std::println("  counter = {}", data->counter);
        std::println("  message = {}", data->message);
    } else {
        std::println("Données pas encore prêtes");
    }

    // Nettoyer
    munmap(const_cast<void*>(ptr), size);
    // Ne pas shm_unlink ici — c'est l'écrivain qui gère le cycle de vie
}
```

```bash
# Terminal 1
$ g++ -std=c++23 -o writer shm_writer.cpp -lrt
$ ./writer
Écrivain: données écrites, en attente du lecteur...
  counter = 42
  message = Hello depuis la mémoire partagée!
Appuyez sur Entrée pour nettoyer...

# Terminal 2
$ g++ -std=c++23 -o reader shm_reader.cpp -lrt
$ ./reader
Lecteur:
  counter = 42
  message = Hello depuis la mémoire partagée!
```

> 💡 Le flag `-lrt` est nécessaire pour lier la librairie POSIX realtime, qui contient `shm_open` et `shm_unlink`. Sur certaines versions récentes de glibc, cette librairie est incluse dans la libc principale et le flag n'est plus requis.

---

## Fichiers mappés en mémoire

### Le concept

`mmap` ne se limite pas à la mémoire partagée pure. Il peut mapper un **fichier ordinaire** directement dans l'espace d'adressage. Le contenu du fichier devient accessible via un simple pointeur — pas de `read()` ni `write()`, pas de buffer intermédiaire. Le noyau gère la pagination : les pages du fichier sont chargées à la demande (lazy loading) et écrites sur le disque quand la pression mémoire l'exige.

```cpp
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <print>
#include <string_view>

void read_file_with_mmap(const char* path) {
    // Ouvrir le fichier
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category(), "open()");
    }

    // Obtenir la taille
    struct stat st;
    fstat(fd, &st);
    size_t size = static_cast<size_t>(st.st_size);

    if (size == 0) {
        close(fd);
        std::println("Fichier vide");
        return;
    }

    // Mapper le fichier entier en lecture
    void* ptr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);  // Le fd n'est plus nécessaire après mmap

    if (ptr == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(), "mmap()");
    }

    // Le fichier entier est accessible via un pointeur
    std::string_view content(static_cast<const char*>(ptr), size);
    std::println("Fichier: {} octets", size);
    std::println("Premières lignes:");

    size_t pos = 0;
    int lines = 0;
    while (pos < content.size() && lines < 5) {
        size_t end = content.find('\n', pos);
        if (end == std::string_view::npos) end = content.size();
        std::println("  {}", content.substr(pos, end - pos));
        pos = end + 1;
        lines++;
    }

    munmap(ptr, size);
}
```

### Fichier mappé partagé entre processus

Avec `MAP_SHARED`, les modifications sont visibles par tous les processus qui mappent le même fichier, **et** sont propagées au fichier sur disque :

```cpp
// Deux processus indépendants mappent le même fichier
// Processus A :
int fd = open("/tmp/shared_data.bin", O_RDWR | O_CREAT, 0666);  
ftruncate(fd, 4096);  
auto* data = static_cast<int*>(  
    mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
close(fd);

data[0] = 42;  // Visible immédiatement par le processus B
                // Et propagé au fichier sur disque (éventuellement)

// Processus B (lancé indépendamment) :
int fd = open("/tmp/shared_data.bin", O_RDWR);  
auto* data = static_cast<int*>(  
    mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
close(fd);

std::println("data[0] = {}", data[0]);  // 42
```

C'est le même effet que la mémoire partagée POSIX (`shm_open`), mais avec persistance sur disque. La différence :

| Aspect | `shm_open` + `mmap` | `open` fichier + `mmap` |
|--------|---------------------|-------------------------|
| Stockage | RAM (`/dev/shm`, tmpfs) | Disque (filesystem) |
| Persistance | Non (disparaît au reboot) | Oui (fichier sur disque) |
| Performance | Plus rapide (RAM pure) | Dépend du cache page |
| Usage | IPC volatile | IPC + persistance, fichiers volumineux |

### `MAP_PRIVATE` : copy-on-write sur fichier

Avec `MAP_PRIVATE`, le processus obtient une copie privée des pages du fichier. Les modifications ne sont **pas** propagées au fichier ni visibles par les autres processus :

```cpp
// Lecture + modification locale (le fichier original n'est pas touché)
auto* data = static_cast<char*>(
    mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));

data[0] = 'X';  // Modification locale — le fichier n'est pas modifié
```

C'est le mécanisme utilisé par le noyau pour charger les exécutables (le code est mappé `MAP_PRIVATE` depuis le fichier ELF) et par `fork()` pour le copy-on-write.

### `msync()` : forcer la synchronisation avec le disque

Pour les mappings `MAP_SHARED` sur fichier, les modifications sont propagées au disque **éventuellement** (quand le noyau en décide). Pour forcer la synchronisation immédiate :

```cpp
#include <sys/mman.h>

// Synchroniser toute la zone mappée avec le fichier sur disque
msync(ptr, size, MS_SYNC);    // Bloquant — attend la fin de l'écriture  
msync(ptr, size, MS_ASYNC);   // Non-bloquant — planifie l'écriture  
```

`MS_SYNC` est l'équivalent de `fsync()` pour les mappings mémoire. Utilisez-le avant de signaler à un autre processus que les données sont prêtes, pour garantir qu'il lira la version à jour depuis le fichier.

---

## Synchronisation inter-processus

### Le problème

La mémoire partagée est un espace brut — le noyau ne fournit **aucune** synchronisation. Si deux processus modifient la même donnée simultanément, le résultat est une race condition exactement comme entre deux threads (chapitre 21). Il faut des primitives de synchronisation explicites.

### Différence avec la synchronisation inter-threads

Les mutexes et condition variables standard (`std::mutex`, `std::condition_variable`) ne fonctionnent **pas** entre processus — ils utilisent des adresses mémoire dans le heap du processus, invisibles aux autres processus. Pour l'IPC, il faut des primitives placées **dans la mémoire partagée elle-même** et configurées avec l'attribut `PTHREAD_PROCESS_SHARED`.

### Mutex inter-processus avec `pthread_mutex`

```cpp
#include <pthread.h>
#include <sys/mman.h>

struct SharedState {
    pthread_mutex_t mutex;
    int counter;
    char data[256];
};

// Initialisation (un seul processus, typiquement le créateur du segment)
void init_shared_state(SharedState* state) {
    // Configurer le mutex pour l'usage inter-processus
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    // Robustesse : le mutex se déverrouille si le propriétaire meurt
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);

    pthread_mutex_init(&state->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    state->counter = 0;
    std::memset(state->data, 0, sizeof(state->data));
}

// Utilisation (n'importe quel processus ayant mappé le segment)
void increment(SharedState* state) {
    int err = pthread_mutex_lock(&state->mutex);

    if (err == EOWNERDEAD) {
        // Le processus qui détenait le mutex est mort
        // Le mutex est dans un état incohérent — récupérer
        pthread_mutex_consistent(&state->mutex);
        // Les données sont potentiellement corrompues — réinitialiser si possible
    }

    state->counter++;
    std::snprintf(state->data, sizeof(state->data),
                  "Counter: %d (PID %d)", state->counter, getpid());

    pthread_mutex_unlock(&state->mutex);
}
```

**`PTHREAD_PROCESS_SHARED`** — C'est le flag critique. Sans lui, le mutex ne fonctionne qu'entre threads du même processus.

**`PTHREAD_MUTEX_ROBUST`** — Un ajout important pour la production. Si le processus qui détient le mutex est tué (crash, `kill -9`), un mutex normal reste verrouillé indéfiniment — tous les autres processus sont bloqués. Un mutex robust signale cette situation avec le code de retour `EOWNERDEAD`, permettant la récupération.

### Sémaphores POSIX nommés

Les sémaphores POSIX nommés sont une alternative plus simple aux mutex pour la synchronisation inter-processus. Ils n'ont pas besoin d'être placés dans la mémoire partagée — ils sont identifiés par un nom, comme les segments `shm_open` :

```cpp
#include <semaphore.h>
#include <fcntl.h>

// Créer ou ouvrir un sémaphore nommé
sem_t* sem = sem_open("/my_semaphore", O_CREAT, 0666, 1);
//                                     ^^^^^^          ^
//                                     créer           valeur initiale (1 = mutex)

if (sem == SEM_FAILED) {
    throw std::system_error(errno, std::system_category(), "sem_open()");
}

// Section critique
sem_wait(sem);       // Décrémenter (bloque si 0)
// ... accès à la ressource partagée ...
sem_post(sem);       // Incrémenter (débloque un waiter)

// Nettoyage
sem_close(sem);      // Fermer la référence locale  
sem_unlink("/my_semaphore");  // Supprimer le nom (un seul processus)  
```

Les sémaphores nommés sont plus simples d'usage que les mutex inter-processus : pas besoin de `PTHREAD_PROCESS_SHARED`, pas besoin de les placer dans le segment partagé. Mais ils sont moins flexibles (pas de condition variables associées, pas de trylock avec timeout facile).

### Atomiques en mémoire partagée

Pour les compteurs simples ou les flags, les opérations atomiques fonctionnent directement sur la mémoire partagée — aucune synchronisation supplémentaire n'est nécessaire, tant que les types sont lock-free :

```cpp
#include <atomic>

struct SharedCounters {
    std::atomic<int64_t> requests;
    std::atomic<int64_t> errors;
    std::atomic<bool> shutdown_requested;
};

// Vérifier que les atomiques sont lock-free
// (sinon ils utilisent un mutex interne qui ne fonctionne pas en IPC)
static_assert(std::atomic<int64_t>::is_always_lock_free,
              "int64_t atomics must be lock-free for shared memory IPC");
static_assert(std::atomic<bool>::is_always_lock_free,
              "bool atomics must be lock-free for shared memory IPC");
```

Si `is_always_lock_free` est `true`, les opérations atomiques sont implémentées par des instructions CPU (comme `lock cmpxchg` sur x86) et fonctionnent correctement entre processus partageant la même mémoire physique. Si ce n'est pas `true`, les atomiques utilisent un mutex interne dans le heap du processus — inutilisable en IPC.

Sur x86_64, les types jusqu'à 8 octets (`int64_t`, `double` via `std::atomic`) sont toujours lock-free.

---

## Wrapper RAII : `SharedMemory`

```cpp
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <system_error>

class SharedMemory {  
public:  
    // Créer un nouveau segment
    static SharedMemory create(const std::string& name, size_t size) {
        int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
        if (fd == -1) {
            throw std::system_error(errno, std::system_category(), "shm_open(create)");
        }

        if (ftruncate(fd, static_cast<off_t>(size)) == -1) {
            close(fd);
            shm_unlink(name.c_str());
            throw std::system_error(errno, std::system_category(), "ftruncate()");
        }

        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);

        if (ptr == MAP_FAILED) {
            shm_unlink(name.c_str());
            throw std::system_error(errno, std::system_category(), "mmap()");
        }

        std::memset(ptr, 0, size);
        return SharedMemory(name, ptr, size, true);
    }

    // Ouvrir un segment existant
    static SharedMemory open(const std::string& name, size_t size, bool read_only = false) {
        int oflags = read_only ? O_RDONLY : O_RDWR;
        int fd = shm_open(name.c_str(), oflags, 0);
        if (fd == -1) {
            throw std::system_error(errno, std::system_category(), "shm_open(open)");
        }

        int prot = PROT_READ | (read_only ? 0 : PROT_WRITE);
        void* ptr = mmap(nullptr, size, prot, MAP_SHARED, fd, 0);
        close(fd);

        if (ptr == MAP_FAILED) {
            throw std::system_error(errno, std::system_category(), "mmap()");
        }

        return SharedMemory(name, ptr, size, false);
    }

    ~SharedMemory() {
        if (ptr_ && ptr_ != MAP_FAILED) {
            munmap(ptr_, size_);
        }
        if (owner_) {
            shm_unlink(name_.c_str());
        }
    }

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    SharedMemory(SharedMemory&& other) noexcept
        : name_{std::move(other.name_)},
          ptr_{other.ptr_}, size_{other.size_}, owner_{other.owner_} {
        other.ptr_ = nullptr;
        other.owner_ = false;
    }

    SharedMemory& operator=(SharedMemory&& other) noexcept {
        if (this != &other) {
            if (ptr_ && ptr_ != MAP_FAILED) munmap(ptr_, size_);
            if (owner_) shm_unlink(name_.c_str());

            name_ = std::move(other.name_);
            ptr_ = other.ptr_;
            size_ = other.size_;
            owner_ = other.owner_;
            other.ptr_ = nullptr;
            other.owner_ = false;
        }
        return *this;
    }

    // Accès typé à la mémoire partagée
    template<typename T>
    T* as() noexcept { return static_cast<T*>(ptr_); }

    template<typename T>
    const T* as() const noexcept { return static_cast<const T*>(ptr_); }

    void* data() noexcept { return ptr_; }
    size_t size() const noexcept { return size_; }

private:
    SharedMemory(std::string name, void* ptr, size_t size, bool owner)
        : name_{std::move(name)}, ptr_{ptr}, size_{size}, owner_{owner} {}

    std::string name_;
    void* ptr_ = nullptr;
    size_t size_ = 0;
    bool owner_ = false;  // Le propriétaire fait shm_unlink à la destruction
};
```

### Utilisation

```cpp
// Processus créateur (serveur)
struct Metrics {
    std::atomic<int64_t> request_count;
    std::atomic<int64_t> error_count;
    std::atomic<double> avg_latency_ms;
};

auto shm = SharedMemory::create("/app_metrics", sizeof(Metrics));  
auto* metrics = shm.as<Metrics>();  

// Initialiser
new (metrics) Metrics{};  // Placement new pour initialiser les atomiques

// Utiliser
metrics->request_count.fetch_add(1, std::memory_order_relaxed);  
metrics->avg_latency_ms.store(12.5, std::memory_order_relaxed);  
```

```cpp
// Processus lecteur (monitoring, lancé indépendamment)
auto shm = SharedMemory::open("/app_metrics", sizeof(Metrics), true);  
auto* metrics = shm.as<const Metrics>();  

std::println("Requests: {}", metrics->request_count.load(std::memory_order_relaxed));  
std::println("Errors:   {}", metrics->error_count.load(std::memory_order_relaxed));  
std::println("Latency:  {:.1f}ms", metrics->avg_latency_ms.load(std::memory_order_relaxed));  
```

Ce pattern — un serveur qui écrit des métriques dans de la mémoire partagée, un agent de monitoring qui les lit — est utilisé en production par des systèmes comme Prometheus `node_exporter` et les exporters de métriques applicatives.

---

## `madvise()` : conseils de performance

`madvise` permet de donner des indications au noyau sur la façon dont vous allez utiliser une zone mappée, lui permettant d'optimiser la pagination :

```cpp
#include <sys/mman.h>

// Lecture séquentielle — précharger les pages suivantes
madvise(ptr, size, MADV_SEQUENTIAL);

// Accès aléatoire — ne pas précharger
madvise(ptr, size, MADV_RANDOM);

// Ces données vont être nécessaires bientôt — précharger
madvise(ptr, size, MADV_WILLNEED);

// Ces données ne sont plus nécessaires — le noyau peut libérer les pages
madvise(ptr, size, MADV_DONTNEED);

// Gros pages (Huge Pages) — réduire l'overhead de la table des pages
madvise(ptr, size, MADV_HUGEPAGE);
```

`MADV_SEQUENTIAL` est particulièrement utile pour le traitement de fichiers volumineux mappés en mémoire — le noyau précharge agressivement les pages suivantes, améliorant le throughput de lecture.

`MADV_HUGEPAGE` demande au noyau d'utiliser des pages de 2 MiB (au lieu de 4 KiB) pour ce mapping. C'est un gain de performance significatif pour les gros segments de mémoire partagée, car la table des pages (TLB) est mieux utilisée.

---

## Cas d'usage en production

### Base de données en mémoire

Redis, SQLite (WAL mode), LMDB — ces bases de données utilisent `mmap` pour mapper leurs fichiers de données directement en mémoire. L'avantage est double : accès par pointeur (pas de copies), et gestion de la mémoire déléguée au noyau (pagination automatique, pas de buffer pool à gérer).

### Communication inter-services ultra-rapide

Pour des services sur la même machine nécessitant une latence minimale (microsecondes), la mémoire partagée avec des atomiques ou un ring buffer lock-free est imbattable — des ordres de grandeur plus rapide que gRPC ou même les sockets Unix.

### Métriques et monitoring

Un processus applicatif écrit ses métriques dans un segment partagé. Un agent de monitoring les lit périodiquement et les envoie à Prometheus/Grafana. Aucun overhead réseau, aucun impact sur les performances de l'application.

### Traitement multimédia

Les pipelines audio/vidéo (GStreamer, PipeWire) partagent des buffers de frames entre processus producteurs et consommateurs via la mémoire partagée — le seul mécanisme assez rapide pour du traitement temps réel.

---

## Pièges et bonnes pratiques

### Toujours vérifier `MAP_FAILED`

`mmap` retourne `MAP_FAILED` (qui vaut `(void*)-1`, pas `nullptr`) en cas d'erreur. Tester avec `== nullptr` est un bug silencieux.

### Ne pas stocker de pointeurs dans la mémoire partagée

Un pointeur dans le processus A est une adresse virtuelle — elle ne signifie rien dans le processus B, car les espaces d'adressage sont différents et le mapping peut être à des adresses différentes. Utilisez des **offsets** par rapport au début du segment à la place :

```cpp
// MAUVAIS — le pointeur sera invalide dans l'autre processus
struct BadShared {
    char* name;  // Adresse dans l'espace d'adressage de l'écrivain
};

// BON — offset depuis le début du segment
struct GoodShared {
    size_t name_offset;  // Offset dans le segment partagé
    size_t name_length;
};
```

### Aligner les données

Les types atomiques et les mutex nécessitent un alignement correct. Utilisez `alignas` pour garantir l'alignement dans vos structures partagées :

```cpp
struct alignas(64) SharedState {  // Aligné sur une cache line
    alignas(64) std::atomic<int64_t> counter;
    // Le padding implicite évite le false sharing
    alignas(64) std::atomic<int64_t> other_counter;
};
```

### Nettoyage des segments orphelins

Les segments `shm_open` persistent dans `/dev/shm` jusqu'à `shm_unlink`. Un crash sans nettoyage laisse un segment orphelin. Vérifiez `/dev/shm` en développement et automatisez le nettoyage au démarrage de vos services.

---

## Résumé

La mémoire partagée est le mécanisme IPC le plus rapide de Linux — zéro copie, accès direct par pointeur :

- **`mmap` avec `MAP_SHARED | MAP_ANONYMOUS`** — Mémoire partagée entre parent et enfants. Le plus simple, mais limité aux processus issus du même fork.
- **`shm_open` + `mmap`** — Mémoire partagée nommée entre processus indépendants. Vit dans `/dev/shm` (tmpfs, RAM pure). Le mécanisme standard pour l'IPC haute performance.
- **Fichier mappé `MAP_SHARED`** — Mémoire partagée avec persistance sur disque. Utilisé par les bases de données et le traitement de fichiers volumineux.
- **La synchronisation est votre responsabilité** — `pthread_mutex` avec `PTHREAD_PROCESS_SHARED` et `PTHREAD_MUTEX_ROBUST`, sémaphores nommés, ou atomiques lock-free.
- **Jamais de pointeurs en mémoire partagée** — Utilisez des offsets.
- **`madvise`** pour optimiser les patterns d'accès (séquentiel, aléatoire, huge pages).

---

> **Prochaine étape** → Section 23.4 : Message queues POSIX — un intermédiaire structuré entre la simplicité des pipes et la puissance de la mémoire partagée.

⏭️ [Message queues POSIX](/23-processus-ipc/04-message-queues.md)
