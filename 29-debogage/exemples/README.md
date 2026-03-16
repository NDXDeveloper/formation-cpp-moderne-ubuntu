# Exemples du Chapitre 29 — Débogage Avancé

Ce répertoire contient les exemples pratiques du chapitre 29. Chaque exemple est un programme C++ intentionnellement buggé, conçu pour être diagnostiqué avec l'outil correspondant (GDB, ASan, UBSan, TSan, std::stacktrace).

## Prérequis

```bash
g++-15 --version    # GCC 15  
gdb --version       # GDB (sudo apt install gdb)  
```

---

## Liste des exemples

### 01\_config\_parser.cpp

| | |
|---|---|
| **Section** | 29.1 |
| **Fichier .md** | `01-gdb-commandes.md` |
| **Description** | Programme fil conducteur du chapitre — parseur de fichiers de configuration avec bugs subtils (espaces autour du `=`, fichier inexistant, confusion clé absente / valeur vide). Conçu pour être exploré avec GDB. |

**Compilation et utilisation avec GDB :**
```bash
g++-15 -ggdb3 -O0 -std=c++20 -Wall -Wextra -o config_parser 01_config_parser.cpp  
gdb ./config_parser  
(gdb) start database.conf
(gdb) break parse_config
(gdb) run database.conf
```

**Sortie attendue (exécution normale) :**
```
Entrées parsées : 5
[L2] db_host = localhost
[L3] db_port = 5432
[L4] db_name = production
[L6] timeout = 30
[L7] max_connections = 100
Connexion à : localhost
```

---

### 02\_heap\_overflow.cpp

| | |
|---|---|
| **Section** | 29.4.1 |
| **Fichier .md** | `04.1-addresssanitizer.md` |
| **Description** | Heap buffer overflow — boucle `<= 10` au lieu de `< 10` sur un tableau de 10 éléments. |

**Compilation :**
```bash
g++-15 -fsanitize=address -g -O1 -fno-omit-frame-pointer -o heap_overflow 02_heap_overflow.cpp
./heap_overflow
```

**Comportement attendu :** ASan détecte `heap-buffer-overflow` avec la pile d'appels de l'accès fautif et de l'allocation.

---

### 03\_stack\_overflow.cpp

| | |
|---|---|
| **Section** | 29.4.1 |
| **Fichier .md** | `04.1-addresssanitizer.md` |
| **Description** | Stack buffer overflow — `strcpy` sans vérification de taille dans un buffer de 16 octets. |

**Compilation :**
```bash
g++-15 -fsanitize=address -g -O1 -fno-omit-frame-pointer -D_FORTIFY_SOURCE=0 \
    -o stack_overflow 03_stack_overflow.cpp
./stack_overflow
```

> **Note :** `-D_FORTIFY_SOURCE=0` est nécessaire sur Ubuntu pour que ASan intercepte le `strcpy` (sinon la protection glibc le détecte avant ASan).

**Comportement attendu :** ASan détecte `stack-buffer-overflow` et identifie la variable locale `buffer` (ligne 6).

---

### 04\_use\_after\_free.cpp

| | |
|---|---|
| **Section** | 29.4.1 |
| **Fichier .md** | `04.1-addresssanitizer.md` |
| **Description** | Use-after-free — accès via une référence à un objet libéré par `delete`. |

**Compilation :**
```bash
g++-15 -fsanitize=address -g -O1 -fno-omit-frame-pointer -o use_after_free 04_use_after_free.cpp
./use_after_free
```

**Comportement attendu :** ASan détecte `heap-use-after-free` avec trois piles d'appels : accès fautif, libération, allocation.

---

### 05\_double\_free.cpp

| | |
|---|---|
| **Section** | 29.4.1 |
| **Fichier .md** | `04.1-addresssanitizer.md` |
| **Description** | Double-free — un tableau est libéré par `cleanup()` puis à nouveau dans `main()`. |

**Compilation :**
```bash
g++-15 -fsanitize=address -g -O1 -fno-omit-frame-pointer -o double_free 05_double_free.cpp
./double_free
```

**Comportement attendu :** ASan détecte `attempting double-free`.

---

### 06\_memory\_leak.cpp

| | |
|---|---|
| **Section** | 29.4.1 |
| **Fichier .md** | `04.1-addresssanitizer.md` |
| **Description** | Memory leak — 100 allocations `new` sans `delete` correspondant. Détecté par LeakSanitizer (intégré à ASan). |

**Compilation :**
```bash
g++-15 -fsanitize=address -g -O1 -fno-omit-frame-pointer -o memory_leak 06_memory_leak.cpp
./memory_leak
```

**Comportement attendu :** LeakSanitizer détecte `detected memory leaks` avec le nombre d'octets et le point d'allocation.

---

### 07\_signed\_overflow.cpp

| | |
|---|---|
| **Section** | 29.4.2 |
| **Fichier .md** | `04.2-ubsan.md` |
| **Description** | Signed integer overflow — `INT_MAX + 1` est un comportement indéfini en C++. |

**Compilation :**
```bash
g++-15 -fsanitize=undefined -g -O1 -fno-omit-frame-pointer -o signed_overflow 07_signed_overflow.cpp
./signed_overflow
```

**Sortie attendue :**
```
07_signed_overflow.cpp:13:9: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
INT_MAX + 1 = -2147483648
```

---

### 08\_data\_race.cpp

| | |
|---|---|
| **Section** | 29.4.3 |
| **Fichier .md** | `04.3-threadsanitizer.md` |
| **Description** | Data race — deux threads incrémentent un compteur global sans synchronisation. |

**Compilation :**
```bash
g++-15 -fsanitize=thread -g -O1 -fno-omit-frame-pointer -std=c++20 -o data_race 08_data_race.cpp
./data_race
```

**Comportement attendu :** TSan détecte `WARNING: ThreadSanitizer: data race` avec les piles des deux accès concurrents.

> **Note :** ASan et TSan sont **incompatibles** — ne pas les combiner dans le même build.

---

### 09\_stacktrace.cpp

| | |
|---|---|
| **Section** | 29.5 |
| **Fichier .md** | `05-stacktrace-debug.md` |
| **Description** | Capture et affichage de la pile d'appels depuis le code avec `std::stacktrace::current()` (C++23). |

**Compilation :**
```bash
g++-15 -std=c++23 -g -O0 -o stacktrace 09_stacktrace.cpp -lstdc++exp
./stacktrace
```

**Sortie attendue :**
```
=== Pile d'appels ===
   0# inner_function() at .../09_stacktrace.cpp:14
   1# middle_function() at .../09_stacktrace.cpp:29
   2# main at .../09_stacktrace.cpp:33
   3# __libc_start_call_main at ...
   ...
```

> **Note :** `-lstdc++exp` est nécessaire avec GCC pour le linkage de `std::stacktrace`. Avec `-O0`, toutes les fonctions apparaissent dans la trace. Avec `-O1`/`-O2`, certaines fonctions inlinées peuvent ne pas apparaître.

---

### 10\_shift\_ub.cpp

| | |
|---|---|
| **Section** | 29.4.2 |
| **Fichier .md** | `04.2-ubsan.md` |
| **Description** | Shifts invalides — décalage >= largeur du type, shift négatif. En C++20, le shift gauche d'un négatif n'est plus un UB. |

**Compilation :**
```bash
g++-15 -fsanitize=undefined -g -O1 -fno-omit-frame-pointer -std=c++20 -o shift_ub 10_shift_ub.cpp
./shift_ub
```

**Comportement attendu :** UBSan détecte `shift exponent 32 is too large` et `shift exponent -3 is negative`.

---

### 11\_div\_zero.cpp

| | |
|---|---|
| **Section** | 29.4.2 |
| **Fichier .md** | `04.2-ubsan.md` |
| **Description** | Division entière par zéro — UB détecté par UBSan. |

**Compilation :**
```bash
g++-15 -fsanitize=undefined -g -O1 -fno-omit-frame-pointer -o div_zero 11_div_zero.cpp
./div_zero
```

**Comportement attendu :** UBSan détecte `runtime error: division by zero`.

---

### 12\_null\_deref.cpp

| | |
|---|---|
| **Section** | 29.4.2 |
| **Fichier .md** | `04.2-ubsan.md` |
| **Description** | Déréférencement de pointeur null — accès à un membre via `nullptr`. |

**Compilation :**
```bash
g++-15 -fsanitize=undefined -g -O1 -fno-omit-frame-pointer -o null_deref 12_null_deref.cpp
./null_deref
```

**Comportement attendu :** UBSan détecte `member access within null pointer of type 'struct Config'`.

---

### 13\_vector\_race.cpp

| | |
|---|---|
| **Section** | 29.4.3 |
| **Fichier .md** | `04.3-threadsanitizer.md` |
| **Description** | Data race sur un `std::vector` — `push_back` concurrent sans synchronisation. |

**Compilation :**
```bash
g++-15 -fsanitize=thread -g -O1 -fno-omit-frame-pointer -std=c++20 -o vector_race 13_vector_race.cpp
./vector_race
```

**Comportement attendu :** TSan détecte `WARNING: ThreadSanitizer: data race`.

---

### 14\_deadlock.cpp

| | |
|---|---|
| **Section** | 29.4.3 |
| **Fichier .md** | `04.3-threadsanitizer.md` |
| **Description** | Lock-order-inversion (deadlock potentiel) — deux threads acquièrent deux mutex dans un ordre inversé. TSan détecte le problème **même si le deadlock ne se produit pas**. |

**Compilation :**
```bash
g++-15 -fsanitize=thread -g -O1 -fno-omit-frame-pointer -std=c++20 -o deadlock 14_deadlock.cpp  
timeout 5 ./deadlock  
```

**Comportement attendu :** TSan détecte `WARNING: ThreadSanitizer: lock-order-inversion (potential deadlock)`.

---

## Nettoyage

```bash
rm -f config_parser heap_overflow stack_overflow use_after_free \
     double_free memory_leak signed_overflow data_race stacktrace \
     shift_ub div_zero null_deref vector_race deadlock
```
