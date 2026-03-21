# Chapitre 41 — Optimisation CPU et Memoire : Exemples

## Compilation

```bash
cd exemples/  
cmake -B build -G Ninja -DCMAKE_CXX_FLAGS="-O2"  
cmake --build build --parallel $(nproc)  
```

> Note : `-O2` est recommande plutot que `-O3` pour ces benchmarks, car `-O3` active
> des optimisations agressives (vectorisation, deroulement) qui peuvent masquer les
> effets que les exemples cherchent a demontrer.

## Exemples

### 01_row_col_major.cpp

| | |
|---|---|
| **Section** | 41.1 — Comprendre le cache CPU et la localite des donnees |
| **Fichier source** | `01-cache-cpu.md` |
| **Description** | Parcours row-major vs column-major d'une matrice 4096x4096. Demontre l'impact de la localite spatiale sur les performances cache. |
| **Sortie attendue** | Row-major ~5-10 ms, column-major ~100-200 ms. Ratio 10-30x en faveur du row-major. Compile avec `-fno-tree-vectorize` pour eviter que le compilateur masque l'effet. |

---

### 02_cache_line_size.cpp

| | |
|---|---|
| **Section** | 41.1.1 — Cache L1, L2, L3 |
| **Fichier source** | `01.1-niveaux-cache.md` |
| **Description** | Detection de la taille de cache line via `std::hardware_destructive_interference_size` (C++17). Avec fallback si non disponible. |
| **Sortie attendue** | `Cache line (destructive): 64 octets` et `Cache line (constructive): 64 octets` sur x86-64. |

---

### 03_random_access.cpp

| | |
|---|---|
| **Section** | 41.1.1 — Cache L1, L2, L3 |
| **Fichier source** | `01.1-niveaux-cache.md` |
| **Description** | Acces aleatoire dans des tableaux de taille croissante. Observe les sauts de latence lors des transitions L1 → L2 → L3 → RAM. |
| **Sortie attendue** | 6 lignes montrant la latence croissante : ~0.7 ns (16 KiB, L1) → ~1.4 ns (256 KiB, L2) → ~3 ns (1 MiB, L2 limite) → ~9 ns (16 MiB, L3) → ~12 ns (64 MiB, RAM). |

---

### 04_tiling_matmul.cpp

| | |
|---|---|
| **Section** | 41.1.1 — Cache L1, L2, L3 |
| **Fichier source** | `01.1-niveaux-cache.md` |
| **Description** | Multiplication matricielle naive (6 boucles) vs tiled (cache blocking, BLOCK=64). Demontre le gain du tiling sur N=1024. |
| **Sortie attendue** | La version tiled est 3-8x plus rapide. Compile avec `-fno-tree-vectorize` pour voir l'effet cache pur. |

---

### 05_false_sharing.cpp

| | |
|---|---|
| **Section** | 41.1.2 — Cache lines et false sharing |
| **Fichier source** | `01.2-cache-lines.md` |
| **Description** | Benchmark false sharing : deux threads incrementent des compteurs atomiques. Packed (meme cache line) vs padded (`alignas(64)`, cache lines separees). |
| **Sortie attendue** | Packed : ~3000-5000 ms, Padded : ~500-700 ms. Ratio ~5-7x. Le false sharing cause un ping-pong de cache line entre les coeurs. |

---

### 06_cache_line_check.cpp

| | |
|---|---|
| **Section** | 41.1.2 — Cache lines et false sharing |
| **Fichier source** | `01.2-cache-lines.md` |
| **Description** | Verification manuelle que deux champs d'une struct sont sur la meme cache line. |
| **Sortie attendue** | Les deux adresses ont le meme numero de cache line (addr/64). Affiche `Meme cache line : OUI`. |

---

### 07_aos_vs_soa.cpp

| | |
|---|---|
| **Section** | 41.1.3 — Data-oriented design |
| **Fichier source** | `01.3-data-oriented.md` |
| **Description** | Benchmark AoS (Array of Structures) vs SoA (Structure of Arrays) sur 100 000 particules, boucle `update_positions`. |
| **Sortie attendue** | SoA 1.5-3x plus rapide que AoS. Le gain vient de la meilleure utilisation de la bande passante cache (seuls x/y/z/vx/vy/vz sont charges, pas les champs inutiles). |

---

### 08_branch_prediction.cpp

| | |
|---|---|
| **Section** | 41.2 — Branch prediction et optimisation des conditions |
| **Fichier source** | `02-branch-prediction.md` |
| **Description** | Impact du tri sur la prediction de branchement. Filtre `val >= 128` sur des donnees triees vs non triees. |
| **Sortie attendue** | Avec `-O2` standard, le compilateur transforme le `if` en `cmov` (branchless), masquant la difference. Pour observer l'effet pur : compiler avec `-fno-if-conversion -fno-if-conversion2 -fno-tree-vectorize`. Alors : non triees ~36 ms, triees ~6 ms (6x). |

---

### 09_branchless.cpp

| | |
|---|---|
| **Section** | 41.2 — Branch prediction et optimisation des conditions |
| **Fichier source** | `02-branch-prediction.md` |
| **Description** | Somme conditionnelle branchless via masque arithmetique `-(val >= 128)`. Les deux versions donnent la meme somme. |
| **Sortie attendue** | Les deux sommes sont identiques. Avec les flags anti-optimisation, le branchless est ~5-6x plus rapide. Avec `-O2` standard, le compilateur produit du code similaire pour les deux versions. |

---

### 10_lookup_table.cpp

| | |
|---|---|
| **Section** | 41.2 — Branch prediction et optimisation des conditions |
| **Fichier source** | `02-branch-prediction.md` |
| **Description** | Remplacement d'un `switch` par une lookup table. Zero branchement, un seul acces memoire indexe. |
| **Sortie attendue** | Les deux sommes sont identiques. Les performances sont similaires avec `-O2` (le compilateur optimise le switch en table de sauts). |

---

### 11_simd_print.cpp

| | |
|---|---|
| **Section** | 41.3.1 — Intrinsics |
| **Fichier source** | `03.1-intrinsics.md` |
| **Description** | Operations de base AVX : `set1`, `setr`, `mul`, `add`, `min`, `max`. Utilise une fonction `print_m256` pour afficher les registres. Necessite le support AVX (flag `-mavx`). |
| **Sortie attendue** | 6 lignes avec les resultats arithmetiques corrects : `set1(2)` → tous a 2.00, `setr` → 1 a 8, `v*2` → 2 a 16, `v+2v` → 3,6,9..24, `min`/`max` element par element. |

```
set1(2): [2.00, 2.00, 2.00, 2.00, 2.00, 2.00, 2.00, 2.00]  
setr   : [1.00, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00]  
v * 2  : [2.00, 4.00, 6.00, 8.00, 10.00, 12.00, 14.00, 16.00]  
v + 2v : [3.00, 6.00, 9.00, 12.00, 15.00, 18.00, 21.00, 24.00]  
min    : [1.00, 2.00, 3.00, 1.00, 5.00, 2.00, 3.00, 4.00]  
max    : [4.00, 9.00, 7.00, 7.00, 8.00, 5.00, 8.00, 9.00]  
```

---

## Nettoyage

```bash
rm -rf build
```
