🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 43.1 — C++ et C : `extern "C"` et ABI Compatibility

## Module 15 : Interopérabilité · Niveau Expert

---

## Introduction

L'interopérabilité entre C++ et C n'est pas un cas particulier : c'est **le socle de toute interopérabilité en C++**. Que vous exposiez une bibliothèque C++ à Python via pybind11, que vous construisiez un bridge vers Rust avec `cxx`, ou que vous compiliez pour WebAssembly avec Emscripten, le mécanisme sous-jacent est toujours le même — une frontière définie par l'ABI C.

Comprendre cette frontière en profondeur — ce qu'elle garantit, ce qu'elle interdit, et comment la franchir proprement — est un prérequis pour tout le reste de ce chapitre.

---

## 43.1.1 — Le problème : le *name mangling* C++

### Pourquoi le C++ modifie les noms de symboles

Le C++ supporte la surcharge de fonctions, les namespaces, les templates et les méthodes de classes. Plusieurs fonctions peuvent donc porter le même nom dans le code source tout en ayant des signatures différentes. Pour que l'éditeur de liens (*linker*) puisse les distinguer, le compilateur **encode** (*mangle*) chaque nom de symbole en y incorporant des informations sur la signature complète.

Considérons ces trois fonctions :

```cpp
// Trois fonctions nommées "process" dans le code source
namespace network {
    void process(int id);
    void process(int id, double weight);
}

void process(const std::string& data);
```

Après compilation avec GCC, les symboles générés ressemblent à ceci (vérifiable avec `nm` ou `objdump`) :

```
_ZN7network7processEi          # network::process(int)
_ZN7network7processEid         # network::process(int, double)
_Z7processRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
                                # process(const std::string&)
```

### Le mangling n'est pas standardisé

Point critique : **le schéma de mangling n'est pas défini par le standard C++**. Il relève de l'ABI du compilateur. GCC et Clang suivent l'Itanium C++ ABI sur Linux (et produisent donc les mêmes symboles), mais MSVC utilise un schéma entièrement différent. Cela signifie qu'un fichier objet compilé avec MSVC ne peut pas être lié directement avec un fichier objet compilé par GCC, même pour du code C++ identique.

En pratique sur Ubuntu, GCC et Clang sont interopérables au niveau objet grâce à l'Itanium ABI partagée. Mais dès qu'on franchit la frontière vers un autre langage — C, Python, Rust, ou tout autre — le mangling C++ devient un obstacle infranchissable : aucun autre langage ne sait le reproduire de manière fiable.

### Inspecter le mangling avec les outils Linux

```bash
# Compiler un fichier objet
g++ -c -o process.o process.cpp

# Voir les symboles manglés
nm process.o

# Démangler les symboles pour les rendre lisibles
nm process.o | c++filt

# Ou démangler un symbole individuel
c++filt _ZN7network7processEi
# Résultat : network::process(int)
```

L'outil `c++filt` est indispensable pour le débogage de problèmes de linkage en interopérabilité. Quand un `undefined reference` apparaît avec un symbole manglé, `c++filt` permet immédiatement d'identifier la fonction concernée.

---

## 43.1.2 — `extern "C"` : désactiver le mangling

### Le mécanisme

La déclaration `extern "C"` indique au compilateur C++ de générer le symbole **sans mangling**, en utilisant la convention de nommage du C. Le nom dans le fichier objet correspond alors exactement au nom écrit dans le code source.

```cpp
// Avec mangling C++ (par défaut)
void process(int id);           // symbole : _Z7processi

// Sans mangling (linkage C)
extern "C" void process(int id); // symbole : process
```

### Syntaxe : déclaration unique et bloc

```cpp
// Déclaration unique
extern "C" void initialize(int config_id);

// Bloc : toutes les déclarations à l'intérieur ont le linkage C
extern "C" {
    void initialize(int config_id);
    void shutdown();
    int  get_status();
    const char* get_version();
}
```

Le bloc `extern "C" { }` ne crée pas de scope — il ne modifie que le linkage des symboles déclarés à l'intérieur.

### Conséquences directes sur le code

L'utilisation de `extern "C"` impose plusieurs contraintes, qui découlent toutes de la nécessité de produire des symboles compatibles C :

**Pas de surcharge de fonctions.** Puisque le mangling est désactivé, deux fonctions `extern "C"` ne peuvent pas porter le même nom, même avec des signatures différentes. Le symbole serait dupliqué et le linker échouerait.

```cpp
extern "C" {
    void process(int id);          // OK : symbole "process"
    void process(int id, double w); // ERREUR : symbole "process" dupliqué
}
```

**Pas de namespaces dans le symbole.** Le nom exporté est le nom brut de la fonction. Les namespaces C++ n'ont aucun effet sur le symbole généré.

```cpp
namespace network {
    extern "C" void process(int id); // symbole : "process" (pas "network_process")
}
```

**Pas de templates.** Une fonction template ne peut pas avoir le linkage C — l'instanciation de templates repose fondamentalement sur le mangling.

**Le corps de la fonction reste du C++.** `extern "C"` n'affecte que le nom du symbole et la convention d'appel. À l'intérieur de la fonction, tout le C++ est disponible : smart pointers, conteneurs STL, exceptions (avec les précautions décrites plus bas), lambdas, etc.

```cpp
extern "C" int compute_total(const int* data, int size) {
    // Le corps est du C++ parfaitement valide
    std::span<const int> view(data, size);
    return std::accumulate(view.begin(), view.end(), 0);
}
```

---

## 43.1.3 — Le header bilingue : le pattern `#ifdef __cplusplus`

### Le problème

Un header d'interface doit être **compilable à la fois par un compilateur C et par un compilateur C++**. Le compilateur C ne comprend pas `extern "C"` (cette syntaxe n'existe pas en C — elle n'a aucune raison d'être puisque le C ne fait jamais de mangling). Le compilateur C++, lui, en a besoin pour désactiver le mangling.

### La solution standard

Le préprocesseur résout ce problème grâce à la macro `__cplusplus`, définie automatiquement par tout compilateur C++ et absente en C :

```cpp
// mylib.h — Header bilingue C/C++

#ifndef MYLIB_H
#define MYLIB_H

#include <stdint.h>  // Types C portables (pas <cstdint>)

#ifdef __cplusplus
extern "C" {
#endif

// --- API publique (compatible C) ---

typedef struct mylib_context mylib_context_t;  // Type opaque

mylib_context_t* mylib_create(const char* config_path);  
int              mylib_process(mylib_context_t* ctx, const uint8_t* data, size_t len);  
void             mylib_destroy(mylib_context_t* ctx);  

const char*      mylib_get_error(const mylib_context_t* ctx);  
int              mylib_version_major(void);  
int              mylib_version_minor(void);  

#ifdef __cplusplus
}
#endif

#endif // MYLIB_H
```

Ce pattern est omniprésent dans les bibliothèques C/C++ open source. Il faut le reconnaître instantanément et savoir le reproduire.

### Détails importants

- Les types utilisés dans l'API doivent être des types C : `int`, `double`, `const char*`, `size_t`, `uint8_t`, pointeurs vers des structs. Pas de `std::string`, pas de `std::vector`, pas de références C++.  
- Le `#include <stdint.h>` (et non `<cstdint>`) garantit la compatibilité avec les compilateurs C.  
- La déclaration `(void)` dans les prototypes sans paramètres (`mylib_version_major(void)`) est une bonne pratique : en C, `f()` signifie "nombre d'arguments non spécifié", alors qu'en C++, `f()` signifie "zéro argument". L'écriture `f(void)` a la même sémantique dans les deux langages.

---

## 43.1.4 — Le pattern du handle opaque

### Principe

Le C ne connaît pas les classes, l'encapsulation, ni les constructeurs/destructeurs. Pour exposer un objet C++ complexe à travers une interface C, on utilise le **pattern du handle opaque** : le code C manipule un pointeur vers un type incomplet (*forward-declared*), sans jamais voir la définition de la structure. Seul le code C++ connaît l'implémentation réelle.

### Implémentation complète

**Le header public (C-compatible) :**

```cpp
// engine.h
#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Type opaque : le code C ne voit qu'un pointeur
typedef struct engine engine_t;

// Cycle de vie
engine_t*   engine_create(const char* config_path);  
void        engine_destroy(engine_t* e);  

// Opérations
int         engine_load_data(engine_t* e, const uint8_t* buf, size_t len);  
double      engine_compute(engine_t* e);  

// Introspection
const char* engine_last_error(const engine_t* e);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_H
```

**L'implémentation C++ :**

```cpp
// engine.cpp
#include "engine.h"
#include <string>
#include <vector>
#include <memory>
#include <cstring>

// La vraie classe C++ — invisible depuis le code C
struct engine {
    std::string               config_path;
    std::vector<uint8_t>      data;
    std::string               last_error;
    // ... membres complexes, smart pointers, conteneurs STL, etc.
};

extern "C" engine_t* engine_create(const char* config_path) {
    try {
        auto* e = new engine();
        e->config_path = config_path;
        return e;
    } catch (const std::exception& ex) {
        // On ne laisse JAMAIS une exception franchir la frontière C
        return nullptr;
    }
}

extern "C" void engine_destroy(engine_t* e) {
    delete e;  // Appelle le destructeur de engine → libère string, vector, etc.
}

extern "C" int engine_load_data(engine_t* e, const uint8_t* buf, size_t len) {
    if (!e || !buf) return -1;
    try {
        e->data.assign(buf, buf + len);
        return 0;  // succès
    } catch (const std::exception& ex) {
        e->last_error = ex.what();
        return -1;  // échec
    }
}

extern "C" double engine_compute(engine_t* e) {
    if (!e || e->data.empty()) return 0.0;
    try {
        double sum = 0.0;
        for (auto byte : e->data) sum += byte;
        return sum / static_cast<double>(e->data.size());
    } catch (const std::exception& ex) {
        e->last_error = ex.what();
        return 0.0;
    }
}

extern "C" const char* engine_last_error(const engine_t* e) {
    if (!e) return "null engine";
    return e->last_error.c_str();
}
```

### Pourquoi ce pattern est fondamental

Ce pattern apparaît partout en interopérabilité :

- **pybind11** l'utilise en interne pour exposer des classes C++ à Python.  
- **cxx** (Rust) génère des wrappers qui suivent exactement cette logique.  
- **Emscripten** s'appuie sur des fonctions `extern "C"` pour les bindings JavaScript.  
- Les **plugins chargés dynamiquement** (`dlopen`/`dlsym`) exigent des symboles non manglés, donc `extern "C"`.

Maîtriser ce pattern une fois, c'est comprendre le mécanisme sous-jacent de toutes les sections qui suivent dans ce chapitre.

---

## 43.1.5 — Les exceptions à la frontière C

### La règle absolue

**Une exception C++ ne doit jamais traverser une frontière `extern "C"`.** Le comportement est indéfini (*undefined behavior*) si une exception se propage au-delà d'une fonction avec linkage C. En pratique, sur la plupart des plateformes Linux, cela provoque un appel à `std::terminate` et un crash immédiat du programme.

Le code C appelant ne dispose d'aucun mécanisme pour intercepter une exception C++ — il n'a pas de `try/catch`, pas de stack unwinding compatible.

### Le pattern de conversion exception → code d'erreur

Chaque fonction `extern "C"` qui appelle du code C++ susceptible de lever des exceptions doit contenir un bloc `try/catch` à son niveau le plus externe :

```cpp
extern "C" int engine_execute(engine_t* e, const char* command) {
    if (!e || !command) return -1;  // Erreur de paramètre

    try {
        e->execute_internal(command);  // Peut lever des exceptions C++
        return 0;                       // Succès

    } catch (const std::invalid_argument& ex) {
        e->last_error = ex.what();
        return -2;  // Erreur de validation

    } catch (const std::runtime_error& ex) {
        e->last_error = ex.what();
        return -3;  // Erreur d'exécution

    } catch (const std::exception& ex) {
        e->last_error = ex.what();
        return -99; // Erreur générique

    } catch (...) {
        e->last_error = "unknown C++ exception";
        return -100; // Exception non-standard
    }
}
```

### Factoriser la conversion avec une macro ou un wrapper

Quand l'API expose de nombreuses fonctions, on factorise le *try/catch* pour éviter la duplication :

```cpp
// Macro de garde — à utiliser dans chaque fonction extern "C"
#define CATCH_AND_STORE_ERROR(ctx, error_code)     \
    catch (const std::exception& ex) {             \
        if (ctx) (ctx)->last_error = ex.what();    \
        return (error_code);                       \
    } catch (...) {                                \
        if (ctx) (ctx)->last_error = "unknown";    \
        return (error_code);                       \
    }

extern "C" int engine_execute(engine_t* e, const char* cmd) {
    if (!e) return -1;
    try {
        e->execute_internal(cmd);
        return 0;
    } CATCH_AND_STORE_ERROR(e, -1)
}

extern "C" int engine_reset(engine_t* e) {
    if (!e) return -1;
    try {
        e->reset_internal();
        return 0;
    } CATCH_AND_STORE_ERROR(e, -1)
}
```

Une alternative plus moderne consiste à utiliser un template wrapper :

```cpp
template <typename F>  
int safe_call(engine_t* e, F&& func) noexcept {  
    if (!e) return -1;
    try {
        func();
        return 0;
    } catch (const std::exception& ex) {
        e->last_error = ex.what();
        return -1;
    } catch (...) {
        e->last_error = "unknown exception";
        return -1;
    }
}

extern "C" int engine_execute(engine_t* e, const char* cmd) {
    return safe_call(e, [&]{ e->execute_internal(cmd); });
}
```

---

## 43.1.6 — ABI Compatibility : au-delà du nom des symboles

### Ce que "ABI compatible" signifie réellement

La compatibilité ABI (*Application Binary Interface*) va bien au-delà du nommage des symboles. Elle couvre l'ensemble des conventions nécessaires pour que du code compilé séparément puisse collaborer correctement au moment de l'exécution :

- **Calling convention** — Comment les arguments sont passés (registres, pile), qui nettoie la pile après l'appel, dans quel registre se trouve la valeur de retour.  
- **Taille et alignement des types** — Un `int` fait-il 4 ou 8 octets ? Comment les champs d'une struct sont-ils alignés et paddés ?  
- **Layout des structures** — L'ordre des champs, le padding entre les membres, la taille totale.  
- **Gestion des exceptions** — Le format des tables d'unwinding, le mécanisme de propagation.  
- **Vtable layout** — L'ordre des fonctions virtuelles dans la vtable, la position du pointeur vptr dans l'objet.

### Règles pratiques pour la compatibilité ABI sur Linux

**GCC et Clang sont ABI-compatibles sur Linux.** Les deux compilateurs implémentent l'Itanium C++ ABI et la System V AMD64 ABI. Un fichier objet compilé avec GCC 15 peut être lié avec un fichier objet compilé avec Clang 20, à condition d'utiliser la même version de la bibliothèque standard (libstdc++ ou libc++).

**Ne pas mélanger libstdc++ et libc++ dans le même binaire.** GCC utilise libstdc++ par défaut, Clang peut utiliser l'une ou l'autre. Mélanger les deux dans le même programme provoque des violations ABI silencieuses — des crashs difficiles à diagnostiquer, car `std::string` et `std::vector` ont des layouts mémoire différents entre les deux implémentations.

```bash
# Vérifier quelle librairie standard un binaire utilise
ldd ./my_program | grep -E "libstdc\+\+|libc\+\+"
```

**La frontière C élimine ces problèmes.** En exposant uniquement des types C à travers `extern "C"`, on s'affranchit de toutes les questions de compatibilité ABI C++ : pas de vtable, pas de `std::string`, pas de templates instanciés dans l'interface publique. C'est précisément pour cette raison que toute interopérabilité sérieuse passe par une interface C.

### Les types sûrs à la frontière ABI

| Sûrs (ABI C stable) | Dangereux (ABI C++ instable) |
|---|---|
| `int`, `long`, `double`, `float` | `std::string` |
| `int32_t`, `uint64_t`, `size_t` | `std::vector<T>` |
| `const char*` (C strings) | `std::unique_ptr<T>` |
| Pointeurs bruts (`T*`) | Références C++ (`T&`) |
| `enum` (avec type sous-jacent fixe) | Classes avec vtable |
| Structs C avec types primitifs | `std::function` |
| Tableaux C de taille fixe | Templates instanciés |

---

## 43.1.7 — Bibliothèques dynamiques et `dlopen`/`dlsym`

### Chargement dynamique de plugins

Le chargement dynamique de bibliothèques partagées (`.so`) avec `dlopen` et `dlsym` est un cas d'usage majeur d'`extern "C"` sur Linux. L'API `dlsym` recherche un symbole **par son nom textuel** — elle ne peut donc fonctionner qu'avec des symboles non manglés.

```cpp
// plugin_interface.h — Interface commune à tous les plugins
#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct plugin plugin_t;

// Chaque plugin doit exporter ces deux symboles
plugin_t*   plugin_create(void);  
void        plugin_destroy(plugin_t* p);  

// Fonctions métier
int         plugin_execute(plugin_t* p, const char* input, char* output, size_t out_size);  
const char* plugin_name(void);  
int         plugin_version(void);  

#ifdef __cplusplus
}
#endif

#endif
```

### Implémentation d'un plugin

```cpp
// my_plugin.cpp
#include "plugin_interface.h"
#include <string>
#include <algorithm>

struct plugin {
    std::string name = "uppercase_plugin";
    int         version = 1;
};

extern "C" plugin_t* plugin_create(void) {
    return new plugin();
}

extern "C" void plugin_destroy(plugin_t* p) {
    delete p;
}

extern "C" const char* plugin_name(void) {
    return "uppercase_plugin";
}

extern "C" int plugin_version(void) {
    return 1;
}

extern "C" int plugin_execute(plugin_t* p, const char* input,
                               char* output, size_t out_size) {
    if (!p || !input || !output) return -1;
    std::string result(input);
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    if (result.size() + 1 > out_size) return -2;  // Buffer trop petit
    std::copy(result.begin(), result.end(), output);
    output[result.size()] = '\0';
    return 0;
}
```

### Compilation et chargement

```bash
# Compiler le plugin comme bibliothèque partagée
g++ -std=c++20 -shared -fPIC -o libmy_plugin.so my_plugin.cpp

# Vérifier que les symboles sont bien non manglés
nm -D libmy_plugin.so | grep plugin_
# T plugin_create
# T plugin_destroy
# T plugin_execute
# T plugin_name
# T plugin_version
```

### Chargement depuis l'application hôte

```cpp
// host.cpp
#include "plugin_interface.h"
#include <dlfcn.h>
#include <cstdio>

int main() {
    // Charger la bibliothèque
    void* handle = dlopen("./libmy_plugin.so", RTLD_LAZY);
    if (!handle) {
        std::fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }

    // Résoudre les symboles — cast vers le type de fonction attendu
    auto create  = reinterpret_cast<plugin_t*(*)()>(dlsym(handle, "plugin_create"));
    auto destroy = reinterpret_cast<void(*)(plugin_t*)>(dlsym(handle, "plugin_destroy"));
    auto execute = reinterpret_cast<int(*)(plugin_t*, const char*, char*, size_t)>(
                       dlsym(handle, "plugin_execute"));
    auto name    = reinterpret_cast<const char*(*)()>(dlsym(handle, "plugin_name"));

    if (!create || !destroy || !execute || !name) {
        std::fprintf(stderr, "dlsym failed: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    // Utiliser le plugin
    std::printf("Loaded plugin: %s\n", name());

    plugin_t* p = create();
    char output[256];
    int rc = execute(p, "hello world", output, sizeof(output));
    if (rc == 0) {
        std::printf("Result: %s\n", output);  // "HELLO WORLD"
    }

    destroy(p);
    dlclose(handle);
    return 0;
}
```

```bash
# Compiler l'hôte (lier avec libdl)
g++ -std=c++20 -o host host.cpp -ldl

# Exécuter
./host
# Loaded plugin: uppercase_plugin
# Result: HELLO WORLD
```

### Contrôler la visibilité des symboles

Par défaut, GCC exporte tous les symboles d'une bibliothèque partagée. Pour une API propre, on limite la visibilité aux seuls symboles publics :

```bash
# Compiler avec visibilité cachée par défaut
g++ -std=c++20 -shared -fPIC -fvisibility=hidden -o libmy_plugin.so my_plugin.cpp
```

Puis on marque explicitement les symboles à exporter :

```cpp
#if defined(_WIN32)
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

extern "C" EXPORT plugin_t* plugin_create(void);  
extern "C" EXPORT void plugin_destroy(plugin_t* p);  
// ...
```

Cette approche réduit la taille de la table de symboles, accélère le chargement dynamique, et évite les collisions de noms entre bibliothèques.

---

## 43.1.8 — Intégration CMake

### Compiler une bibliothèque partagée avec interface C

```cmake
cmake_minimum_required(VERSION 3.20)  
project(mylib VERSION 1.0 LANGUAGES CXX)  

# La bibliothèque partagée
add_library(mylib SHARED
    src/engine.cpp
)

target_include_directories(mylib
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
)

target_compile_features(mylib PUBLIC cxx_std_20)

# Visibilité des symboles : tout cacher sauf l'API publique
set_target_properties(mylib PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION   ${PROJECT_VERSION}
)
```

### Projet consommateur (application ou autre bibliothèque)

```cmake
# L'application hôte qui charge le plugin dynamiquement
add_executable(host src/host.cpp)  
target_link_libraries(host PRIVATE ${CMAKE_DL_LIBS})  # libdl  

# OU : lien direct (sans dlopen)
add_executable(app src/main.cpp)  
target_link_libraries(app PRIVATE mylib)  
```

La variable `${CMAKE_DL_LIBS}` est la manière portable de lier avec `libdl` (nécessaire pour `dlopen`/`dlsym`) dans CMake.

---

## 43.1.9 — Pièges courants et checklist

### Erreurs fréquentes

**Oublier `extern "C"` dans l'implémentation.** Le header déclare les fonctions avec linkage C, mais si le fichier `.cpp` ne voit pas le header (ou le ré-déclare sans `extern "C"`), le compilateur génère des symboles manglés. Le linker échoue avec un `undefined reference` au nom non manglé.

**Retourner un `const char*` vers une string locale.** Le pointeur retourné doit pointer vers une donnée qui survit à l'appel. Retourner `.c_str()` d'un `std::string` local est un dangling pointer garanti.

```cpp
// ERREUR : dangling pointer
extern "C" const char* get_name() {
    std::string name = compute_name();
    return name.c_str();  // 'name' détruit à la sortie → pointeur invalide
}

// CORRECT : stocker la string dans un membre persistant
extern "C" const char* engine_last_error(const engine_t* e) {
    return e->last_error.c_str();  // 'last_error' survit à l'appel
}
```

**Exporter des types C++ dans l'interface.** Passer un `std::vector<int>&` ou un `std::string` dans une fonction `extern "C"` compile (le compilateur C++ accepte), mais le code C ne pourra ni l'appeler, ni interpréter les données. C'est une violation ABI silencieuse.

**Ignorer la gestion de la mémoire côté appelant.** Quand la bibliothèque alloue de la mémoire (par exemple un `engine_t*`), elle doit fournir la fonction de libération correspondante (`engine_destroy`). Ne jamais laisser le code appelant appeler `free()` sur un objet alloué avec `new` — les allocateurs peuvent différer.

### Checklist pour une interface C propre

- [ ] Toutes les fonctions publiques sont déclarées `extern "C"`  
- [ ] Le header utilise le guard `#ifdef __cplusplus`  
- [ ] Seuls des types C apparaissent dans les signatures (pas de `std::string`, `std::vector`, etc.)  
- [ ] Les fonctions sans paramètre utilisent `(void)` en C  
- [ ] Chaque allocation (`_create`) a une libération correspondante (`_destroy`)  
- [ ] Aucune exception C++ ne peut traverser la frontière  
- [ ] Les erreurs sont communiquées via codes de retour et/ou messages stockés  
- [ ] La visibilité des symboles est contrôlée (`-fvisibility=hidden` + attributs)  
- [ ] Les symboles exportés sont vérifiés avec `nm -D` après compilation

---

## Résumé

`extern "C"` n'est pas une curiosité syntaxique — c'est le **mécanisme fondamental** qui permet au C++ de communiquer avec le reste du monde. En désactivant le name mangling et en imposant l'ABI C, il fournit une frontière binaire stable que tout langage, tout outil de chargement dynamique et tout système d'exploitation sait interpréter.

Le pattern du handle opaque, la conversion systématique des exceptions en codes d'erreur, et le contrôle de la visibilité des symboles forment ensemble la base technique de toute interopérabilité C++. Les sections suivantes — pybind11, cxx (Rust), Emscripten — construisent des abstractions de plus haut niveau au-dessus de ces mêmes principes.

> 📎 *La section 43.2 utilise directement ces concepts pour exposer des classes C++ à Python via pybind11 et nanobind.*

⏭️ [Appeler du C++ depuis Python (pybind11)](/43-interoperabilite/02-pybind11.md)
