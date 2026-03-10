# Exemples — Chapitre 6 : Classes et Encapsulation

Chaque fichier source correspond à une section du chapitre 6 et peut être compilé indépendamment avec :

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -o <nom> <fichier(s).cpp>
```

---

## Section 6.1 — Définition des classes

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `01_sensor_c_vs_cpp.cpp` | Comparaison struct C vs classe C++ | `g++ ... -o 01_sensor_c_vs_cpp 01_sensor_c_vs_cpp.cpp` | Affiche les valeurs du capteur en style C puis C++ |
| `01_rectangle.cpp` | Classe Rectangle avec area, perimeter, instanciation stack/heap | `g++ ... -o 01_rectangle 01_rectangle.cpp` | `12`, `10`, `70`, `12` |
| `01_this_pointer.cpp` | Pointeur `this` — Counter, Widget (désambiguïsation), QueryBuilder (method chaining) | `g++ ... -o 01_this_pointer 01_this_pointer.cpp` | Counter, Widget name, requête SQL chaînée |
| `01_const_methods.cpp` | Méthodes `const` — Circle avec `area()` const et `scale()` non-const | `g++ ... -o 01_const_methods 01_const_methods.cpp` | `78.5398`, `314.159` |
| `01_static_members.cpp` | Membres statiques — Connection counter, MathUtils::clamp | `g++ ... -o 01_static_members 01_static_members.cpp` | Compteur de connexions, `clamp(15,0,10) = 10` |
| `01_sensor_separated.h` / `.cpp` / `_main.cpp` | Séparation header/source — Sensor multi-fichiers | `g++ ... -o 01_sensor_separated 01_sensor_separated_main.cpp 01_sensor_separated.cpp` | `Temperature (#1): 23.5` |
| `01_dynarray_v1.h` / `.cpp` / `_main.cpp` | DynArray v1 — constructeur, destructeur, operator[], size, empty | `g++ ... -o 01_dynarray_v1 01_dynarray_v1_main.cpp 01_dynarray_v1.cpp` | Taille, accès, exception out_of_range |

---

## Section 6.2 — Constructeurs

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `02_constructeurs_intro.cpp` | Vue d'ensemble — Timestamp, Flexible(=default), Distance(explicit), Point init syntaxes, vector {} vs () | `g++ ... -o 02_constructeurs_intro 02_constructeurs_intro.cpp` | Divers constructeurs, `v1 size: 5`, `v2 size: 2` |

### Section 6.2.1 — Constructeur par défaut

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `02_1_constructeur_defaut.cpp` | Constructeur par défaut — A, B, ImplicitDefault, Config(=default), Widget, DynArray | `g++ ... -o 02_1_constructeur_defaut 02_1_constructeur_defaut.cpp` | Valeurs par défaut, Widget(), DynArray vide/rempli |

### Section 6.2.2 — Constructeur paramétré

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `02_2_constructeur_parametre.cpp` | Surcharge, délégation, factory functions (Color), validation (PortRange), DynArray délégué | `g++ ... -o 02_2_constructeur_parametre 02_2_constructeur_parametre.cpp` | RGB/hex/gray, délégation Connection, PortRange count, exception validation |

### Section 6.2.3 — Constructeur de copie

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `02_3_constructeur_copie.cpp` | Copie profonde — DynArray, vérification indépendance, copie tableau vide | `g++ ... -o 02_3_constructeur_copie 02_3_constructeur_copie.cpp` | `a[0]: 42`, `b[0]: 99`, `empty_copy size: 0` |

### Section 6.2.4 — Constructeur de déplacement

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `02_4_constructeur_deplacement.cpp` | Move semantics — DynArray, transfert O(1), état moved-from, retour de fonction | `g++ ... -o 02_4_constructeur_deplacement 02_4_constructeur_deplacement.cpp` | `5`, `42`, `0`, `1`, `range size: 10`, `range[9]: 9` |

### Section 6.2.5 — Liste d'initialisation

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `02_5_liste_initialisation.cpp` | Init list — Logger(référence), Server(defaults), Container({} vs ()), Correct(ordre), Dog(héritage) | `g++ ... -o 02_5_liste_initialisation 02_5_liste_initialisation.cpp` | Server ports/timeouts, `v1 size: 5`, `v2 size: 2`, `total: 640`, `Rex (German Shepherd), 4 legs` |

---

## Section 6.3 — Destructeurs et RAII

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `03_destructeurs_raii.cpp` | Destructeur automatique, ordre de destruction inversé, scope, temporaires, stack unwinding | `g++ ... -o 03_destructeurs_raii 03_destructeurs_raii.cpp` | Acquisition/Libération, ordre inversé, stack unwinding avec exception |

### Section 6.3.2 — Exemples pratiques de RAII

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `03_2_raii_exemples.cpp` | ScopedTimer (profiling), CoutFlagsGuard (restauration état cout), ScopeGuard (action fin de portée, dismiss) | `g++ ... -o 03_2_raii_exemples 03_2_raii_exemples.cpp` | Timers (µs variables), hex 0x000A/0x00FF/0x1000, flags restaurés, rollback/dismiss |

---

## Section 6.4 — Modificateurs d'accès

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `04_modificateurs_acces.cpp` | public/private/protected, friend — Shape/Circle, Temperature (validation), DynArray operator<<, Engine/Debugger friend class | `g++ ... -o 04_modificateurs_acces 04_modificateurs_acces.cpp` | Circle area/color, Temperature K/°C, DynArray affiché, Engine state |

---

## Section 6.5 — Règle des 5

### Section 6.5.3 — Opérateur d'affectation par copie

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `05_3_copy_assignment.cpp` | Copy assignment — allouer d'abord, auto-affectation, chaînage, swap/ADL, move assignment | `g++ ... -o 05_3_copy_assignment 05_3_copy_assignment.cpp` | Affectation, indépendance, auto-affectation, chaînage `e=d=c`, swap, rvalue |

### Section 6.5.5 — DynArray version finale (Règle des 5 complète)

| Fichier | Description | Compilation | Sortie attendue |
|---------|-------------|-------------|-----------------|
| `05_5_dynarray_final.h` / `.cpp` / `_main.cpp` | DynArray complet — destructeur, copie ctor/=, move ctor/=, swap, operator<<. Tous les scénarios testés. | `g++ ... -o 05_5_dynarray_final 05_5_dynarray_final_main.cpp 05_5_dynarray_final.cpp` | Construction, copie, move, affectation, auto-affectation, chaînage, swap, temporaires |

---

## Notes

- **Compilateur** : `g++-15` avec `-std=c++17 -Wall -Wextra -Wpedantic`
- **Sections sans exemple dédié** : les sections 6.3.1 (RAII concept), 6.5 (Règle des 5 vue d'ensemble), 6.5.1 (destructeur relecture), 6.5.2 (copie ctor relecture), 6.5.4 (move ctor relecture) sont des sections conceptuelles ou de relecture dont les exemples de code sont déjà couverts par les fichiers ci-dessus.
- Le fichier `05_5_dynarray_final` produit un warning attendu `-Wself-move` sur la ligne `e = std::move(e)` — c'est le piège d'auto-affectation par déplacement documenté dans la section 6.5.5.
