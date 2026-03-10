# Chapitre 10 — Sémantique de Mouvement : Exemples compilables

## Compilation

Tous les exemples se compilent avec GCC 15 en C++23 :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o nom_executable nom_fichier.cpp
```

**Flags utilisés :**

- `-std=c++23` : standard C++23 (requis pour `std::print`)
- `-Wall -Wextra -Wpedantic` : warnings maximum
- `-g` : symboles de débogage

**Aucune dépendance externe** : tous les exemples utilisent uniquement la bibliothèque standard.

---

## Liste des exemples

### 10_intro_buffer.cpp

- **Section :** 10.0 — Introduction
- **Fichier source :** README.md
- **Description :** Buffer avec copie profonde vs déplacement, catégories de valeurs (lvalue → copie, rvalue → move, `std::move` → move), vector move (10 millions d'éléments).
- **Sortie attendue :**
  - `[Buffer] Allocation de 1024 octets` puis `[Buffer] Libération`
  - Copie : `[Buffer] Copie de 1024 octets`
  - Move : `[Buffer] Déplacement de 1024 octets (coût : ~0)`
  - Vector : `deplace.size() = 10000000`, `v.size() après move = 0`

### 10_lvalues_rvalues.cpp

- **Section :** 10.1 — L-values vs R-values (&&)
- **Fichier source :** 01-lvalues-rvalues.md
- **Description :** Lvalues (variables, pointeurs, références), rvalues (littéraux, temporaires), test d'adresse, références rvalue (`T&&`), `const T&` lié aux rvalues, paradoxe (référence rvalue nommée est une lvalue), surcharge lvalue/rvalue (MyString avec copie et move), forwarding references et `auto&&`.
- **Sortie attendue :**
  - Lvalues : `x = 10`, `ref = 10`
  - Rvalues : `3.14 + 2.0 = 5.14`
  - Références rvalue : `rref = 42`, `rref2 = 43`, `rref3 = Hi`
  - Paradoxe : `a (copie) = Hello`, `b (move) = Hello`, `s après move = ''`
  - Surcharge : `[copie]` pour `b = a`, `[move]` pour `d = std::move(a)`, RVO pour `c = MyString("World")` (pas de trace move)
  - Forwarding ref : `a = 42, b = 42`

### 10_std_move.cpp

- **Section :** 10.2 — std::move : Transfert de propriété sans copie
- **Fichier source :** 02-std-move.md
- **Description :** Modèle mental de `std::move` (cast, pas déplacement), transfert `unique_ptr`, Service (pass-by-value + move), `push_back` avec move, move sur `const` (copie silencieuse), move sur primitifs (inutile), conteneurs (`vector`, `map`), `std::swap`, algorithme `std::move`, `emplace_back`, classe Connection (move avec neutralisation `fd = -1`).
- **Sortie attendue :**
  - Modèle mental : `s = 'Hello'` après move sans récepteur, `s = ''` après move avec récepteur
  - `unique_ptr` : `config est nul: true`
  - `push_back` : `s = ''` après move
  - Move const : `s après move const = 'Hello'` (copie, pas move)
  - Primitif : `x = 42, y = 42` (inchangé)
  - Conteneurs : `v1.size() = 0`, `m1.size() = 0` après move
  - `std::swap` : tailles échangées (2000000, 1000000)
  - Algorithme move : source vidée (`'' '' ''`)
  - Connection : `fd=-1` après move, `fd=42` sur la destination

### 10_move_constructors.cpp

- **Section :** 10.3 — Move constructors et move assignment operators
- **Fichier source :** 03-move-constructors.md
- **Description :** Buffer (move constructor, move assignment operator), Session (move récursif de membres `string`, `unique_ptr`, `vector`), idiome copy-and-swap (un seul `operator=` pour copie et move), vérification `noexcept` avec `static_assert`, Règle du 0 (UserProfile), ResourceHandle complet (Règle des 5), `= default` avec destructeur virtuel.
- **Sortie attendue :**
  - Buffer : `[Buffer] Copie`, `[Buffer] Move`, `[Buffer] Move assign`
  - Session : `id='ABC', conn=localhost` → après move : `id='', conn=null`
  - Copy-and-swap : copie + swap, puis move + swap
  - `noexcept` : `static_assert` passe pour Buffer, SwapBuffer
  - Règle du 0 : UserProfile move sans déclaration explicite
  - ResourceHandle : toutes les opérations (copie, move, move assign, copy assign) fonctionnent
  - `= default` : `Base` est nothrow move constructible malgré le destructeur virtuel

### 10_perfect_forwarding.cpp

- **Section :** 10.4 — Perfect Forwarding avec std::forward
- **Fichier source :** 04-perfect-forwarding.md
- **Description :** Perte de catégorie de valeur (v1 const T&, v2 T&& sans forward → toujours copie), perfect forwarding avec `std::forward<T>` (copie pour lvalue, move pour rvalue), `mon_make_unique` (implémentation simplifiée), `emplace_back` vs `push_back`, wrapper avec logging (`avec_log`), wrapper chrono (`chrono_mesure`), lambdas C++20 avec forwarding (C++14 vs C++20), capture par forwarding, piège multi-forward.
- **Sortie attendue :**
  - v1/v2 : `[copie]` pour toutes les variantes
  - `creer` avec forward : `[copie]` pour lvalue, `[move]` pour rvalue
  - `mon_make_unique` : Config créé correctement, `nom` vidé après move
  - `emplace_back` : `cle` vidée après move
  - Wrapper log : `[LOG] Début/Fin`, widget créé par move
  - Chrono : durée affichée en µs
  - Lambdas : `destination` appelée avec C++14 et C++20
  - Capture : `nom` vidé à la capture, widget créé à l'exécution
  - Multi-forward : `g` reçoit la valeur, `h` reçoit le forward

### 10_rvo_copy_elision.cpp

- **Section :** 10.5 — Return Value Optimization (RVO) et Copy Elision
- **Fichier source :** 05-rvo-copy-elision.md
- **Description :** NRVO avec Trace (Construction + Destruction uniquement), RVO obligatoire C++17 (NonCopiable sans constructeur de copie ni move), NRVO chemin unique, multiples chemins (move implicite), `construire_chemin`, RVO vector prvalue, retour par valeur moderne, pessimisation `return std::move(v)` vs `return v`, Builder avec ref-qualifier `&&`, paramètre move implicite C++20.
- **Sortie attendue :**
  - NRVO : `[Trace] Construction` + `[Trace] Destruction` seulement (pas de COPIE ni MOVE)
  - RVO C++17 : `NonCopiable` construit sans copie ni move (constructeurs `= delete`)
  - NRVO chemin unique : `'Base'` et `'Base — détails complets'`
  - Multi-chemins : `'Alpha'` et `'Beta'` (move implicite)
  - `construire_chemin` : `'/home/data.txt'`
  - RVO vector : `{1, 2, 3, 4, 5}`
  - Retour par valeur : `r[10] = 100`
  - Pessimisation : `return v` → 0 move (NRVO), `return std::move(v)` → 1 MOVE
  - Builder : `texte2 = 'Hello World'`
  - Paramètre : `'Hello transformé'`
- **Note :** Le warning `-Wpessimizing-move` est attendu sur `generer_mauvais()` — c'est exactement ce que la formation enseigne.
