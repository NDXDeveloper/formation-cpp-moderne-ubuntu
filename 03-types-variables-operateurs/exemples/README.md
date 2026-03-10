# Exemples — Chapitre 3 : Types, Variables et Opérateurs

Tous les exemples sont compilables avec `g++-15 -std=c++23 -Wall -Wextra`.

---

## Liste des exemples

### 01_1_auto.cpp
- **Section** : 3.1.1 — `auto` : Déduction automatique du type
- **Fichier source** : `01.1-auto.md`
- **Description** : Règles de déduction `auto`, déduction avec `const`, références (`auto&`, `auto&&`), initialisation par accolades, lambdas génériques, structured bindings, range-based for
- **Sortie attendue** : Affichage des valeurs déduites, vérification des types via `static_assert`
- **Comportement attendu** : Toutes les `static_assert` passent, les types déduits correspondent aux règles documentées

### 01_2_decltype.cpp
- **Section** : 3.1.2 — `decltype` : Extraction du type d'une expression
- **Fichier source** : `01.2-decltype.md`
- **Description** : Règles de `decltype` (identifiant vs expression), piège des parenthèses, `decltype(auto)`, trailing return type avec template, `execute_and_log` avec `if constexpr`
- **Sortie attendue** : Vérification de tous les types via `static_assert`, démonstration de `decltype(auto)` retournant une référence modifiable
- **Comportement attendu** : `get_first_ref()` retourne une référence, la modification via cette référence affecte le vecteur global

### 02_types_primitifs.cpp
- **Section** : 3.2 — Types primitifs, tailles et représentation mémoire
- **Fichier source** : `02-types-primitifs.md`
- **Description** : `sizeof` et `alignof` des types fondamentaux (`int`, `double`, `char`)
- **Sortie attendue** : `int` = 4 octets, `double` = 8 octets, `char` = 1 octet ; alignements correspondants
- **Comportement attendu** : Vérification de la relation d'ordre `sizeof(char) ≤ sizeof(short) ≤ sizeof(int) ≤ sizeof(long) ≤ sizeof(long long)`

### 02_1_entiers.cpp
- **Section** : 3.2.1 — Entiers : `int`, `long`, `short`, `int32_t`, `int64_t`
- **Fichier source** : `02.1-entiers.md`
- **Description** : Types signés/non signés, types à largeur fixe (`<cstdint>`), promotion intégrale, wrap-around non signé, comparaison signée/non signée, `std::ssize`, troncation, littéraux dans différentes bases
- **Sortie attendue** : `uint32_t(0) - 1 = 4294967295`, troncation de 5000000000 en 705032704, les 4 bases donnent 42
- **Comportement attendu** : Démonstration des pièges documentés (wrap-around, troncation)

### 02_2_flottants.cpp
- **Section** : 3.2.2 — Flottants : `float`, `double`, `long double`
- **Fichier source** : `02.2-flottants.md`
- **Description** : IEEE 754, limites (`numeric_limits`), NaN, infini, zéro négatif, erreurs d'arrondi (`0.1 + 0.2 ≠ 0.3`), comparaison epsilon, absorption
- **Sortie attendue** : `NaN != NaN`, `0.1 + 0.2` différent de `0.3`, absorption du 1.0 dans `1e16 + 1.0`
- **Comportement attendu** : Toutes les propriétés IEEE 754 vérifiées, `nearly_equal` retourne `true` pour `0.1+0.2` vs `0.3`

### 02_3_sizeof_alignof.cpp
- **Section** : 3.2.3 — `sizeof` et `alignof` : Taille et alignement
- **Fichier source** : `02.3-sizeof-alignof.md`
- **Description** : Padding des structures, `BadLayout` (32 octets) vs `GoodLayout` (16 octets), `alignas(64)`, `offsetof` sur `NetworkPacket`, `[[no_unique_address]]`, `Particle_Bad` vs `Particle_Good`
- **Sortie attendue** : Toutes les `static_assert` passent (tailles, offsets, alignements vérifiés)
- **Comportement attendu** : Démonstration que l'ordre des membres affecte la taille (32 vs 16 octets)

### 03_1_static_cast.cpp
- **Section** : 3.3.1 — `static_cast` : Conversions sûres
- **Fichier source** : `03.1-static-cast.md`
- **Description** : Conversions numériques (float→int, division flottante), conversions entre tailles d'entiers, signé↔non-signé, `enum` et `enum class`, `std::to_underlying` (C++23)
- **Sortie attendue** : `static_cast<int>(3.14159) = 3`, division flottante `3.5`, `-1` converti en `4294967295`
- **Comportement attendu** : Toutes les conversions produisent les résultats documentés

### 03_2_reinterpret_cast.cpp
- **Section** : 3.3.2 — `reinterpret_cast` : Réinterprétation mémoire
- **Fichier source** : `03.2-reinterpret-cast.md`
- **Description** : Inspection octet par octet (little-endian), conversion pointeur↔`uintptr_t`, `std::memcpy` et `std::bit_cast` pour lire les bits IEEE 754
- **Sortie attendue** : `0xDEADBEEF` affiché `EF BE AD DE` (little-endian), bits de `3.14f` = `0x4048F5C3`
- **Comportement attendu** : `bit_cast` et `memcpy` donnent le même résultat, `static_assert` vérifie la valeur

### 03_3_const_cast.cpp
- **Section** : 3.3.3 — `const_cast` : Manipulation de `const`
- **Fichier source** : `03.3-const-cast.md`
- **Description** : Retrait de `const` sur objet mutable, `TextBuffer` avec délégation const/non-const via `std::as_const`, `Cache` avec `mutable`
- **Sortie attendue** : Modification via `const_cast` = 99, TextBuffer modifiable, Cache calcule puis retourne depuis le cache
- **Comportement attendu** : Pattern de délégation const/non-const fonctionne, `mutable` permet la modification en contexte `const`

### 03_4_dynamic_cast.cpp
- **Section** : 3.3.4 — `dynamic_cast` : Cast polymorphique
- **Fichier source** : `03.4-dynamic-cast.md`
- **Description** : Downcast avec pointeurs (`nullptr` si échec), downcast avec références (`bad_cast` si échec), cross-cast entre branches d'héritage multiple, `typeid`
- **Sortie attendue** : Dog reconnu, Cat non reconnu comme Dog, `bad_cast` levée, cross-cast `Drawable*→Clickable*` réussi
- **Comportement attendu** : `dynamic_cast` sur `nullptr` retourne `nullptr`, `typeid` identifie le type dynamique

### 04_portee_duree_vie.cpp
- **Section** : 3.4 — Portée des variables et durée de vie
- **Fichier source** : `04-portee-duree-vie.md`
- **Description** : Portée de bloc, boucle `for`, `if` avec initialisation C++17, variable locale `static` (compteur persistant), prolongation de durée de vie par référence `const`
- **Sortie attendue** : `generate_id()` retourne 1, 2, 3 successivement ; la référence const prolonge la durée de vie du temporaire
- **Comportement attendu** : La variable statique persiste entre les appels, `int{}` vaut 0

### 05_const_constexpr_consteval_intro.cpp
- **Section** : 3.5 — `const`, `constexpr` et `consteval` — Introduction
- **Fichier source** : `05-const-constexpr-consteval.md`
- **Description** : Spectre d'évaluation — `constexpr square`, `consteval cube`, `consteval compile_time_hash`
- **Sortie attendue** : `square(5)=25`, `cube(3)=27`, hash de "user_created" calculé à la compilation
- **Comportement attendu** : Toutes les valeurs sont calculées au compile-time (`static_assert` passent)

### 05_1_const.cpp
- **Section** : 3.5.1 — `const` : Immutabilité à l'exécution
- **Fichier source** : `05.1-const.md`
- **Description** : Variables `const`, pointeurs `const` (3 variantes), références `const`, `Sensor` avec méthodes `const`, `const` par défaut, `const` vs `#define`
- **Sortie attendue** : Démonstration de la const-correctness — lecture/écriture selon les qualificateurs
- **Comportement attendu** : Le `Sensor` est accessible en lecture via référence `const`, modifiable via `update()`

### 05_2_constexpr.cpp
- **Section** : 3.5.2 — `constexpr` : Calcul à la compilation
- **Fichier source** : `05.2-constexpr.md`
- **Description** : Variables `constexpr`, `factorial`, `if constexpr` avec templates, `std::vector` constexpr (C++20), classe `Point` constexpr, lookup table, `clamp`, `is_power_of_two`, `rgb`, détection UB via `constexpr`
- **Sortie attendue** : `factorial(10)=3628800`, `sum_of_squares(10)=385`, `squares[42]=1764`, `red=0xFF0000`
- **Comportement attendu** : Toutes les `static_assert` passent, confirmant l'évaluation compile-time

### 05_3_consteval.cpp
- **Section** : 3.5.3 — `consteval` : Fonctions obligatoirement compile-time
- **Fichier source** : `05.3-consteval.md`
- **Description** : Table CRC `consteval`, hash FNV-1a dans un `switch`, validation de port, encodage RGBA, `if consteval` avec `fast_sqrt`, `consteval` appelant `constexpr`
- **Sortie attendue** : Hashes compile-time, switch sur hash de chaînes, couleurs RGBA, `fast_sqrt(2.0)` ≈ 1.414213562373095
- **Comportement attendu** : Toutes les valeurs sont résolues au compile-time, `quadruple(3)=12`

---

## Compilation

```bash
# Compiler un exemple
g++-15 -std=c++23 -Wall -Wextra -o nom_exemple nom_exemple.cpp

# Compiler tous les exemples
for f in *.cpp; do
    echo "Compilation de $f..."
    g++-15 -std=c++23 -Wall -Wextra -o "${f%.cpp}" "$f" || echo "ECHEC: $f"
done
```

## Nettoyage

```bash
# Supprimer les binaires
rm -f 01_1_auto 01_2_decltype 02_types_primitifs 02_1_entiers 02_2_flottants \
      02_3_sizeof_alignof 03_1_static_cast 03_2_reinterpret_cast 03_3_const_cast \
      03_4_dynamic_cast 04_portee_duree_vie 05_intro 05_1_const 05_2_constexpr \
      05_3_consteval
```
