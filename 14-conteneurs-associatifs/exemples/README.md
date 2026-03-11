# Exemples — Chapitre 14 : Conteneurs Associatifs

## Compilation

Chaque fichier est un programme autonome compilable individuellement :

```bash
g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g nom_du_fichier.cpp -o nom_du_fichier
```

Compiler et exécuter tous les exemples d'un coup :

```bash
for f in *.cpp; do
    echo "=== $f ==="
    g++-15 -std=c++23 -Wall -Wextra -Wpedantic -g "$f" -o "${f%.cpp}" && ./"${f%.cpp}"
    echo
done
```

---

## 14.1 — std::map et std::multimap (01-map-multimap.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `01_map_01_initialisation.cpp` | Déclarer et initialiser | Init list, vide+insertion, plage d'itérateurs | `Lyon : 522969`, trié alphabétiquement |
| `01_map_02_insertion.cpp` | Insertion d'éléments | operator[], insert, insert_or_assign, emplace, try_emplace | `Alice=100`, `success=true/false`, `inserted=false` |
| `01_map_03_recherche.cpp` | Recherche et accès | find, contains (C++20), at avec exception | `Lyon : 522969 habitants`, `Erreur : map::at` |
| `01_map_04_parcours.cpp` | Parcours et itération | Ordonné, inverse, lower/upper_bound, equal_range | Ordre alphabétique, XIXe siècle : 1804-1870 |
| `01_map_05_suppression.cpp` | Suppression d'éléments | erase (clé/itérateur/plage), extract (C++17), merge (C++17) | `Alicia=95`, `m1: 1:a 2:b 3:c 4:d 5:e` |
| `01_map_06_comparateurs.cpp` | Comparaison personnalisée | Ordre décroissant, case insensitive, transparent comparators | `3: trois, 2: deux, 1: un`, `ci_map.size() == 1` |
| `01_map_07_multimap.cpp` | std::multimap | Construction, insertion, equal_range, count, suppression | `nb_fruits = 5`, `pomme, banane, cerise, fraise, fraise` |

## 14.2 — std::unordered_map (02-unordered-map.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `02_umap_01_introduction.cpp` | Introduction | Déclaration, recherche O(1), parcours non ordonné | `Lyon : 522969 habitants`, ordre imprévisible |
| `02_umap_02_api.cpp` | API essentielle | Insertion, recherche, suppression, parcours | `Café : 1.80€`, `Non trouvé : unordered_map::at` |
| `02_umap_03_buckets.cpp` | Contrôle interne | Buckets, facteur de charge, reserve | Nombre de buckets, load factor, `lookup: size=10000` |
| `02_umap_04_multimap.cpp` | unordered_multimap | Clés dupliquées, count, equal_range | `Scores d'Alice : 3` |

## 14.2.1 — Fonctionnement des hash tables (02.1-hash-tables.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `02_1_bucket_distribution.cpp` | Visualiser la distribution | Diagnostic de la distribution des buckets | Statistiques buckets, max_len faible, load factor |

## 14.2.2 — Custom hash functions (02.2-custom-hash.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `02_2_hash_01_specialisation.cpp` | Spécialiser std::hash | Hash pour type utilisateur Coordinate | `(10, 20) = Point A`, `(30, 40) = Point B` |
| `02_2_hash_02_functor.cpp` | Hash functor externe | Struct callable et lambda comme hash | `labels size = 2`, `labels2 size = 1` |
| `02_2_hash_03_combine.cpp` | hash_combine variadic | Technique Boost 64 bits, combined_hash, non-commutativité | `hash(3,7) != hash(7,3) : true` |
| `02_2_hash_04_pair_tuple.cpp` | Hash pair/tuple/Employee | PairHash, TupleHash, type complexe avec hash récursif | `grid size = 2`, `team size = 2` |

## 14.3 — std::set et std::unordered_set (03-set-unordered-set.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `03_set_01_ordered.cpp` | std::set | Construction, insertion, recherche, plages, extract, transparent | `2 3 5 7 11`, `Scores entre 30 et 80 : 30 42 55 67 78` |
| `03_set_02_unordered.cpp` | std::unordered_set | Ensemble non ordonné, hash personnalisé, reserve | `Lyon déjà visitée`, `cloud size = 2` |
| `03_set_03_multi.cpp` | multiset / unordered_multiset | Doublons, count, equal_range, erase | `Nombre de 85 : 3`, `92 92` |
| `03_set_04_operations.cpp` | Opérations ensemblistes | Union, intersection, différence, symétrique, includes | `Union : 1 2 3 4 5 6 7`, `C ⊆ A : true` |

## 14.4 — std::flat_map et std::flat_set (04-flat-map-flat-set.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `04_flat_01_introduction.cpp` | Introduction flat_map | Déclaration, insertion, recherche, parcours | `Lyon : 522969 habitants`, trié alphabétiquement |
| `04_flat_02_sorted_unique.cpp` | Construction sorted_unique | Construction O(n) depuis données triées | `1:a 2:b 3:c 4:d 5:e` |
| `04_flat_03_api.cpp` | API flat_map | Interface map, keys/values, extract/replace, deque | `clé=99`, `extracted keys size=2` |
| `04_flat_04_set.cpp` | flat_set / flat_multiset | Ensemble trié, opérations ensemblistes, multiset | `2 3 5 7 11`, `Nombre de 5 : 3` |
| `04_flat_05_build_once.cpp` | Pattern build once read many | Construction en bloc, tri, unique, sorted_unique | `alpha : 1.1`, `beta : 2.2`, `gamma : 3.3` |

## 14.5 — Comparaison de performances (05-comparaison-performances.md)

| Fichier | Section | Description | Sortie attendue |
|---|---|---|---|
| `05_vector_trie.cpp` | Vecteur trié DIY | Vecteur trié avec recherche binaire via std::ranges | `Trouvé : b` |
