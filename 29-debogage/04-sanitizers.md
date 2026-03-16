🔝 Retour au [Sommaire](/SOMMAIRE.md)

# 29.4 — Sanitizers

## Chapitre 29 : Débogage Avancé · Module 10

---

## Introduction

GDB est un outil d'investigation. Vous avez un symptôme — un crash, un résultat incorrect — et vous cherchez la cause en inspectant l'état du programme. Mais GDB part du principe que vous savez déjà qu'il y a un problème. Il ne détecte rien par lui-même.

Les sanitizers inversent l'approche. Ce sont des outils de **détection automatique** : vous activez un flag de compilation, vous exécutez votre programme normalement (ou via vos tests), et le sanitizer signale les bugs qu'il rencontre — même si ces bugs ne provoquent aucun symptôme visible.

C'est cette dernière partie qui change tout. En C++, un bug mémoire peut rester silencieux pendant des mois. Un buffer overflow de 3 octets écrase une zone mémoire qui, par chance, n'est utilisée par rien d'important. Un use-after-free lit une valeur qui, par chance, n'a pas encore été réécrite. Une race condition produit le bon résultat 99,99 % du temps. Ces bugs ne crashent pas, ne produisent pas de résultat visiblement faux — mais ils sont là, et ils attendent le pire moment pour se manifester.

Les sanitizers transforment ces bugs silencieux en erreurs bruyantes et immédiates, avec un diagnostic précis : type d'erreur, adresse fautive, pile d'appels complète, et souvent le contexte d'allocation ou de libération de la mémoire concernée.

---

## Les quatre sanitizers

GCC et Clang intègrent quatre sanitizers principaux, chacun spécialisé dans une catégorie de bugs :

### 29.4.1 — AddressSanitizer (ASan)

Le plus utilisé. ASan détecte les erreurs d'accès mémoire :

- **Buffer overflow** — accès au-delà des limites d'un tableau (stack, heap, ou global)
- **Use-after-free** — accès à de la mémoire déjà libérée
- **Use-after-return** — accès à une variable locale après le retour de sa fonction
- **Double-free** — libérer deux fois la même allocation
- **Memory leaks** — mémoire allouée mais jamais libérée (mode leak detector)

ASan est l'outil qui aurait détecté la majorité des vulnérabilités CVE liées à la mémoire dans les logiciels C/C++ des 20 dernières années. C'est le sanitizer à activer en premier, systématiquement.

### 29.4.2 — UndefinedBehaviorSanitizer (UBSan)

Détecte les comportements indéfinis — ces opérations que le standard C++ dit "peut faire n'importe quoi" et que le compilateur a le droit d'exploiter de manière imprévisible :

- **Signed integer overflow** — `INT_MAX + 1` n'est pas défini
- **Null pointer dereference** — accès via un pointeur null
- **Shift overflow** — décalage de bits au-delà de la taille du type
- **Division par zéro** (entière)
- **Accès mal aligné** — lire un `int` à une adresse non alignée sur 4 octets
- **Conversion de type invalide** — cast vers un type incompatible

UBSan est léger et rapide. Il a un impact quasi négligeable sur les performances, ce qui le rend utilisable même dans des builds de test proches de la production.

### 29.4.3 — ThreadSanitizer (TSan)

Détecte les **data races** — des accès concurrents non synchronisés à une même variable depuis plusieurs threads, dont au moins un est une écriture. Les data races sont la catégorie de bugs la plus difficile à diagnostiquer manuellement : elles sont non déterministes, dépendent du timing du scheduler, et produisent des symptômes intermittents qui peuvent ne se manifester que sous charge.

TSan intercepte chaque accès mémoire et vérifie qu'il est correctement synchronisé. Le surcoût est significatif (5-15x de ralentissement, 5-10x de mémoire supplémentaire), mais c'est incomparablement plus rapide que de traquer une race condition à la main.

### 29.4.4 — MemorySanitizer (MSan)

Détecte les **lectures de mémoire non initialisée**. Quand vous déclarez une variable locale sans l'initialiser, sa valeur est indéterminée — elle contient ce qui traînait sur la pile à cet emplacement. Lire cette valeur est un comportement indéfini, et les résultats dépendent de l'état de la pile, du niveau d'optimisation, et de la phase de la lune.

MSan traque chaque octet de mémoire et signale toute lecture d'un octet qui n'a jamais été écrit. C'est le sanitizer le plus restrictif et le plus exigeant : il ne fonctionne qu'avec Clang (pas GCC), et il nécessite que toutes les bibliothèques soient compilées avec MSan — y compris la bibliothèque standard.

---

## Principe de fonctionnement

Les sanitizers ne sont pas des outils externes comme Valgrind. Ils fonctionnent par **instrumentation à la compilation** : le compilateur insère du code de vérification autour de chaque opération mémoire, chaque accès à une variable, chaque appel de fonction. Le programme résultant est plus gros et plus lent, mais chaque opération est vérifiée automatiquement.

```
┌──────────────────────────────────────────────────────┐
│  Code source                                         │
│    int* p = new int[10];                             │
│    p[15] = 42;    // Buffer overflow silencieux      │
│    delete[] p;                                       │
└───────────────┬──────────────────────────────────────┘
                │ Compilation avec -fsanitize=address
                ▼
┌──────────────────────────────────────────────────────┐
│  Code instrumenté (simplifié)                        │
│    int* p = asan_malloc(10 * sizeof(int));           │
│    asan_check_write(p + 15, sizeof(int));  ← ERREUR  │
│    *(p + 15) = 42;                                   │
│    asan_free(p);                                     │
└──────────────────────────────────────────────────────┘
```

La vérification `asan_check_write` détecte que `p + 15` est en dehors de la zone allouée (10 éléments) et arrête le programme avec un rapport détaillé. Sans ASan, ce `p[15] = 42` écrase silencieusement 4 octets de mémoire adjacente — un bug qui pourrait ne causer un crash que bien plus tard, dans un code complètement différent.

### Différence avec Valgrind

Valgrind (couvert dans le chapitre 30) fonctionne par **instrumentation dynamique** : il intercepte le binaire à l'exécution sans recompilation. L'avantage est qu'il fonctionne avec n'importe quel binaire. Les inconvénients sont un ralentissement considérable (20-50x) et une détection moins fine que les sanitizers pour certaines catégories de bugs.

| Critère | Sanitizers | Valgrind |
|---|---|---|
| Activation | Flag de compilation (`-fsanitize=...`) | Outil externe (`valgrind ./prog`) |
| Recompilation nécessaire | Oui | Non |
| Ralentissement (ASan) | ~2x | ~20-50x |
| Détection stack overflow | Oui | Non |
| Détection global overflow | Oui | Non |
| Fonctionne avec tout binaire | Non | Oui |
| Intégrable en CI | Facile (compilation + exécution) | Facile (exécution seule) |

En pratique, les deux outils sont complémentaires. Les sanitizers sont plus rapides et plus précis pour les bugs qu'ils détectent. Valgrind couvre certains cas que les sanitizers ne traitent pas (notamment le profiling mémoire avec Massif, chapitre 30). La recommandation moderne est d'utiliser les sanitizers comme premier choix et Valgrind en complément quand les sanitizers ne suffisent pas.

---

## Activation : le flag `-fsanitize=`

Tous les sanitizers s'activent par le même mécanisme — un flag passé au compilateur **et** au linker :

```bash
# AddressSanitizer
g++ -fsanitize=address -g -O1 -o prog main.cpp

# UndefinedBehaviorSanitizer
g++ -fsanitize=undefined -g -O1 -o prog main.cpp

# ThreadSanitizer
g++ -fsanitize=thread -g -O1 -o prog main.cpp

# MemorySanitizer (Clang uniquement)
clang++ -fsanitize=memory -g -O1 -o prog main.cpp
```

Le flag `-g` est indispensable pour que les rapports d'erreur incluent les numéros de lignes et les noms de fichiers source. Sans `-g`, vous obtenez des adresses mémoire brutes au lieu de `main.cpp:42`.

### Pourquoi `-O1` et pas `-O0` ?

Contrairement au débogage avec GDB où `-O0` est recommandé, les sanitizers fonctionnent mieux avec `-O1` :

- `-O0` produit un code plus volumineux et plus lent, ce qui amplifie le surcoût des sanitizers.
- `-O1` active des optimisations qui aident les sanitizers à fonctionner plus efficacement (inlining léger, élimination de code mort) sans perturber le diagnostic.
- `-O2` fonctionne aussi, mais certaines optimisations agressives (réutilisation de stack slots, élimination de variables) peuvent masquer des bugs ou rendre les rapports moins lisibles.

La combinaison recommandée est `-fsanitize=... -g -O1 -fno-omit-frame-pointer`. Le flag `-fno-omit-frame-pointer` garantit des backtraces complètes et lisibles dans les rapports de sanitizers.

```bash
# La ligne de compilation "gold standard" pour les sanitizers
g++ -fsanitize=address -g -O1 -fno-omit-frame-pointer -o prog main.cpp
```

### Intégration CMake

Pour un projet CMake, l'activation des sanitizers se fait via les options de compilation et de linkage :

```cmake
# CMakeLists.txt — option sanitizer configurable
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)  
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)  
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)  

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()
```

Utilisation :

```bash
cmake -B build -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug  
cmake --build build  
./build/mon_programme    # ASan est actif
```

> 📎 *Pour l'intégration des sanitizers dans un pipeline CI/CD, voir section 38.4.*

---

## Incompatibilités entre sanitizers

**Les sanitizers ne sont pas tous combinables entre eux.** C'est une contrainte technique fondamentale : chacun instrumente la mémoire de manière différente, et ces instrumentations entrent en conflit.

| Combinaison | Compatible ? | Remarque |
|---|---|---|
| ASan + UBSan | ✅ Oui | Combinaison recommandée — couvre mémoire + UB |
| ASan + TSan | ❌ Non | Instrumentations mémoire incompatibles |
| ASan + MSan | ❌ Non | Instrumentations mémoire incompatibles |
| TSan + MSan | ❌ Non | Instrumentations mémoire incompatibles |
| TSan + UBSan | ✅ Oui | Fonctionne, rarement nécessaire |
| MSan + UBSan | ✅ Oui | Fonctionne (Clang uniquement pour MSan) |

La conséquence pratique est que vous devez maintenir plusieurs builds sanitisés :

```bash
# Build 1 : Mémoire + comportement indéfini (le plus courant)
cmake -B build-asan -DENABLE_ASAN=ON -DENABLE_UBSAN=ON

# Build 2 : Concurrence
cmake -B build-tsan -DENABLE_TSAN=ON

# Build 3 : Mémoire non initialisée (Clang uniquement, optionnel)
cmake -B build-msan -DCMAKE_CXX_COMPILER=clang++ -DENABLE_MSAN=ON
```

En CI, ces builds tournent en parallèle — chacun détecte sa catégorie de bugs indépendamment.

---

## Lire un rapport de sanitizer

Tous les sanitizers produisent des rapports dans un format similaire. Voici la structure type d'un rapport ASan :

```
=================================================================
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address
0x60200000001c at pc 0x0000004f4b27 bp 0x7ffc12345678 sp 0x7ffc12345670
WRITE of size 4 at 0x60200000001c thread T0
    #0 0x4f4b26 in parse_config(std::string const&) config_parser.cpp:30
    #1 0x4f5012 in main main.cpp:53
    #2 0x7f1234567890 in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x29d90)

0x60200000001c is located 4 bytes after 10-byte region [0x602000000010,0x60200000001a)
allocated by thread T0 here:
    #0 0x4e8c30 in operator new[](unsigned long) (/home/dev/prog+0x4e8c30)
    #1 0x4f4a10 in parse_config(std::string const&) config_parser.cpp:25
    #2 0x4f5012 in main main.cpp:53
=================================================================
```

Décryptons les éléments clés :

**La première ligne** identifie le sanitizer et le type d'erreur : `AddressSanitizer: heap-buffer-overflow`. Vous savez immédiatement que c'est un débordement de buffer sur le heap.

**`WRITE of size 4`** — l'opération fautive est une écriture de 4 octets (un `int`).

**La première pile d'appels** (`#0`, `#1`, `#2`) montre où l'accès fautif s'est produit : `config_parser.cpp:30`.

**`is located 4 bytes after 10-byte region`** — l'accès touche une zone 4 octets après la fin d'un bloc alloué de 10 octets. C'est le diagnostic précis : vous avez alloué 10 octets et vous écrivez au-delà.

**La seconde pile d'appels** (`allocated by thread T0 here`) montre où le bloc de mémoire a été alloué : `config_parser.cpp:25`. C'est la pièce du puzzle qui fait souvent la différence — vous voyez non seulement où le bug se manifeste, mais aussi quelle allocation est trop petite.

Chaque sous-section (29.4.1 à 29.4.4) détaille les rapports spécifiques à chaque sanitizer avec des exemples concrets.

---

## Contrôle d'exécution via variables d'environnement

Les sanitizers sont configurables via des variables d'environnement, sans recompilation :

```bash
# ASan : configurer le comportement à la détection d'erreur
ASAN_OPTIONS="halt_on_error=0:detect_leaks=1:log_path=/tmp/asan" ./prog

# UBSan : continuer après une erreur au lieu d'abort
UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=0" ./prog

# TSan : réduire le bruit sur des races connues
TSAN_OPTIONS="suppressions=tsan_suppressions.txt" ./prog
```

Les options les plus utilisées :

| Variable | Option | Effet |
|---|---|---|
| `ASAN_OPTIONS` | `halt_on_error=0` | Continue l'exécution après la première erreur |
| `ASAN_OPTIONS` | `detect_leaks=1` | Active la détection de fuites mémoire (par défaut sur Linux) |
| `ASAN_OPTIONS` | `log_path=/tmp/asan` | Redirige les rapports vers un fichier |
| `UBSAN_OPTIONS` | `print_stacktrace=1` | Inclut la pile d'appels dans les rapports |
| `UBSAN_OPTIONS` | `halt_on_error=1` | Arrête le programme à la première erreur |
| `TSAN_OPTIONS` | `suppressions=<file>` | Fichier de suppressions pour les faux positifs |
| `TSAN_OPTIONS` | `history_size=7` | Augmente la profondeur d'historique (plus de contexte) |

### Fichiers de suppressions

Quand un sanitizer détecte un faux positif (ou un bug connu dans une bibliothèque tierce que vous ne pouvez pas corriger), vous pouvez le supprimer sans désactiver le sanitizer :

```
# tsan_suppressions.txt
race:third_party::legacy_logger  
race:libz.so  
```

Le fichier liste des patterns de fonctions ou de bibliothèques à ignorer. C'est un mécanisme pragmatique : vous nettoyez les vrais bugs d'abord, vous supprimez le bruit des bibliothèques tierces, et vous gardez le sanitizer actif pour détecter les régressions.

---

## Stratégie d'utilisation recommandée

### En développement local

Activez ASan + UBSan comme configuration par défaut pendant le développement. Le surcoût de performance (~2x) est acceptable pour un programme de développement, et la détection immédiate des bugs mémoire et des comportements indéfinis vous fait gagner un temps considérable.

```bash
# CMake preset pour le développement quotidien
cmake -B build-dev \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_ASAN=ON \
    -DENABLE_UBSAN=ON
```

Exécutez vos tests unitaires avec cette configuration. Si un test passe mais qu'un sanitizer détecte une erreur, le test est un faux positif — il valide un résultat correct obtenu par chance malgré un bug.

### En CI/CD

Exécutez trois builds en parallèle :

1. **ASan + UBSan** — détecte les bugs mémoire et les comportements indéfinis
2. **TSan** — détecte les data races (build séparé, incompatible avec ASan)
3. **Build normal sans sanitizer** — vérifie que le programme fonctionne en conditions standard

Si l'un des builds sanitisés échoue, le pipeline échoue. C'est une porte de qualité qui empêche les bugs mémoire d'atteindre la production.

### En production

Les sanitizers ne sont généralement pas activés en production à cause du surcoût de performance. Cependant, UBSan en mode non-fatal (`-fsanitize=undefined -fno-sanitize-recover=...` sélectif) est parfois utilisé en production pour détecter des UB sans arrêter le programme.

> 📎 *L'utilisation des sanitizers comme technique de hardening en production est traitée en section 45.6.3.*

---

## Support compilateur

| Sanitizer | GCC | Clang | Remarques |
|---|---|---|---|
| AddressSanitizer | ✅ Depuis GCC 4.8 | ✅ Depuis Clang 3.1 | Mature et fiable partout |
| UndefinedBehaviorSanitizer | ✅ Depuis GCC 4.9 | ✅ Depuis Clang 3.3 | Couverture de checks plus large sous Clang |
| ThreadSanitizer | ✅ Depuis GCC 4.8 | ✅ Depuis Clang 3.2 | Rapports légèrement plus détaillés sous Clang |
| MemorySanitizer | ❌ Non supporté | ✅ Depuis Clang 3.3 | Nécessite que tout soit compilé avec MSan |

Avec GCC 15 et Clang 20 (versions couvertes par cette formation), les quatre sanitizers sont stables et matures. Clang offre un avantage sur MSan et propose parfois des diagnostics plus détaillés, mais GCC couvre les trois sanitizers les plus courants (ASan, UBSan, TSan) de manière tout à fait fiable.

---

## Structure des fichiers

```
29-debogage/
└── 04-sanitizers.md                  ← vous êtes ici
    ├── 04.1-addresssanitizer.md
    ├── 04.2-ubsan.md
    ├── 04.3-threadsanitizer.md
    └── 04.4-memorysanitizer.md
```

---

> **À retenir** : les sanitizers détectent des bugs que vous ne verriez pas autrement — pas parce que vous êtes inattentif, mais parce que ces bugs sont silencieux par nature. Un buffer overflow de 3 octets, une race condition qui se manifeste une fois sur 10 000, un entier signé qui overflow sans crash — les sanitizers les transforment en erreurs explicites avec un diagnostic complet. Activez ASan + UBSan par défaut, TSan pour le code concurrent, et traitez chaque rapport comme un bug critique.

⏭️ [AddressSanitizer (-fsanitize=address)](/29-debogage/04.1-addresssanitizer.md)
