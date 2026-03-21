# Chapitre 44 — Patterns de Conception en C++ : Exemples

## Compilation

Necessite **GCC 15+** pour `std::print`, `std::format` et `deducing this` (C++23).

```bash
cd exemples/  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build --parallel $(nproc)  
```

## Exemples

### 01_variant_factory.cpp

| | |
|---|---|
| **Section** | 44.1 — Singleton, Factory, Builder |
| **Fichier source** | `01-creational-patterns.md` |
| **Description** | Factory a base de `std::variant` — polymorphisme sans vtable, zero allocation heap. Cree Circle et Rectangle, calcule l'aire via `std::visit`. |
| **Sortie attendue** | `Circle area: 78.54` et `Rectangle area: 12.00` |

---

### 02_singleton.cpp

| | |
|---|---|
| **Section** | 44.1.1 — Singleton Thread-Safe |
| **Fichier source** | `01.1-singleton.md` |
| **Description** | Meyers' Singleton : `static` local thread-safe (C++11+). Copie/move supprimes. Verification que `instance()` retourne toujours la meme adresse. |
| **Sortie attendue** | `Logger created`, 3 lignes `[LOG]`, `Same instance: true`, `Logger destroyed`. |

---

### 03_factory_registry.cpp

| | |
|---|---|
| **Section** | 44.1.2 — Factory et Abstract Factory |
| **Fichier source** | `01.2-factory.md` |
| **Description** | Factory avec registry auto-enregistrant. Les compresseurs s'enregistrent via une macro `REGISTER_COMPRESSOR`. Types inconnus levent une exception. |
| **Sortie attendue** | `Created: gzip`, `Created: lz4`, `Created: zstd`, `Error: Unknown compressor: brotli`. |

---

### 04_builder.cpp

| | |
|---|---|
| **Section** | 44.1.3 — Builder |
| **Fichier source** | `01.3-builder.md` |
| **Description** | Builder fluent pour `HttpServer`. Chainage `.port().threads().tls().build()`. Validation dans `build()` (port 0 interdit, TLS sans certificat interdit). |
| **Sortie attendue** | `Server started on 0.0.0.0:9090`, `TLS: true`, `Validation error: Port cannot be 0`. |

---

### 05_observer.cpp

| | |
|---|---|
| **Section** | 44.2 — Observer, Strategy, Command |
| **Fichier source** | `02-behavioral-patterns.md` |
| **Description** | Observer avec `Signal<Args...>` et `std::function`. Inscription, notification, desinscription par token. |
| **Sortie attendue** | `Display: 25.0 C`, `Display: 42.0 C`, `ALERT: critical temperature!`, puis apres disconnect : 1 seul observer reste. |

---

### 06_strategy.cpp

| | |
|---|---|
| **Section** | 44.2 — Observer, Strategy, Command |
| **Fichier source** | `02-behavioral-patterns.md` |
| **Description** | Strategies de retry via `std::function` : constant (500ms fixe), linear (200ms*n), exponential (100ms*2^n). |
| **Sortie attendue** | 3 blocs de 5 lignes montrant les delais croissants pour chaque strategie. |

---

### 07_crtp.cpp

| | |
|---|---|
| **Section** | 44.3 — CRTP |
| **Fichier source** | `03-crtp.md` |
| **Description** | CRTP classique (`Shape<Derived>` + `static_cast`) et `deducing this` C++23 (`this const Self&`). Les deux produisent le meme resultat sans vtable. |
| **Sortie attendue** | 4 lignes : `CRTP: aire = 78.54`, `CRTP: aire = 12.00`, `C++23: aire = 78.54`, `C++23: aire = 12.00`. |

---

### 08_type_erasure.cpp

| | |
|---|---|
| **Section** | 44.4 — Type Erasure et std::any |
| **Fichier source** | `04-type-erasure.md` |
| **Description** | Custom type erasure `AnyDrawable` : collection heterogene (Circle, Rectangle, Text) dans un `std::vector` sans aucun heritage. Pattern Concept/Model avec `clone()`. |
| **Sortie attendue** | 3 paires de lignes (draw + describe) pour circle, rect, text. |

```
Drawing circle at (100.0, 100.0) r=50.0
  -> Circle(r=50.0)
Drawing rect at (200.0, 150.0) 80x40
  -> Rect(80x40)
Drawing 'Hello' at (300.0, 200.0)
  -> Text('Hello')
```

---

## Nettoyage

```bash
rm -rf build
```
