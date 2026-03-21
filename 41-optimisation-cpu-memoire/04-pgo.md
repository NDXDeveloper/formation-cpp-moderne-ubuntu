🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 41.4 — Profile-Guided Optimization (PGO)

> **Chapitre 41 : Optimisation CPU et Mémoire** · Section 4  
> **Niveau** : Expert · **Prérequis** : Sections 41.1–41.3 (cache, branchements, SIMD), Chapitre 26 (CMake), Chapitre 31 (profiling)

---

## Introduction

Les sections précédentes ont présenté des optimisations que le développeur effectue *manuellement* : réorganiser les données (SoA), éliminer les branchements (branchless), aider la vectorisation (pragmas). Toutes ces techniques exigent d'identifier les hotspots, de comprendre le problème, et de modifier le code source.

Le ***Profile-Guided Optimization*** (PGO) prend l'approche inverse : **laisser le compilateur observer l'exécution réelle du programme, puis recompiler en exploitant ces observations**. Le compilateur ne devine plus quelles branches sont probables ni quelles fonctions sont chaudes — il le *sait*, grâce à des données de profiling collectées lors d'une exécution représentative.

Le résultat est un binaire dont le layout du code, les décisions d'inlining, les prédictions de branchement et les paramètres de vectorisation sont calibrés sur le comportement réel du programme. Les gains typiques sont de **10 à 25 %** sur l'ensemble du programme, sans modifier une seule ligne de code source. Sur certains workloads (compilateurs, navigateurs, bases de données), les gains atteignent 30–40 %.

---

## Principe : le workflow en trois phases

Le PGO se déroule en trois étapes distinctes :

```
 Phase 1 : INSTRUMENTATION          Phase 2 : PROFILING           Phase 3 : OPTIMISATION
┌───────────────────────┐       ┌───────────────────────┐       ┌────────────────────────┐
│                       │       │                       │       │                        │
│  Code source (.cpp)   │       │  Binaire instrumenté  │       │  Code source (.cpp)    │
│         +             │       │         +             │       │         +              │
│  Compilation avec     │──────►│  Exécution sur un     │──────►│  Recompilation avec    │
│  -fprofile-generate   │       │  workload réaliste    │       │  -fprofile-use         │
│                       │       │                       │       │         +              │
│  → Binaire instrumenté│       │  → Fichiers .gcda /   │       │  Données de profiling  │
│    (plus lent ~15-30%)│       │    .profdata          │       │                        │
└───────────────────────┘       └───────────────────────┘       │  → Binaire OPTIMISÉ    │
                                                                │    (10-25% plus rapide)│
                                                                └────────────────────────┘
```

**Phase 1 — Instrumentation.** Le compilateur insère des compteurs dans le binaire : à chaque branchement, à chaque appel de fonction, à chaque entrée de boucle. Le binaire résultant est fonctionnellement identique mais ~15–30 % plus lent à cause de l'overhead des compteurs.

**Phase 2 — Profiling.** Le binaire instrumenté est exécuté sur un workload *représentatif* du cas d'usage réel. Les compteurs enregistrent les fréquences de passage de chaque branchement, le nombre d'itérations de chaque boucle, et le nombre d'appels de chaque fonction. Ces données sont écrites dans des fichiers de profiling.

**Phase 3 — Optimisation guidée.** Le compilateur recompile le code source en lisant les fichiers de profiling. Il utilise ces données pour prendre des décisions d'optimisation informées au lieu de se fier à des heuristiques statiques.

---

## Ce que le PGO optimise concrètement

Le compilateur utilise les données de profiling pour ajuster plusieurs aspects du code généré :

### Layout du code (hot/cold splitting)

Les fonctions et blocs de base fréquemment exécutés (*hot*) sont regroupés dans des sections contiguës du binaire. Les chemins rarement pris (*cold*) — gestion d'erreurs, cas limites — sont déplacés dans des sections séparées. Cela améliore la localité du cache d'instructions (L1i) en concentrant le code chaud dans un espace mémoire réduit.

```
SANS PGO :                          AVEC PGO :
┌──────────────────┐                ┌──────────────────┐
│ func_a (hot)     │                │ .text.hot        │
│ func_b (cold)    │                │   func_a         │
│ func_c (hot)     │                │   func_c         │
│ func_d (cold)    │                │   func_e         │
│ func_e (hot)     │                │ .text.unlikely   │
│ error_handler    │                │   func_b         │
└──────────────────┘                │   func_d         │
                                    │   error_handler  │
Code hot et cold                    └──────────────────┘  
entrelacés                          Code hot contigu →  
→ cache L1i pollué                  meilleure localité L1i
```

### Inlining calibré

Sans PGO, le compilateur utilise des heuristiques basées sur la taille de la fonction pour décider de l'inlining. Avec PGO, il inline agressivement les petites fonctions appelées des millions de fois dans le hot path, et refuse d'inliner les fonctions rarement appelées même si elles sont petites — ce qui évite de gonfler le binaire inutilement.

### Prédiction de branchement statique

Le compilateur annote chaque branchement avec sa probabilité réelle. Au lieu de supposer qu'un `if` est 50/50, il sait que le `then` est pris 99,7 % du temps. Cela influence le layout du code machine : la branche probable est placée en *fall-through* (pas de saut), et la branche rare est déplacée loin du hot path.

C'est l'équivalent automatique de `[[likely]]` / `[[unlikely]]` (section 41.2), mais appliqué *partout* dans le programme avec des probabilités exactes, pas des annotations manuelles.

### Paramètres de vectorisation et déroulement

Le PGO fournit au compilateur le nombre moyen d'itérations de chaque boucle. Une boucle qui itère en moyenne 3 fois ne bénéficie pas d'un déroulement ×8 — le compilateur ajuste le facteur de déroulement et le choix de vectorisation en conséquence. Inversement, une boucle qui itère 10 millions de fois justifie un déroulement et une vectorisation agressifs.

### Optimisation des appels indirects

Pour les appels via pointeurs de fonction ou les méthodes virtuelles, le PGO identifie les cibles les plus fréquentes et peut les **spécialiser** (*devirtualization*) : le compilateur insère un test rapide « si la cible est `Foo::process()` (90 % des cas), appeler directement ; sinon, appel indirect ».

```cpp
// Code source : appel virtuel
base_ptr->process(data);

// Code généré avec PGO (pseudo) :
if (base_ptr->vtable == &Foo::vtable) {   // vérifié en 90% des cas
    Foo::process(data);                     // appel direct, inlinable
} else {
    base_ptr->process(data);                // fallback indirect
}
```

Cette transformation élimine le coût de l'appel indirect (indirection mémoire + misprediction de cible) dans le cas dominant.

---

## Mise en œuvre avec GCC

### Phase 1 : compilation instrumentée

```bash
# Compiler avec instrumentation
g++ -O2 -march=native -fprofile-generate=./pgo-data \
    -o my_program_instrumented \
    src/*.cpp

# L'option -fprofile-generate :
#   - Insère des compteurs dans le code
#   - Spécifie le répertoire de sortie des données de profiling
#   - Le répertoire est créé automatiquement s'il n'existe pas
```

### Phase 2 : exécution de profiling

```bash
# Créer le répertoire (si non existant)
mkdir -p pgo-data

# Exécuter avec un workload REPRÉSENTATIF
./my_program_instrumented --typical-workload input_data.bin

# Les fichiers .gcda sont générés dans pgo-data/
ls pgo-data/
# src/main.gcda  src/parser.gcda  src/engine.gcda  ...
```

> ⚠️ **Le workload de profiling est critique.** Il doit être représentatif de l'utilisation réelle. Si le programme est un serveur HTTP et que le profiling est effectué avec un seul type de requête, les branchements liés aux autres types de requêtes seront mal optimisés. L'idéal est d'utiliser un jeu de test qui couvre les cas d'usage principaux dans les proportions réalistes.

### Phase 3 : recompilation optimisée

```bash
# Recompiler en utilisant les données de profiling
g++ -O2 -march=native -fprofile-use=./pgo-data \
    -o my_program_optimized \
    src/*.cpp

# Options supplémentaires recommandées avec PGO :
#   -fprofile-correction   : tolère les données légèrement incohérentes
#   -Wmissing-profile      : avertit si un fichier source n'a pas de profil
```

### Nettoyage et itération

```bash
# Supprimer les données de profiling (avant un nouveau cycle)
rm -rf pgo-data/

# Cycle complet en une commande
g++ -O2 -march=native -fprofile-generate=./pgo-data -o instr src/*.cpp \
    && ./instr --workload data.bin \
    && g++ -O2 -march=native -fprofile-use=./pgo-data -o optimized src/*.cpp
```

---

## Mise en œuvre avec Clang (LLVM)

Clang utilise un workflow similaire mais avec des noms de flags différents et un outil supplémentaire pour fusionner les profils.

### Phase 1 : compilation instrumentée

```bash
clang++ -O2 -march=native -fprofile-instr-generate=default.profraw \
    -o my_program_instrumented \
    src/*.cpp
```

### Phase 2 : exécution et fusion des profils

```bash
# Exécuter le programme instrumenté
./my_program_instrumented --typical-workload input_data.bin
# → génère default.profraw

# Convertir le profil brut en format indexé
llvm-profdata merge -output=default.profdata default.profraw

# Si plusieurs exécutions (recommandé pour couvrir plus de cas) :
./my_program_instrumented --workload-1 data1.bin   # → profraw1
./my_program_instrumented --workload-2 data2.bin   # → profraw2
./my_program_instrumented --workload-3 data3.bin   # → profraw3

# Fusionner tous les profils
llvm-profdata merge -output=merged.profdata \
    default_*.profraw
```

La fusion de profils issus de workloads variés produit un profil plus représentatif que celui d'une seule exécution. C'est une bonne pratique systématique.

### Phase 3 : recompilation optimisée

```bash
clang++ -O2 -march=native -fprofile-instr-use=merged.profdata \
    -o my_program_optimized \
    src/*.cpp
```

### Vérification de la couverture du profil

```bash
# Vérifier quels fichiers sont couverts par le profil
clang++ -O2 -fprofile-instr-use=merged.profdata \
    -Wprofile-instr-unprofiled \
    -c src/nouveau_fichier.cpp
# Warning si nouveau_fichier.cpp n'a pas de données de profiling
```

---

## Comparaison GCC vs Clang pour le PGO

| Aspect | GCC | Clang |
|--------|-----|-------|
| **Flag instrumentation** | `-fprofile-generate` | `-fprofile-instr-generate` |
| **Flag utilisation** | `-fprofile-use` | `-fprofile-instr-use` |
| **Format des données** | `.gcda` (un par fichier source) | `.profraw` → `.profdata` (fusion) |
| **Outil de fusion** | Non nécessaire (merge implicite) | `llvm-profdata merge` (obligatoire) |
| **Profils multi-run** | Écrase ou accumule selon les options | Fusion explicite avec `llvm-profdata` |
| **Gains typiques** | 10–20 % | 10–25 % (LLVM tend à exploiter le PGO plus agressivement) |

---

## Intégration dans CMake

Pour un projet CMake, le PGO peut être intégré via des presets ou des options de configuration :

```cmake
# CMakeLists.txt — support PGO via une option
option(ENABLE_PGO_GENERATE "Compile with PGO instrumentation" OFF)  
option(ENABLE_PGO_USE "Compile with PGO optimization" OFF)  

set(PGO_DATA_DIR "${CMAKE_BINARY_DIR}/pgo-data")

if(ENABLE_PGO_GENERATE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-fprofile-generate=${PGO_DATA_DIR})
        add_link_options(-fprofile-generate=${PGO_DATA_DIR})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fprofile-instr-generate=${PGO_DATA_DIR}/default.profraw)
        add_link_options(-fprofile-instr-generate=${PGO_DATA_DIR}/default.profraw)
    endif()
elseif(ENABLE_PGO_USE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-fprofile-use=${PGO_DATA_DIR} -fprofile-correction)
        add_link_options(-fprofile-use=${PGO_DATA_DIR})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fprofile-instr-use=${PGO_DATA_DIR}/merged.profdata)
        add_link_options(-fprofile-instr-use=${PGO_DATA_DIR}/merged.profdata)
    endif()
endif()
```

Script de build PGO complet :

```bash
#!/bin/bash
set -e

BUILD_DIR="build-pgo"  
PGO_DATA="$BUILD_DIR/pgo-data"  

echo "=== Phase 1 : Compilation instrumentée ==="  
cmake -B "$BUILD_DIR" -G Ninja \  
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_PGO_GENERATE=ON
cmake --build "$BUILD_DIR" --parallel

echo "=== Phase 2 : Profiling ==="  
mkdir -p "$PGO_DATA"  
"$BUILD_DIR/my_program" --benchmark-workload data/representative_input.bin

# Pour Clang uniquement : fusionner les profils
if command -v llvm-profdata &> /dev/null; then
    llvm-profdata merge -output="$PGO_DATA/merged.profdata" \
        "$PGO_DATA"/*.profraw 2>/dev/null || true
fi

echo "=== Phase 3 : Recompilation optimisée ==="  
cmake -B "$BUILD_DIR" -G Ninja \  
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_PGO_GENERATE=OFF \
    -DENABLE_PGO_USE=ON
cmake --build "$BUILD_DIR" --parallel

echo "=== Binaire PGO-optimisé prêt ==="  
ls -la "$BUILD_DIR/my_program"  
```

---

## Intégration en CI/CD

Le PGO s'intègre naturellement dans un pipeline CI/CD. Le profiling peut être exécuté automatiquement sur chaque release :

```yaml
# .github/workflows/pgo-build.yml (GitHub Actions — simplifié)
name: PGO Release Build

on:
  push:
    tags: ['v*']

jobs:
  pgo-build:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: sudo apt-get install -y ninja-build

      # Phase 1 : Build instrumenté
      - name: Build instrumented
        run: |
          cmake -B build-pgo -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DENABLE_PGO_GENERATE=ON
          cmake --build build-pgo --parallel

      # Phase 2 : Profiling
      - name: Run profiling workload
        run: |
          ./build-pgo/my_program --benchmark tests/pgo_workload.bin

      # Phase 3 : Build optimisé
      - name: Build PGO-optimized
        run: |
          cmake -B build-pgo -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DENABLE_PGO_GENERATE=OFF \
            -DENABLE_PGO_USE=ON
          cmake --build build-pgo --parallel

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: my_program-pgo
          path: build-pgo/my_program
```

Le workload de profiling doit être versionné et maintenu avec le code source (dans `tests/` ou `benchmarks/`), comme tout autre artefact de test.

---

## Sampling PGO (AutoFDO) : l'alternative légère

Le PGO classique (instrumentation) a un inconvénient : le binaire instrumenté est 15–30 % plus lent, ce qui le rend impraticable pour le profiling en production.

L'***AutoFDO*** (*Automatic Feedback-Directed Optimization*) est une variante qui utilise le **sampling** au lieu de l'instrumentation : on profile un binaire *normal* (non instrumenté) en production avec `perf record`, puis on convertit les données `perf` en profil exploitable par le compilateur.

### Workflow AutoFDO

```bash
# 1. Compiler normalement (avec infos de debug pour le mapping)
g++ -O2 -march=native -g -o my_program src/*.cpp

# 2. Profiler en production avec perf (overhead < 2%)
perf record -b -e cycles:u -o perf.data -- ./my_program --production-workload

# 3. Convertir le profil perf en profil GCC
# Outil : create_gcov (Google AutoFDO tools)
create_gcov --binary=my_program \
            --profile=perf.data \
            --gcov=profile.afdo

# 4. Recompiler avec le profil AutoFDO
g++ -O2 -march=native -fauto-profile=profile.afdo \
    -o my_program_optimized src/*.cpp
```

### Workflow AutoFDO avec Clang (CSPGO)

Clang propose une variante appelée *Context-Sensitive PGO* qui peut utiliser des profils `perf` :

```bash
# 1. Compiler avec debug info
clang++ -O2 -march=native -gline-tables-only -o my_program src/*.cpp

# 2. Profiler avec perf
perf record -b -e cycles:u -o perf.data -- ./my_program

# 3. Convertir avec llvm-profgen
llvm-profgen --binary=my_program \
             --perfdata=perf.data \
             --output=profile.spgo

# 4. Recompiler
clang++ -O2 -march=native -fprofile-sample-use=profile.spgo \
    -o my_program_optimized src/*.cpp
```

### Comparaison instrumentation vs sampling

| Aspect | PGO classique (instrumentation) | AutoFDO (sampling) |
|--------|--------------------------------|---------------------|
| **Overhead à l'exécution** | 15–30 % | < 2 % |
| **Utilisable en production** | Non (trop lent) | Oui |
| **Précision du profil** | Exacte (compteurs) | Statistique (sampling) |
| **Gain typique** | 15–25 % | 8–15 % |
| **Complexité de setup** | Faible | Moyenne (outils supplémentaires) |
| **Représentativité** | Dépend du workload de test | Workload de production réel |

L'AutoFDO capture un profil moins précis que l'instrumentation, d'où un gain légèrement inférieur. Mais il a l'avantage majeur de profiler le **vrai workload de production**, pas un substitut de test. Pour les services à longue durée de vie (serveurs, bases de données), l'AutoFDO est souvent la meilleure option.

---

## Cas d'usage et gains documentés

Le PGO est utilisé en production par de nombreux projets majeurs :

| Projet | Gain PGO rapporté | Source |
|--------|-------------------|--------|
| **Clang/LLVM** (compilateur) | ~20 % sur la compilation | Documentation LLVM officielle |
| **GCC** (compilateur) | ~7–10 % sur la compilation | Benchmarks communautaires |
| **Chromium** (navigateur) | ~10–15 % sur les benchmarks web | Blog Google Chrome |
| **Firefox** | ~5–12 % sur Speedometer | Bugzilla Mozilla |
| **CPython 3.12+** | ~5 % sur pyperformance | PEP 744, documentation CPython |
| **PostgreSQL** | ~5–10 % sur pgbench | Wiki PostgreSQL |
| **RocksDB** | ~10 % sur les benchmarks I/O | Blog Facebook Engineering |

Le gain est systématiquement plus élevé sur les programmes ayant un code footprint important avec beaucoup de branchements imprévisibles statiquement (compilateurs, interpréteurs, bases de données). Il est plus modeste sur les programmes dominés par une boucle serrée déjà bien optimisée.

---

## PGO et LTO : la combinaison gagnante

Le PGO atteint son plein potentiel lorsqu'il est combiné avec le ***Link-Time Optimization*** (LTO, section 41.5). Le LTO permet l'optimisation inter-fichiers, et le PGO fournit les données de profiling pour guider ces optimisations à travers tout le programme.

```bash
# La combinaison maximale : PGO + LTO + march=native
# Phase 1
g++ -O2 -march=native -flto -fprofile-generate=./pgo-data \
    -o instrumented src/*.cpp

# Phase 2
./instrumented --workload data.bin

# Phase 3
g++ -O2 -march=native -flto -fprofile-use=./pgo-data \
    -o optimized src/*.cpp
```

Le gain combiné PGO + LTO est typiquement **supérieur à la somme** des gains individuels : le LTO permet des inlining inter-fichiers que le PGO peut ensuite calibrer, et le PGO guide le LTO vers les optimisations les plus rentables.

| Configuration | Gain typique vs `-O2` seul |
|---------------|---------------------------|
| PGO seul | 10–20 % |
| LTO seul | 5–10 % |
| PGO + LTO | 15–30 % |

---

## Pièges et bonnes pratiques

### Représentativité du workload

Le piège le plus courant : un workload de profiling non représentatif. Si le profiling est fait sur un micro-benchmark qui ne couvre que 20 % des chemins de code, les 80 % restants seront *moins bien optimisés* qu'avec les heuristiques par défaut — le compilateur suppose qu'ils sont froids et les déplace hors du hot path.

**Bonne pratique** : utiliser un workload qui couvre les cas d'usage principaux dans des proportions réalistes. Pour un serveur web : un mélange de GET, POST, requêtes statiques et dynamiques, avec la distribution réelle. Pour un compilateur : compiler un ensemble varié de fichiers source.

### Staleness des profils

Les données de profiling deviennent obsolètes (*stale*) quand le code source évolue significativement. Un profil datant de 6 mois sur un projet actif est probablement inutile — les fonctions ont été renommées, déplacées, ou supprimées. Le compilateur gère gracieusement les profils partiellement obsolètes (il ignore les fonctions inconnues), mais le gain diminue.

**Bonne pratique** : régénérer les profils à chaque release majeure, ou intégrer le PGO dans le pipeline CI/CD (voir section précédente).

### Reproductibilité

Les profils PGO rendent le build **non déterministe** si le workload de profiling n'est pas strictement reproductible. Deux machines différentes peuvent produire des profils légèrement différents (timing, ordonnancement des threads), menant à des binaires différents.

**Bonne pratique** : versionner le workload de profiling et les fichiers `.profdata` / `.gcda` dans le système de gestion de version (ou un artifact store), comme tout autre artefact de build.

### Coût du pipeline

Le PGO triple le temps de build (compilation × 2 + exécution du profiling). Pour les gros projets, cela peut ajouter 10–30 minutes au pipeline. L'utilisation de ccache (section 2.3) et de Ninja (section 28.3) atténue le problème, mais le coût reste réel.

**Bonne pratique** : réserver le PGO pour les builds de release, pas pour le développement quotidien.

---

## Résumé

| Aspect | Détail |
|--------|--------|
| **Principe** | Compiler → profiler → recompiler avec les données d'exécution réelle |
| **Gain typique** | 10–25 % sans modification du code source |
| **GCC** | `-fprofile-generate` / `-fprofile-use` |
| **Clang** | `-fprofile-instr-generate` / `-fprofile-instr-use` + `llvm-profdata merge` |
| **Alternative légère** | AutoFDO : profiling `perf` en production, overhead < 2 % |
| **Combinaison recommandée** | PGO + LTO + `-march=native` pour les builds de release |
| **Point critique** | Le workload de profiling **doit** être représentatif de la production |
| **Fréquence** | Régénérer à chaque release majeure |
| **Projets qui en bénéficient le plus** | Compilateurs, interpréteurs, bases de données, navigateurs |

---


⏭️ [Link-Time Optimization (LTO)](/41-optimisation-cpu-memoire/05-lto.md)
