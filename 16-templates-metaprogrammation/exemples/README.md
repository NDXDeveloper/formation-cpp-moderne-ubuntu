# Chapitre 16 — Templates et métaprogrammation : Exemples

## Prérequis

- **Compilateur** : GCC 15 (`g++-15`)
- **Standard** : C++23 (`-std=c++23`)
- **Flags recommandés** : `-Wall -Wextra -Wpedantic -g`

## Compilation et exécution

Chaque exemple se compile et s'exécute de manière indépendante :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o <nom_sans_ext> <nom>.cpp
./<nom_sans_ext>
```

Exemple :
```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 01_syntaxe_base 01_syntaxe_base.cpp
./01_syntaxe_base
```

Pour compiler et exécuter tous les exemples d'un coup :
```bash
for f in *.cpp; do
    nom="${f%.cpp}"
    echo "=== Compilation de $f ==="
    g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o "$nom" "$f" && ./"$nom"
    echo
done
```

Pour nettoyer les binaires :
```bash
for f in *.cpp; do rm -f "${f%.cpp}"; done
```

---

## 01_syntaxe_base.cpp
- **Section** : 16.1 — Syntaxe de base des templates de fonctions
- **Fichier source** : `01-syntaxe-base.md`
- **Description** : Template `maximum` (déduction, instanciation explicite), `addition` multi-types, NTTP `multiplier<N>`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 01_syntaxe_base 01_syntaxe_base.cpp`
- **Sortie attendue** :
  ```
  max(3,7) = 7
  max(2.5,1.3) = 2.5
  max(hello,world) = world
  max<double>(3,2.5) = 3
  addition(1,2.5) = 3.5
  multiplier<3>(5) = 15
  multiplier<10>(7) = 70
  ```

---

## 01_nttp_auto.cpp
- **Section** : 16.1 — NTTP avec `auto` (C++17)
- **Fichier source** : `01-syntaxe-base.md`
- **Description** : `Constant<auto V>`, `std::array<T, N>` avec NTTP, `RepeatString<N>`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 01_nttp_auto 01_nttp_auto.cpp`
- **Sortie attendue** :
  ```
  42
  3.14
  true
  sum(array) = 15
  hahaha
  ```

---

## 01_surcharge_defaut.cpp
- **Section** : 16.1 — Surcharge et valeurs par défaut
- **Fichier source** : `01-syntaxe-base.md`
- **Description** : Surcharge template/non-template, paramètres par défaut, `puissance` récursif constexpr
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 01_surcharge_defaut 01_surcharge_defaut.cpp`
- **Sortie attendue** :
  ```
  traiter(42) = entier (non-template)
  traiter(3.14) = type general: 3.14
  traiter(hello) = type general: hello
  creer<string>(hello) = hello (string = std::string)
  puissance<2>(5) = 25
  puissance<3>(3) = 27
  puissance<0>(99) = 1
  ```

---

## 02_stack_ctad.cpp
- **Section** : 16.2 — Templates de classes (Stack, CTAD, Pair)
- **Fichier source** : `02-classes-templates.md`
- **Description** : `Stack<T>` (push/pop/top/size), CTAD avec deduction guide, `Pair<T,U>` avec CTAD
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 02_stack_ctad 02_stack_ctad.cpp`
- **Sortie attendue** :
  ```
  top: 3 size: 3
  top: world size: 2
  pair: {42, hello}
  pair2: {1, 3.14}
  ```

---

## 02_nttp_statique.cpp
- **Section** : 16.2 — NTTP, membres statiques, alias
- **Fichier source** : `02-classes-templates.md`
- **Description** : `FixedBuffer<T,N>`, `Grid<Rows,Cols,T>`, `Compteur<T>` static, `DynamicArray<T>` alias, `afficher_premier`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 02_nttp_statique 02_nttp_statique.cpp`
- **Sortie attendue** :
  ```
  buf[0] = 42
  grid(0,0) = 1
  a=1, b=2, total=2
  da size: 3
  Premier: 10
  ```

---

## 02_heritage_friend.cpp
- **Section** : 16.2 — Héritage, friend, mixin, itérateur
- **Fichier source** : `02-classes-templates.md`
- **Description** : `LinkedList<T>` avec `Iterator`, héritage `Base/Derived`, mixin `Loggable<T>`, `Wrapper<T>` friend `operator==`, `StringMap<V>` alias, `Boite<T>` lazy instantiation
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 02_heritage_friend 02_heritage_friend.cpp`
- **Sortie attendue** :
  ```
  Liste: 3 2 1
  Derived value: 42
  [LOG] App started
  w1 == w2: true
  map["cle"] = 42
  boite: 42
  ```

---

## 03_specialisation.cpp
- **Section** : 16.3 — Spécialisation totale et partielle
- **Fichier source** : `03-specialisation.md`
- **Description** : `Serializer` (générique, `std::string`, `bool`, `T*`, `std::vector<T>`), `convertir`, `traiter`, `FixedBuffer` spécialisé bool, traits `is_pointer`/`remove_const`, `Formatter` member specialization
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 03_specialisation 03_specialisation.cpp`
- **Sortie attendue** :
  ```
  int: [42]
  string: "hello"
  bool: true
  ptr: *[42]
  vector: {[1],[2],[3]}
  convertir<double>(42) = 42
  convertir<string>(42) = 42
  traiter(42) -> entier: 42
  traiter(3.14) -> flottant: 3.14
  traiter(hello) -> type general
  FixedBuffer<bool,8>: 0 1 0 0 0 0 0 0
  is_pointer<int> = false
  is_pointer<int*> = true
  remove_const: true
  Formatter<int>: [42]
  Formatter<string>: 'hello'
  ```

---

## 04_sfinae.cpp
- **Section** : 16.4 — SFINAE
- **Fichier source** : `04-sfinae.md`
- **Description** : `extraire` (container vs scalaire), `enable_if` (return type, paramètre), `NumericOps<T>`, `decltype`/`declval`, `void_t` traits (`has_size`, `has_value_type`, `has_begin_end`), `is_iterable`, alias `require_`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 04_sfinae 04_sfinae.cpp`
- **Sortie attendue** :
  ```
  extraire(vector) = 1
  extraire(42) = 42
  doubler(21) = 42
  traiter(42) = entier: 42
  traiter(3.14) = flottant: 3.14
  abs(-5) = 5
  carre(4) = 16
  multiply result type = double
  has_size<vector<int>> = true
  has_size<int> = false
  has_value_type<vector<int>> = true
  has_begin_end<vector<int>> = true
  is_iterable<vector<int>> = true
  is_iterable<int> = false
  process(42) = entier
  process(3.14) = flottant
  ```

---

## 05_variadic_templates.cpp
- **Section** : 16.5 — Variadic templates
- **Fichier source** : `05-variadic-templates.md`
- **Description** : `sizeof...`, expansion de pack, récursion, `if constexpr`, `SimpleTuple`, `Emetteur<Listeners...>`, pattern Overload, `afficher_tuple`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 05_variadic_templates 05_variadic_templates.cpp`
- **Sortie attendue** :
  ```
  somme(1,2,3,4,5) = 15
  somme() = 0
  1 hello 3.14
  min_de(3,1,4,1,5) = 1
  Tuple: get<0>=42, get<1>=hello, get<2>=3.14
  Event recu (Console): click
  Event recu (File): click
  Overload: entier 42
  Overload: double 3.14
  Overload: string hello
  Tuple: [42, hello, 3.14]
  ```

---

## 06_concepts.cpp
- **Section** : 16.6 — Concepts (C++20) pour contraindre les templates
- **Fichier source** : `06-concepts.md`
- **Description** : Concepts `Numeric`/`Addable`, trois syntaxes d'utilisation, résolution de surcharge avec subsomption, `Counter`/`Wrapper` contraints, concepts + variadic
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 06_concepts 06_concepts.cpp`
- **Sortie attendue** :
  ```
  s1: 42
  s2: 42
  s3: 42
  tripler: 42
  ajouter(10, 5) = 15
  traiter(3.14) = type quelconque
  traiter(42u) = type entier
  traiter(42) = type entier signe
  Counter<int>: 11
  Wrapper numerique
  Wrapper generique
  1 2.0 hello
  Addable OK
  ```

---

## 06_1_requires.cpp
- **Section** : 16.6.1 — Syntaxe `requires`
- **Fichier source** : `06.1-requires.md`
- **Description** : Clause `requires` (positions), expression `requires`, 4 types d'exigences (simple, type, composée, imbriquée), subsomption `Animal`/`Oiseau`, `SmartContainer`, `formater` avec `requires` + `if constexpr`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 06_1_requires 06_1_requires.cpp`
- **Sortie attendue** :
  ```
  Concepts basiques OK
  Type petit (4 octets): 42
  Type petit (8 octets): 3.14
  decrire(Chien) = Un animal
  decrire(Aigle) = Un oiseau
  1 2 3
  sum = 6
  42
  3.14
  ```

---

## 06_2_concepts_standard.cpp
- **Section** : 16.6.2 — Concepts standard de la STL
- **Fichier source** : `06.2-concepts-standard.md`
- **Description** : Tous les concepts standard par famille (core, arithmetic, object, comparison, callable, iterator, range), fonctions `compter`, `trouver_min_max`, `creer`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 06_2_concepts_standard 06_2_concepts_standard.cpp`
- **Sortie attendue** :
  ```
  static_assert all passed
  Callable concepts OK
  compter(vector) = 5
  Min: 1, Max: 9
  Min: 2, Max: 8
  creer<string> = hello
  ```

---

## 06_3_concepts_personnalises.cpp
- **Section** : 16.6.3 — Création de concepts personnalisés
- **Fichier source** : `06.3-concepts-personnalises.md`
- **Description** : `Numeric`/`SignedNumeric`/`SortableRange`, `Serializable`, `StlContainer`, `InsertableInto`, hiérarchie `Named`/`Describable`/`JsonExportable`, `LogPolicy`/`Service`, `buffer_traits`/`Bufferable`, `FilterableBy`
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 06_3_concepts_personnalises 06_3_concepts_personnalises.cpp`
- **Sortie attendue** :
  ```
  app:v3
  vector: 42
  set size: 1
  Nom : Simple
  Desc: descriptible
  {"name":"Json"}
  [1] Service demarre
  [1] Service termine
  2 4 6 8
  ```

---

## 07_fold_expressions.cpp
- **Section** : 16.7 — Fold expressions (C++17)
- **Fichier source** : `07-fold-expressions.md`
- **Description** : Les 4 formes de fold, `somme`/`produit`, `print_all`, `tous_vrais`/`au_moins_un`, `est_parmi`, `log_stream` (left fold `<<`), `push_all`, concepts + fold (`somme_entiers`, `AllRegular`/`MultiStore`), `somme_carres`, `appliquer`, `afficher_indexed`, associativité (`sub_left`/`sub_right`)
- **Compilation** : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g -o 07_fold_expressions 07_fold_expressions.cpp`
- **Sortie attendue** :
  ```
  15
  24
  0
  1
  1 hello 3.14 true
  true
  false
  true
  true
  false
  User Alice logged in at 1709123456
  1 2 3 4 5
  somme_entiers = 6
  MultiStore OK
  somme_carres = 30
  2 4 6
  [0] = 42 [1] = 3.14 [2] = hello
  sub_left(10,3,2,1) = 4
  sub_right(10,3,2,1) = 8
  ```
