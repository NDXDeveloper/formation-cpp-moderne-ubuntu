🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 41.3 — SIMD et vectorisation (SSE, AVX)

> **Chapitre 41 : Optimisation CPU et Mémoire** · Section 3  
> **Niveau** : Expert · **Prérequis** : Sections 41.1 (cache CPU), 41.2 (branch prediction), Chapitre 3 (types primitifs)

---

## Introduction

Les deux sections précédentes ont traité de la façon dont le processeur *attend* les données (cache misses) et *hésite* devant les décisions (branch mispredictions). Cette section aborde un troisième levier fondamental : faire en sorte que le processeur **travaille davantage par instruction**.

Un processeur scalaire classique additionne deux entiers en une instruction. Mais les CPU modernes disposent d'unités de calcul capables de traiter **4, 8, 16 ou même 32 valeurs simultanément** en une seule instruction. C'est le principe du ***SIMD*** — *Single Instruction, Multiple Data* : une même opération appliquée en parallèle sur un vecteur de données.

Le SIMD n'est pas une technologie de niche réservée au calcul scientifique. Il est présent dans chaque processeur x86-64 vendu depuis 20 ans, et le compilateur tente de l'exploiter automatiquement à chaque compilation avec `-O2`. Comprendre comment il fonctionne — et surtout comment aider le compilateur à l'utiliser — est un levier d'optimisation qui peut multiplier les performances d'une boucle par 4×, 8× ou même 16× sur des cas favorables.

---

## Le modèle SIMD : une instruction, plusieurs données

### Traitement scalaire vs SIMD

Considérons l'addition de deux tableaux de 8 floats :

**Scalaire** — 8 instructions, une valeur par instruction :

```
Instruction 1 :  a[0] + b[0] → c[0]  
Instruction 2 :  a[1] + b[1] → c[1]  
Instruction 3 :  a[2] + b[2] → c[2]  
...
Instruction 8 :  a[7] + b[7] → c[7]
```

**SIMD (AVX, 256 bits)** — 1 instruction, 8 valeurs simultanément :

```
                ┌───────────────────────────────────────────────────────┐
Registre ymm0 : │ a[0] │ a[1] │ a[2] │ a[3] │ a[4] │ a[5] │ a[6] │ a[7] │
                └───────────────────────────────────────────────────────┘
                               +
                ┌───────────────────────────────────────────────────────┐
Registre ymm1 : │ b[0] │ b[1] │ b[2] │ b[3] │ b[4] │ b[5] │ b[6] │ b[7] │
                └───────────────────────────────────────────────────────┘
                               =
                ┌───────────────────────────────────────────────────────┐
Registre ymm2 : │ c[0] │ c[1] │ c[2] │ c[3] │ c[4] │ c[5] │ c[6] │ c[7] │
                └───────────────────────────────────────────────────────┘

1 seule instruction : vaddps ymm2, ymm0, ymm1
```

Le speedup théorique est de **8×** pour un `float` de 32 bits dans un registre de 256 bits. En pratique, le gain réel dépend de la bande passante mémoire, de l'alignement des données, et de la capacité du compilateur à exprimer le traitement en instructions SIMD.

---

## Générations de jeux d'instructions SIMD sur x86-64

L'écosystème SIMD x86 s'est construit par couches successives, chaque génération élargissant les registres et ajoutant des opérations :

| Jeu d'instructions | Année | Largeur des registres | Registres | Capacité par instruction (float 32-bit) |
|--------------------|-------|-----------------------|-----------|------------------------------------------|
| **SSE** | 1999 | 128 bits | `xmm0`–`xmm7` | 4 floats |
| **SSE2** | 2001 | 128 bits | `xmm0`–`xmm15` (x86-64) | 4 floats / 2 doubles |
| **SSE4.1/4.2** | 2008 | 128 bits | `xmm0`–`xmm15` | 4 floats (+ opérations avancées) |
| **AVX** | 2011 | 256 bits | `ymm0`–`ymm15` | 8 floats / 4 doubles |
| **AVX2** | 2013 | 256 bits | `ymm0`–`ymm15` | 8 floats / 8 int32 (entiers 256-bit) |
| **AVX-512** | 2017 | 512 bits | `zmm0`–`zmm31` | 16 floats / 8 doubles |

### Ce qu'on peut considérer comme acquis en 2026

- **SSE2** : présent sur **100 %** des CPU x86-64. C'est le minimum garanti. Les compilateurs ciblent SSE2 par défaut sur x86-64.  
- **SSE4.1/4.2** : présent sur la quasi-totalité du parc matériel en production (tout CPU depuis ~2008).  
- **AVX2** : présent sur tout CPU Intel depuis Haswell (2013) et AMD depuis Excavator (2015). C'est le **sweet spot** en 2026 — la cible recommandée pour du code haute performance destiné à des machines desktop et serveur.  
- **AVX-512** : disponible sur Intel Xeon (Skylake-X+), Intel Core de 11e gen (Ice Lake), et AMD Zen 4/5. Le support est large sur les serveurs récents mais pas universel sur le desktop. Attention : sur certains CPU Intel (Alder Lake, Raptor Lake), AVX-512 est désactivé en raison de l'architecture hybride P-core/E-core.

### Le piège du throttling AVX

Les instructions AVX (256 bits) et surtout AVX-512 consomment davantage d'énergie que les instructions scalaires ou SSE. Sur certains CPU Intel, l'exécution d'instructions AVX-512 peut déclencher une **réduction de fréquence** (*frequency throttling*) de 100–300 MHz pour respecter l'enveloppe thermique. Le résultat paradoxal : du code AVX-512 peut être *plus lent* que du code AVX2 si le throttling annule le gain de la largeur supérieure.

C'est un facteur à mesurer empiriquement sur chaque plateforme cible. La commande `perf stat` rapporte la fréquence effective :

```bash
perf stat -e cycles,instructions,cpu-clock ./mon_programme
# Vérifier si la fréquence effective (cycles / cpu-clock) baisse
# lors de l'utilisation d'AVX-512
```

Pour cette raison, **AVX2 (256 bits) est souvent le meilleur compromis en 2026** : largeur suffisante pour des gains significatifs, support universel, pas de throttling sur les architectures modernes.

---

## SIMD sur ARM : NEON et SVE

Le SIMD n'est pas exclusif à x86. Sur les architectures ARM, deux familles d'instructions sont disponibles :

| Jeu d'instructions | Architecture | Largeur | Particularité |
|--------------------|-------------|---------|---------------|
| **NEON** | ARMv7/ARMv8 (AArch64) | 128 bits fixe | Présent sur tous les ARM 64-bit (Raspberry Pi 4/5, Apple M-series, Graviton) |
| **SVE / SVE2** | ARMv8.2+ (AArch64) | 128–2048 bits *variable* | Largeur définie par l'implémentation matérielle, code indépendant de la largeur |

SVE (*Scalable Vector Extension*) est particulièrement intéressant : le même binaire fonctionne sur des CPU avec des largeurs de registre différentes (128, 256, 512 bits). AWS Graviton 3 implémente SVE avec des registres de 256 bits ; les processeurs Fujitsu A64FX (supercalculateur Fugaku) utilisent 512 bits.

Le compilateur GCC 15 et Clang 20 supportent l'auto-vectorisation pour NEON et SVE via `-march=armv8-a+sve` ou `-march=native` sur ARM.

Les principes d'optimisation SIMD présentés dans cette section (alignement, absence de branchements, données contiguës, SoA) s'appliquent identiquement à ARM NEON/SVE.

---

## Les deux voies de la vectorisation

Il existe deux manières d'exploiter le SIMD en C++ :

### Voie 1 : auto-vectorisation par le compilateur

Le compilateur analyse les boucles et tente de les transformer automatiquement en code SIMD. C'est la voie **recommandée en priorité** : pas de code intrusif, portabilité préservée, maintenance simplifiée.

```cpp
// Le compilateur peut auto-vectoriser cette boucle
void add_arrays(const float* a, const float* b, float* c, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        c[i] = a[i] + b[i];
    }
}
```

Avec `-O2 -march=native`, GCC et Clang génèrent du code AVX2 pour cette boucle sans intervention du développeur. Le rôle du développeur est de **structurer le code pour que l'auto-vectorisation réussisse** — c'est l'objet de la sous-section 41.3.2.

### Voie 2 : intrinsics SIMD

Les *intrinsics* sont des fonctions C/C++ qui correspondent directement à des instructions assembleur SIMD. Elles offrent un contrôle total sur les instructions émises, au prix d'un code verbeux et spécifique à une architecture.

```cpp
#include <immintrin.h>

// Même opération, écrite avec des intrinsics AVX
void add_arrays_avx(const float* a, const float* b, float* c, std::size_t n) {
    for (std::size_t i = 0; i + 7 < n; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vc = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(&c[i], vc);
    }
    // Traiter les éléments restants en scalaire...
}
```

Les intrinsics sont détaillés dans la sous-section 41.3.1.

### Quelle voie choisir ?

```
                    ┌─────────────────────────────┐
                    │ Le code est-il dans un      │
                    │ hotspot confirmé par perf ? │
                    └──────────────┬──────────────┘
                                   │
                          ┌────────┴────────┐
                          │                 │
                         NON               OUI
                          │                 │
                   Ne rien faire   ┌────────┴────────────┐
                                   │ L'auto-vectorisation│
                                   │ fonctionne-t-elle ? │
                                   └───────┬─────────────┘
                                           │
                                  ┌────────┴────────┐
                                  │                 │
                                 OUI               NON
                                  │                 │
                        Vérifier avec        Restructurer le code
                        les rapports         pour aider le compilateur
                        du compilateur       (section 41.3.2)
                                  │                 │
                                  │         ┌───────┴───────────┐
                                  │         │ Toujours pas      │
                                  │         │ vectorisé ?       │
                                  │         └───────┬───────────┘
                                  │                 │
                                  │                OUI
                                  │                 │
                                  │         Utiliser les intrinsics
                                  │         (section 41.3.1)
                                  │                 │
                                  └────────┬────────┘
                                           │
                                    Mesurer le gain
```

En résumé : **laisser le compilateur vectoriser d'abord**, vérifier qu'il l'a fait, restructurer le code si nécessaire, et ne recourir aux intrinsics qu'en dernier recours sur les boucles critiques où le compilateur échoue.

---

## Conditions nécessaires à la vectorisation

Que ce soit pour l'auto-vectorisation ou les intrinsics, les données doivent respecter certaines conditions pour que le SIMD soit efficace. Ces conditions découlent directement des principes vus en 41.1 (localité mémoire) et 41.2 (branch prediction).

### 1. Données contiguës en mémoire

Le SIMD charge et stocke des blocs de 16, 32 ou 64 octets consécutifs. Les données doivent être dans un tableau contigu — `std::vector`, `std::array`, tableau C, ou tout buffer alloué avec `new[]` ou un allocateur. Les conteneurs à nœuds (`std::list`, `std::map`, `std::set`) sont fondamentalement incompatibles avec le SIMD.

```cpp
// ✅ Vectorisable : données contiguës
std::vector<float> data(N);  
for (std::size_t i = 0; i < N; ++i)  
    data[i] *= 2.0f;

// ❌ Non vectorisable : nœuds dispersés
std::list<float> data_list;  
for (auto& val : data_list)  
    val *= 2.0f;
```

### 2. Données homogènes (même type)

Une instruction SIMD opère sur un vecteur d'éléments du même type. Le layout SoA (section 41.1.3) est naturellement vectorisable car chaque tableau contient des éléments d'un seul type. Le layout AoS entrelace des types différents, ce qui rend la vectorisation difficile voire impossible sans opérations coûteuses de *gather/scatter*.

```cpp
// ✅ SoA → vectorisation naturelle
struct ParticleSystem {
    std::vector<float> x, y, z;
};
// La boucle sur x[] opère sur un flux homogène de floats

// ❌ AoS → vectorisation difficile
struct Particle { float x, y, z; int flags; };  
std::vector<Particle> particles;  
// Les x sont entrelacés avec y, z, flags → pas de flux contigu de x
```

### 3. Absence de dépendances entre itérations

Le compilateur ne peut vectoriser une boucle que si les itérations sont **indépendantes** — le résultat de l'itération `i` ne dépend pas du résultat de l'itération `i-1`.

```cpp
// ✅ Itérations indépendantes → vectorisable
for (std::size_t i = 0; i < N; ++i)
    c[i] = a[i] + b[i];    // c[i] ne dépend que de a[i] et b[i]

// ❌ Dépendance séquentielle → NON vectorisable
for (std::size_t i = 1; i < N; ++i)
    a[i] = a[i-1] * 2.0f;  // a[i] dépend de a[i-1]
```

Les réductions (`sum += data[i]`) sont un cas spécial : il y a une dépendance portée (la variable `sum`), mais le compilateur sait transformer les réductions en opérations SIMD grâce à l'associativité de l'addition (avec `-ffast-math` ou `-O3` pour les flottants).

### 4. Absence de branchements dans le corps de boucle

Les branchements à l'intérieur d'une boucle compliquent la vectorisation. Si chaque élément prend un chemin différent, le SIMD ne peut pas appliquer la même instruction à tous.

```cpp
// ❌ Branchement → vectorisation compromise
for (std::size_t i = 0; i < N; ++i) {
    if (data[i] > 0)
        result[i] = data[i] * 2.0f;
    else
        result[i] = 0.0f;
}

// ✅ Version branchless → vectorisable (le compilateur utilise des masques SIMD)
for (std::size_t i = 0; i < N; ++i) {
    result[i] = (data[i] > 0) ? data[i] * 2.0f : 0.0f;
}
```

Le compilateur peut parfois vectoriser la version branchless en utilisant des **masques SIMD** : il calcule les deux résultats, génère un masque de bits indiquant quelle branche s'applique à chaque élément, et fusionne (*blend*) les résultats. C'est exactement le prolongement SIMD de la technique branchless vue en section 41.2.

### 5. Alignement des données

Les instructions de chargement SIMD alignées (`_mm256_load_ps`) exigent que l'adresse source soit un multiple de la largeur du registre (32 octets pour AVX). Les instructions non alignées (`_mm256_loadu_ps`) fonctionnent sur des adresses quelconques mais peuvent être légèrement plus lentes sur les anciens CPU.

Sur les architectures modernes (Intel depuis Haswell, AMD depuis Zen), la pénalité pour les accès non alignés a presque disparu — **sauf** si l'accès chevauche une frontière de cache line (64 octets). Pour les performances optimales, aligner les buffers sur 64 octets couvre tous les cas :

```cpp
// Allocation alignée sur 64 octets (cache line + AVX-512)
float* buffer = static_cast<float*>(std::aligned_alloc(64, N * sizeof(float)));
// ...
std::free(buffer);

// Ou avec un std::vector et un allocateur aligné (C++17) :
#include <memory_resource>
alignas(64) std::byte pool[N * sizeof(float)];
// ... ou utiliser un allocateur custom avec alignas
```

> 💡 **En pratique**, l'alignement n'est plus le facteur limitant qu'il était avant 2013. Les chargements non alignés (`loadu`) ont un coût quasi nul sur les CPU modernes tant qu'ils ne chevauchent pas une frontière de cache line. Concentrez vos efforts sur la contiguïté et la structure des données plutôt que sur l'alignement seul.

---

## Vérifier la vectorisation du compilateur

Avant d'écrire des intrinsics, il est essentiel de vérifier ce que le compilateur fait *déjà*. Les deux compilateurs majeurs offrent des rapports de vectorisation détaillés.

### Rapports Clang

```bash
clang++ -O2 -march=native \
    -Rpass=loop-vectorize \
    -Rpass-missed=loop-vectorize \
    -Rpass-analysis=loop-vectorize \
    -c mon_fichier.cpp
```

Sortie typique :

```
mon_fichier.cpp:12:5: remark: vectorized loop (vectorization width: 8, interleaved count: 2)
    [-Rpass=loop-vectorize]
    for (std::size_t i = 0; i < n; ++i)
    ^
mon_fichier.cpp:20:5: remark: loop not vectorized: could not determine number 
    of loop iterations [-Rpass-missed=loop-vectorize]
```

Le rapport indique la largeur de vectorisation (8 = AVX avec des float 32-bit), le facteur d'entrelacement (*interleaving* — le compilateur traite plusieurs vecteurs par itération pour masquer la latence), et les raisons d'échec pour les boucles non vectorisées.

### Rapports GCC

```bash
g++ -O2 -march=native \
    -fopt-info-vec-optimized \
    -fopt-info-vec-missed \
    -c mon_fichier.cpp
```

Sortie typique :

```
mon_fichier.cpp:12:21: optimized: loop vectorized using 32 byte vectors  
mon_fichier.cpp:20:21: missed: couldn't vectorize loop  
mon_fichier.cpp:20:21: missed: not vectorized: complicated access pattern.  
```

### Inspection sur Compiler Explorer

Le moyen le plus direct de vérifier la vectorisation est d'inspecter le code assembleur sur [godbolt.org](https://godbolt.org). Les instructions SIMD sont reconnaissables à leurs préfixes :

| Préfixes assembleur | Jeu d'instructions | Largeur |
|---------------------|-------------------|---------|
| `addps`, `mulps`, `movaps` | SSE (128-bit) | `xmm` |
| `vaddps`, `vmulps`, `vmovaps` | AVX (256-bit) | `ymm` |
| `vaddps zmm...` | AVX-512 (512-bit) | `zmm` |

Si la boucle génère des instructions `vmulps ymm...` avec `-march=native` sur un CPU AVX2, la vectorisation a réussi. Si elle génère des `mulss xmm...` (scalaire), elle a échoué.

---

## Flags de compilation pour le SIMD

### Le flag critique : `-march=native`

Par défaut, GCC et Clang ciblent un jeu d'instructions minimal (SSE2 sur x86-64). Pour exploiter l'AVX2 ou l'AVX-512 de la machine hôte :

```bash
# Détecter automatiquement le CPU hôte et activer toutes ses extensions
g++ -O2 -march=native -o prog prog.cpp

# Cibler explicitement AVX2 (portable sur tout CPU ≥ Haswell/Excavator)
g++ -O2 -march=haswell -o prog prog.cpp

# Cibler AVX-512 (uniquement pour les plateformes supportées)
g++ -O2 -march=skylake-avx512 -o prog prog.cpp
```

> ⚠️ **`-march=native` produit un binaire non portable** — il ne fonctionnera que sur un CPU avec les mêmes extensions (ou un surensemble). Pour distribuer un binaire à des utilisateurs variés, cibler explicitement `-march=haswell` (AVX2) ou utiliser le *runtime dispatch* (voir ci-dessous).

### Runtime dispatch : choisir le code au lancement

Pour distribuer un binaire unique qui exploite le meilleur jeu d'instructions disponible, on peut compiler plusieurs versions d'une fonction et sélectionner la bonne au démarrage :

```cpp
// GCC : attribut target_clones — génère automatiquement plusieurs versions
__attribute__((target_clones("avx512f", "avx2", "sse4.2", "default")))
void process(float* data, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i)
        data[i] = std::sqrt(data[i]);
}
// Le linker génère un resolver qui détecte le CPU au démarrage
// et branche vers la meilleure version disponible
```

Clang offre un mécanisme similaire via `__attribute__((target("avx2")))` combiné à des checks `__builtin_cpu_supports()`.

### Flags de vectorisation agressifs

```bash
# Autoriser le compilateur à réordonner les opérations flottantes
# (rompt la conformité IEEE 754 stricte, mais permet la vectorisation
# des réductions et d'autres patterns)
g++ -O2 -march=native -ffast-math -o prog prog.cpp

# Alternative plus contrôlée : seulement les réassociations
g++ -O2 -march=native -funsafe-math-optimizations -o prog prog.cpp

# -O3 active des transformations supplémentaires (déroulement agressif,
# vectorisation de boucles non comptées, etc.)
g++ -O3 -march=native -o prog prog.cpp
```

Le flag `-ffast-math` est puissant mais dangereux : il autorise le compilateur à réordonner les additions flottantes (ce qui change le résultat en raison des erreurs d'arrondi), à remplacer les divisions par des multiplications réciproques, et à supposer que les NaN et infinis n'existent pas. À utiliser avec discernement, et uniquement sur les boucles où la précision IEEE stricte n'est pas requise.

---

## Speedup théorique vs réel

Le speedup théorique du SIMD est le rapport entre la largeur du registre et la taille de l'élément :

| Type | Taille | SSE (128-bit) | AVX2 (256-bit) | AVX-512 (512-bit) |
|------|--------|---------------|-----------------|---------------------|
| `float` | 4 octets | 4× | 8× | 16× |
| `double` | 8 octets | 2× | 4× | 8× |
| `int32_t` | 4 octets | 4× | 8× | 16× |
| `int16_t` | 2 octets | 8× | 16× | 32× |
| `int8_t` | 1 octet | 16× | 32× | 64× |

En pratique, le speedup réel est inférieur pour plusieurs raisons :

**Bande passante mémoire.** Si la boucle est limitée par le débit RAM (et non par le calcul), des instructions plus larges ne font que demander les données plus vite — sans les obtenir plus vite. Le SIMD est le plus efficace sur les boucles ***compute-bound*** (ratio calcul/mémoire élevé).

**Overhead de gestion.** Le code de prélude (alignement, masquage du reste) et de fin de boucle (traitement des éléments restants) ajoute un surcoût fixe. Pour des boucles très courtes (< 32 éléments), ce surcoût peut annuler le gain.

**Instructions non vectorisables.** Si le corps de boucle contient un appel de fonction non-inline, une division dépendante, ou un accès indirect (*gather*), le speedup SIMD est réduit.

**Throttling.** Comme évoqué plus haut, les instructions AVX larges peuvent réduire la fréquence du CPU.

En ordre de grandeur réaliste sur du code bien structuré :

| Situation | Speedup réel (AVX2) |
|-----------|---------------------|
| Boucle compute-bound, float, pas de dépendances | 5–7× |
| Boucle mixte compute/memory | 3–5× |
| Boucle memory-bound | 1–2× |
| Code avec gather/scatter | 1,5–3× |

---

## Les deux sous-sections

### 41.3.1 — Intrinsics

Le contrôle manuel du SIMD. Vous apprendrez la nomenclature des intrinsics Intel (`_mm256_*`), les types de données SIMD (`__m128`, `__m256`, `__m512`), les opérations de chargement/stockage, arithmétiques, de comparaison et de réarrangement (*shuffle*, *permute*, *blend*). Cette sous-section couvre aussi les techniques de gestion du reste de boucle (*epilog*) et les opérations horizontales (réduction d'un vecteur en une seule valeur).

### 41.3.2 — Auto-vectorisation du compilateur

Comment écrire du C++ que le compilateur vectorise efficacement *sans* intrinsics. Vous verrez les patterns favorables (boucles simples, SoA, absence de dépendances), les patterns bloquants (aliasing de pointeurs, appels de fonctions, effets de bord), et les annotations qui aident le compilateur (`restrict`, `#pragma omp simd`, `#pragma GCC ivdep`). Cette sous-section est la plus rentable en pratique : un petit changement structurel peut débloquer la vectorisation sur l'ensemble d'un programme.

---

## Résumé

| Aspect | Détail |
|--------|--------|
| **Principe** | Appliquer une même opération à 4–16+ valeurs en une instruction |
| **Jeu d'instructions recommandé (2026)** | AVX2 (256 bits) — bon compromis performance/universalité |
| **Speedup théorique (float, AVX2)** | 8× |
| **Speedup réel typique** | 3–7× selon le ratio compute/memory |
| **Approche prioritaire** | Auto-vectorisation (laisser le compilateur faire) |
| **Dernier recours** | Intrinsics manuels sur les hotspots |
| **Conditions nécessaires** | Données contiguës, homogènes, itérations indépendantes, pas de branchements |
| **Flag minimal** | `-O2 -march=native` |
| **Vérification** | Rapports `-Rpass=loop-vectorize` (Clang) / `-fopt-info-vec` (GCC), Compiler Explorer |

---


⏭️ [Intrinsics](/41-optimisation-cpu-memoire/03.1-intrinsics.md)
