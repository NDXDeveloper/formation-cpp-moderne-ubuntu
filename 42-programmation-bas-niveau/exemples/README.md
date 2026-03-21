# Chapitre 42 — Programmation Bas Niveau : Exemples

## Compilation

```bash
cd exemples/  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release  
cmake --build build --parallel $(nproc)  
```

> Note : les exemples d'inline assembly (01-03) sont specifiques a x86-64.
> Les exemples concurrents (07-10) utilisent pthread.

## Exemples

### 01_asm_add.cpp

| | |
|---|---|
| **Section** | 42.1 — Inline Assembly en C++ |
| **Fichier source** | `01-inline-assembly.md` |
| **Description** | Addition de deux int64 via asm etendu avec noms symboliques. |
| **Sortie attendue** | `result = 60` (42 + 18) |

---

### 02_rdtscp.cpp

| | |
|---|---|
| **Section** | 42.1 — Inline Assembly en C++ |
| **Fichier source** | `01-inline-assembly.md` |
| **Description** | Lecture du Time-Stamp Counter (rdtscp) pour mesurer les cycles CPU. Pattern `do_not_optimize` pour empecher l'elimination du code. |
| **Sortie attendue** | Nombre de cycles (variable), Core ID, et la somme calculee. |

---

### 03_idiv.cpp

| | |
|---|---|
| **Section** | 42.1 — Inline Assembly en C++ |
| **Fichier source** | `01-inline-assembly.md` |
| **Description** | Division entiere 10/3 via l'instruction `idivl`, avec registres nommes (`"a"`, `"d"`) et clobber `"cc"`. |
| **Sortie attendue** | `10 / 3 = 3 remainder 1` |

---

### 04_bits.cpp

| | |
|---|---|
| **Section** | 42.2 — Manipulation de Bits et Bitfields |
| **Fichier source** | `02-manipulation-bits.md` |
| **Description** | Operations bit a bit (set/clear/toggle/extract), idiomes classiques (puissance de 2), et fonctions C++20 `<bit>` (popcount, countr_zero, bit_ceil, endianness). |
| **Sortie attendue** | Toutes les valeurs de bits conformes, RGB565 R=26 G=52 B=11, popcount=3, countr_zero=18, bit_ceil(100)=128, little-endian. |

---

### 05_bitmap_allocator.cpp

| | |
|---|---|
| **Section** | 42.2 — Manipulation de Bits et Bitfields |
| **Fichier source** | `02-manipulation-bits.md` |
| **Description** | Allocateur bitmap gerant 256 blocs via 4 mots de 64 bits. Utilise `std::countr_zero` et `std::popcount`. |
| **Sortie attendue** | 256 blocs libres initialement, allocation de 0/1/2, liberation de 1, reallocation de 1 (reutilise). |

---

### 06_enum_bitmask.cpp

| | |
|---|---|
| **Section** | 42.2 — Manipulation de Bits et Bitfields |
| **Fichier source** | `02-manipulation-bits.md` |
| **Description** | Pattern `enum class` + trait `EnableBitmask` pour des flags type-safe avec operateurs `|`, `&`, `~`. |
| **Sortie attendue** | Read=1, Execute=0, apres `|= Execute`: 1, apres `&= ~Write`: 0. |

---

### 07_acquire_release.cpp

| | |
|---|---|
| **Section** | 42.3 — Memory Ordering et Barrieres Memoire |
| **Fichier source** | `03-memory-ordering.md` |
| **Description** | Pattern acquire/release : un producteur publie `data=42` via release store, un consommateur lit via acquire load. Verifie 1000 fois que l'assertion tient. |
| **Sortie attendue** | `1000 iterations: acquire/release pattern correct` |

---

### 08_spinlock.cpp

| | |
|---|---|
| **Section** | 42.3 — Memory Ordering et Barrieres Memoire |
| **Fichier source** | `03-memory-ordering.md` |
| **Description** | Spin-lock minimaliste avec `exchange(true, acquire)` / `store(false, release)`. 4 threads incrementent un compteur 100 000 fois chacun. |
| **Sortie attendue** | `Counter: 400000 (expected: 400000)` et `PASS`. |

---

### 09_cas_counter.cpp

| | |
|---|---|
| **Section** | 42.4 — Lock-free Programming |
| **Fichier source** | `04-lock-free.md` |
| **Description** | Compteur lock-free via CAS-loop (`compare_exchange_weak`). 8 threads incrementent 100 000 fois chacun. |
| **Sortie attendue** | `CAS counter: 800000 (expected: 800000)` et `PASS`. |

---

### 10_lockfree_stack.cpp

| | |
|---|---|
| **Section** | 42.4.1 — Structures Lock-free |
| **Fichier source** | `04.1-structures-lock-free.md` |
| **Description** | Pile lock-free naive avec push/pop via CAS sur pointeur `head`. Test mono-thread verifiant l'ordre LIFO. |
| **Sortie attendue** | Pop retourne 3, 2, 1, empty (LIFO). `PASS`. |

```
Pop 1: 3  
Pop 2: 2  
Pop 3: 1  
Pop 4: empty  
PASS  
```

---

## Nettoyage

```bash
rm -rf build
```
