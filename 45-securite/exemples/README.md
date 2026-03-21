# Chapitre 45 — Securite en C++ : Exemples

## Compilation

Necessite **GCC 15+** pour `std::print` et `std::format` (C++23).

```bash
cd exemples/  
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-15  
cmake --build build --parallel $(nproc)  
```

## Exemples de code sur (compilation standard)

### 01_safe_access.cpp

| | |
|---|---|
| **Section** | 45.1 — Buffer overflows et protection |
| **Fichier source** | `01-buffer-overflows.md` |
| **Description** | Acces securise avec `.at()` (leve `std::out_of_range`) et iteration sur `std::span` (bornes automatiques). |
| **Sortie attendue** | Message d'erreur sur accès hors bornes, puis `1 2 3 4 5`. |

---

### 02_safe_string.cpp

| | |
|---|---|
| **Section** | 45.1 — Buffer overflows et protection |
| **Fichier source** | `01-buffer-overflows.md` |
| **Description** | `std::format` vs `sprintf` — formatage type-safe sans risque de buffer overflow, meme avec des chaines longues. |
| **Sortie attendue** | `Bonjour, Nicolas !` et `Bonjour, Un nom tres long...` (sans overflow). |

---

### 03_integer_overflow.cpp

| | |
|---|---|
| **Section** | 45.2 — Integer overflows et underflows |
| **Fichier source** | `02-integer-overflows.md` |
| **Description** | Wrap-around des entiers non signes (comportement defini). `UINT32_MAX + 1 = 0`, `0 - 1 = UINT32_MAX`. |
| **Sortie attendue** | `a = 4294967295`, `a + 1 = 0`, `0 - 1 = 4294967295`. |

---

### 04_smart_pointers.cpp

| | |
|---|---|
| **Section** | 45.3 — Use-after-free et temporal safety |
| **Fichier source** | `03-use-after-free.md` |
| **Description** | Prevention des UAF avec `unique_ptr` (transfert exclusif), `shared_ptr` (propriete partagee) et `weak_ptr` (observation sans possession). |
| **Sortie attendue** | `unique_ptr: ownership transferred`, `shared_ptr: object survived inner scope`, `weak_ptr: object expired (correct!)`. |

---

## Demos sanitizer (compilation speciale)

### 05_uaf_asan_demo.cpp

| | |
|---|---|
| **Section** | 45.3 — Use-after-free et temporal safety |
| **Fichier source** | `03-use-after-free.md` |
| **Description** | Use-after-free **volontaire** detecte par AddressSanitizer. |
| **Compilation** | `g++-15 -std=c++23 -O0 -fsanitize=address -g -o test 05_uaf_asan_demo.cpp` |
| **Sortie attendue** | `ERROR: AddressSanitizer: heap-use-after-free` avec stack trace complete. |

---

### 06_ubsan_demo.cpp

| | |
|---|---|
| **Section** | 45.2 — Integer overflows et underflows |
| **Fichier source** | `02-integer-overflows.md` |
| **Description** | Signed integer overflow **volontaire** detecte par UndefinedBehaviorSanitizer. |
| **Compilation** | `g++-15 -std=c++23 -O0 -fsanitize=undefined -o test 06_ubsan_demo.cpp` |
| **Sortie attendue** | `runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'`. |

---

## Fuzz target (Clang uniquement)

### 07_fuzz_target.cpp

| | |
|---|---|
| **Section** | 45.5.2 — LibFuzzer et integration |
| **Fichier source** | `05.2-libfuzzer.md` |
| **Description** | Fuzz target minimal pour LibFuzzer. Verifie si les donnees commencent par la signature PNG (0x89 P N G). |
| **Compilation** | `clang++ -std=c++20 -fsanitize=fuzzer,address -o fuzz_test 07_fuzz_target.cpp` |
| **Execution** | `./fuzz_test` (fuzzing continu) ou `timeout 10 ./fuzz_test` (10 secondes). |

---

## Nettoyage

```bash
rm -rf build
```
