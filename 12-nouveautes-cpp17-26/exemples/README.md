# Exemples du chapitre 12 — Nouveautés C++17-26

## Compilation

Tous les exemples (sauf mention contraire) se compilent avec :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o <nom_binaire> <fichier>.cpp
```

Pour `12_12_stacktrace.cpp`, ajouter `-lstdc++exp` :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 12_12_stacktrace 12_12_stacktrace.cpp -lstdc++exp
```

---

## Liste des fichiers

### 12_01_structured_bindings.cpp

- **Section** : 12.1 — Structured Bindings (C++17)
- **Fichier source** : `01-structured-bindings.md`
- **Description** : Exemples complets de liaisons structurées — paires, tuples, structures, tableaux C, qualificateurs (const, référence mutable), cas d'usage courants (parse_config, insert/try_emplace, if-init, enumerate C++23), décomposition personnalisée (Person avec tuple_size/tuple_element/get).
- **Sortie attendue** :
  - Map iteration : `Alice a obtenu 95/100`, `Bob a obtenu 87/100`
  - Tuple : `Alice — score: 95, moyenne: 17.5`
  - Struct : `x=0, y=0, z=0`
  - C-style array : `r=255, g=128, b=0`
  - Mutable reference : `cfg.port = 9090`
  - parse_config : `Connexion a localhost:8080`
  - insert : `Insere : 42`
  - try_emplace : `La cle existait deja, valeur actuelle : 42`
  - enumerate : `[0] Alice`, `[1] Bob`, `[2] Clara`
  - Custom : `name=Alice, age=30`

### 12_02_optional_variant_any.cpp

- **Section** : 12.2 — std::optional, std::variant, std::any (C++17)
- **Fichier source** : `02-optional-variant-any.md`
- **Description** : Les trois types vocabulaires — optional (construction, accès, structured binding combo, monadic operations C++23 : and_then, transform, or_else), variant (accès, visit avec lambda générique et overloaded, machine à états, monostate), any (cast, API, emplace, make_any).
- **Sortie attendue** :
  - optional basic : `Score : 95`, `Score de Charlie : 0`
  - optional construction : vérification des états vide/plein
  - monadic : `port=8080`, `port_str=Port: 8080`, `fallback=8080`
  - variant visit : `Valeur : hello`, `Flottant : 3.14`
  - state machine : `Connecting to api.example.com:443`
  - any : `Valeur : 3.14`, `emplace: xxxxx`

### 12_03_std_span.cpp

- **Section** : 12.3 — std::span (C++20)
- **Fichier source** : `03-std-span.md`
- **Description** : Vue légère non-owning — process acceptant vector/array/C-array via span, interface (size, front, back, first, last, subspan), span mutable (double_values), span statique vs dynamique.
- **Sortie attendue** :
  - process : `1 2 3 4 5`, `10 20 30 40`, `100 200 300`, `2 3 4`
  - interface : `size=5`, `front=1`, `back=5`, sous-vues correctes
  - mutable : `2 4 6` (après doublement de {1,2,3})
  - static/dynamic : `size=5` pour les deux

### 12_04_concepts.cpp

- **Section** : 12.4 — Concepts (C++20)
- **Fichier source** : `04-concepts.md`
- **Description** : Contraintes sur templates — sort_container avec random_access_range, quatre syntaxes (requires, concept en lieu de typename, trailing, auto contraint), multiply avec requires, concept Numeric personnalisé, Printable, Hashable, HashableEquality, résolution par contraintes (overload).
- **Sortie attendue** :
  - sort : `1 2 3 4 5`
  - quatre syntaxes : toutes retournent `7`
  - multiply : `12`, `3.75`
  - Printable/Hashable : `true`
  - overload : `Entier : 42`, `Flottant : 3.14`, `Valeur quelconque`

### 12_05_ranges.cpp

- **Section** : 12.5 — Ranges (C++20)
- **Fichier source** : `05-ranges.md`
- **Description** : Pipelines fonctionnels — ranges::sort, filter view, pipeline filter+transform+take, filter/transform séparés, take/drop/take_while/drop_while, iota (fini et infini), reverse/split/join, enumerate/zip/chunk/slide (C++23), ranges::to, pipeline complet de mesures de température.
- **Sortie attendue** :
  - pipeline : `64 36 4`
  - iota perfect_squares : `1 4 9 16 25`
  - split : `'Alice' 'Bob' 'Clara' 'Dave'`
  - zip : `Alice : 95`, `Bob : 87`, `Clara : 92`
  - ranges::to : `4 16 36 64 100`
  - Top 5 températures : `24.0`, `23.1`, `22.5`, `22.3`, `21.0`

### 12_06a_coroutine_manual.cpp

- **Section** : 12.6 — Coroutines (C++20) — Implémentation manuelle
- **Fichier source** : `06-coroutines.md`
- **Description** : Generator<T> minimal avec promise_type, coroutine_handle, co_yield. Générateur range(1, 6).
- **Sortie attendue** : `1 2 3 4 5`

### 12_06b_std_generator.cpp

- **Section** : 12.6 — Coroutines (C++20) — std::generator (C++23)
- **Fichier source** : `06-coroutines.md`
- **Description** : Générateur fibonacci utilisant std::generator avec intégration pipeline ranges (views::take).
- **Sortie attendue** : `0 1 1 2 3 5 8 13 21 34`

### 12_07_print_format.cpp

- **Section** : 12.7 — std::print et std::format (C++23)
- **Fichier source** : `07-std-print-format.md`
- **Description** : Formatage moderne — format basique, print/println, indexation des arguments, alignement (<, >, ^, fill), entiers (d/b/o/x/X, #, padding), flottants (f/e/g, précision, signe), chaînes (troncature, escape ?), booléens, chrono, formatter personnalisé (Point avec modes c/v), UserId héritant de formatter<uint64_t>, format_to, format_to_n, formatted_size.
- **Sortie attendue** :
  - alignement : `[hello     ]`, `[     hello]`, `[  hello   ]`, etc.
  - entiers : `255`, `11111111`, `377`, `ff`, `FF`, `0b11111111`, etc.
  - flottants : `3.14159`, `3.141590`, `3.14`, `3.141590e+00`, etc.
  - custom Point : `(3.14, 2.72)`, `Point(x=3.14, y=2.72)`
  - UserId : `User #00003039`
  - formatted_size : `23`

### 12_08_std_expected.cpp

- **Section** : 12.8 — std::expected (C++23)
- **Fichier source** : `08-std-expected.md`
- **Description** : Gestion d'erreurs sans exceptions — expected basique (has_value, value, error, value_or), pipeline and_then (read_file → parse_int → validate_port), transform, or_else (fallback), transform_error, expected<void, E>.
- **Sortie attendue** :
  - basic : `Contenu : 8080`, `Erreur : 2` (FileNotFound)
  - pipeline : `port = 8080`
  - transform : `port_str = :8080`
  - or_else : `port with fallback = 8080`
  - expected<void> : `Ecriture reussie`, puis erreur

### 12_09_flat_containers.cpp

- **Section** : 12.9 — std::flat_map et std::flat_set (C++23)
- **Fichier source** : `09-flat-containers.md`
- **Description** : Conteneurs ordonnés à mémoire contiguë — flat_map basique (insert, emplace, find, contains, itération ordonnée), flat_set, construction sorted_unique, accès keys/values, extract.
- **Sortie attendue** :
  - flat_map : `Alice : 95`, `Bob : 87`, `Clara : 92`, `Dave : 88`, `Eve : 91`
  - flat_set : `cpp devops linux`
  - sorted_unique : construction sans tri
  - extract : `fm2 apres extract, size: 0`

### 12_09_2_constexpr_lookup.cpp

- **Section** : 12.9.2 — Cas d'usage et limites (constexpr lookup)
- **Fichier source** : `09.2-cas-usage-limites.md`
- **Description** : Table de recherche constexpr avec std::array trié et std::ranges::lower_bound. Vérification par static_assert.
- **Sortie attendue** : `info=1`, `error=3`, `unknown=-1`, `trace=0`, `warning=2`

### 12_09_2_erase_if.cpp

- **Section** : 12.9.2 — Cas d'usage et limites (erase_if)
- **Fichier source** : `09.2-cas-usage-limites.md`
- **Description** : Pattern erase dans une boucle et std::erase_if sur un flat_map — suppression conditionnelle optimisée.
- **Sortie attendue** : `After erase loop: b=2 c=3`, `After erase_if: b=2 c=3`

### 12_10_mdspan.cpp

- **Section** : 12.10 — std::mdspan (C++23)
- **Fichier source** : `10-mdspan.md`
- **Description** : Vues multidimensionnelles — mdspan basique, extents (dynamic, static, mixed, CTAD), traitement d'image (RGB, grayscale), multiplication matricielle, inspection (rank, extent, size, empty).
- **Comportement** : **Ne compile pas** — ni GCC 15 (libstdc++) ni Clang 18 (libc++) ne fournissent `<mdspan>` sur cette machine. Nécessite GCC 16+ ou libc++ 19+.
- **Sortie attendue** (quand compilable) :
  - basic : `matrix[1,2] = 3.14`
  - matmul : `C[0,0]=58, C[0,1]=64, C[1,0]=139, C[1,1]=154`
  - inspection : `rank=2, extent(0)=3, extent(1)=4, size=12, empty=false`

### 12_11_generator.cpp

- **Section** : 12.11 — std::generator (C++23)
- **Fichier source** : `11-generator.md`
- **Description** : Générateurs paresseux — fibonacci, naturals avec pipeline (filter+transform+take), powers, primes, parcours inorder d'arbre (récursif), perfect_squares, collatz, generator<const string&>.
- **Sortie attendue** :
  - fibonacci : `0 1 1 2 3 5 8 13 21 34`
  - naturals pipeline : `9 36 81 144 225`
  - powers(2) : `1 2 4 8 16 32`
  - primes : `2 3 5 7 11 13 17`
  - inorder : `1 2 3 4 5 6 7`
  - perfect_squares : `1 4 9 16 25 36`
  - collatz(12) : `12 6 3 10 5 16 8 4 2 1`
  - iterate_names : `Alice Bob Clara`

### 12_12_stacktrace.cpp

- **Section** : 12.12 — std::stacktrace (C++23)
- **Fichier source** : `12-stacktrace.md`
- **Compilation** : Nécessite `-lstdc++exp` en plus des flags standards
- **Description** : Capture de pile d'appels — basic capture (inner→middle→main), format integration (std::format), anatomy (size, empty), entry details (description, source_file, source_line), skip parameter (current(1)), traced_error exception avec full_report, assert avec trace et source_location.
- **Comportement attendu** :
  - La pile d'appels affiche les noms de fonctions et les lignes source
  - Le skip parameter (current(1)) exclut la fonction courante de la trace
  - traced_error capture la trace au moment du throw
  - ASSERT affiche le fichier, la ligne et la pile en cas d'échec

---

## Fichiers .md sans exemples compilables

Les fichiers suivants ne génèrent pas d'exemples car leurs extraits de code sont conceptuels ou utilisent des fonctionnalités non encore supportées :

| Fichier | Raison |
|---------|--------|
| `09.1-avantages-performance.md` | Extraits conceptuels uniquement (types non définis : Config, Data, etc.) |
| `13-modules.md` | Modules C++20 — nécessitent un build system spécial (CMake) |
| `13.1-avantages-modules.md` | Modules C++20 — idem |
| `13.2-etat-support.md` | État du support — pas d'exemples autonomes |
| `13.3-maturite-2026.md` | Maturité — pas d'exemples autonomes |
| `14-cpp26-overview.md` | C++26 — pas encore supporté par les compilateurs |
| `14.1-contracts.md` | Contrats C++26 — pas encore supporté |
| `14.2-static-reflection.md` | Réflexion statique C++26 — pas encore supporté |
| `14.3-pattern-matching.md` | Pattern matching C++26/C++29 — pas encore supporté |
| `14.4-std-execution.md` | std::execution C++26 — pas encore supporté |
| `14.4.1-sender-receiver.md` | Senders/Receivers C++26 — pas encore supporté |
| `14.4.2-remplacement-async.md` | Remplacement async C++26 — pas encore supporté |
| `14.4.3-schedulers.md` | Schedulers C++26 — pas encore supporté |
| `14.5-etat-support-cpp26.md` | État du support — aucun code |
