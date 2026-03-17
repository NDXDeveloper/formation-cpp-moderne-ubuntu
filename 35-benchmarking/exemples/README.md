# Exemples du Chapitre 35 — Benchmarking

## Prérequis

```bash
g++-15 --version    # GCC 15  
cmake --version     # CMake 3.20+  
ninja --version     # Ninja  
```

> Google Benchmark est téléchargé automatiquement par FetchContent.

---

## 01\_benchmarks.cpp

| | |
|---|---|
| **Section** | 35.1 |
| **Fichier .md** | `01-google-benchmark.md` |
| **Description** | Benchmarks illustrant les concepts clés : tri de vecteur, `DoNotOptimize`, `ClobberMemory`, benchmarks paramétrés, `SetItemsProcessed`. |

### Compilation et exécution

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-15 -DCMAKE_BUILD_TYPE=Release  
cmake --build build  
./build/benchmarks
```

### Sortie attendue (valeurs indicatives)

```
-------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations
-------------------------------------------------------------------
BM_VectorSort                  ~60 ns        ~60 ns      ~2M  
BM_StringCreation              ~1.5 ns       ~1.5 ns     ~100M  
BM_VectorPushBack              ~86 ns        ~86 ns      ~1.6M  
BM_VectorSortParam/100         ~900 ns       ~900 ns     ~150K items/s=~100M/s  
BM_VectorSortParam/1000       ~5500 ns      ~5500 ns      ~25K items/s=~180M/s  
BM_VectorSortParam/10000     ~68000 ns     ~68000 ns      ~2K  items/s=~146M/s  
BM_MapInsert                 ~55000 ns     ~55000 ns      ~2.5K  
```

> Les valeurs exactes varient selon le CPU et la charge système.

---

## Nettoyage

```bash
rm -rf build
```
