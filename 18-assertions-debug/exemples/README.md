# Chapitre 18 — Assertions et Débogage Défensif : Exemples

## Compilation

Tous les exemples (sauf 04) se compilent avec :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o <nom_binaire> <fichier>.cpp
```

L'exemple 04 (stacktrace) nécessite le linkage avec `libstdc++exp` :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g <fichier>.cpp -o <nom_binaire> \
    -L/usr/lib/gcc/x86_64-linux-gnu/15 -lstdc++exp
```

### Compilation individuelle

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 01_assert_static_assert 01_assert_static_assert.cpp  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -DDEBUG -DDEBUG_LEVEL=3 -g -O0 -o 02_compilation_conditionnelle 02_compilation_conditionnelle.cpp  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 03_logging_traces 03_logging_traces.cpp  
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g 04_stacktrace.cpp -o 04_stacktrace -L/usr/lib/gcc/x86_64-linux-gnu/15 -lstdc++exp  
```

### Nettoyage

```bash
rm -f 01_assert_static_assert 02_compilation_conditionnelle 03_logging_traces 04_stacktrace
```

---

## Liste des exemples

### 01_assert_static_assert.cpp

**Section** : 18.1 — `assert` et `static_assert`  
**Source** : 01-assert-static-assert.md  
**Description** : Démonstration des assertions runtime (`assert`) et compile-time (`static_assert`). Couvre : préconditions de fonctions (divide, set_pixel), invariants de classe (CircularBuffer), machine à états avec branche impossible, templates contraints (add), vérifications de plateforme (sizeof, alignof, endianness), structures réseau (packed), type traits (copiabilité, move noexcept).  

**Sortie attendue** :
```
=== 1. assert basique (divide) ===
  divide(10, 3) = 3.3333333333333335
  divide(100, 7) = 14.2857

=== 2. Préconditions (set_pixel) ===
  3 pixels placés dans image 100x100

=== 3. Invariant de classe (CircularBuffer) ===
  CircularBuffer : 3 éléments (capacité 4)

=== 4. Machine à états ===
  État: Running
  État: Paused
  État: Stopped

=== 5. static_assert — templates ===
  add(3, 4) = 7
  add(1.5, 2.3) = 3.8

=== 6. static_assert — vérifications plateforme ===
  sizeof(int) = 4
  sizeof(void*) = 8
  sizeof(NetworkPacketHeader) = 8
  endian = little : true

Tous les assert et static_assert passés !
```

---

### 02_compilation_conditionnelle.cpp

**Section** : 18.2 — Compilation conditionnelle (`#ifdef DEBUG`)  
**Source** : 02-compilation-conditionnelle.md  
**Description** : Macro `DEBUG` et `DBG_LOG`, niveaux de verbosité (`DEBUG_LEVEL` 0-3), ConnectionPool avec compteurs conditionnels, `if constexpr` avec `is_debug`, `consteval` pour vérification compile-time, `SortedContainer` avec vérification d'intégrité O(n) en debug.  
**Note** : Compiler avec `-DDEBUG -DDEBUG_LEVEL=3` pour voir toutes les traces. Sans `-DDEBUG`, les traces debug sont absentes du binaire.  

**Sortie attendue** (avec `-DDEBUG -DDEBUG_LEVEL=3`) :
```
=== 1. ConnectionPool avec DEBUG ===
[DEBUG 02_compilation_conditionnelle.cpp:51] Acquisition de connexion pour: user_service
[DEBUG 02_compilation_conditionnelle.cpp:54] Total acquisitions: 1
[DEBUG 02_compilation_conditionnelle.cpp:51] Acquisition de connexion pour: auth_service
[DEBUG 02_compilation_conditionnelle.cpp:54] Total acquisitions: 2
[DEBUG 02_compilation_conditionnelle.cpp:59] Libération de connexion pour: user_service
[DEBUG] === Pool Statistics ===
  Acquisitions totales: 2

=== 2. Niveaux de verbosité (DEBUG_LEVEL=3) ===
[TRACE] Entering main
[INFO]  Application démarrée
[WARN]  Config absente, utilisation des défauts
[ERROR] Connexion à la base échouée
[TRACE] Exiting main

=== 3. if constexpr (is_debug=1) ===
[DEBUG] Processing data of size: 5
[DEBUG] Processing data of size: 5

=== 4. consteval ===
  checked_divide(42, 7) = 6

=== 5. SortedContainer avec vérification debug ===
  Contenu trié (5 éléments): 5 10 20 25 30

Programme terminé.
```

---

### 03_logging_traces.cpp

**Section** : 18.3 — Logging et traces d'exécution  
**Source** : 03-logging-traces.md  
**Description** : SimpleLogger thread-safe avec `std::source_location`, niveaux de sévérité (Trace → Fatal), macros `LOG_*`, ScopeTracer RAII (trace ENTER/EXIT automatique), TimedScope (chronométrage de blocs), filtrage par niveau à l'exécution, formatage avec `std::format`.  

**Sortie attendue** (horodatage variable) :
```
HH:MM:SS.mmm [INFO ] 03_logging_traces.cpp:199 — Démarrage de l'application

--- load_config ---
HH:MM:SS.mmm [INFO ] 03_logging_traces.cpp:165 — Chargement de la configuration  
HH:MM:SS.mmm [DEBUG] 03_logging_traces.cpp:166 — Chemin: /etc/app/config.yaml  
HH:MM:SS.mmm [WARN ] 03_logging_traces.cpp:167 — Fichier non trouvé, ...  

--- authenticate (ScopeTracer) ---
HH:MM:SS.mmm [TRACE] 03_logging_traces.cpp:171 — → ENTER authenticate  
HH:MM:SS.mmm [DEBUG] 03_logging_traces.cpp:172 — Authentification de l'utilisateur 'alice'  
HH:MM:SS.mmm [TRACE] 03_logging_traces.cpp:175 — → ENTER vérification mot de passe  
HH:MM:SS.mmm [TRACE] 03_logging_traces.cpp:175 — ← EXIT  vérification mot de passe  
HH:MM:SS.mmm [TRACE] 03_logging_traces.cpp:180 — → ENTER chargement permissions  
HH:MM:SS.mmm [TRACE] 03_logging_traces.cpp:180 — ← EXIT  chargement permissions  
HH:MM:SS.mmm [TRACE] 03_logging_traces.cpp:171 — ← EXIT  authenticate  

--- process_batch (TimedScope) ---
HH:MM:SS.mmm [DEBUG] 03_logging_traces.cpp:186 — ⏱ START process_batch  
HH:MM:SS.mmm [INFO ] 03_logging_traces.cpp:189 — Traitement du batch : 3 éléments  
HH:MM:SS.mmm [TRACE] 03_logging_traces.cpp:192 —   [0] → alpha  
HH:MM:SS.mmm [TRACE] 03_logging_traces.cpp:192 —   [1] → beta  
HH:MM:SS.mmm [TRACE] 03_logging_traces.cpp:192 —   [2] → gamma  
HH:MM:SS.mmm [DEBUG] 03_logging_traces.cpp:186 — ⏱ END   process_batch — NNN µs  

--- Filtrage par niveau ---
HH:MM:SS.mmm [WARN ] 03_logging_traces.cpp:216 — Ce message S'AFFICHE (Warning >= Warning)  
HH:MM:SS.mmm [ERROR] 03_logging_traces.cpp:217 — Ce message S'AFFICHE (Error >= Warning)  

HH:MM:SS.mmm [INFO ] 03_logging_traces.cpp:222 — Arrêt normal
```

**Comportement attendu** :
- Les messages Trace/Debug/Info sont filtrés quand le niveau est relevé à Warning
- ScopeTracer émet automatiquement ENTER à la construction et EXIT à la destruction
- TimedScope mesure la durée en microsecondes entre START et END

---

### 04_stacktrace.cpp

**Section** : 18.4 — `std::stacktrace` (C++23)  
**Source** : 04-stacktrace.md  
**Compilation spéciale** : nécessite `-lstdc++exp` après le fichier source  
**Description** : Capture basique de pile d'appels (`std::stacktrace::current()`), `ASSERT_WITH_TRACE` (assertion enrichie avec trace complète), `TracedException` (exception capturant la trace au moment du throw), `SortedBuffer` (vérification d'invariant avec trace), itération et filtrage des frames par chemin de fichier, `current(skip, max_depth)` pour limiter la profondeur.  

**Sortie attendue** (chemins et lignes variables) :
```
=== 1. Capture basique de stacktrace ===
  Pile d'appels :
   0# fonction_profonde() at .../04_stacktrace.cpp:NNN
   1# fonction_intermediaire() at .../04_stacktrace.cpp:NNN
   2# main at .../04_stacktrace.cpp:NNN
   ...

=== 2. TracedException (capture au moment du throw) ===
Exception : Erreur de configuration: Fichier '/etc/myapp/config.yaml' introuvable

Capturée à :
   0# TracedException::TracedException(...) at .../04_stacktrace.cpp:NNN
   1# ConfigError::ConfigError(...) at .../04_stacktrace.cpp:NNN
   2# load_config(...) at .../04_stacktrace.cpp:NNN
   3# initialize_service() at .../04_stacktrace.cpp:NNN
   4# main at .../04_stacktrace.cpp:NNN
   ...

=== 3. ASSERT_WITH_TRACE ===

╔══ ASSERTION FAILED ══════════════════
║ Condition : from.balance() >= amount
║ Message   : Solde insuffisant pour le transfert
║ Fichier   : 04_stacktrace.cpp
║ Ligne     : NNN
╠══ STACK TRACE ═══════════════════════
   0# transfer(...) at .../04_stacktrace.cpp:NNN
   1# execute_transfers(...) at .../04_stacktrace.cpp:NNN
   2# main at .../04_stacktrace.cpp:NNN
   ...
╚══════════════════════════════════════

=== 4. SortedBuffer — invariant correct ===
  Contenu (trié): 10 20 30

=== 5. SortedBuffer — invariant violé ===
[INVARIANT VIOLÉ] SortedBuffer n'est plus trié !
  Contenu: 10 20 5
  Pile d'appels:
   0# SortedBuffer::check_invariant() ...
   1# SortedBuffer::unsafe_push_back(int) ...
   2# main ...

=== 6. Itération et filtrage des frames ===
  Trace filtrée :
    #0 main at .../04_stacktrace.cpp:NNN

=== 7. current(skip, max_depth) ===
  Trace limitée (max 3 frames) :
   0# main at .../04_stacktrace.cpp:NNN
   ...

Programme terminé.
```

**Comportement attendu** :
- La trace est capturée au moment du `throw`, pas du `catch` (pile originale intacte)
- L'invariant `SortedBuffer` détecte que `unsafe_push_back(5)` casse l'ordre de tri
- Le filtrage ne retient que les frames du fichier projet (pas libc, pas le runtime)
- `current(0, 3)` limite la capture à 3 frames maximum
