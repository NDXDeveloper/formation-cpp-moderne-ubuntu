# Exemples — Chapitre 11 : Lambdas

Compilateur : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g`

## Fichiers

### `11_01_syntaxe_captures.cpp`
- **Section** : 11.1 — Syntaxe des lambdas et types de captures
- **Source** : `01-syntaxe-captures.md`
- **Description** : Anatomie d'une lambda, type unique, sizeof, conversion en pointeur de fonction, qsort, modes de capture (explicite, défaut, mixte), moment de la capture, mot-clé mutable, compteur sûr, safe patterns, constexpr/consteval, inférence de type retour
- **Sortie attendue** : Chaque bloc affiche le résultat de la lambda correspondante avec les valeurs documentées dans le .md
- **Comportement attendu** : Compilation sans warning, toutes les assertions statiques passent

### `11_01_1_capture_valeur.cpp`
- **Section** : 11.1.1 — Capture par valeur `[=]`
- **Source** : `01.1-capture-valeur.md`
- **Description** : Copie au moment de la capture, capture explicite vs défaut, boucle (0 1 2 3 4), string snapshot, coût de la copie, `std::as_const`, `std::move`, pointeurs bruts, `shared_ptr`, types move-only (`unique_ptr`), capture de `this`
- **Sortie attendue** :
  - `10` (snapshot), `area = 480000`, `volume = 15360000`
  - `0 1 2 3 4` (boucle), `Hello, Alice!` (string snapshot)
  - `size = 1000000`, `moved size = 1000000, original empty = true`
  - `99` (pointeur brut), `42` (shared_ptr), `42` (unique_ptr)
  - `interval = 100` (capture de this)
- **Comportement attendu** : Compilation sans warning

### `11_01_2_capture_reference.cpp`
- **Section** : 11.1.2 — Capture par référence `[&]`
- **Source** : `01.2-capture-reference.md`
- **Description** : Mutation bidirectionnelle, algorithmes STL (analyze, prefix sums), boucle avec capture par valeur, capture de const, `std::as_const`, init captures, performance ref vs valeur, concurrence (atomic, async)
- **Compilation** : nécessite `-pthread`
- **Sortie attendue** :
  - `15`, `Total: 30, Count: 2`, `Average: 15`
  - `2`, `2`, `100` (mutation bidirectionnelle)
  - `Above: 3, Below: 3`, `3 4 8 9 14 23`
  - `counter = 200000`, `total = 200000` (concurrence)
- **Comportement attendu** : Compilation sans warning, résultats déterministes (atomic garantit 200000)

### `11_01_3_capture_this.cpp`
- **Section** : 11.1.3 — Capture de `this`
- **Source** : `01.3-capture-this.md`
- **Description** : `[this]` vs `[*this]`, dangling évité par copie, snapshot Config, `[*this]() mutable`, init capture de membres, héritage (Base/Derived), polymorphisme virtuel (Shape/Circle), `shared_from_this`
- **Sortie attendue** :
  - `temp_01: 23.5` (×2, une avec [this], une avec [*this] après destruction)
  - `1`, `2` (Counter [this]), `30` (Config snapshot)
  - `1`, `2`, `original count: 0` (indépendant [*this] mutable)
  - `filter(50) = false, filter(150) = true`
  - `Reconnecting to localhost:8080...`
  - `0:default`, `I am a Circle`, `Handling session: sess_001`
- **Comportement attendu** : Compilation sans warning, le callback [*this] survit à la destruction de l'objet original

### `11_01_4_captures_mixtes.cpp`
- **Section** : 11.1.4 — Captures mixtes et init captures
- **Source** : `01.4-captures-mixtes.md`
- **Description** : Captures mixtes `[=, &var]` et `[&, var]`, combinaison avec `this`, init captures (renommer, expression, move, boucle factory, ref const), évaluation unique, constexpr, anti-pattern this + init, masquage de variable
- **Sortie attendue** :
  - `Matches: 3, Sum: 60`, `ok1=6, ok2=6, ok3=6, ok4=6`
  - `Area: 480000, Aspect ratio: 1.33`
  - `item_0` à `item_4`, `ptr is null: true`
  - `Size after move: 0`, `1 2 3 4 5 6 7`
  - `[OK] cpu: 78.5%`, `[OK] memory: 62.3%`, `[HIGH] disk: 91.0%`
  - `original`, `modified`, `hello world`
  - `1`, `1`, `1` (évaluation unique)
  - `Snapshot port: 8080, Current port: 9090`
  - `10`, `20` (masquage)
- **Comportement attendu** : Compilation sans warning

### `11_02_lambdas_generiques.cpp`
- **Section** : 11.2 — Lambdas génériques
- **Source** : `02-lambdas-generiques.md`
- **Description** : Lambdas avec `auto`, `const auto&`, `auto&&`, variadic, logger, lambdas templatées C++20 (`<typename T>`), NTTP, combinaison auto+template, concepts (`requires`, abrégés), comparateurs, projections `by_field`, visiteur `std::variant` (overloaded), catch-all `const auto&`, déduction type retour, merge, constexpr
- **Sortie attendue** :
  - `7`, `4.2`, `Hello, World` (add générique)
  - `string: hello` (variant — pas "other type")
  - `By age: Bob(25) Alice(30) Carol(35)`
  - `1 2 3 4 5 6` (merge), `max_of(100,42) = 100`
- **Comportement attendu** : Compilation sans warning, le catch-all `const auto&` ne vole pas le match string

### `11_03_lambdas_stl.cpp`
- **Section** : 11.3 — Lambdas et algorithmes STL
- **Source** : `03-lambdas-stl.md`
- **Description** : find_if, find_if_not, count_if, all_of/any_of/none_of, prédicats avec capture, sort (comparateur, multi-critères, std::tie), stable_sort, partial_sort, transform (toupper, in-place, binaire), for_each (Stats), erase-remove, erase_if C++20, copy_if, partition, accumulate (sum, product, string, type change), generate (fibonacci, IDs), generate_n (random), analyze_logs, set avec comparateur lambda, inner_product, adjacent_difference, transform_reduce, comparateurs génériques, field_comparator, avant/après C++03 vs C++14
- **Sortie attendue** :
  - `Premier > 20 : 25`, `Premier impair : 7`
  - `Mots commençant par 'a' : 3`
  - `Top 3: 99 95 88`, `ALICE BOB CAROL`
  - `0 1 1 2 3 5 8 13 21 34` (fibonacci)
  - `total = 190` (accumulate Orders)
  - `avg luminosity = 85.0` (transform_reduce Pixel)
- **Comportement attendu** : Compilation sans warning

### `11_04_std_function.cpp`
- **Section** : 11.4 — std::function et wrappers de fonctions
- **Source** : `04-std-function.md`
- **Description** : Callable objects (fn_ptr, lambda, foncteur, bind), std::function (assignation, validité, réinitialisation), SBO, coût type erasure, callbacks (Button), signal/slot, concepts (predicate, invocable), std::invoke (fonction, lambda, membre, data member), logged_call template, std::bind, std::move_only_function (C++23), const vs mutable, bonnes pratiques (const& vs valeur)
- **Sortie attendue** :
  - `7` (×4 callables), `12` (multiplication), `17` (avec offset)
  - `callback est vide`, `Pas de callback enregistré`
  - `sizeof(std::function<void()>) = 32`
  - `Form submitted!`, `Logger: 42`, `UI update: 42`
  - `6 7 8` (process_concept), `50` (bind × lambda)
  - `42` (move_only_function), `const: 42`, `mut1: 1`, `mut2: 2`
- **Comportement attendu** : Compilation sans warning
