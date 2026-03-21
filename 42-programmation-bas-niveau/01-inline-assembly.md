🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 42.1 — Inline Assembly en C++

> **Niveau** : Expert  
> **Prérequis** : Chapitre 1.3 (Cycle de compilation), Chapitre 41 (Optimisation CPU et Mémoire), notions élémentaires d'assembleur x86-64  
> **Fichiers source** : `42-programmation-bas-niveau/01-inline-assembly/`

---

## Introduction

L'inline assembly permet d'insérer des instructions en langage assembleur directement dans du code source C++. Le compilateur intègre alors ces instructions telles quelles (ou presque) dans le flux de code machine qu'il génère, sans passer par un fichier assembleur externe.

Cette technique existe depuis les premiers compilateurs C et reste disponible en C++ via le mot-clé `asm` (ou `__asm__` en mode strict). GCC et Clang partagent la même syntaxe étendue, dite *extended asm*, qui constitue le standard de fait sur Linux. MSVC utilise une syntaxe complètement différente (`__asm { }`) que nous n'aborderons pas ici — notre contexte est Ubuntu avec GCC 15 et Clang 20.

L'inline assembly est un outil de dernier recours. Dans la très grande majorité des cas, le compilateur génère un code aussi bon — voire meilleur — que ce qu'un humain écrirait à la main, car il dispose d'une vision globale sur l'allocation des registres, le scheduling des instructions et les opportunités d'optimisation inter-procédurales. Mais il existe des situations où l'assembleur inline reste indispensable.

---

## Quand l'inline assembly est-il justifié ?

Avant de plonger dans la syntaxe, il est essentiel de comprendre les rares cas où cette technique apporte une valeur réelle.

### Cas légitimes

**Accès à des instructions spécialisées sans intrinsic disponible.** Les processeurs modernes possèdent des instructions qui n'ont pas d'équivalent en C++ standard ni même en *compiler intrinsics*. Par exemple, l'instruction `rdtsc` (Read Time-Stamp Counter) lit le compteur de cycles du processeur avec une précision au cycle près. Bien que certains compilateurs fournissent `__rdtsc()`, l'inline assembly permet de contrôler exactement comment cette lecture est effectuée et sérialisée.

**Manipulation directe de registres système.** Sur les systèmes embarqués ou dans le code noyau, il est parfois nécessaire de lire ou écrire des registres de contrôle du processeur (registres MSR, registres de segments, etc.) qui ne sont accessibles que par des instructions privilégiées spécifiques.

**Barrières mémoire et instructions de sérialisation.** L'instruction `mfence` (memory fence) sur x86-64 ou `cpuid` (utilisée comme barrière de sérialisation) sont parfois insérées manuellement pour garantir un ordre d'exécution précis. Cependant, dans la majorité des cas, les `std::atomic_thread_fence` du modèle mémoire C++ (section 42.3) rendent cette approche inutile.

**Vérification pédagogique.** Comprendre l'inline assembly aide à lire le code généré par le compilateur (via `objdump` ou Compiler Explorer) et à comprendre ce qui se passe réellement au niveau machine.

### Cas où l'inline assembly est une mauvaise idée

**Optimisation de boucles de calcul.** Le compilateur, armé de l'auto-vectorisation, du déroulage de boucles, du software pipelining et du PGO (section 41.4), produit presque toujours un meilleur code que l'assembleur écrit à la main. De plus, le code inline assembly est une « boîte noire » pour l'optimiseur : il ne peut ni réordonner les instructions environnantes, ni propager les constantes à travers le bloc `asm`, ni éliminer du code mort autour de celui-ci.

**Code portable.** L'inline assembly est par définition lié à une architecture (x86-64, ARM, RISC-V…). Tout code contenant du `asm` doit être isolé derrière des `#ifdef` ou des wrappers, ce qui alourdit considérablement la maintenance.

**Ce qui peut être fait avec des intrinsics.** Les intrinsics SIMD (`_mm256_add_ps`, `_mm_crc32_u64`…) ou les builtins du compilateur (`__builtin_popcount`, `__builtin_ctz`…) sont préférables dans tous les cas, car le compilateur conserve sa capacité d'optimisation autour de ces appels.

---

## Syntaxe de base : `asm` simple

La forme la plus simple de l'inline assembly est le *basic asm*. Elle insère une chaîne d'instructions sans interaction avec les variables C++ :

```cpp
// Basic asm — aucune interaction avec les variables C++
asm("nop");          // Instruction NOP (no operation)  
asm("cli");          // Désactive les interruptions (nécessite le ring 0)  
```

Le mot-clé `asm` est un mot-clé standard C++. `__asm__` est la variante GNU qui reste disponible même avec `-std=c++23 -pedantic` (où `asm` peut produire un warning selon le contexte).

```cpp
__asm__("nop");      // Équivalent, compatible -pedantic
```

Le basic asm a un usage très limité car il ne peut ni lire ni écrire de variables C++. Pour toute interaction avec le code environnant, il faut utiliser la syntaxe étendue.

---

## Syntaxe étendue : Extended Asm

La syntaxe étendue est le cœur de l'inline assembly sur GCC/Clang. Elle permet de déclarer explicitement quelles variables C++ sont lues, écrites ou détruites par le bloc assembleur.

### Structure générale

```cpp
asm [volatile] [goto] (
    "instructions assembleur"
    : /* sorties (outputs) */
    : /* entrées (inputs) */
    : /* clobbers (registres/mémoire détruits) */
);
```

Les quatre sections sont séparées par des deux-points `:`. Les sections vides peuvent être omises en fin de liste, mais les deux-points intermédiaires doivent être présents si une section ultérieure est remplie.

### Un premier exemple commenté

Additionnons deux entiers via l'instruction `add` x86-64 :

```cpp
#include <cstdint>
#include <print>

int main() {
    int64_t a = 42;
    int64_t b = 18;
    int64_t result;

    asm(
        "movq %[src1], %[dst]\n\t"   // dst = a
        "addq %[src2], %[dst]"        // dst += b
        : [dst] "=r" (result)         // Sortie : result dans un registre
        : [src1] "r" (a),             // Entrée : a dans un registre
          [src2] "r" (b)              // Entrée : b dans un registre
        : /* pas de clobbers */
    );

    std::println("result = {}", result);  // Affiche : result = 60
    return 0;
}
```

Décomposons chaque élément.

---

## Les contraintes d'entrée/sortie (Operand Constraints)

Les contraintes indiquent au compilateur comment faire le lien entre les variables C++ et les opérandes de l'assembleur. C'est la partie la plus importante — et la plus subtile — de l'inline assembly.

### Syntaxe d'un opérande

Chaque opérande suit le format :

```
[nom_symbolique] "contrainte" (expression_cpp)
```

Le **nom symbolique** (entre crochets) est facultatif mais fortement recommandé. Sans lui, on référence les opérandes par leur numéro d'ordre (`%0`, `%1`, `%2`…), ce qui rend le code fragile et illisible dès qu'il y a plus de deux opérandes.

### Contraintes de sortie (outputs)

Les opérandes de sortie commencent toujours par `=` (écriture seule) ou `+` (lecture-écriture) :

| Préfixe | Signification | Exemple |
|---------|---------------|---------|
| `=r` | Écriture seule dans un registre général | `[out] "=r" (result)` |
| `+r` | Lecture-écriture dans un registre général | `[val] "+r" (counter)` |
| `=m` | Écriture seule en mémoire | `[out] "=m" (buffer[0])` |
| `+m` | Lecture-écriture en mémoire | `[val] "+m" (flag)` |

### Contraintes d'entrée (inputs)

Les opérandes d'entrée n'ont pas de préfixe `=` ou `+` :

| Contrainte | Signification | Exemple |
|------------|---------------|---------|
| `"r"` | Registre général quelconque | `[in] "r" (value)` |
| `"m"` | Adresse mémoire | `[in] "m" (array[i])` |
| `"i"` | Constante entière immédiate | `[shift] "i" (8)` |
| `"a"` | Registre `rax`/`eax` spécifiquement | `[in] "a" (dividend)` |
| `"d"` | Registre `rdx`/`edx` spécifiquement | `[in] "d" (high_bits)` |
| `"c"` | Registre `rcx`/`ecx` spécifiquement | `[in] "c" (shift_count)` |

Les contraintes de registre nommé (`"a"`, `"d"`, `"c"`, `"b"`, `"S"`, `"D"`) sont spécifiques à x86/x86-64. Elles sont nécessaires lorsque l'instruction assembleur exige un registre précis — par exemple, `div` utilise implicitement `rax` et `rdx`.

### Contraintes multiples

Un opérande peut accepter plusieurs alternatives, séparées par des virgules à l'intérieur de la contrainte. Le compilateur choisit la plus efficace :

```cpp
// Le compilateur choisira un registre OU une adresse mémoire
asm("incl %[val]"
    : [val] "+rm" (counter)  // "rm" = registre ou mémoire
);
```

### Le modificateur de taille

Sur x86-64, les suffixes d'instruction (`b`, `w`, `l`, `q`) indiquent la taille de l'opérande. Pour laisser le compilateur choisir le bon suffixe automatiquement, utilisez les modificateurs de taille sur l'opérande :

| Modificateur | Taille | Exemple avec `%[val]` = registre `rax` |
|-------------|--------|------------------------------------------|
| `%b[val]` | 8 bits | `al` |
| `%w[val]` | 16 bits | `ax` |
| `%k[val]` | 32 bits | `eax` |
| `%q[val]` | 64 bits | `rax` |

```cpp
uint64_t val = 0xFF00;  
asm("bswapq %q[v]"         // Byte-swap sur 64 bits  
    : [v] "+r" (val)
);
// val == 0x00FF000000000000
```

---

## Les Clobbers

La liste des clobbers déclare au compilateur les effets de bord du bloc assembleur — c'est-à-dire les ressources qu'il modifie en dehors des opérandes de sortie explicites. Oublier un clobber est l'une des erreurs les plus courantes et les plus insidieuses en inline assembly.

### Clobbers de registres

Si le bloc assembleur modifie un registre qui n'apparaît pas dans les opérandes de sortie, il faut le déclarer en clobber :

```cpp
uint64_t tsc;  
asm volatile(  
    "rdtsc\n\t"                  // Écrit dans edx:eax
    "shlq $32, %%rdx\n\t"       // Décale rdx de 32 bits
    "orq  %%rdx, %%rax"         // Combine dans rax
    : "=a" (tsc)                 // Sortie : rax → tsc
    :                            // Pas d'entrées
    : "rdx"                      // Clobber : rdx est modifié
);
```

Sans le clobber `"rdx"`, le compilateur pourrait stocker une variable importante dans `rdx` juste avant le bloc `asm`, et cette variable serait silencieusement écrasée.

### Clobbers spéciaux

Deux clobbers spéciaux méritent une attention particulière :

**`"memory"`** — Indique que le bloc assembleur lit ou écrit de la mémoire de manière imprévisible (au-delà des opérandes déclarés). Cela force le compilateur à vider (flush) toutes les variables en registre vers la mémoire avant le bloc, et à les recharger après. C'est l'équivalent d'une barrière mémoire pour le compilateur (mais pas pour le processeur — la distinction est cruciale, voir section 42.3).

```cpp
asm volatile("" ::: "memory");  // Barrière compilateur pure
```

**`"cc"`** — Indique que le bloc assembleur modifie les flags du processeur (registre `EFLAGS` sur x86-64). Sur x86, la quasi-totalité des instructions arithmétiques et logiques modifient les flags, donc ce clobber est presque toujours nécessaire. GCC sur x86-64 l'assume implicitement dans la plupart des cas, mais le déclarer explicitement est une bonne pratique et devient obligatoire sur d'autres architectures.

```cpp
int32_t a = 10, b = 3, quotient, remainder;  
asm(  
    "cdq\n\t"                     // Étend le signe de eax dans edx
    "idivl %[divisor]"            // eax = quotient, edx = reste
    : "=a" (quotient),            // Sortie : eax
      "=d" (remainder)            // Sortie : edx
    : "a"  (a),                   // Entrée : eax = a
      [divisor] "r" (b)           // Entrée : b dans un registre
    : "cc"                        // Clobber : flags modifiés
);
// quotient == 3, remainder == 1
```

---

## Le qualificateur `volatile`

Par défaut, le compilateur traite un bloc `asm` étendu comme une « boîte de calcul » pure : si les sorties ne sont jamais utilisées, il peut éliminer le bloc entier (dead code elimination). Il peut également le déplacer, le dupliquer ou le fusionner avec d'autres instances s'il juge que cela optimise le code.

Le qualificateur `volatile` interdit ces transformations :

```cpp
// SANS volatile — le compilateur peut supprimer ce bloc
// s'il juge que 'result' n'est jamais lu
asm("addq %[b], %[a]"
    : [a] "+r" (result)
    : [b] "r" (delta)
);

// AVEC volatile — le bloc est toujours exécuté, exactement une fois,
// à sa position dans le flux de contrôle
asm volatile("addq %[b], %[a]"
    : [a] "+r" (result)
    : [b] "r" (delta)
);
```

**Quand utiliser `volatile`** :

- Le bloc a des effets de bord au-delà de ses sorties déclarées (I/O mappé en mémoire, modification de registres système…).  
- Le bloc doit s'exécuter un nombre exact de fois (benchmarking, timing).  
- Le bloc contient une instruction dont l'exécution elle-même est l'objectif (barrière mémoire, `nop` pour l'alignement, `pause` dans un spin-lock).

En pratique, la majorité des blocs inline assembly en production utilisent `volatile`.

---

## Le qualificateur `goto`

Introduit avec GCC 4.5 et supporté par Clang, `asm goto` permet au bloc assembleur de brancher vers un label C++ :

```cpp
int check_overflow(int a, int b) {
    int result;
    asm goto(
        "addl %[b], %[a]\n\t"
        "jo %l[overflow]"            // Jump if Overflow → label C++
        : [a] "+r" (a)
        : [b] "r" (b)
        : "cc"
        : overflow                   // Liste des labels cibles
    );
    return a;  // Pas d'overflow

overflow:
    return -1; // Overflow détecté
}
```

> ⚠️ **Limitation importante** : avec `asm goto`, les opérandes de sortie n'étaient pas autorisés avant GCC 11. À partir de GCC 11 et Clang 13, les sorties sont acceptées dans un `asm goto`, mais le comportement peut varier selon la version du compilateur. Testez systématiquement avec vos versions cibles.

---

## Cas d'usage concrets

### Lecture du Time-Stamp Counter (RDTSC)

Le compteur TSC est l'un des cas d'usage les plus classiques de l'inline assembly. L'instruction `rdtscp` (notez le `p` — *serializing*) lit le compteur tout en sérialisant les instructions précédentes, ce qui donne une mesure plus fiable :

```cpp
#include <cstdint>

struct TscReading {
    uint64_t ticks;
    uint32_t aux;     // Identifiant du cœur CPU (IA32_TSC_AUX)
};

inline TscReading rdtscp() {
    uint32_t lo, hi, aux;
    asm volatile(
        "rdtscp"
        : "=a" (lo),        // EAX = 32 bits bas du TSC
          "=d" (hi),        // EDX = 32 bits hauts du TSC
          "=c" (aux)        // ECX = IA32_TSC_AUX (core ID)
        :
        : /* rdtscp ne clobber que eax, edx, ecx — déjà en sorties */
    );
    return {
        (static_cast<uint64_t>(hi) << 32) | lo,
        aux
    };
}
```

L'utilisation typique encadre la section de code à mesurer :

```cpp
auto start = rdtscp();
// ... code à chronométrer ...
auto end = rdtscp();  
uint64_t cycles = end.ticks - start.ticks;  
```

> 💡 **Alternative sans inline assembly** : GCC et Clang fournissent l'intrinsic `__builtin_ia32_rdtscp(&aux)` et le header `<x86intrin.h>` expose `__rdtscp()`. Ces alternatives sont préférables en production car elles laissent au compilateur plus de latitude pour l'optimisation.

### Empêcher l'optimisation d'une variable (Benchmark Escape)

Lors du benchmarking, le compilateur peut éliminer un calcul dont le résultat n'est jamais « observé ». L'inline assembly permet de forcer le compilateur à considérer une variable comme utilisée, sans générer aucune instruction :

```cpp
// Force le compilateur à matérialiser 'val' dans un registre
// mais ne génère AUCUNE instruction machine
template <typename T>  
inline void do_not_optimize(T& val) {  
    asm volatile("" : "+r"(val) : : "memory");
}
```

Ce pattern est utilisé par Google Benchmark sous le nom `benchmark::DoNotOptimize`. Le `"memory"` clobber empêche en plus le compilateur de réordonner les accès mémoire autour de cet appel.

```cpp
int result = compute_something(input);  
do_not_optimize(result);  // Le compilateur ne peut pas éliminer compute_something  
```

### Instruction `pause` pour les spin-locks

L'instruction `pause` sur x86 signale au processeur qu'il est dans une boucle d'attente active (spin-wait). Elle réduit la consommation d'énergie et évite les pénalités liées au pipeline de spéculation :

```cpp
inline void cpu_relax() {
    asm volatile("pause" ::: "memory");
}

// Utilisation dans un spin-lock simplifié
void spin_wait(std::atomic<bool>& flag) {
    while (flag.load(std::memory_order_relaxed)) {
        cpu_relax();
    }
}
```

> 💡 **Alternative** : depuis C++20, `std::atomic::wait()` offre une approche plus portable et souvent plus efficace (attente passive via le noyau). L'instruction `pause` manuelle reste pertinente dans les chemins ultra-critiques en latence où le passage par le noyau est inacceptable.

---

## Bonnes pratiques

### 1. Préférer les intrinsics et builtins

Avant d'écrire de l'inline assembly, vérifiez si un builtin ou un intrinsic existe :

```cpp
// ❌ Inline assembly pour compter les bits à 1
uint32_t count;  
asm("popcntl %[in], %[out]"  
    : [out] "=r" (count)
    : [in]  "r"  (value)
);

// ✅ Builtin — le compilateur optimise et gère la portabilité
uint32_t count = __builtin_popcount(value);

// ✅✅ C++20 — standard et portable
#include <bit>
uint32_t count = std::popcount(value);
```

### 2. Isoler l'assembleur dans des fonctions inline dédiées

Ne dispersez jamais de blocs `asm` au milieu de la logique métier. Encapsulez-les dans des fonctions `inline` (ou `__attribute__((always_inline))`) avec un nom explicite :

```cpp
// Bien : fonction dédiée, nommée, documentée
[[gnu::always_inline]]
inline uint64_t read_tsc() {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

// Mal : asm en plein milieu du code applicatif
void process_request(Request& req) {
    // ... logique métier ...
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));  // 😬
    // ... logique métier ...
}
```

### 3. Protéger par des gardes d'architecture

L'inline assembly x86-64 ne compile pas sur ARM et vice versa. Utilisez systématiquement des macros de détection d'architecture :

```cpp
#if defined(__x86_64__) || defined(_M_X64)
inline void memory_fence() {
    asm volatile("mfence" ::: "memory");
}
#elif defined(__aarch64__)
inline void memory_fence() {
    asm volatile("dmb ish" ::: "memory");
}
#else
inline void memory_fence() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}
#endif
```

### 4. Utiliser les noms symboliques, toujours

Les opérandes numérotés (`%0`, `%1`, `%2`) rendent le code illisible et fragile — ajouter un opérande décale tous les numéros :

```cpp
// ❌ Opérandes numérotés — fragile et illisible
asm("addq %2, %0" : "=r"(result) : "0"(a), "r"(b));

// ✅ Noms symboliques — clair et maintenable
asm("addq %[rhs], %[lhs]"
    : [lhs] "=r" (result)
    : "0"   (a),             // "0" lie cet input au même registre que l'output 0
      [rhs] "r"  (b)
);
```

### 5. Vérifier le code généré

Après chaque modification d'un bloc inline assembly, vérifiez le code machine produit. Le moyen le plus rapide est Compiler Explorer (godbolt.org), mais vous pouvez aussi utiliser `objdump` sur le binaire compilé :

```bash
# Compiler avec les symboles de debug
g++ -std=c++23 -O2 -g -o my_program my_program.cpp

# Désassembler la fonction ciblée
objdump -d -S --no-show-raw-insn my_program | grep -A 30 '<read_tsc>'
```

L'option `-S` intercale le code source avec l'assembleur quand les informations de debug sont disponibles.

---

## Pièges courants

### Clobbers manquants

Le piège le plus dangereux. Le code semble fonctionner en `-O0` (où le compilateur ne garde presque rien en registre) puis plante mystérieusement en `-O2` quand le compilateur optimise l'allocation des registres :

```cpp
// ❌ BUG : rdtsc écrit dans edx mais edx n'est ni en sortie ni en clobber
uint32_t lo;  
asm volatile("rdtsc" : "=a"(lo));  
// Le compilateur peut utiliser edx pour autre chose → corruption silencieuse

// ✅ CORRECT : edx déclaré en clobber
uint32_t lo;  
asm volatile("rdtsc" : "=a"(lo) : : "edx");  
```

### Confusion entre syntaxe AT&T et Intel

GCC et Clang utilisent par défaut la syntaxe AT&T (opérande source à gauche, destination à droite), qui est l'inverse de la syntaxe Intel :

```nasm
; Syntaxe AT&T (défaut GCC/Clang) : source, destination
movq %rax, %rbx          ; rbx = rax

; Syntaxe Intel : destination, source  
mov rbx, rax              ; rbx = rax
```

Il est possible de passer en syntaxe Intel avec la directive `.intel_syntax noprefix` au début du bloc, mais cela complique l'interaction avec les contraintes d'opérandes. La recommandation est de rester en syntaxe AT&T pour l'inline assembly.

### Oublier `volatile` sur du code à effet de bord

```cpp
// ❌ Le compilateur peut supprimer ce bloc si 'timestamp' n'est pas lu ensuite
uint64_t timestamp;  
asm("rdtsc" : "=A"(timestamp));  

// ✅ volatile empêche l'élimination
uint64_t timestamp;  
asm volatile("rdtsc" : "=A"(timestamp));  
```

### Conflits de registres avec les opérandes early-clobber

Quand une sortie est écrite avant que toutes les entrées n'aient été lues, il faut déclarer cette sortie comme *early clobber* avec le modificateur `&`. Sans cela, le compilateur peut allouer la même registre à une entrée et à cette sortie :

```cpp
// ❌ POTENTIEL BUG : le compilateur peut allouer le même registre
// pour 'out' et 'in2' si 'out' est écrit avant la lecture de 'in2'
asm("movq %[in1], %[out]\n\t"
    "addq %[in2], %[out]"
    : [out] "=r" (result)
    : [in1] "r" (a), [in2] "r" (b)
);

// ✅ CORRECT : '&' indique que 'out' est écrit avant la fin des lectures
asm("movq %[in1], %[out]\n\t"
    "addq %[in2], %[out]"
    : [out] "=&r" (result)       // '&' = early clobber
    : [in1] "r" (a), [in2] "r" (b)
);
```

---

## Inline assembly vs fichier assembleur externe

Pour des blocs de plus de 10–15 instructions, l'inline assembly devient difficile à maintenir. Dans ce cas, un fichier assembleur séparé (`.s` ou `.S`) est préférable :

| Critère | Inline assembly | Fichier `.S` externe |
|---------|-----------------|----------------------|
| Interaction avec les variables C++ | Directe via contraintes | Via conventions d'appel (ABI) |
| Optimisation du code environnant | Limitée (boîte noire) | Non applicable (fonction séparée) |
| Taille du bloc | Quelques instructions | Pas de limite |
| Lisibilité | Correcte si < 10 lignes | Coloration syntaxique native |
| Portabilité | Mêmes gardes `#ifdef` | Fichiers séparés par architecture |
| Débogage | Difficile (pas de step) | Possible avec `gdb` |

L'approche fichier externe se combine bien avec CMake :

```cmake
# CMakeLists.txt
enable_language(ASM)

add_library(low_level
    src/optimized_memcpy.S    # Fichier assembleur
    src/wrapper.cpp           # Wrapper C++ qui appelle les fonctions ASM
)
```

---

## Résumé

L'inline assembly en C++ est un outil puissant mais dangereux. Il permet d'accéder à des instructions que le compilateur ne peut pas émettre seul, mais au prix d'une complexité accrue, d'une portabilité réduite et d'une interaction difficile avec l'optimiseur.

Pour résumer les points essentiels :

- Utilisez la syntaxe étendue (`extended asm`) avec des noms symboliques pour les opérandes.  
- Déclarez scrupuleusement tous les clobbers — un oubli produit des bugs intermittents insaisissables.  
- Ajoutez `volatile` dès que le bloc a un effet de bord ou doit s'exécuter inconditionnellement.  
- Vérifiez systématiquement le code généré avec Compiler Explorer ou `objdump`.  
- Préférez toujours les builtins (`__builtin_*`), les intrinsics (`<x86intrin.h>`), et les fonctions standard C++20/23 (`std::popcount`, `std::atomic_thread_fence`) quand elles couvrent le besoin.

> 📎 *La section suivante, **42.2 — Manipulation de Bits et Bitfields**, explore les opérations bit à bit qui constituent souvent le complément naturel de l'inline assembly dans le code bas niveau.*

⏭️ [Manipulation de bits et bitfields](/42-programmation-bas-niveau/02-manipulation-bits.md)
