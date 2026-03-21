# Chapitre 43 — C++ et Autres Langages : Exemples

## Compilation

```bash
cd exemples/  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release  
cmake --build build --parallel $(nproc)  
```

> Note : seuls les exemples C++ natifs sont compiles ici. Les sections Python
> (pybind11/nanobind), Rust (cxx/autocxx) et WebAssembly (Emscripten) necessitent
> des outils externes non inclus dans ce build.

## Exemples

### 01_engine_test.cpp + engine.h + engine.cpp

| | |
|---|---|
| **Section** | 43.1.4 — Le pattern du handle opaque |
| **Fichier source** | `01-cpp-et-c.md` |
| **Description** | Handle opaque : un objet C++ (`engine`) encapsule dans une interface C (`extern "C"`). Demontre create/load_data/compute/destroy via des types C purs. Les exceptions sont converties en codes de retour. |
| **Sortie attendue** | `load_data: 0`, `compute (average): 30.0`, `Engine destroyed` |

---

### 02_plugin.cpp + plugin_interface.h

| | |
|---|---|
| **Section** | 43.1.7 — Bibliotheques dynamiques et dlopen/dlsym |
| **Fichier source** | `01-cpp-et-c.md` |
| **Description** | Plugin compile en bibliotheque partagee (`.so`). Exporte des symboles `extern "C"` non mangles. Transforme un texte en majuscules. |
| **Comportement attendu** | `nm -D build/libplugin.so \| grep plugin_` montre des symboles non mangles. |

Compilation manuelle (sans CMake) :
```bash
g++ -std=c++20 -shared -fPIC -o libplugin.so 02_plugin.cpp  
nm -D libplugin.so | grep plugin_  
# T plugin_create
# T plugin_destroy
# T plugin_execute
# T plugin_name
# T plugin_version
```

---

### 03_host.cpp

| | |
|---|---|
| **Section** | 43.1.7 — Bibliotheques dynamiques et dlopen/dlsym |
| **Fichier source** | `01-cpp-et-c.md` |
| **Description** | Application hote qui charge le plugin via `dlopen`/`dlsym`, resout les symboles et appelle `plugin_execute`. |
| **Sortie attendue** | `Loaded plugin: uppercase_plugin` et `Result: HELLO WORLD` |

Test (depuis le dossier `build/`) :
```bash
cd build/
./03_host
# Loaded plugin: uppercase_plugin
# Result: HELLO WORLD
```

---

### 04_wasm_stats.cpp

| | |
|---|---|
| **Section** | 43.4.2 — Compilation et integration JavaScript |
| **Fichier source** | `04.2-compilation-js.md` |
| **Description** | Programme de statistiques (moyenne, variance, ecart-type) compilable en natif et en WebAssembly. En natif : `g++`. En Wasm : `em++`. |
| **Sortie attendue (natif)** | `Mean: 3.00`, `Variance: 2.00`, `Std Dev: 1.41` |

Compilation Wasm (si Emscripten est installe) :
```bash
em++ -std=c++20 -O2 -o stats.html 04_wasm_stats.cpp
# Ouvrir stats.html dans un navigateur
```

---

## Exemples non compilables ici (necessitent des outils externes)

| Section | Outil requis | Exemple dans le .md |
|---------|-------------|---------------------|
| 43.2 (Python) | pybind11 + Python-dev | `PYBIND11_MODULE(...)`, classes C++ exposees a Python |
| 43.2.4 (Python) | nanobind + Python-dev | `NB_MODULE(...)`, alternative legere a pybind11 |
| 43.3.1 (Rust FFI) | Rust toolchain + cargo | `extern "C"` cote Rust, `#[no_mangle]` |
| 43.3.2 (Rust cxx) | Rust + crate cxx | `#[cxx::bridge]` bridge declaratif |
| 43.3.3 (Rust autocxx) | Rust + crate autocxx | Generation automatique de bindings |
| 43.4.1 (Wasm) | Emscripten SDK (emsdk) | `em++` compilation vers .wasm |

---

## Nettoyage

```bash
rm -rf build
```
