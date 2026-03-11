# Exemples — Chapitre 13 : Conteneurs Séquentiels

Compilation : `g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g`  
Benchmarks : ajouter `-O2`  

---

## 13.1 — std::vector (01-vector.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `01_vector_01_declaration.cpp` | Déclaration et inclusion | Déclaration avec différentes initialisations | `mots contient 2 éléments` |
| `01_vector_02_constructions.cpp` | Constructions courantes | Vide, taille, init list, copie, move, itérateurs, CTAD | `v6 : size=5, v4 : size=0` |
| `01_vector_03_ajout.cpp` | Ajout d'éléments | push_back et emplace_back | `Nombre de logs : 3` |
| `01_vector_04_acces.cpp` | Accès aux éléments | [], at(), front(), back(), data() | `v[2] = 30`, `front = 10`, `back = 50` |
| `01_vector_05_suppression.cpp` | Suppression d'éléments | pop_back, erase, clear | `size=0, empty=true` |
| `01_vector_06_parcours.cpp` | Parcours | Range-for, index, itérateurs | `10 20 30 40 50` (trois fois) |
| `01_vector_07_size_capacity.cpp` | Size vs Capacity | Doublement de la capacité lors de push_back | Capacités : 1→2→4→8→16 |
| `01_vector_08_bool.cpp` | vector\<bool\> | Proxy object au lieu de référence | `flags[0] = true`, `size = 3` |

## 13.1.1 — Fonctionnement interne (01.1-fonctionnement-interne.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `01_1_reallocs.cpp` | Amortissement | Compteur de réallocations sur 1M éléments | 21 réallocations, capacité 1048576 |
| `01_1_reserve.cpp` | reserve() | Pré-allocation de capacité | `size=0, capacity=1000000` puis `size=1000000, capacity=1000000` |
| `01_1_bench_reserve.cpp` | Benchmark reserve() | Comparaison avec/sans reserve (compiler avec -O2) | Avec reserve plus rapide |
| `01_1_reserve_vs_resize.cpp` | reserve() vs resize() | Différence entre allouer la capacité et créer des éléments | `size=0` vs `size=100`, `v3[50] = -1` |
| `01_1_shrink.cpp` | shrink_to_fit() | Libération de la mémoire excédentaire | `capacity=10000` → `capacity=0` |
| `01_1_swap.cpp` | Idiome swap | Technique pré-C++11 pour libérer la mémoire | `size=0, capacity=0` |
| `01_1_noexcept.cpp` | noexcept move | Copies vs moves lors des réallocations | Sans noexcept : 2 copies ; avec : 0 copie |
| `01_1_emplace.cpp` | emplace_back vs push_back | Trace des constructeurs appelés | push_back : Constructeur+Move ; emplace_back : Constructeur seul |
| `01_1_visu.cpp` | Visualisation interne | Réallocations, adresses, relation taille/capacité | Changements d'adresse à chaque réalloc, `Adresse constante : oui` avec reserve |

## 13.1.2 — Méthodes essentielles (01.2-methodes-essentielles.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `01_2_push_emplace.cpp` | push_back / emplace_back | Copie, move, construction en place | `[bonjour] [bonjour] [monde] [xxxxx]` |
| `01_2_insert.cpp` | insert | Insertion à position arbitraire (valeur, copies, plage, init list) | `0 7 8 9 0 0 10 20 25 30 40 99 98 97` |
| `01_2_emplace_point.cpp` | emplace | Construction en place d'un Point | `(1, 2)`, `(3, 4)`, `(5, 6)` |
| `01_2_erase.cpp` | pop_back / erase | Suppression par itérateur et plage | `size=2, back=20` ; `10 50 60` |
| `01_2_erase_remove.cpp` | erase-remove / erase_if | Suppression par critère (pré-C++20 et C++20) | `1 3 5 7 9` ; `1 3 5` |
| `01_2_acces.cpp` | Accès complet | [], at(), front/back, data(), clear | Valeurs attendues + exception out_of_range |
| `01_2_iterateurs.cpp` | Itérateurs | Base, inverses, arithmétique | `20 40 60 80 100` ; `50 40 30 20 10` ; distances et comparaisons |
| `01_2_comparaison.cpp` | Comparaison | ==, <, spaceship <=> | `a == b : true`, `a < c : true`, `a < b` |
| `01_2_swap_assign.cpp` | swap / assign | Échange O(1) et remplacement de contenu | `a : 100 200` ; `7 8 9 10` |

## 13.1.3 — Invalidation des itérateurs (01.3-invalidation-iterateurs.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `01_3_erase_loop.cpp` | Suppression en boucle | Pattern correct avec retour de erase() | `1 3 5 7` |
| `01_3_index.cpp` | Index vs itérateurs | Robustesse des index face aux réallocations | `Bob` via index et via pointeur |
| `01_3_insert_index.cpp` | Insertion par index | Insertion sûre pendant un parcours | `1 99 2 3` |
| `01_3_end.cpp` | end() invalide | Travailler avec la taille initiale | `1 2 3 4 5 10 20 30 40 50` |

## 13.2 — std::array (02-array.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `02_array_01_declaration.cpp` | Déclaration | Type explicite, CTAD, value-initialization | `a : size=5, b : size=3` |
| `02_array_02_vs_c.cpp` | array vs tableau C | Décroissance, copie, comparaison, tri STL | `10 20 30 40 50 60` |
| `02_array_03_init_fill.cpp` | Initialisation / fill | Partielle, vide, fill(42) | `c[0]=0` ; `42 42 42 42 42` |
| `02_array_04_to_array.cpp` | to_array (C++20) | Conversion tableau C → std::array | `a.size()=4, b.size()=3, c.size()=2` |
| `02_array_05_api.cpp` | API complète | [], at, front/back, data, itérateurs, size, fill, swap | Valeurs attendues + exception out_of_range |
| `02_array_06_constexpr.cpp` | constexpr | Table de carrés calculée à la compilation | `1 4 9 16 25` |
| `02_array_07_bindings.cpp` | Structured bindings | Décomposition en variables nommées | `Latitude : 48.8566` etc. |

## 13.3 — std::list et std::forward_list (03-list-forward-list.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `03_list_01_construction.cpp` | Construction | list et forward_list (taille, init list, CTAD) | `l3 size : 4`, `fl3 éléments : 4` |
| `03_list_02_insertion.cpp` | Insertion | push_front/back, insert, emplace, before_begin | `1 5 10 20 25 30 40 50 100` ; `1 5 10 15 20 30 40` |
| `03_list_03_suppression.cpp` | Suppression | pop, erase, remove, remove_if, erase_after | `20 15` ; `10 20` |
| `03_list_04_operations.cpp` | Opérations spécifiques | splice, sort, merge, unique, reverse | splice O(1), tri, fusion, doublons, inversion |
| `03_list_05_stabilite.cpp` | Stabilité itérateurs | Itérateurs valides après insertions/suppressions | `*it30 = 30` (trois fois) |

## 13.4 — std::deque (04-deque.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `04_deque_01_construction.cpp` | Construction | Vide, taille, init list, copie, move, CTAD | `d3 size=4, d6 size=4` |
| `04_deque_02_extremites.cpp` | Opérations aux extrémités | push/pop front/back, emplace | `front=10, back=40` ; `10 20 30 40` |
| `04_deque_03_acces.cpp` | Accès et insertion/suppression | [], at(), insert, erase au milieu | `10 20 25 40 50` |
| `04_deque_04_invalidation.cpp` | Invalidation itérateurs | Pointeurs/refs valides après push_back, itérateurs non | `ref=30, *ptr=30` |
| `04_deque_05_stack_queue.cpp` | stack et queue | Adaptateurs utilisant deque par défaut | `top = 30`, `front = 10` |

## 13.5 — std::span (05-span.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `05_span_01_probleme.cpp` | Le problème | Trois versions d'une même fonction sans span | `15` pour chaque source |
| `05_span_02_solution.cpp` | Solution avec span | Une seule fonction pour toutes les sources | `15, 15, 15, 9` |
| `05_span_03_construction.cpp` | Construction | Conversions implicites depuis vector/array/C/ptr | `[10, 20, 30]` etc. |
| `05_span_04_const.cpp` | const et mutabilité | span\<int\> vs span\<const int\> vs const span | `v[0] = 99`, `s_ro[0] = 99` |
| `05_span_05_api.cpp` | API complète | Accès, taille, sous-vues, itérateurs, tri | Valeurs attendues |
| `05_span_06_cas_usage.cpp` | Cas d'usage | est_trie, moyenne, analyse de paquet réseau | `Header : 4 octets`, `Payload : 3 octets` |

## 13.5.1 — Span statique vs dynamique (05.1-span-statique-dynamique.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `05_1_span_dynamique.cpp` | Dynamique/statique | sizeof=16 vs sizeof=8, déduction automatique | `sizeof(s) = 16` (dyn), `sizeof(s) = 8` (stat) |
| `05_1_span_conversions.cpp` | Conversions | first\<N\>/last\<N\> statique, first(n)/last(n) dynamique | sizeof 8 (stat) vs 16 (dyn) |
| `05_1_span_extent.cpp` | extent + cas d'usage | if constexpr, clé AES 32 octets, norme 3D | `Norme : 5`, `Scalaire : 3` |

## 13.5.2 — Remplacement pointeur+taille (05.2-remplacement-pointeur-taille.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `05_2_migration.cpp` | Migration de base | remplir_zeros, somme, moyenne, normaliser, recherche | `Moyenne : 25`, `-15 -5 5 15`, `Index de 30 : 2` |
| `05_2_callback_api_c.cpp` | Callback + API C | Callback moderne, wrapper pour API C legacy | `Reçu 10/5/5 échantillons`, `Résultat : 15` |
| `05_2_sortie_multiple.cpp` | Sortie multiple | Buffer de sortie avec sous-span retourné | `Lus : 3 éléments`, `100 200 300` |

## 13.5.3 — Interopérabilité conteneurs (05.3-interoperabilite-conteneurs.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `05_3_vector_array.cpp` | Interopérabilité complète | vector, array, C array, ptr+taille, mutabilité, produit scalaire, string/string_view | Produit scalaire, conversions, texte/octets |

## 13.6 — Complexité Big O (06-complexite-big-o.md)

| Fichier | Section | Description | Comportement attendu |
|---|---|---|---|
| `06_big_o_bench.cpp` | Benchmark insertion | Insertion au milieu : vector vs list vs deque (compiler avec -O2) | vector plus rapide que list malgré O(n) vs O(1) |
