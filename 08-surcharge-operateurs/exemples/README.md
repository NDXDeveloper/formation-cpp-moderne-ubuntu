# Chapitre 8 — Surcharge d'Opérateurs et Conversions : Exemples

## Compilation

Chaque fichier est autonome et se compile individuellement :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o nom_exemple nom_exemple.cpp
./nom_exemple
```

- **Compilateur** : GCC 15 (`g++-15`)
- **Standard** : C++23 (`-std=c++23`)
- **Warnings** : `-Wall -Wextra -Wpedantic`
- **Debug** : `-g`
- **Aucune dépendance externe**, pas de CMake ni de bibliothèque tierce

---

## Section 8.1 — Opérateurs arithmétiques et de comparaison

| Fichier | Description | Source | Sortie attendue |
|---------|-------------|--------|-----------------|
| `01_1_vec2_complet.cpp` | Vec2 complet : opérateurs composés (+=, -=, *=, /=), binaires (+, -, *, /), unaires (-, +), norme, comparaison (==, <=>), flux (<<) | `01-operateurs-arithmetiques.md` | Affiche résultats des opérations sur Vec2 |
| `01_1_vec2_spaceship.cpp` | Vec2 avec `operator<=> = default` (C++20) — génère les 6 opérateurs de comparaison | `01-operateurs-arithmetiques.md` | `true true true true` |
| `01_1_hidden_friend.cpp` | Pattern hidden friend — opérateurs visibles uniquement par ADL | `01-operateurs-arithmetiques.md` | Addition et comparaison via ADL |
| `01_1_fraction.cpp` | Fraction complète : arithmétique (+, -, *, /), comparaison (<=>), flux (<<), réduction par PGCD, opérations mixtes Fraction/int | `01-operateurs-arithmetiques.md` | Opérations sur fractions avec résultats réduits |

## Section 8.2 — Opérateurs d'affectation composés

| Fichier | Description | Source | Sortie attendue |
|---------|-------------|--------|-----------------|
| `02_copy_and_swap.cpp` | Buffer avec copy-and-swap idiom — operator= exception-safe, move assignment | `02-operateurs-affectation.md` | Copie, move et self-assignment corrects |
| `02_permissions.cpp` | Opérateurs bit-à-bit (\|=, &=) pour flags de permissions (Lecture, Écriture, Exécution) | `02-operateurs-affectation.md` | `r-x` → `rwx` → `rw-` |
| `02_string.cpp` | String avec Rule of Five, copy-and-swap, operator+= et operator+ | `02-operateurs-affectation.md` | Concaténation et move semantics |

## Section 8.3 — Opérateurs de conversion

| Fichier | Description | Source | Sortie attendue |
|---------|-------------|--------|-----------------|
| `03_pourcentage.cpp` | Conversion implicite vs explicite vers double — démo des dangers de la conversion implicite | `03-operateurs-conversion.md` | Conversions implicites et explicites |
| `03_explicit_bool.cpp` | `explicit operator bool()` — conversion contextuelle (if, while, !, &&, \|\|, ?:) | `03-operateurs-conversion.md` | Contextes booléens autorisés |
| `03_optionalref.cpp` | OptionalRef<T> avec operator*, operator->, operator bool | `03-operateurs-conversion.md` | Déréférencement et test de nullité |
| `03_identifiant.cpp` | Conversion implicite vers string_view, explicite vers string | `03-operateurs-conversion.md` | Affichage et conversions d'Identifiant |

## Section 8.4 — Opérateur d'appel de fonction (operator())

| Fichier | Description | Source | Sortie attendue |
|---------|-------------|--------|-----------------|
| `04_salueur.cpp` | Foncteur basique avec état configurable (préfixe de salutation) | `04-operateur-appel.md` | `Bonjour, Alice !` et `Salut, Bob !` |
| `04_superieur_find.cpp` | Foncteur SuperieurA vs fonction libre avec std::find_if | `04-operateur-appel.md` | `true false false`, premier > 10 = 12, premier > 15 = 18 |
| `04_stl_foncteurs.cpp` | Foncteurs prédéfinis STL : std::greater, std::plus, std::sort avec comparateur | `04-operateur-appel.md` | Tri décroissant, somme = 25 |
| `04_compteur.cpp` | Foncteur mutable (operator() non-const) + std::ref avec algorithmes STL | `04-operateur-appel.md` | Multiplication cumulative, total = 5 après for_each |
| `04_formateur_visiteur.cpp` | Surcharges multiples de operator() — Formateur par type + std::visit sur std::variant | `04-operateur-appel.md` | Formatage typé et visite de variant |
| `04_cache_policy.cpp` | Policy-based design — Cache paramétré par foncteur (ToutCacher, PetitesValeurs) | `04-operateur-appel.md` | cache_complet: 2, cache_leger: 1 |
| `04_std_function.cpp` | std::function — effacement de type pour stocker foncteurs, lambdas et fonctions libres | `04-operateur-appel.md` | `true false true` |
| `04_lambda_foncteur.cpp` | Équivalence lambda/foncteur — capture, mutable, lambda générique | `04-operateur-appel.md` | Filtre, compteur incrémental, affichage multi-type |

## Section 8.5 — Opérateur spaceship <=> (C++20)

| Fichier | Description | Source | Sortie attendue |
|---------|-------------|--------|-----------------|
| `05_threeway_basic.cpp` | Comparaison tripartite de base — strong_ordering, partial_ordering, NaN | `05-operateur-spaceship.md` | `a < b`, greater = true, unordered = true |
| `05_case_insensitive.cpp` | weak_ordering — CaseInsensitiveString, "Hello" ≡ "HELLO" | `05-operateur-spaceship.md` | `Hello == HELLO → true`, comparaisons insensibles à la casse |
| `05_etudiant.cpp` | `= default` — comparaison automatique membre par membre (lexicographique) | `05-operateur-spaceship.md` | `true true true true` |
| `05_enregistrement.cpp` | Implémentation personnalisée — ignorer timestamp_ dans la comparaison | `05-operateur-spaceship.md` | Égalité malgré timestamps différents |
| `05_angle.cpp` | Comparaison avec transformation — normalisation modulo 360°, partial_ordering | `05-operateur-spaceship.md` | `Angle(30) == Angle(390) → true` |
| `05_version.cpp` | Comparaison hétérogène Version vs int — réécriture automatique (`1 < v`) | `05-operateur-spaceship.md` | `true true true` |
| `05_cle_set.cpp` | <=> et conteneurs STL — std::set et std::map avec operator< généré par <=> | `05-operateur-spaceship.md` | ensemble: 2, dictionnaire: 2 |
| `05_date.cpp` | Exemple complet — Date avec = default, operator<<, std::sort | `05-operateur-spaceship.md` | Comparaisons et tri chronologique |
