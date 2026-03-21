🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 41.5 — Link-Time Optimization (LTO)

> **Chapitre 41 : Optimisation CPU et Mémoire** · Section 5  
> **Niveau** : Expert · **Prérequis** : Section 1.3 (cycle de compilation), Section 41.4 (PGO), Chapitre 26 (CMake)

---

## Introduction

Dans le modèle de compilation traditionnel du C++, chaque fichier source (`.cpp`) est compilé indépendamment en un fichier objet (`.o`). L'optimiseur du compilateur travaille sur chaque unité de compilation isolément — il n'a aucune visibilité sur ce qui se passe dans les autres fichiers. Lorsque l'éditeur de liens (*linker*) assemble les fichiers objets en exécutable, il se contente de résoudre les symboles et de fusionner les sections. Il n'optimise pas.

Cette frontière entre compilation et édition de liens est un obstacle majeur pour l'optimiseur :

- Une fonction définie dans `utils.cpp` et appelée dans `main.cpp` ne peut **jamais** être inlinée, quelle que soit sa taille.  
- Une fonction exportée dans un `.o` ne peut pas être éliminée comme code mort, même si aucun autre fichier ne l'appelle.  
- Le compilateur ne peut pas propager de constantes entre fichiers, ni analyser les alias inter-fichiers, ni réordonner les appels globalement.

Le ***Link-Time Optimization*** (LTO) repousse l'optimisation à l'étape d'édition de liens, quand le compilateur a enfin une **vue globale** de l'ensemble du programme. Au lieu de produire du code machine dans chaque `.o`, le compilateur produit une représentation intermédiaire (IR) que l'éditeur de liens optimise globalement avant de générer le binaire final.

Le résultat : des gains de **5 à 15 %** sur l'ensemble du programme, sans modifier le code source, et une synergie puissante avec le PGO (section 41.4).

---

## Ce que le LTO déverrouille

### Inlining inter-fichiers

C'est le gain le plus significatif. Sans LTO, seules les fonctions définies dans le même fichier (ou dans des headers) peuvent être inlinées. Avec LTO, toute fonction du programme est candidate à l'inlining, quel que soit le fichier où elle est définie.

```
SANS LTO :

  main.cpp                     utils.cpp
  ┌─────────────────────┐       ┌──────────────────────┐
  │ void hot_loop() {   │       │ int compute(int x) { │
  │   for (...)         │       │   return x * x + 1;  │
  │     result +=       │       │ }                    │
  │       compute(val); │──────►│                      │
  │ }                   │ CALL  │ // Jamais inliné     │
  └─────────────────────┘       │ // car dans un autre │
                                │ // fichier .o        │
                                └──────────────────────┘

AVEC LTO :

  Programme global (après fusion des IR)
  ┌────────────────────────────────────────┐
  │ void hot_loop() {                      │
  │   for (...)                            │
  │     result += val * val + 1;  ← INLINÉ │
  │ }                                      │
  └────────────────────────────────────────┘
```

L'inlining inter-fichiers est particulièrement bénéfique pour les projets bien structurés avec de nombreuses petites fonctions utilitaires réparties dans des fichiers séparés — exactement le style encouragé par les bonnes pratiques (chapitre 46).

### Élimination du code mort global (Dead Code Elimination)

Sans LTO, le compilateur ne peut pas savoir si une fonction exportée dans un `.o` est utilisée par un autre fichier. Il la conserve par précaution. Avec LTO, le linker voit l'ensemble du programme et élimine les fonctions, variables et sections qui ne sont jamais référencées.

Sur un programme typique, cela réduit la taille du binaire de **5 à 20 %** — moins de code signifie aussi moins de pression sur le cache d'instructions.

### Propagation de constantes inter-fichiers

```cpp
// config.cpp
const int MAX_BUFFER_SIZE = 4096;

// engine.cpp
extern const int MAX_BUFFER_SIZE;  
void process() {  
    char buffer[MAX_BUFFER_SIZE];  // Sans LTO : taille inconnue à la compilation
                                    // Avec LTO : le compilateur sait que c'est 4096
}
```

Avec LTO, le compilateur peut propager la valeur `4096` dans `engine.cpp` et optimiser en conséquence (allouer sur la stack avec une taille fixe, dérouler les boucles bornées, etc.).

### Dévirtualisation inter-fichiers

Le LTO permet au compilateur de résoudre les appels virtuels lorsque le type concret de l'objet peut être déduit globalement — même si la classe et l'appel sont dans des fichiers différents. Combiné au PGO, cela permet la dévirtualisation spéculative (section 41.4) sur l'ensemble du programme.

### Réordonnancement global des sections

Le linker peut réordonner les fonctions dans le binaire pour maximiser la localité du cache d'instructions. Les fonctions fréquemment appelées en séquence sont placées côte à côte, indépendamment du fichier source où elles sont définies.

---

## Full LTO vs Thin LTO

Il existe deux variantes de LTO, avec des compromis très différents.

### Full LTO

Le mode original. **Toutes** les unités de compilation sont fusionnées en une seule représentation intermédiaire monolithique, que l'optimiseur traite comme un programme unique.

```bash
# GCC : Full LTO
g++ -O2 -flto -o my_program src/*.cpp

# Clang : Full LTO
clang++ -O2 -flto -o my_program src/*.cpp
```

**Avantages :**  
- Optimisation globale maximale — le compilateur a une vision complète.  
- Meilleur gain de performance (le plus haut possible avec LTO).

**Inconvénients :**  
- **Temps de link très long.** La phase de link devient la phase d'optimisation complète du programme entier. Sur un gros projet, cela peut prendre plusieurs minutes, voire des dizaines de minutes.  
- **Consommation mémoire élevée.** L'IR du programme entier est chargée en mémoire. Pour un projet de plusieurs millions de lignes, cela peut dépasser 16–32 GiB de RAM.  
- **Parallélisation limitée.** L'optimisation se fait en un seul flux, utilisant peu de cœurs.

### Thin LTO

Développé par Google et intégré dans LLVM/Clang (et partiellement dans GCC depuis la version 12+), Thin LTO est un compromis conçu pour les très gros projets.

```bash
# Clang : Thin LTO
clang++ -O2 -flto=thin -o my_program src/*.cpp

# GCC : slim LTO (équivalent conceptuel, activé par défaut avec -flto depuis GCC 12)
g++ -O2 -flto=auto -o my_program src/*.cpp
# -flto=auto utilise autant de jobs que de cœurs disponibles
```

**Fonctionnement :**

1. Chaque fichier source produit un fichier objet contenant l'IR *et* un résumé compact (*summary*) de ses fonctions (signatures, fréquences d'appel, tailles).
2. À l'édition de liens, le linker lit les résumés et construit un **graphe d'appels global** — sans charger l'IR complète.
3. Sur la base de ce graphe, il prend des décisions d'importation : quelles fonctions d'un fichier doivent être copiées dans un autre pour permettre l'inlining.
4. Chaque fichier est ensuite **réoptimisé en parallèle**, avec les fonctions importées, par des workers indépendants.

```
Full LTO :
  [fichier1.o] ─┐
  [fichier2.o] ─┼─► [IR monolithique] ─► [optimisation séquentielle] ─► binaire
  [fichier3.o] ─┘

Thin LTO :
  [fichier1.o + summary] ─┐     ┌─► [worker 1 : réoptimise fichier1 + imports] ─┐
  [fichier2.o + summary] ─┼─►───┼─► [worker 2 : réoptimise fichier2 + imports] ─┼─► binaire
  [fichier3.o + summary] ─┘     └─► [worker 3 : réoptimise fichier3 + imports] ─┘
                                 ▲
                          Décisions d'import
                          basées sur les summaries
```

**Avantages :**  
- **Parallélisation massive.** Chaque fichier est réoptimisé indépendamment → utilise tous les cœurs disponibles.  
- **Mémoire réduite.** Seule l'IR d'un fichier + ses imports est en mémoire à la fois.  
- **Temps de build inférieur au Full LTO.** Typiquement 2–3× plus rapide à linker.  
- **Compilation incrémentale possible.** Seuls les fichiers modifiés et leurs dépendants sont réoptimisés.

**Inconvénients :**  
- Gain légèrement inférieur au Full LTO (1–3 % de moins) car l'optimisation n'est pas globalement monolithique.  
- Nécessite un linker compatible (LLD pour Clang, `gold` avec plugin pour GCC).

### Comparaison

| Aspect | Full LTO | Thin LTO |
|--------|----------|----------|
| **Gain performance** | Maximal (5–15 %) | Quasi-maximal (4–13 %) |
| **Temps de link** | Long (séquentiel) | 2–3× plus court (parallèle) |
| **Mémoire à l'édition de liens** | Très élevée | Modérée |
| **Parallélisation** | Faible | Excellente |
| **Build incrémental** | Non | Oui (Clang/LLD) |
| **Recommandé pour** | Petits/moyens projets, release finale | Gros projets, CI/CD |

**Recommandation 2026** : utiliser **Thin LTO** par défaut. Le surcoût en performance par rapport au Full LTO est minime, et le gain en temps de build et en scalabilité est significatif. Réserver le Full LTO pour la build de release finale d'un projet de taille modérée où chaque pourcent compte.

---

## Mise en œuvre avec GCC

### Compilation et link basiques

```bash
# Full LTO
g++ -O2 -march=native -flto -o my_program src/*.cpp

# LTO avec parallélisation automatique (GCC 12+)
# Utilise N jobs parallèles pour la phase LTO
g++ -O2 -march=native -flto=auto -o my_program src/*.cpp

# LTO avec nombre de jobs explicite
g++ -O2 -march=native -flto=8 -o my_program src/*.cpp
```

### Points importants pour GCC

**Le flag `-flto` doit être présent à la compilation ET à l'édition de liens.** Si vous compilez avec `-flto` mais linkez sans, les fichiers `.o` contiennent de l'IR GIMPLE au lieu de code machine — le linker standard échoue.

```bash
# ✅ Correct : -flto aux deux étapes
g++ -O2 -flto -c fichier1.cpp -o fichier1.o  
g++ -O2 -flto -c fichier2.cpp -o fichier2.o  
g++ -O2 -flto fichier1.o fichier2.o -o my_program  

# ❌ Incorrect : -flto manquant au link
g++ -O2 -flto -c fichier1.cpp -o fichier1.o  
g++ -O2 fichier1.o -o my_program    # ERREUR ou résultat dégradé  
```

**GCC nécessite le plugin LTO pour `ar` et `ranlib`** si vous créez des bibliothèques statiques avec LTO :

```bash
# Utiliser gcc-ar et gcc-ranlib au lieu de ar et ranlib
gcc-ar rcs libutils.a utils.o helpers.o  
gcc-ranlib libutils.a  
```

**Les flags d'optimisation doivent être cohérents.** Les options passées au link (`-O2`, `-march=native`) déterminent le niveau d'optimisation appliqué pendant la phase LTO. Passer `-O0` au link annule les optimisations, même si les fichiers ont été compilés avec `-O2`.

---

## Mise en œuvre avec Clang

### Full LTO

```bash
clang++ -O2 -march=native -flto -o my_program src/*.cpp
```

### Thin LTO (recommandé)

```bash
# Thin LTO avec le linker LLD (recommandé pour Clang)
clang++ -O2 -march=native -flto=thin -fuse-ld=lld -o my_program src/*.cpp

# Contrôler le nombre de jobs parallèles Thin LTO
clang++ -O2 -flto=thin -fuse-ld=lld \
    -Wl,--thinlto-jobs=8 \
    -o my_program src/*.cpp
```

### Thin LTO et cache

Clang supporte un **cache Thin LTO** qui stocke les résultats de réoptimisation des fichiers inchangés, accélérant les builds incrémentaux :

```bash
# Activer le cache Thin LTO
clang++ -O2 -flto=thin -fuse-ld=lld \
    -Wl,--thinlto-cache-dir=./thinlto-cache \
    -Wl,--thinlto-cache-policy=cache_size_bytes=1g \
    -o my_program src/*.cpp
```

Le cache fonctionne de manière similaire à ccache (section 2.3), mais au niveau de la phase LTO. Sur un rebuild incrémental, seuls les fichiers dont l'IR ou les imports ont changé sont réoptimisés — les autres sont servis depuis le cache.

---

## Intégration dans CMake

CMake supporte nativement le LTO depuis la version 3.9 via la propriété `INTERPROCEDURAL_OPTIMIZATION` :

```cmake
cmake_minimum_required(VERSION 3.16)  
project(my_project CXX)  

add_executable(my_program src/main.cpp src/engine.cpp src/utils.cpp)

# Activer LTO pour cette cible
set_target_properties(my_program PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
)

# Ou globalement pour toutes les cibles :
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
```

### Vérification du support LTO

CMake peut vérifier si le compilateur supporte LTO avant de l'activer :

```cmake
include(CheckIPOSupported)  
check_ipo_supported(RESULT lto_supported OUTPUT lto_error)  

if(lto_supported)
    message(STATUS "LTO activé")
    set_target_properties(my_program PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
    )
else()
    message(WARNING "LTO non supporté : ${lto_error}")
endif()
```

### Choisir Thin LTO avec CMake

CMake ne distingue pas nativement Full vs Thin LTO. Pour forcer Thin LTO avec Clang :

```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(my_program PRIVATE -flto=thin)
    target_link_options(my_program PRIVATE -flto=thin -fuse-ld=lld)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # GCC : -flto=auto pour la parallélisation
    target_compile_options(my_program PRIVATE -flto=auto)
    target_link_options(my_program PRIVATE -flto=auto)
endif()
```

### Gestion des bibliothèques statiques avec LTO

Si votre projet produit des bibliothèques statiques (`.a`) utilisées par d'autres cibles, les outils d'archivage doivent supporter les fichiers objets LTO :

```cmake
# Pour GCC : utiliser gcc-ar et gcc-ranlib
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_RANLIB "gcc-ranlib")
endif()
```

Avec Clang + LLD, les fichiers `.o` Thin LTO sont directement compatibles avec `llvm-ar`.

---

## Impact sur le temps de build

Le LTO ajoute un coût non négligeable au temps de build, concentré sur la phase de link :

| Phase | Sans LTO | Full LTO | Thin LTO |
|-------|----------|----------|----------|
| Compilation des `.o` | 100 % (référence) | 100 % (fichiers IR au lieu de code machine) | 100 % (+ génération des summaries) |
| Édition de liens | ~5 % du temps total | 50–200 % du temps de compilation | 20–60 % du temps de compilation |
| **Total** | 100 % | 150–300 % | 120–160 % |

Pour un projet de taille moyenne (100 fichiers, 50 000 lignes), le Full LTO peut transformer un link de 2 secondes en 30–60 secondes. Le Thin LTO ramène ce temps à 10–20 secondes avec parallélisation.

### Atténuer le coût

Plusieurs stratégies réduisent l'impact du LTO sur le workflow de développement :

**Activer LTO uniquement en Release.** C'est la pratique standard — le LTO n'apporte rien en debug et ralentit considérablement le cycle edit-compile-run.

```cmake
# Activer LTO uniquement en Release et RelWithDebInfo
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set_target_properties(my_program PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
```

**Utiliser Thin LTO + cache.** Le cache Thin LTO (section précédente) réduit drastiquement le coût des builds incrémentaux.

**Combiner avec ccache.** ccache (section 2.3) cache les résultats de compilation pré-LTO. Si seul un fichier change, seul ce fichier est recompilé — la phase LTO est la seule partie recalculée.

**Utiliser Ninja.** Le générateur Ninja (section 28.3) parallélise mieux les builds que Make, et la phase LTO de GCC (`-flto=auto`) tire parti de cette parallélisation.

---

## LTO et bibliothèques dynamiques

Le LTO a des interactions spécifiques avec les bibliothèques dynamiques (`.so`) :

### Symboles visibles

Par défaut, toutes les fonctions d'une `.so` sont exportées et ne peuvent pas être éliminées par le LTO. Pour permettre l'élimination du code mort dans une bibliothèque partagée, restreindre la visibilité des symboles :

```bash
# Masquer tous les symboles par défaut, exporter uniquement ceux marqués
g++ -O2 -flto -fvisibility=hidden -o libengine.so -shared src/engine.cpp
```

```cpp
// Exporter uniquement les fonctions de l'API publique
#define PUBLIC_API __attribute__((visibility("default")))

PUBLIC_API void api_function();          // exportée  
void internal_helper();                   // masquée, éliminable par LTO  
```

### LTO à travers les frontières de `.so`

Le LTO ne traverse **pas** les frontières des bibliothèques dynamiques. Les optimisations inter-fichiers (inlining, propagation de constantes) ne s'appliquent qu'au sein d'une même unité de linkage — soit un exécutable, soit une `.so`. Pour maximiser l'impact du LTO, préférer le linkage statique des bibliothèques internes.

---

## Debugging avec LTO

Le LTO complique le debugging car les optimisations inter-fichiers (inlining, réordonnancement, élimination de code) rendent la correspondance entre le code source et le code machine moins directe.

### Informations de debug

Les flags `-g` et `-flto` sont compatibles :

```bash
g++ -O2 -g -flto -o my_program src/*.cpp
```

Le binaire résultant contient des informations DWARF utilisables par GDB. L'inlining inter-fichiers est visible dans les backtraces :

```gdb
(gdb) bt
#0  compute (x=42) at utils.cpp:5           ← inlinée dans hot_loop
#1  hot_loop () at main.cpp:12
```

### Builds LTO debuggables

Pour un meilleur compromis debugging/performance, utiliser `-Og` avec LTO :

```bash
g++ -Og -g -flto -o my_program_debug src/*.cpp
# -Og : optimise sans compromettre la debuggabilité
```

Ou utiliser le build type `RelWithDebInfo` dans CMake, qui combine `-O2 -g`.

---

## Pièges courants

### Incompatibilité entre compilateurs

Les fichiers objets LTO contiennent de l'IR spécifique au compilateur (GIMPLE pour GCC, LLVM IR pour Clang). **Il est impossible de mélanger des `.o` LTO de GCC et Clang dans le même link.** Tous les fichiers objets d'un programme LTO doivent être compilés par le même compilateur, à la même version majeure.

### Incompatibilité entre versions de GCC

Les fichiers `.o` LTO de GCC ne sont pas garantis compatibles entre versions majeures (par exemple GCC 14 et GCC 15). Si vous distribuez des bibliothèques statiques compilées avec LTO, assurez-vous que l'utilisateur final utilise la même version de GCC — ou distribuez des `.o` sans LTO comme fallback.

### Flags incohérents entre fichiers

Si certains fichiers sont compilés avec `-O2` et d'autres avec `-O0`, le comportement LTO est mal défini (GCC utilise le flag du link, Clang peut se comporter différemment). Assurer la cohérence des flags sur toutes les unités de compilation.

### Augmentation de la taille du binaire

L'inlining agressif du LTO peut *augmenter* la taille du code. Pour la plupart des programmes, l'élimination du code mort compense largement. Mais sur des projets avec beaucoup de petites fonctions appelées depuis de nombreux endroits, l'inlining excessif peut gonfler le binaire et dégrader la performance du cache L1i.

Si le binaire grossit significativement avec LTO, ajouter `-finline-limit=N` ou `-mllvm -inline-threshold=N` pour limiter l'inlining. Ou mesurer : si le programme est plus rapide malgré le binaire plus gros, l'inlining est bénéfique.

---

## Résumé

| Aspect | Détail |
|--------|--------|
| **Principe** | Optimiser à l'édition de liens avec une vue globale du programme |
| **Gain typique** | 5–15 % sans modification du code source |
| **Full LTO** | Optimisation maximale, mais lent et gourmand en mémoire |
| **Thin LTO** | Quasi-même gain, parallèle, scalable — **recommandé** |
| **GCC** | `-flto=auto` (parallèle), `gcc-ar`/`gcc-ranlib` pour les `.a` |
| **Clang** | `-flto=thin -fuse-ld=lld` + cache optionnel |
| **CMake** | `INTERPROCEDURAL_OPTIMIZATION TRUE` |
| **Combinaison optimale** | LTO + PGO + `-march=native` = 15–30 % de gain total |
| **Quand l'activer** | Builds Release uniquement |
| **Piège principal** | Tous les `.o` doivent venir du même compilateur + même version |

---


⏭️ [std::flat_map/flat_set et performance cache](/41-optimisation-cpu-memoire/06-flat-containers-perf.md)
